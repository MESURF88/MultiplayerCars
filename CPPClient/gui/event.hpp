#ifndef _EVENT_
#define _EVENT_
#include <string>

enum BEventType
{
	BEventTimeStampMessage = 0,
	BEventPositionUpdateMessage,
	BEventColorUpdateMessage,
	BEventExternalConnectionExitMessage,
	BEventTextUpdateMessage,
	BEventRaceStartMessage,


	BEventPositionDebugUpdateMessage = 20,
};

enum ClientUpdateEventType
{
	EventPositionMessage = 0,
	EventColorUpdateMessage,
	EventTextUpdateMessage,
	EventRaceStartMessage,



	EventPositionDebugMessage = 20,

};


#endif // _EVENT_
