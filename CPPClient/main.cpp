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
#include <simdjson.h>

// timing benchmark
//#define TIMING_BENCHMARK

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
static constexpr int MAX_BATCHED_POSITIONS_THRESHOLD = 2;
ThreadSafeQueue<std::string> wsUpdatedJsonQueue;
ThreadSafeQueue<std::string> guiJsonQueue;
ThreadSafeQueue<std::string> positionJsonQueue;
std::atomic<bool> g_gameRunning = false;



// Handler classes
//----------------------------------------------------------------------------------

class MessageRelay
{
public:
    MessageRelay() { };
    ~MessageRelay() { };

    void relayWorkerThread() {
        simdjson::ondemand::parser onDemandTypeParser;
        simdjson::ondemand::document onDemanddoc;
        simdjson::ondemand::object onDemandObject;
        uint64_t type;
        do {
            std::string& jsondata = wsUpdatedJsonQueue.front(); // no copy, just a reference
            auto tmpJson = simdjson::padded_string(jsondata);
            if (jsondata.length() > 0)
            {
                onDemanddoc = onDemandTypeParser.iterate(tmpJson);
                auto error = onDemanddoc["Type"].get(type);
                if (!error)
                {
                    //int type = onDemanddoc.get_object()["Type"].get_uint64();
                    if ((BEventType::BEventPositionUpdateMessage == type) || (BEventType::BEventPositionDebugUpdateMessage == type))
                    {
                        positionJsonQueue.push(std::move(jsondata)); // move the data to the position queue
                    }
                    else
                    {
                        guiJsonQueue.push(std::move(jsondata)); // move the data to the other general queue
                    }
                }
                else
                {
                    std::cout << "relayWorkerThread() out of range: " << std::endl;
                }
            }
            wsUpdatedJsonQueue.pop();
        } while (g_gameRunning);
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
        // notify can get new data
        wsUpdatedJsonQueue.push("" + msg); // must be rvalue 
    }


    simdjson::dom::object m_parsedJson;
    simdjson::dom::parser m_jsonParser;
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
                    if (!positionJsonQueue.isEmpty()) // position update queue, get fifo
                    {
                        // online json parser
                        auto error = listener.m_jsonParser.parse(positionJsonQueue.front()).get(listener.m_parsedJson);
                        if (!error)
                        {
                            simdjson::dom::object& parsedJson = listener.m_parsedJson;
                            try {
                                int type = listener.m_parsedJson["Type"].get_uint64();
                                switch (type)
                                {
                                    case BEventType::BEventPositionUpdateMessage:
                                    {
                                        std::string uuidOfPlayerCar = std::string{ parsedJson["UUID"].get_string().value() };
                                        if (0 == gui_externalplayers.count(uuidOfPlayerCar))
                                        {
                                            // new player
                                            gui_externalplayers.emplace(uuidOfPlayerCar, CarContext(parsedJson["X"].get_int64(), parsedJson["Y"].get_int64(), std::string{ parsedJson["Color"].get_string().value() }));
                                            // need to send new player our own position, because client owns position
                                            move = true;
                                        }
                                        else
                                        {
                                            gui_externalplayers.at(uuidOfPlayerCar).m_coords.m_X = parsedJson["X"].get_int64();
                                            gui_externalplayers.at(uuidOfPlayerCar).m_coords.m_Y = parsedJson["Y"].get_int64();
                                            gui_externalplayers.at(uuidOfPlayerCar).m_color = std::string{ parsedJson["Color"].get_string().value() };
                                        }
                                    }
                                    break;
                                    case BEventType::BEventPositionDebugUpdateMessage:
                                    {
                                        std::string uuidOfPlayerCar = std::string{ parsedJson["UUID"].get_string().value() };
                                        if (0 == gui_externalplayers.count(uuidOfPlayerCar))
                                        {
                                            // new player
                                            gui_externalplayers.emplace(uuidOfPlayerCar, CarContext(parsedJson["X"].get_int64(), parsedJson["Y"].get_int64(), std::string{ parsedJson["Color"].get_string().value() }));
                                            // need to send new player our own position, because client owns position
                                            move = true;
                                        }
                                        else
                                        {
                                            gui_externalplayers.at(uuidOfPlayerCar).m_coords.m_X = parsedJson["X"].get_int64();
                                            gui_externalplayers.at(uuidOfPlayerCar).m_coords.m_Y = parsedJson["Y"].get_int64();
                                            gui_externalplayers.at(uuidOfPlayerCar).m_color = std::string{ parsedJson["Color"].get_string().value() };
                                        }
    #ifdef TIMING_BENCHMARK
                                        stop2 = std::chrono::high_resolution_clock::now();
                                        auto duration0 = std::chrono::duration_cast<std::chrono::microseconds>(stop0 - start);
                                        auto duration1 = std::chrono::duration_cast<std::chrono::microseconds>(stop1 - start);
                                        auto duration2 = std::chrono::duration_cast<std::chrono::microseconds>(stop2 - start);
                                        // To get the value of duration use the count()
                                        // member function on the duration object
                                        timingReport << "Loopback: sendpos dur0: " << duration0.count() << " [msec]" << std::endl;
                                        timingReport << "Loopback: diff dur0 dur1: " << duration1.count() - duration0.count() << " [msec]" << std::endl;
                                        timingReport << "Loopback: Move time after json parse dur2: " << duration2.count() << " [msec]" << std::endl;
    #endif
                                    }
                                    break;
                                }
                            }
                            catch (std::out_of_range& e)
                            {
                                std::cout << "positionJsonQueue Update out of range: " << e.what() << std::endl;
                            }
                    }
                    else
                    {
                        std::cout << "error parsing json: " << error << std::endl;
                    }
                    positionJsonQueue.pop();
                }

                if (!guiJsonQueue.isEmpty() && (positionJsonQueue.getSize() <= MAX_BATCHED_POSITIONS_THRESHOLD)) // gui update queue, get fifo, if position queue small
                {
                    // online json parser
                    auto error = listener.m_jsonParser.parse(guiJsonQueue.front()).get(listener.m_parsedJson);
                    if (!error)
                    {
                        simdjson::dom::object &parsedJson = listener.m_parsedJson;
                        try {
                            int type = listener.m_parsedJson["Type"].get_uint64();
                            switch (type)
                            {
                            case BEventType::BEventTimeStampMessage:
                                {
                                    gui_timestamp = std::string{ parsedJson["TimeStamp"].get_string().value() };
                                }
                                break;
                                case BEventType::BEventColorUpdateMessage:
                                {
                                    std::string uuidOfPlayerCar = std::string{ parsedJson["UUID"].get_string().value() };
                                    if (gui_externalplayers.count(uuidOfPlayerCar))
                                    {
                                        gui_externalplayers.at(uuidOfPlayerCar).m_color = std::string{ parsedJson["Color"].get_string().value() };
                                    }
                                }
                                break;
                                case BEventType::BEventExternalConnectionExitMessage:
                                {
                                    std::string uuidOfPlayerCar = std::string{ parsedJson["UUID"].get_string().value() };
                                    if (gui_externalplayers.count(uuidOfPlayerCar))
                                    {
                                        gui_externalplayers.erase(uuidOfPlayerCar);
                                    }
                                }
                                break;
                                case BEventType::BEventTextUpdateMessage:
                                {
                                    std::string uuidOfSender = std::string{ parsedJson["FromUUID"].get_string().value() };
                                    // sender is not current player
                                    if (uuidOfSender != session->getClientUUID())
                                    {
                                        // only display last 5 messages for now...
                                        std::string senderColor = std::string{ parsedJson["Color"].get_string().value() };
                                        std::string senderText = std::string{ parsedJson["Text"].get_string().value() };
                                        std::string senderTimeStamp = std::string{ parsedJson["TimeStamp"].get_string().value() };
                                        gui_textmessagesdisplay.pop_back();
                                        gui_textmessagesdisplay.push_front(TextContext(true, senderColor, senderText, senderTimeStamp));
                                    }
                                }
                                break;
                            }
                        }
                        catch (std::out_of_range& e)
                        {
                            std::cout << "guiJsonQueue Update out of range: " << e.what() << std::endl;
                        }
                    }
                    else
                    {
                        std::cout << "error parsing json: " << error << std::endl;
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
                if (positionJsonQueue.getSize() > MAX_BATCHED_POSITIONS_THRESHOLD)
                {
                    // batching losing a few is not an issue since positions are pixel updates
                    positionJsonQueue.pop();
                    positionJsonQueue.pop();
                    // skip drawing text if behind on new positions to update
                    continue;
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
#ifdef TIMING_BENCHMARK
                stop1 = std::chrono::high_resolution_clock::now();
#endif
            }
            session->closeConnection();
            g_gameRunning = false;
            wsUpdatedJsonQueue.push(""); // get out of deadlock
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
