#ifndef _WEBSOCKETCONNECT_
#define _WEBSOCKETCONNECT_
#include "websocketSession.hpp"
#include <string>
#include <functional>

std::shared_ptr<WebsocketSession> WebsocketConn(net::io_context& ioc, ssl::context& ctx, std::string host, std::string port, std::string otp, std::function<void(const std::string&)>);
#endif // _WEBSOCKETCONNECT_