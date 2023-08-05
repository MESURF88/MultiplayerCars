package main

import (
	"log"
	"net/http"
	"sync"
	"errors"
	"time"
	"context"
	"encoding/json"

	"github.com/gorilla/websocket"
)

var (
	/**
	websocketUpgrader is used to upgrade incoming HTTP requests into a persitent websocket connection
	*/
	websocketUpgrader = websocket.Upgrader{
		// Apply the Origin Checker
		CheckOrigin:     checkOrigin,
		ReadBufferSize:  1024,
		WriteBufferSize: 1024,
	}
)

var (
	ErrEventNotSupported = errors.New("this event type is not supported")
)

// checkOrigin will check origin and return true if its allowed
func checkOrigin(r *http.Request) bool {

	// Grab the request origin
	origin := r.Header.Get("Origin")
	
	switch origin {
	case "https://localhost:3000":
		return true
	default:
		return false
	}
}

// Manager is used to hold references to all Clients Registered, and Broadcasting etc
type Manager struct {
	clients ClientList

	// Using a syncMutex here to be able to lock state before editing clients
	// Could also use Channels to block
	sync.RWMutex
	// handlers are functions that are used to handle Events
	handlers map[int]EventHandler
	// otps is a map of allowed OTP to accept connections from
	otps RetentionMap
}

// NewManager is used to initalize all the values inside the manager
func NewManager(ctx context.Context) *Manager {
	m := &Manager{
		clients: make(ClientList),
		handlers: make(map[int]EventHandler),
		// Create a new retentionMap that removes Otps older than 5 seconds
		otps: NewRetentionMap(ctx, 5*time.Second),
	}
	m.setupEventHandlers()
	return m
}

// setupEventHandlers configures and adds all handlers
func (m *Manager) setupEventHandlers() {
	m.handlers[EventPositionMessage] = func(e Event, c *Client) error {
        // send raw payload as passthrough down, its faster
		m.broadcastUpdateToPeers(c.UUID, e.Payload)
		return nil
	}
	m.handlers[EventColorUpdateMessage] = func(e Event, c *Client) error {
		var colorMsg ColorUpdateEvent
		if err := json.Unmarshal(e.Payload, &colorMsg); err != nil {
			log.Printf("error marshalling color message: %v", err)
		} else {
			c.color = colorMsg.Color;
			// notify clients of color change
			clientDataPayload := BroadcastEvent{BEventColorUpdateMessage, c.UUID, time.Now().Format(time.RFC3339Nano), 0, 0, 0.0, c.color}
			bytepayload, jsonerr := json.Marshal(clientDataPayload)
			if jsonerr != nil {
				log.Printf("error creating json broadcast message: %v", jsonerr)
			}
			m.broadcastUpdateToPeers(c.UUID, bytepayload)
		}
		return nil
	}
	m.handlers[EventTextUpdateMessage] = func(e Event, c *Client) error {
		var textMsg TextUpdateEvent
		if err := json.Unmarshal(e.Payload, &textMsg); err != nil {
			log.Printf("error marshalling text message: %v", err)
		} else {
			// notify clients of color change
			clientDataPayload := BroadcastTextMessageEvent{BEventTextUpdateMessage, textMsg.FromUUID, textMsg.ToUUID, textMsg.Color, textMsg.Text, time.Now().Format(time.RFC3339Nano), textMsg.Global}
			bytepayload, jsonerr := json.Marshal(clientDataPayload)
			if jsonerr != nil {
				log.Printf("error creating json broadcast message: %v", jsonerr)
			}
			if textMsg.Global == true {
				m.broadcastUpdateToPeers(c.UUID, bytepayload)
			} else {
				m.sendUpdateToPeer(textMsg.ToUUID, bytepayload)
			}
		}
		return nil
	}

	m.handlers[EventPositionDebugMessage] = func(e Event, c *Client) error {
		// send raw payload as passthrough down, its faster
		if err := c.connection.WriteMessage(websocket.TextMessage, e.Payload); err != nil {
			log.Println(err)
		}
		return nil
	}

}

// routeEvent is used to make sure the correct event goes into the correct handler
func (m *Manager) routeEvent(event Event, c *Client) error {
	// Check if Handler is present in Map
	if handler, ok := m.handlers[event.Type]; ok {
		// Execute the handler and return any err
		if err := handler(event, c); err != nil {
			return err
		}
		return nil
	} else {
		return ErrEventNotSupported
	}
}

// loginHandler is used to verify an user authentication and return a one time password
func (m *Manager) loginHandler(w http.ResponseWriter, r *http.Request) {
	type userLoginRequest struct {
		Username string `json:"username"`
		Password string `json:"password"`
	}

	var req userLoginRequest
	err := json.NewDecoder(r.Body).Decode(&req)
	if err != nil {
		http.Error(w, err.Error(), http.StatusBadRequest)
		return
	}

	// Authenticate user / Verify Access token, what ever auth method you use
	if req.Username == "hill" && req.Password == "1995" {
		// format to return otp in to the frontend
		type response struct {
			OTP string `json:"otp"`
		}

		// add a new OTP
		otp := m.otps.NewOTP()

		resp := response{
			OTP: otp.Key,
		}

		data, err := json.Marshal(resp)
		if err != nil {
			log.Println(err)
			return
		}
		// Return a response to the Authenticated user with the OTP
		w.WriteHeader(http.StatusOK)
		w.Write(data)
		return
	}

	// Failure to auth
	w.WriteHeader(http.StatusUnauthorized)
}

// serveWS is a HTTP Handler that the has the Manager that allows connections
func (m *Manager) serveWS(w http.ResponseWriter, r *http.Request) {

	// Grab the OTP in the Get param
	otp := r.URL.Query().Get("otp")
	if otp == "" {
		// Tell the user its not authorized
		w.WriteHeader(http.StatusUnauthorized)
		return
	}

	// Grab the the uuid from new client in the Get param
	uuidStr := r.URL.Query().Get("uuid")
	if uuidStr == "" {
		// Tell the user its not authorized
		w.WriteHeader(http.StatusUnauthorized)
		return
	}

	// Grab the the color from new client in the Get param
	colorStr := r.URL.Query().Get("color")
	if colorStr == "" {
		// Tell the user its not authorized
		w.WriteHeader(http.StatusUnauthorized)
		return
	}

	// Verify OTP is existing
	if !m.otps.VerifyOTP(otp) {
		w.WriteHeader(http.StatusUnauthorized)
		return
	}

	log.Println("New connection")
	// Begin by upgrading the HTTP request
	conn, err := websocketUpgrader.Upgrade(w, r, nil)
	if err != nil {
		log.Println(err)
		return
	}

	// Create New Client
	client := NewClient(conn, m, uuidStr, colorStr)
	// Add the newly created client to the manager
	m.addClient(client)
	// Start the read / write processes for now
	m.broadcastNewClientInitPosition(client)
	log.Println("Client Connected, Begin Transmitting")
	go client.readMessages()
	go client.writeMessages()
}

// addClient will add clients to our clientList
func (m *Manager) addClient(client *Client) {
	m.Lock()
	defer m.Unlock()

	// Add Client
	m.clients[client] = true
}

// removeClient will remove the client and clean up
func (m *Manager) removeClient(client *Client) {
	m.Lock()
	defer m.Unlock()

	// Check if Client exists, then delete it
	if _, ok := m.clients[client]; ok {
		// close connecion
		client.connection.Close()
		// remove
		delete(m.clients, client)
		// notify other clients
		// for each client that is not the same uuid as sending client broadcast update of car x y uuid, timestamp and color
		clientExitPayload := BroadcastEvent{BEventExternalConnectionExitMessage, client.UUID, time.Now().String(), 0, 0, 0.0, client.color}
		bytepayload, jsonerr := json.Marshal(clientExitPayload)
		if jsonerr != nil {
			log.Printf("error creating json broadcast message: %v", jsonerr)
		}
		m.broadcastUpdateToPeers(client.UUID, bytepayload)
	}
}

func (m *Manager) broadcastNewClientInitPosition(c *Client) {
	// for each client that is not the same uuid as sending client broadcast new car details x y uuid, timestamp and color
	clientDataPayload := BroadcastEvent{BEventPositionUpdateMessage, c.UUID, time.Now().String(), 0, 0, 0.0, c.color}
	bytepayload, jsonerr := json.Marshal(clientDataPayload)
	if jsonerr != nil {
		log.Printf("error creating json broadcast message: %v", jsonerr)
	}
	m.broadcastUpdateToPeers(c.UUID, bytepayload)
}

func (m *Manager) broadcastUpdateToPeers(sendingUUID string, bytepayload []byte) {
	for clientElement, connected := range m.clients {
		if (clientElement.UUID != sendingUUID) && (connected)	{
			clientElement.egress <- bytepayload
		}
	}
}

func (m *Manager) sendUpdateToPeer(toUUID string, bytepayload []byte) {
	for clientElement, connected := range m.clients {
		if (clientElement.UUID == toUUID) && (connected)	{
			clientElement.egress <- bytepayload
		}
	}
}
