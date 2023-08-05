#include "websocketSession.hpp"
#include "uuidGenerator.hpp"
#include "event.hpp"
#include <iostream>
#include <simdjson.h>

WebsocketSession::WebsocketSession(net::io_context& ioc, ssl::context& ctx, std::function<void(const std::string&)> readcb, std::string colorStr):
m_isConnected(false),
m_resolver(net::make_strand(ioc)),    // These objects perform our I/O
m_wss(net::make_strand(ioc), ctx),
m_readCallback(readcb),
m_currColor(colorStr)
{
    m_uuid = uuid::generate_uuid_v4();
}

WebsocketSession::~WebsocketSession()
{
	m_isConnected = false;
}

bool WebsocketSession::connectWebSocket(std::string host, std::string port, std::string otp)
{
	bool succ = false;
    m_host = host;
    m_port = port;
    m_otp = otp;

    // bind worker thread
    m_threadList.push_back(std::thread(&WebsocketSession::workerRead, this));
    succ = true;

	return succ;
}

bool WebsocketSession::sendPosition(int X, int Y, float Angle)
{
    bool succ = false;
    if (m_isConnected && m_wss.is_message_done())
    {
        //pack x and y into buffer
        sprintf(m_posRawJson, R"( { "Type": %d , "Payload": { "Color": "%s", "Type": %d, "UUID": "%s", "X": %d, "Y": %d, "Angle" : %.8g } } )", EventPositionMessage, getSessionColor().c_str(), BEventPositionUpdateMessage, getClientUUID().c_str(), X, Y, Angle);
        size_t posBufferLength = strlen(m_posRawJson);
        // Create a buffer to receive the minified string. Make sure that there is enough room (length bytes).
        std::unique_ptr<char[]> posBuffer{ new char[posBufferLength] };
        size_t new_length{};
        auto error = simdjson::minify(m_posRawJson, posBufferLength, posBuffer.get(), new_length);
        asyncWriteQueue.push(std::string(posBuffer.get(), new_length));
    }
    return succ;
}

bool WebsocketSession::sendColorUpdate(std::string hexValueColor)
{
    bool succ = false;
    setSessionColor(hexValueColor); // update stored color for online mode
    if (m_isConnected && m_wss.is_message_done())
    {
        //pack color into buffer
        sprintf(m_colorRawJson, R"( { "Type": %d , "Payload": { "Color": "%s" } } )", EventColorUpdateMessage, hexValueColor.c_str());
        size_t colorBufferLength = std::strlen(m_colorRawJson);
        // Create a buffer to receive the minified string. Make sure that there is enough room (length bytes).
        std::unique_ptr<char[]> colorBuffer{ new char[colorBufferLength] };
        size_t new_length{};
        auto error = simdjson::minify(m_colorRawJson, colorBufferLength, colorBuffer.get(), new_length);
        beast::error_code ec;
        m_wss.write(net::buffer(std::string(colorBuffer.get(), new_length)), ec);
        if (ec)
        {
            std::cout << ec.message() << "\n";
            std::cout << "error: could not write buffer" << std::endl;
        }
    }
    return succ;
}

bool WebsocketSession::sendTextMessage(std::string toUUID, std::string colorStr, std::string text, bool global)
{
    bool succ = false;
    if (colorStr == "")
    {
        colorStr = m_currColor;
    }
    if (m_isConnected && m_wss.is_message_done())
    {
        //pack text message into buffer
        sprintf(m_textMsgRawJson, R"( { "Type": %d , "Payload": { "Color": "%s", "FromUUID": "%s", "ToUUID": "%s", "Text": "%s", "Global": %s } } )", EventTextUpdateMessage, colorStr.c_str(), getClientUUID().c_str(), toUUID.c_str(), text.c_str(), (global)? "true" : "false" );
        size_t textMsgBufferLength = std::strlen(m_textMsgRawJson);
        // Create a buffer to receive the minified string. Make sure that there is enough room (length bytes).
        std::unique_ptr<char[]> textMsgBuffer{ new char[textMsgBufferLength] };
        size_t new_length{};
        auto error = simdjson::minify(m_textMsgRawJson, textMsgBufferLength, textMsgBuffer.get(), new_length);
        beast::error_code ec;
        m_wss.write(net::buffer(std::string(textMsgBuffer.get(), new_length)), ec);
        if (ec)
        {
            std::cout << ec.message() << "\n";
            std::cout << "error: could not write buffer" << std::endl;
        }
    }
    return succ;
}

void WebsocketSession::setSessionColor(std::string currColor)
{
    m_currColor = currColor;
}

std::string WebsocketSession::getSessionColor()
{
    return m_currColor;
}

void WebsocketSession::workerWrite()
{
    do
    {
        std::string tmp = asyncWriteQueue.front();
        beast::error_code ec;
        m_wss.write(net::buffer(tmp), ec);
        if (ec)
        {
            std::cout << ec.message() << "\n";
            std::cout << "error: could not write buffer" << std::endl;
        }
        asyncWriteQueue.pop();
    } while (m_isConnected);
}

void WebsocketSession::workerRead()
{
    try
    {
        // Look up the domain name
        auto const results = m_resolver.resolve(m_host, m_port);
        // Make the connection on the IP address we get from a lookup
        auto ep = net::connect(get_lowest_layer(m_wss), results);

        // Set SNI Hostname (many hosts need this to handshake successfully)
        if (!SSL_set_tlsext_host_name(m_wss.next_layer().native_handle(), m_host.c_str()))
            throw beast::system_error(
                beast::error_code(
                    static_cast<int>(::ERR_get_error()),
                    net::error::get_ssl_category()),
                "Failed to set SNI Hostname");

        // Update the host_ string. This will provide the value of the
        // Host HTTP header during the WebSocket handshake.
        // See https://tools.ietf.org/html/rfc7230#section-5.4
        m_host += ':' + std::to_string(ep.port());

        // Perform the SSL handshake
        m_wss.next_layer().handshake(ssl::stream_base::client);

        // Set a decorator to change the User-Agent of the handshake
        m_wss.set_option(websocket::stream_base::decorator(
            [](websocket::request_type& req)
            {
                req.set(http::field::user_agent,
                    std::string(BOOST_BEAST_VERSION_STRING) +
                    " websocket-client-coro");

                req.set(http::field::origin, "https://localhost:3000");
            }));

        // Perform the websocket handshake
        m_wss.handshake(m_host, "/ws?otp=" + m_otp + "&uuid=" + m_uuid + "&color=" + m_currColor); //query parameter strings

        m_isConnected = true;

        m_threadList.push_back(std::thread(&WebsocketSession::workerWrite, this));
        do
        {
            beast::flat_buffer buffer;
            beast::error_code ec;
            m_wss.read(buffer, ec);
            if (ec)
            {
                std::cout << ec.message() << "\n";
                std::cout << "error: could not read buffer" << std::endl;
            }
            else
            {
                std::ostringstream os;
                os << beast::make_printable(buffer.data());
                m_readCallback(os.str());
            }
        } while (m_isConnected);

    }
    catch (std::exception const& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}

void WebsocketSession::closeConnection()
{
    m_isConnected = false;
    asyncWriteQueue.push(""); // get out of deadlock
    for (auto& t : m_threadList) t.join();
    beast::error_code ecCancel;
    m_wss.next_layer().next_layer().cancel(ecCancel);
    if (ecCancel)
    {
        std::cout << ecCancel.message() << "\n";
        std::cout << "error: could not cancel websocket" << std::endl;
    }
    beast::error_code ecClose;
    m_wss.close(websocket::close_code::normal, ecClose);
    if (ecClose)
    {
        std::cout << ecClose.message() << "\n";
        std::cout << "error: could not close websocket" << std::endl;
    }
}