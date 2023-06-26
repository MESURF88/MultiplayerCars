#include "threadSafeQueue.hpp"
#include "postRequest.hpp"
#include "websocketConnect.hpp"
#include "windowContext.hpp"
#include "carClass.hpp"
#include "event.hpp"
#include <functional>
#include <iostream>
#include <thread>
#include <mutex>
#include <string>
#include <nlohmann/json.hpp>

// Globals
ThreadSafeQueue<std::string> wsUpdatedJsonQueue;
ThreadSafeQueue<std::string> guiJsonQueue;
std::atomic<bool> g_gameRunning = false;

// Handler classes
//----------------------------------------------------------------------------------

class MessageRelay
{
public:
    MessageRelay() { };
    ~MessageRelay() { };

    void relayWorkerThread() {
        while (g_gameRunning) {
            if (!wsUpdatedJsonQueue.isEmpty())
            {
                std::string& jsondata = wsUpdatedJsonQueue.front(); // no copy, just a reference
                guiJsonQueue.push(std::move(jsondata)); // move the data to the other queue
                wsUpdatedJsonQueue.pop();
            }
            else
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
                if (!g_gameRunning)
                {
                    break;
                }
            }
        }
    }
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
        // notify can get new timestamp
        wsUpdatedJsonQueue.push("" + msg); // must be rvalue 
    }

    nlohmann::json parseMessage(const std::string& msg)
    {
        try
        {
            return nlohmann::json::parse(msg);
        }
        catch (std::out_of_range& e)
        {
            std::cout << "out of range: " << e.what() << std::endl;
        }
        return nlohmann::json{ };
    }

private:

};

class CartesianCoordinates
{
public:
    CartesianCoordinates() {
        m_X = 0;
        m_Y = 0;
    }
    int m_X;
    int m_Y;
};

class CarContext
{
public:
    CarContext() {
        m_color = "ffffff";
    }
    CarContext(int X, int Y, std::string color)
    {
        m_coords.m_X = X;
        m_coords.m_Y = Y;
        m_color = color;
    }
    CartesianCoordinates m_coords;
    std::string m_color;
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
    std::cout << "DEBUG MODE" << std::endl;
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
        std::shared_ptr<WebsocketSession> session = WebsocketConn(ioc, ctx, host, port, otp, stdf_message, getCarColorString());

        if (session)
        {
            // Initialize gui variables here
            //----------------------------------------------------------------------------------
            bool move = false;
            std::string gui_timestamp;
            std::map<std::string, CarContext> gui_externalplayers;
            int g_X = 0;
            int g_Y = 0;
            //----------------------------------------------------------------------------------
            // End Initialize gui variables here

            g_gameRunning = true;
            std::thread guiMessagingThread(&MessageRelay::relayWorkerThread, relay);
            // Main game loop
            while (windowShouldCloseWrapper() && g_gameRunning) {   // Detect window close button or ESC key
                // Update
                //----------------------------------------------------------------------------------
                // Update your variables here
                if (!guiJsonQueue.isEmpty()) // gui update queue, get fifo
                {
                    // online json parser
                    nlohmann::json parsedJson = listener.parseMessage(guiJsonQueue.front());
                    if (!parsedJson.empty())
                    {
                        try {
                            int type = parsedJson.at("Type");
                            switch (type)
                            {
                            case BEventType::BEventTimeStampMessage:
                                {
                                    gui_timestamp = parsedJson.at("TimeStamp");
                                }
                                break;
                                case BEventType::BEventPositionUpdateMessage:
                                {
                                    std::string uuidOfPlayerCar = parsedJson.at("UUID");
                                    if (0 == gui_externalplayers.count(uuidOfPlayerCar))
                                    {
                                        // new player
                                        gui_externalplayers.emplace(uuidOfPlayerCar, CarContext(parsedJson.at("X"), parsedJson.at("Y"), parsedJson.at("Color")));
                                        // need to send new player our own position, because client owns position
                                        move = true;
                                    }
                                    else
                                    {
                                        gui_externalplayers.at(uuidOfPlayerCar).m_coords.m_X = parsedJson.at("X");
                                        gui_externalplayers.at(uuidOfPlayerCar).m_coords.m_Y = parsedJson.at("Y");
                                        gui_externalplayers.at(uuidOfPlayerCar).m_color = parsedJson.at("Color");
                                    }
                                }
                                break;
                                case BEventType::BEventColorUpdateMessage:
                                {
                                    std::string uuidOfPlayerCar = parsedJson.at("UUID");
                                    if (gui_externalplayers.count(uuidOfPlayerCar))
                                    {
                                        gui_externalplayers.at(uuidOfPlayerCar).m_color = parsedJson.at("Color");
                                    }
                                }
                                break;
                                case BEventType::BEventExternalConnectionExitMessage:
                                {
                                    std::string uuidOfPlayerCar = parsedJson.at("UUID");
                                    if (gui_externalplayers.count(uuidOfPlayerCar))
                                    {
                                        gui_externalplayers.erase(uuidOfPlayerCar);
                                    }
                                }
                                break;
                            }
                            
                        }
                        catch (std::out_of_range& e)
                        {
                            std::cout << "out of range: " << e.what() << std::endl;
                        }
                    }
                    guiJsonQueue.pop();
                }
                //----------------------------------------------------------------------------------
                // End Update

                if (windowIsKeyPressedUp())
                {
                    if (g_Y > 1)
                    {
                        g_Y-=2;
                        move = true;
                    }
                }
                if (windowIsKeyPressedDown())
                {
                    if (g_Y < (windowYBoundary() - getCarHeight() - 1))
                    {
                        g_Y+=2;
                        move = true;
                    }
                }
                if (windowIsKeyPressedLeft())
                {
                    if (g_X > 1)
                    {
                        g_X-=2;
                        move = true;
                    }
                }
                if (windowIsKeyPressedRight())
                {
                    if (g_X < (windowScreenWidth() - getCarWidth() -1))
                    {
                        g_X+=2;
                        move = true;
                    }
                }
                if (windowIsMouseButtonPressed())
                {
                    int colorSelection = windowIsMouseInColorSelection();
                    switch (colorSelection)
                    {
                    case colorSelectionType::NOCOLOR:
                        break;
                    case colorSelectionType::BLUECOLOR:
                    case colorSelectionType::GREENCOLOR:
                    case colorSelectionType::REDCOLOR:
                    case colorSelectionType::MAGENTACOLOR:
                    case colorSelectionType::ORANGECOLOR:
                    case colorSelectionType::YELLOWCOLOR:
                    case colorSelectionType::SKYBLUECOLOR:
                    case colorSelectionType::LIGHTGREYCOLOR:
                        if (windowGetColorSelectionMap().count(colorSelection))
                        {
                            setCarColor(windowGetColorSelectionMap().at(colorSelection).hexValue);
                            session->sendColorUpdate(windowGetColorSelectionMap().at(colorSelection).hexString);
                        }
                        break;
                    default:
                        break;
                    }
                    if (windowIsMouseInEscape())
                    {
                        g_gameRunning = false;
                    }
                }
                if (move)
                {
                    session->sendPosition(g_X, g_Y);
                    move = false;
                }
                // Draw
                //----------------------------------------------------------------------------------
                windowBeginDrawing();
                windowDrawBackground();
                drawDefaultSquaresColor();
                drawCar(g_X, g_Y);
                for (auto coords = gui_externalplayers.begin(); coords != gui_externalplayers.end(); coords++)
                {
                    drawCar(coords->second.m_coords.m_X, coords->second.m_coords.m_Y, colorHexToString(coords->second.m_color));
                }
                drawTextTestBox(gui_timestamp);
                drawChatBoxContainer();
                drawEscButton();
                windowEndDrawing();
                //----------------------------------------------------------------------------------
                // End Draw
            }
            session->closeConnection();
            g_gameRunning = false;
            guiMessagingThread.join();  
        }
    }
    else 
    {
        std::cout << "Error: couldn't login, http status code: " << statusCode << std::endl;
    }
    windowCloseWindow();
    return 0;
}
