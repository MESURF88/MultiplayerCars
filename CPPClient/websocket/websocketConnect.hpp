#ifndef _WEBSOCKETCONNECT_
#define _WEBSOCKETCONNECT_
#include "websocketSession.hpp"
#include <string>
#include <functional>

std::shared_ptr<WebsocketSession> WebsocketConn(std::string host, std::string port, std::string otp, std::function<void(std::string)>);
#endif // _WEBSOCKETCONNECT_