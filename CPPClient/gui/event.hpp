#ifndef _EVENT_
#define _EVENT_
#include <string>

enum BEventType
{
	BEventTimeStampMessage = 0,
	BEventPositionUpdateMessage,
	BEventColorUpdateMessage,
	BEventExternalConnectionExitMessage,
};

enum ClientUpdateEventType
{
	EventPositionMessage = 0,
	EventColorUpdateMessage,

};


#endif // _EVENT_