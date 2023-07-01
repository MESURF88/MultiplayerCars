#include "threadSafeQueue.hpp"
#include "postRequest.hpp"
#include "websocketConnect.hpp"
#include "windowContext.hpp"
#include "carClass.hpp"
#include "event.hpp"
#include <functional>
#include <iostream>
#include <fstream>
#include <thread>
#include <mutex>
#include <string>
#include <nlohmann/json.hpp>

// timing benchmark
#define TIMING_BENCHMARK

#ifdef TIMING_BENCHMARK
#include <chrono>
std::chrono::time_point<std::chrono::high_resolution_clock> start;
std::chrono::time_point<std::chrono::high_resolution_clock> stop0;
std::chrono::time_point<std::chrono::high_resolution_clock> stop1;
std::chrono::time_point<std::chrono::high_resolution_clock> stop2;
#endif


// Globals
static constexpr int MAX_DISPLAYED_TEXT_MESSAGES = 5;
static constexpr int MAX_INPUT_CHARS = 105;
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

#ifdef TIMING_BENCHMARK
    // open timing benchmark
    std::ofstream timingReport;
    timingReport.open("benchmark.txt");
    if (timingReport.is_open())
    {
        std::cout << "Timing Benchmark Open" << std::endl;
    }
#endif

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
            bool text = false;
            bool mouseOnText = false;
            char textmessage[MAX_INPUT_CHARS + 1] = "\0";      // NOTE: One extra space required for null terminator char '\0'
            int framesCounter = 0;
            int letterCount = 0;
            std::string gui_timestamp;
            std::map<std::string, CarContext> gui_externalplayers;
            std::deque<TextContext> gui_textmessagesdisplay;
            // pre-fill blank text message deque
            for (int i = 0; i < MAX_DISPLAYED_TEXT_MESSAGES; i++)
            {
                gui_textmessagesdisplay.push_back(TextContext(false, "", "", ""));
            }
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
                                case BEventType::BEventTextUpdateMessage:
                                {
                                    std::string uuidOfSender = parsedJson.at("FromUUID");
                                    // sender is not current player
                                    if (uuidOfSender != session->getClientUUID())
                                    {
                                        // only display last 5 messages for now...
                                        std::string senderColor = parsedJson.at("Color");
                                        std::string senderText = parsedJson.at("Text");
                                        std::string senderTimeStamp = parsedJson.at("TimeStamp");
                                        gui_textmessagesdisplay.pop_back();
                                        gui_textmessagesdisplay.push_front(TextContext(true, senderColor, senderText, senderTimeStamp));
                                    }
                                }
                                break;
                                case BEventType::BEventPositionDebugUpdateMessage:
                                {
#ifdef TIMING_BENCHMARK
                                    stop1 = std::chrono::high_resolution_clock::now();
#endif
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
#ifdef TIMING_BENCHMARK
                                    stop2 = std::chrono::high_resolution_clock::now();
                                    auto duration0 = std::chrono::duration_cast<std::chrono::microseconds>(stop0 - start);
                                    auto duration1 = std::chrono::duration_cast<std::chrono::microseconds>(stop1 - start);
                                    auto duration2 = std::chrono::duration_cast<std::chrono::microseconds>(stop2 - start);
                                    // To get the value of duration use the count()
                                    // member function on the duration object
                                    timingReport << "Loopback: sendPos dur0: " << duration0.count() << " [msec]" << std::endl;
                                    timingReport << "Loopback: Move time dur1: " << duration1.count() << " [msec]" << std::endl;
                                    timingReport << "Loopback: Move time after json parse dur2: " << duration2.count() << " [msec]" << std::endl;
#endif
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
                    if (windowIsMouseCollidesChatBox())
                    {
                        mouseOnText = true;
                    }
                    else
                    {
                        mouseOnText = false;
                    }
                    if (windowIsMouseCollidesChatSendButton())
                    {
                        text = true;
                        // append locally first the broadcast
                        gui_textmessagesdisplay.pop_back();
                        gui_textmessagesdisplay.push_front(TextContext(false, getCarColorString(), std::string(textmessage), gui_timestamp));
                    }
                    if (windowIsMouseInEscape())
                    {
                        g_gameRunning = false;
                    }
                }
                if (move)
                {
#ifdef TIMING_BENCHMARK
                    start = std::chrono::high_resolution_clock::now();
#endif
                    session->sendPosition(g_X, g_Y);
#ifdef TIMING_BENCHMARK
                    stop0 = std::chrono::high_resolution_clock::now();
#endif       
                    move = false;
                }
                // text handling
                if (mouseOnText)
                {
                    // Set the window's cursor to the I-Beam
                    windowSetMouseCursorIBeam();

                    // Get char pressed (unicode character) on the queue
                    int key = windowGetCharPressed();

                    // Check if more characters have been pressed on the same frame
                    while (key > 0)
                    {
                        // NOTE: Only allow keys in range [32..125]
                        if ((key >= 32) && (key <= 125) && (letterCount < MAX_INPUT_CHARS))
                        {
                            textmessage[letterCount] = (char)key;
                            textmessage[letterCount + 1] = '\0'; // Add null terminator at the end of the string.
                            letterCount++;
                        }

                        key = windowGetCharPressed();  // Check next character in the queue
                    }

                    if (windowIsKeyPressedBackSpace())
                    {
                        letterCount--;
                        if (letterCount < 0) letterCount = 0;
                        textmessage[letterCount] = '\0';
                    }
                    if (windowIsKeyReleasedEnter())
                    {
                        text = true;
                        // append locally first the broadcast
                        gui_textmessagesdisplay.pop_back();
                        gui_textmessagesdisplay.push_front(TextContext(false, getCarColorString(), std::string(textmessage), gui_timestamp));
                    }
                }
                else
                {
                    windowSetMouseCursorDefault();
                }
                if (mouseOnText) framesCounter++;
                else framesCounter = 0;
                if (text)
                {
                    session->sendTextMessage("", "", std::string(textmessage)); // global is true, default current color
                    text = false;
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
                drawChatSendBox(mouseOnText, textmessage);
                drawSendTextButton();
                if (mouseOnText)
                {
                    if (letterCount < MAX_INPUT_CHARS)
                    {
                        // Draw blinking underscore char
                        drawChatSendBoxBlinkingUnderscore(framesCounter, textmessage);
                    }
                }
                int idx = 0;
                for (const TextContext &context: gui_textmessagesdisplay)
                {
                    drawTextLine(idx, context);
                    idx++;
                }
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
#ifdef TIMING_BENCHMARK
    timingReport.close();
#endif
    return 0;
}
