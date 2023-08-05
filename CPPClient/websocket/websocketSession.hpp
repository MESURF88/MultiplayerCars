#ifndef _WEBSOCKETSESSION_
#define _WEBSOCKETSESSION_
#include "threadSafeQueue.hpp"
#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <boost/asio/strand.hpp>
#include <boost/thread/thread.hpp>
#include <cstdlib>
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include <thread>

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
namespace ssl = boost::asio::ssl;       // from <boost/asio/ssl.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

class WebsocketSession : public std::enable_shared_from_this<WebsocketSession>
{
public:
    WebsocketSession(net::io_context &ioc, ssl::context& ctx, std::function<void(const std::string&)> readcb, std::string colorStr);
    ~WebsocketSession();

    bool connectWebSocket(std::string host, std::string port, std::string otp);
    void closeConnection();
    bool sendPosition(int X, int Y, float Angle);
    bool sendColorUpdate(std::string hexValueColor);
    bool sendTextMessage(std::string toUUID, std::string colorStr, std::string text, bool global = true);

    void setSessionColor(std::string currColor);
    std::string getSessionColor();

    std::string getClientUUID() const
    {
        return m_uuid;
    }

private:
    void workerWrite();
    void workerRead();
    websocket::stream < beast::ssl_stream<tcp::socket>> m_wss;
    tcp::resolver m_resolver;
    std::function<void(const std::string&)> m_readCallback;
    std::vector<std::thread> m_threadList;
    std::string m_uuid;
    std::string m_host;
    std::string m_port;
    std::string m_otp;
    std::string m_currColor;
    ThreadSafeQueue<std::string> asyncWriteQueue;
    bool m_isConnected;
    char m_posRawJson[300];
    char m_colorRawJson[300];
    char m_textMsgRawJson[300];
};

#endif // _WEBSOCKETSESSION_