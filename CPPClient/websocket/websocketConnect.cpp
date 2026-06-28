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
#include <vector>
#if defined(WIN32)
#include <windows.h>
#else
#include <limits.h>
#include <unistd.h>
#endif

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
namespace ssl = boost::asio::ssl;       // from <boost/asio/ssl.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

namespace
{
    char pathSeparator()
    {
#if defined(WIN32)
        return '\\';
#else
        return '/';
#endif
    }

    bool fileExists(const std::string& path)
    {
        std::ifstream file(path);
        return file.good();
    }

    std::string directoryName(const std::string& path)
    {
        const std::size_t separator = path.find_last_of("/\\");
        if (separator == std::string::npos)
        {
            return "";
        }
        return path.substr(0, separator);
    }

    std::string joinPath(const std::string& left, const std::string& right)
    {
        if (left.empty())
        {
            return right;
        }
        if (left.back() == '/' || left.back() == '\\')
        {
            return left + right;
        }
        return left + pathSeparator() + right;
    }

    std::string executableDirectory()
    {
#if defined(WIN32)
        char path[MAX_PATH];
        DWORD length = GetModuleFileNameA(NULL, path, MAX_PATH);
        if (length == 0 || length == MAX_PATH)
        {
            return "";
        }
        std::string executablePath(path, length);
#else
        char path[PATH_MAX];
        ssize_t length = readlink("/proc/self/exe", path, sizeof(path) - 1);
        if (length <= 0)
        {
            return "";
        }
        path[length] = '\0';
        std::string executablePath(path);
#endif
        return directoryName(executablePath);
    }

    std::string findClientFile(const std::string& fileName)
    {
        const std::string exeDir = executableDirectory();
        const std::string parentDir = directoryName(exeDir);
        std::vector<std::string> candidates = {
            fileName,
            joinPath("CPPClient", fileName)
        };

        if (!exeDir.empty())
        {
            candidates.push_back(joinPath(exeDir, fileName));
        }
        if (!parentDir.empty())
        {
            candidates.push_back(joinPath(parentDir, fileName));
        }

        for (const std::string& candidate : candidates)
        {
            if (fileExists(candidate))
            {
                return candidate;
            }
        }
        return "";
    }
}

std::shared_ptr<WebsocketSession> WebsocketConn(net::io_context& ioc, ssl::context& ctx, std::string host, std::string port, std::string otp, std::function<void(const std::string&)> readcb, std::string colorStr)
{
    std::cout << "now connecting to " << host << " " << port << std::endl;
    std::ifstream envAuthCert;
    std::string response;

    // This holds the root certificate used for verification
    std::string cert = "";
    const std::string certPath = findClientFile("server.crt");
    if (!certPath.empty())
    {
        std::cout << "Using websocket certificate " << certPath << std::endl;
        envAuthCert.open(certPath);
    }

    if (envAuthCert.is_open()) { // always check whether the file is open
        std::stringstream strStream;
        strStream << envAuthCert.rdbuf(); //read the file
        cert = strStream.str(); //str holds the content of the file
    }
    else
    {
        std::cout << "error: could not open server.crt. Checked server.crt, CPPClient/server.crt, and the executable directory." << std::endl;
    }

    if (!cert.empty())
    {
        beast::error_code ec;
        ctx.add_certificate_authority(
            boost::asio::buffer(cert.data(), cert.size()), ec);
        if (ec)
        {
            std::cout << "error: could not assign authority from " << certPath << ": " << ec.message() << std::endl;
        }
    }

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
