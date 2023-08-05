package main

import (
	"encoding/json"
	"log"
	"time"

	"github.com/gorilla/websocket"
)

const socketIOInterval = 1 * time.Second

var (
	// pongWait is how long we will await a pong response from client
	pongWait = 10 * time.Second
	// pingInterval has to be less than pongWait, We cant multiply by 0.9 to get 90% of time
	// Because that can make decimals, so instead *9 / 10 to get 90%
	// The reason why it has to be less than PingRequency is becuase otherwise it will send a new Ping before getting response
	pingInterval = (pongWait * 9) / 10
)

// ClientList is a map used to help manage a map of clients
type ClientList map[*Client]bool

// Client is a websocket client, basically a frontend visitor
type Client struct {
	// the websocket connection
	connection *websocket.Conn

	// manager is the manager used to manage the client
	manager *Manager
	// egress is used to avoid concurrent writes on the WebSocket
	egress chan []byte

	// unique identifier for client
	UUID string

	// color of client
	color string
}

// NewClient is used to initialize a new Client with all required values initialized
func NewClient(conn *websocket.Conn, manager *Manager, uuidStr string, colorStr string) *Client {
	return &Client{
		connection: conn,
		manager:    manager,
		egress:     make(chan []byte),
		UUID:       uuidStr,
		color:      colorStr,
	}
}

// readMessages will start the client to read messages and handle them
// appropriately.
// This is suppose to be ran as a goroutine
func (c *Client) readMessages() {
	defer func() {
		// Graceful Close the Connection once this
		// function is done
		c.manager.removeClient(c)
	}()
	// Set Max Size of Messages in Bytes
	c.connection.SetReadLimit(512)
    // Configure Wait time for Pong response, use Current time + pongWait
	// This has to be done here to set the first initial timer.
	if err := c.connection.SetReadDeadline(time.Now().Add(pongWait)); err != nil {
		log.Println(err)
		return
	}
	// Configure how to handle Pong responses
	c.connection.SetPongHandler(c.pongHandler)

	// Loop Forever
	for {
		// ReadMessage is used to read the next message in queue
		// in the connection
		_, payload, err := c.connection.ReadMessage()

		if err != nil {
			// If Connection is closed, we will Recieve an error here
			// We only want to log Strange errors, but simple Disconnection
			if websocket.IsUnexpectedCloseError(err, websocket.CloseGoingAway, websocket.CloseAbnormalClosure) {
				log.Printf("error reading message: %v", err)
			}
			break // Break the loop to close conn & Cleanup
		}
		// Marshal incoming data into a Event struct
		var request Event
		if err := json.Unmarshal(payload, &request); err != nil {
			log.Printf("error marshalling message: %v", err)
			break // Breaking the connection here might be harsh xD
		}
		// Route the Event
		if err := c.manager.routeEvent(request, c); err != nil {
			log.Println("Error handeling Message: ", err)
		}
	}
}

// pongHandler is used to handle PongMessages for the Client
func (c *Client) pongHandler(pongMsg string) error {
	// Current time + Pong Wait time
	//log.Println("pong " + c.UUID[0:8])
	return c.connection.SetReadDeadline(time.Now().Add(pongWait))
}


// writeMessages is a process that listens for new messages to output to the Client
func (c *Client) writeMessages() {
	// Create a ticker that triggers a ping at given interval
	tickler := time.NewTicker(pingInterval)
	defer func() {
		tickler.Stop()
		// Graceful close if this triggers a closing
		c.manager.removeClient(c)
	}()
	periodicTimeTickler := time.NewTicker(socketIOInterval)
	defer func() {
		periodicTimeTickler.Stop()
	}()

	for {
		select {
		case message, ok := <-c.egress:
			// Ok will be false Incase the egress channel is closed
			if !ok {
				// Manager has closed this connection channel, so communicate that to frontend
				if err := c.connection.WriteMessage(websocket.CloseMessage, nil); err != nil {
					// Log that the connection is closed and the reason
					log.Println("connection closed: ", err)
				}
				// Return to close the goroutine
				return
			}
			// Write a Regular text message to the connection
			if err := c.connection.WriteMessage(websocket.TextMessage, message); err != nil {
				log.Println(err)
			}
		case <-tickler.C:
			// Send the Ping
			if err := c.connection.WriteMessage(websocket.PingMessage, []byte{}); err != nil {
				log.Println("writemsg: ", err)
				return // return to break this goroutine triggeing cleanup
			}
		case <-periodicTimeTickler.C:
			// Send the Time
			payload := BroadcastEvent{BEventTimeStampMessage, c.UUID, time.Now().Format(time.RFC3339Nano), 0, 0, 0.0, ""}
			bytepayload, jsonerr := json.Marshal(payload)
			if jsonerr != nil {
				log.Printf("error creating json message: %v", jsonerr)
				return
			}
			err := c.connection.WriteMessage(websocket.TextMessage, bytepayload)
			if err != nil {
				log.Println(err)
				return
			}
		}

	}
}
