
package main

import "encoding/json"

const (
	// EventPositionMessage is the event name for position IMPORTANT MUST MATCH CLIENT ENUM
	EventPositionMessage = 0
	EventColorUpdateMessage = 1
)

// Event is the Messages sent over the websocket
// Used to differ between different actions
type Event struct {
	// Type is the message type sent
	Type int `json:"Type"`
	// Payload is the data Based on the Type
	Payload json.RawMessage `json:"Payload"`
}

// EventHandler is a function signature that is used to affect messages on the socket and triggered
// depending on the type
type EventHandler func(event Event, c *Client) error
	
const (

	//BroadcastEvent Types IMPORTANT MUST MATCH CLIENT ENUM
	BEventTimeStampMessage = 0
	BEventPositionUpdateMessage = 1
	BEventColorUpdateMessage = 2
	BEventExternalConnectionExitMessage = 3
)

type BroadcastEvent struct {
	BType int `json:"Type"`
	UUID string `json:"UUID"`
	TimeStamp string `json:"TimeStamp"`
	XPos int `json:"X"`
	YPos int `json:"Y"`
	Color string `json:"Color"`
}

type PositionCartesianCoordEvent struct {
	XPos int `json:"X"`
	YPos int `json:"Y"`
}

type ColorUpdateEvent struct {
	Color string `json:"Color"`
}

// SendMessageEvent is the payload sent in the
// send_message event
type SendMessageEvent struct {
	Message string `json:"message"`
	From    string `json:"from"`
}