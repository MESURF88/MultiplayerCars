#include "websocketConnect.hpp"
#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <boost/asio/strand.hpp>
#include <cstdlib>
#include <functional>
#include <fstream>
#include <iostream>
#include <sstream>
#include <memory>

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
namespace ssl = boost::asio::ssl;       // from <boost/asio/ssl.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

std::shared_ptr<WebsocketSession> WebsocketConn(net::io_context& ioc, ssl::context& ctx, std::string host, std::string port, std::string otp, std::function<void(const std::string&)> readcb, std::string colorStr)
{
    std::cout << "now connecting to " << host << " " << port << std::endl;
    std::ifstream envAuthCert;
    std::string response;

    // This holds the root certificate used for verification
    std::string cert = "";
#ifdef WIN32
    char pBuf[256];
    size_t len = sizeof(pBuf);
    int bytes = GetModuleFileName(NULL, pBuf, len);
    std::string exePath(pBuf);
    // remove .exe
    while (exePath.back() != '\\')
    {
        exePath.pop_back();
        if (exePath.empty())
        {
            break;
        }
    }
    envAuthCert.open(exePath +"server.crt");
#else
    envAuthCert.open("server.crt");
#endif
    envAuthCert.open("server.crt");
    if (envAuthCert.is_open()) { // always check whether the file is open
        std::stringstream strStream;
        strStream << envAuthCert.rdbuf(); //read the file
        cert = strStream.str(); //str holds the content of the file
    }
    else
    {
        std::cout << "error: could not open file" << std::endl;
    }

    beast::error_code ec;
    ctx.add_certificate_authority(
        boost::asio::buffer(cert.data(), cert.size()), ec);
    if (ec)
        std::cout << "error: could not assign authority" << std::endl;

    std::shared_ptr<WebsocketSession> session = std::make_shared<WebsocketSession>(ioc, ctx, readcb, colorStr);
    bool succ = session->connectWebSocket(host, port, otp);
   
    if (succ)
    {
        return session;
    }
    else
    {
        return nullptr;
    }
}