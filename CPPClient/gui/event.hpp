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
};

enum ClientUpdateEventType
{
	EventPositionMessage = 0,
	EventColorUpdateMessage,
	EventTextUpdateMessage,

};


#endif // _EVENT_