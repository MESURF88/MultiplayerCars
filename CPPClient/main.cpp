#ifndef _WIN32
#include "json.hpp" //nlohmann::json cannot build in MSVC
#endif
#include "threadSafeQueue.hpp"
#include "postRequest.hpp"
#include "websocketConnect.hpp"
#include "windowContext.hpp"
#include <functional>
#include <iostream>
#include <thread>
#include <mutex>
#include <string>

// Globals
ThreadSafeQueue<std::string> wsUpdatedJsonQueue;
ThreadSafeQueue<std::string> guiJsonQueue;

// Handler classes
//----------------------------------------------------------------------------------

class MessageRelay
{
public:
    MessageRelay() { m_gameRunning = false;  };
    ~MessageRelay() {};

    void setGameRunning(bool gameRunning)
    {
        m_gameRunning = gameRunning;
    }

    bool getGameRunning()
    {
        return m_gameRunning;
    }

    void relayWorkerThread() {
        while (m_gameRunning) {
            std::string& jsondata = wsUpdatedJsonQueue.front(); // no copy, just a reference
            guiJsonQueue.push(std::move(jsondata)); // move the data to the other queue
            wsUpdatedJsonQueue.pop();
        }
    }
private:
    bool m_gameRunning;
};

class ConnectionListener
{
public:
    ConnectionListener() { };
    ~ConnectionListener() {};
    // use maybe?
    void onClose()
    {
        std::cout<<"socket closed "<<std::endl;
        exit(0);
    }
    
    void onFail()
    {
        std::cout<<"socket failed "<<std::endl;
        exit(0);
    }

    void onMessage(const std::string &msg)
    {
        parseMessage(msg);
    }

private:
    void parseMessage(const std::string& msg)
    {
        // notify can get new timestamp
        wsUpdatedJsonQueue.push("" + msg); // must be rvalue 
    }
};

//----------------------------------------------------------------------------------
// end Handler classes

int main() {
    windowSetTargetFPS(60);

    // Initialize Handler Classes
    //--------------------------------------------------------------------------------------
    MessageRelay relay;
    ConnectionListener listener;
    std::function<void(const std::string&)> stdf_message = [&](const std::string &s) { listener.onMessage(s); };
    //--------------------------------------------------------------------------------------
    // End Initialize Handler Classes

    //login
#if DEBUG_CLIENT
    std::string host = "127.0.0.1"; 
    std::string port = "3000";
#else
    std::string host = "onlinecarsimgame-ab2533447e53.herokuapp.com";
    std::string port = "443"; //https
#endif

    int statusCode;     // get status code by reference

#if DEBUG_CLIENT
    std::string otp = PostRequestPassword("https://" + host + ":" + port + "/login", statusCode);
#else
    std::string otp = PostRequestPassword("https://" + host + "/login", statusCode);
#endif
    if ((statusCode == 200) && (otp != ""))
    {
        // The io_context is required for all I/O must be declared so that it is placed in memory
        net::io_context ioc;

        // The SSL context is required, and holds certificates
        ssl::context ctx{ ssl::context::tlsv12_client };

        // boost websockets
        std::shared_ptr<WebsocketSession> session = WebsocketConn(ioc, ctx, host, port, otp, stdf_message);

        if (session)
        {
            // Initialize gui variables here
            //----------------------------------------------------------------------------------
            std::string gui_timestamp;
            //----------------------------------------------------------------------------------
            // End Initialize gui variables here

            relay.setGameRunning(true);
            std::thread guiMessagingThread(&MessageRelay::relayWorkerThread, relay);
            // Main game loop
            while (windowShouldCloseWrapper()) {   // Detect window close button or ESC key
                // Update
                //----------------------------------------------------------------------------------
                // Update your variables here
                if (!guiJsonQueue.isEmpty()) // gui update queue, get fifo
                {
                    gui_timestamp = guiJsonQueue.front();
                    guiJsonQueue.pop();
                }
                //----------------------------------------------------------------------------------
                // End Update

                // Draw
                //----------------------------------------------------------------------------------
                drawTextTestBox(gui_timestamp);
                //----------------------------------------------------------------------------------
                // End Draw
            }
            session->closeConnection();
            relay.setGameRunning(false);
            guiMessagingThread.join();
        }
    }
    else 
    {
        std::cout << "Error: couldn't login, http status code: " << statusCode << std::endl;
    }

    return 0;
}
