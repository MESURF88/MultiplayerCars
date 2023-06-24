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

std::shared_ptr<WebsocketSession> WebsocketConn(std::string host, std::string port, std::string otp, std::function<void(std::string)>)
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
    // The io_context is required for all I/O
    net::io_context ioc;

    // The SSL context is required, and holds certificates
    ssl::context ctx{ ssl::context::tlsv12_client };

    beast::error_code ec;
    ctx.add_certificate_authority(
        boost::asio::buffer(cert.data(), cert.size()), ec);
    if (ec)
        std::cout << "error: could not assign authority" << std::endl;

    std::shared_ptr<WebsocketSession> session = std::make_shared<WebsocketSession>();

    // These objects perform our I/O
    tcp::resolver resolver{ ioc };
    websocket::stream<beast::ssl_stream<tcp::socket>> ws{ ioc, ctx };

    try
    {
        // Look up the domain name
        auto const results = resolver.resolve(host, port);
        // Make the connection on the IP address we get from a lookup
        auto ep = net::connect(get_lowest_layer(ws), results);

        // Set SNI Hostname (many hosts need this to handshake successfully)
        if (!SSL_set_tlsext_host_name(ws.next_layer().native_handle(), host.c_str()))
            throw beast::system_error(
                beast::error_code(
                    static_cast<int>(::ERR_get_error()),
                    net::error::get_ssl_category()),
                "Failed to set SNI Hostname");

        // Update the host_ string. This will provide the value of the
        // Host HTTP header during the WebSocket handshake.
        // See https://tools.ietf.org/html/rfc7230#section-5.4
        host += ':' + std::to_string(ep.port());

        // Perform the SSL handshake
        ws.next_layer().handshake(ssl::stream_base::client);

        // Set a decorator to change the User-Agent of the handshake
        ws.set_option(websocket::stream_base::decorator(
            [](websocket::request_type& req)
            {
                req.set(http::field::user_agent,
                    std::string(BOOST_BEAST_VERSION_STRING) +
                    " websocket-client-coro");

                req.set(http::field::origin, "https://localhost:3000");
            }));

        // Perform the websocket handshake
        ws.handshake(host, "/ws?otp=" + otp); //TODO: add query strings

        do
        {
            // This buffer will hold the incoming message
            beast::flat_buffer buffer;

            // Read a message into our buffer
            ws.read(buffer, ec);

            if (ec)
                std::cout << "error: could not read buffer" << std::endl;

            // The make_printable() function helps print a ConstBufferSequence
            std::cout << beast::make_printable(buffer.data()) << std::endl;
        } while (true);

        // Close the WebSocket connection
        //TODO: ws.close(websocket::close_code::normal);
    }
    catch (std::exception const& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return nullptr;
    }

    return session;
}