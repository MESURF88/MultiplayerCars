#include "websocketSession.hpp"
#include <iostream>

WebsocketSession::WebsocketSession(net::io_context& ioc, ssl::context& ctx, std::function<void(const std::string&)> readcb):
m_isConnected(false),
m_resolver(net::make_strand(ioc)),    // These objects perform our I/O
m_wss(net::make_strand(ioc), ctx),
m_readCallback(readcb)
{

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
        m_wss.handshake(m_host, "/ws?otp=" + m_otp); //query strings

        m_isConnected = true;

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
            std::ostringstream os;
            os << beast::make_printable(buffer.data());
            m_readCallback(os.str());
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
    for (auto& t : m_threadList) t.join();
    m_wss.close(websocket::close_code::normal);
}