#include "threadSafeQueue.hpp"
#include "postRequest.hpp"

// raylib has to be included in cpp file to avoid name conflicts with windows.h
#if defined(WIN32)           
#define NOGDI             // All GDI defines and routines
#define NOUSER            // All USER defines and routines
#endif

#include "websocketConnect.hpp"

#if defined(WIN32)           // raylib uses these names as function parameters
#undef near
#undef far
#endif

#include "event.hpp"
#include "windowContext.hpp"
#include "carClass.hpp"
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

// game states
typedef enum {
    STATE_MENU,
    STATE_LOBBY,
    STATE_COUNTDOWN,
    STATE_RACING,
    STATE_RESULTS,
    STATE_GAME_OVER
} GameState;

// cursor states
typedef enum {
    STATE_CURSOR_ENABLED,
    STATE_CURSOR_DISABLED,
} CursorState;

// Globals
static constexpr float SCALEFACTOR = 1000.0f;
static constexpr int MAX_DISPLAYED_TEXT_MESSAGES = 5;
static constexpr int MAX_INPUT_CHARS = 105;
static constexpr int MAX_BATCHED_POSITIONS_THRESHOLD = 2;
static constexpr int PERIODIC_POSITION_BATCH_HANDLING_MS = 1500;
ThreadSafeQueue<std::string> wsUpdatedJsonQueue;
ThreadSafeQueue<std::string> guiJsonQueue;
ThreadSafeQueue<std::string> positionJsonQueue;
std::atomic<bool> g_gameRunning = false;
std::atomic<bool> g_handleBatch = false;
bool g_in_state_transition = false;
GameState currentGameState = STATE_LOBBY;
CursorState currentCursorState = STATE_CURSOR_ENABLED;

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

    void relayBatchHandlerThread() {
        boost::this_thread::sleep_for(boost::chrono::milliseconds(PERIODIC_POSITION_BATCH_HANDLING_MS));
        g_handleBatch = true;
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
    SetTargetFPS(60);

    // Initialize Handler Classes
    //--------------------------------------------------------------------------------------
    MessageRelay relay;
    ConnectionListener listener;
    std::function<void(const std::string&)> stdf_message = [&](const std::string& s) { listener.onMessage(s); };
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
            int letterIdx = 0;
            bool playerInRacePortal = false;
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
            int g_SafetyX = 0;
            int g_SafetyY = 0;
            //----------------------------------------------------------------------------------
            // End Initialize gui variables here


            // Initialize model/3d variables here
            //----------------------------------------------------------------------------------
            // Define the camera to look into our 3d world
            Camera camera = { 0 };
            camera.position = { 0.2f, 0.4f, 0.2f };    // Camera position
            camera.target = { 0.185f, 0.4f, 0.0f };    // Camera looking at point
            camera.up = { 0.0f, 1.0f, 0.0f };          // Camera up vector (rotation towards target)
            camera.fovy = 45.0f;                                // Camera field-of-view Y
            camera.projection = CAMERA_PERSPECTIVE;             // Camera projection type
            Vector3 oldCamPos;
            Vector3 position = { 0.0f, 0.0f, 0.0f };            // Set model position
            float playerRadius = 0.1f;  // Collision radius (player is modelled as a cilinder for collision)
            Vector2 playerPos;

            const char* pBuf = GetApplicationDirectory();
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

#if defined(WIN32)  
            Image imMap = LoadImage(std::string(exePath + "resources\\cubicmap.png").c_str());      // Load cubicmap image (RAM)
#else
            Image imMap = LoadImage(std::string(exePath + "resources/cubicmap.png").c_str());
#endif
            Texture2D cubicmap = LoadTextureFromImage(imMap);       // Convert image to texture to display (VRAM)
            Mesh mesh = GenMeshCubicmap(imMap, { 1.0f, 1.0f, 1.0f });
            Model model = LoadModelFromMesh(mesh);

            // NOTE: By default each cube is mapped to one part of texture atlas
#if defined(WIN32)  
            Texture2D texture = LoadTexture(std::string(exePath + "resources\\cubicmap_atlas.png").c_str());    // Load map texture
#else
            Texture2D texture = LoadTexture(std::string(exePath + "resources/cubicmap_atlas.png").c_str());
#endif
            model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texture;    // Set map diffuse texture

            // Get map image data to be used for collision detection
            Color* mapPixels = LoadImageColors(imMap);
            UnloadImage(imMap);             // Unload image from RAM

            // Create a RenderTexture2D to be used for render to texture
            RenderTexture2D target3DArea = LoadRenderTexture(windowScreenWidth(), windowYBoundary());

#if defined(WIN32)  
            Model carModel = LoadModel(std::string(exePath + "resources\\raceFuture.obj").c_str());
#else
            Model carModel = LoadModel(std::string(exePath + "resources/raceFuture.obj").c_str());
#endif

            Vector3 mapPosition = { -16.0f, 0.0f, -8.0f };  // Set model position
            //----------------------------------------------------------------------------------
            // End Initialize model/3d variables here

            // Main game loop
            g_gameRunning = true;
            std::vector<std::thread> m_threadList;
            m_threadList.push_back(std::thread(&MessageRelay::relayWorkerThread, relay));
            m_threadList.push_back(std::thread(&MessageRelay::relayBatchHandlerThread, relay));
            // Main game loop
            while (!WindowShouldClose() && g_gameRunning)   // Detect window close button or ESC key
            {
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
                        simdjson::dom::object& parsedJson = listener.m_parsedJson;
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

                // Update and input handling
                //----------------------------------------------------------------------------------
                switch (currentGameState)
                {
                    case STATE_RACING:
                        oldCamPos = camera.position;    // Store old camera position

                        UpdateCameraPro(&camera,
                            {
                            (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) * 0.1f -      // Move forward-backward
                                (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) * 0.1f,
                                (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) * 0.1f -   // Move right-left
                                (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) * 0.1f,
                                0.0f                                                // Move up-down
                        },
                            {
                                GetMouseDelta().x * 0.05f,                            // Rotation: yaw
                                GetMouseDelta().y * 0.05f,                            // Rotation: pitch
                                0.0f                                                // Rotation: roll
                        },
                                0.0f);                              // Move to target (zoom) constant
                        
                        if ((oldCamPos.x != camera.position.x) || (oldCamPos.z != camera.position.z))
                        {
                            move = true;
                        }

                        // Check player collision (we simplify to 2D collision detection)
                        playerPos = { camera.position.x, camera.position.z };

                        g_SafetyX = (int)(playerPos.x - mapPosition.x + 0.5f);
                        g_SafetyY = (int)(playerPos.y - mapPosition.z + 0.5f);

                        // Out-of-limits security check
                        if (g_SafetyX < 0) g_SafetyX = 0;
                        else if (g_SafetyX >= cubicmap.width) g_SafetyX = cubicmap.width - 1;

                        if (g_SafetyY < 0) g_SafetyY = 0;
                        else if (g_SafetyY >= cubicmap.height) g_SafetyY = cubicmap.height - 1;

                        g_X = (playerPos.x)* SCALEFACTOR;
                        g_Y = (playerPos.y)* SCALEFACTOR;

                        // Check map collisions using image data and player position
                        // TODO: Improvement: Just check player surrounding cells for collision
                        for (int y = 0; y < cubicmap.height; y++)
                        {
                            for (int x = 0; x < cubicmap.width; x++)
                            {
                                if ((mapPixels[y * cubicmap.width + x].r == 255) &&       // Collision: white pixel, only check R channel
                                    (CheckCollisionCircleRec(playerPos, playerRadius,
                                        {
                                    mapPosition.x - 0.5f + x * 1.0f, mapPosition.z - 0.5f + y * 1.0f, 1.0f, 1.0f
                                        })))
                                {
                                    // Collision detected, reset camera position
                                    camera.position = oldCamPos;
                                }
                            }
                        }
                        if (windowIsKeyOnlyPressed(KEY_C))
                        {
                            if (STATE_CURSOR_DISABLED == currentCursorState)
                            {
                                EnableCursor();
                                currentCursorState = STATE_CURSOR_ENABLED;
                            }
                            else
                            {
                                DisableCursor();
                                currentCursorState = STATE_CURSOR_DISABLED;
                            }
                        }
                        if (!mouseOnText && windowIsKeyOnlyPressed(KEY_LEFT_SHIFT))
                        {
                            g_in_state_transition = true;
                            currentGameState = STATE_LOBBY;
                            currentCursorState = STATE_CURSOR_ENABLED;
                            g_X = 0;
                            g_Y = 125;
                        }
                        break;
                    case STATE_LOBBY:
                        if (windowIsKeyPressedUp() || windowIsKeyPressed(KEY_W))
                        {
                            if (g_Y > 1)
                            {
                                g_Y -= 2;
                                move = true;
                            }
                        }
                        if (windowIsKeyPressedDown() || windowIsKeyPressed(KEY_S))
                        {
                            if (g_Y < (windowYBoundary() - getCarHeight() - 1))
                            {
                                g_Y += 2;
                                move = true;
                            }
                        }
                        if (windowIsKeyPressedLeft() || windowIsKeyPressed(KEY_A))
                        {
                            if (g_X > 1)
                            {
                                g_X -= 2;
                                move = true;
                            }
                        }
                        if (windowIsKeyPressedRight() || windowIsKeyPressed(KEY_D))
                        {
                            if (g_X < (windowScreenWidth() - getCarWidth() - 1))
                            {
                                g_X += 2;
                                move = true;
                            }
                        }
                        
                        // portal handling
                        if (windowIsPlayerCollidesRacePortal(g_X, g_Y))
                        {
                            playerInRacePortal = true;
                        }
                        else
                        {
                            playerInRacePortal = false;
                        }
                        if (!mouseOnText && playerInRacePortal && windowIsKeyOnlyPressed(KEY_E))
                        {
                            g_in_state_transition = true;
                            currentGameState = STATE_RACING;
                            currentCursorState = STATE_CURSOR_DISABLED;
                        }
                        break;
                }
                if ((currentGameState == STATE_LOBBY) || ((currentCursorState == STATE_CURSOR_ENABLED) && (currentGameState == STATE_RACING)))
                {
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

                //----------------------------------------------------------------------------------
                // End Update and input handling

                // State transition
                //----------------------------------------------------------------------------------
                if (g_in_state_transition)
                {
                    // one-time use variables
                    if (STATE_CURSOR_ENABLED == currentCursorState)
                    {
                        EnableCursor();
                    }
                    else
                    {
                        DisableCursor(); // Limit cursor to relative movement inside the window
                    }
                    g_in_state_transition = false;
                }
                //----------------------------------------------------------------------------------
                // End State transition

                // Draw
                //----------------------------------------------------------------------------------
                switch (currentGameState)
                {
                    case STATE_RACING:
                        BeginTextureMode(target3DArea);       // Enable drawing to texture
                        ClearBackground(RAYWHITE);
                        BeginMode3D(camera);
                        DrawModel(model, mapPosition, 1.0f, WHITE);                     // Draw map
                        for (auto coords = gui_externalplayers.begin(); coords != gui_externalplayers.end(); coords++)
                        {
                            DrawCube({ static_cast<float>(coords->second.m_coords.m_X)/SCALEFACTOR, 0.0f, static_cast<float>(coords->second.m_coords.m_Y)/ SCALEFACTOR }, 0.5f, 1.0f, 0.5f, GetColor(colorHexToString(coords->second.m_color)));
                        }
                        DrawModel(carModel, { 0.0f, 0.0f, 0.0f }, 0.5f, GRAY);
                        EndMode3D();
                        EndTextureMode();

                        BeginDrawing();

                        ClearBackground(RAYWHITE);
                        DrawFPS(10, 10);
                        DrawTextureRec(target3DArea.texture, { 0, 0, (float)target3DArea.texture.width, (float)-target3DArea.texture.height }, { 0, 0 }, WHITE);

                        DrawRectangle(1350, 5, 245, 115, Fade(SKYBLUE, 0.45f));
                        DrawRectangleLines(1350, 5, 245, 115, DARKBLUE);

                        // draw camera player status
                        DrawText("Camera status:", 1360, 15, 12, BLACK);
                        DrawText("Use C Key to toggle cursor", 1360, 30, 12, BLACK);
                        DrawText("Use Left Shift Key to return to lobby", 1360, 45, 12, BLACK);
                        DrawText(TextFormat("- Projection: %s", (camera.projection == CAMERA_PERSPECTIVE) ? "PERSPECTIVE" :
                            (camera.projection == CAMERA_ORTHOGRAPHIC) ? "ORTHOGRAPHIC" : "CUSTOM"), 1360, 60, 12, BLACK);
                        DrawText(TextFormat("- Position: (%06.3f, %06.3f, %06.3f)", camera.position.x, camera.position.y, camera.position.z), 1360, 75, 12, BLACK);
                        DrawText(TextFormat("- Target: (%06.3f, %06.3f, %06.3f)", camera.target.x, camera.target.y, camera.target.z), 1360, 90, 12, BLACK);
                        DrawText(TextFormat("- Up: (%06.3f, %06.3f, %06.3f)", camera.up.x, camera.up.y, camera.up.z), 1360, 105, 12, BLACK);

                        // TODO: make function
                        drawFadeBackgroundLowerBox();
                        drawDefaultSquaresColor();
                        if (g_handleBatch && (positionJsonQueue.getSize() > MAX_BATCHED_POSITIONS_THRESHOLD))
                        {
                            // batching losing a few is not an issue since positions are pixel updates
                            for (int i = 0; i < positionJsonQueue.getSize() - 1; i++)
                            {
                                positionJsonQueue.pop();
                            }
                            g_handleBatch = false;
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
                        letterIdx = 0;
                        for (const TextContext& context : gui_textmessagesdisplay)
                        {
                            drawTextLine(letterIdx, context);
                            letterIdx++;
                        }
                        drawEscButton();
                        EndDrawing();
                        break;
                    case STATE_LOBBY:
                        BeginDrawing();
                        windowDrawBackground();
                        for (auto coords = gui_externalplayers.begin(); coords != gui_externalplayers.end(); coords++)
                        {
                            drawCar(coords->second.m_coords.m_X, coords->second.m_coords.m_Y, colorHexToString(coords->second.m_color));
                        }
                        drawCar(g_X, g_Y);
                        
                        // TODO: make function
                        drawFadeBackgroundLowerBox();
                        drawDefaultSquaresColor();
                        if (g_handleBatch && (positionJsonQueue.getSize() > MAX_BATCHED_POSITIONS_THRESHOLD))
                        {
                            // batching losing a few is not an issue since positions are pixel updates
                            for (int i = 0; i < positionJsonQueue.getSize() - 1; i++)
                            {
                                positionJsonQueue.pop();
                            }
                            g_handleBatch = false;
                            // skip drawing text if behind on new positions to update
                            continue;
                        }
                        drawTextTestBox(gui_timestamp);
                        drawChatBoxContainer();
                        drawChatSendBox(mouseOnText, textmessage);
                        drawSendTextButton();
                        drawPortalRectangles(g_X, g_Y);
                        drawPortalRaceInfoPane(playerInRacePortal);
                        if (mouseOnText)
                        {
                            if (letterCount < MAX_INPUT_CHARS)
                            {
                                // Draw blinking underscore char
                                drawChatSendBoxBlinkingUnderscore(framesCounter, textmessage);
                            }
                        }
                        letterIdx = 0;
                        for (const TextContext& context : gui_textmessagesdisplay)
                        {
                            drawTextLine(letterIdx, context);
                            letterIdx++;
                        }
                        drawEscButton();
                        EndDrawing();
                        break;
                }
                //----------------------------------------------------------------------------------
                // End Draw
#ifdef TIMING_BENCHMARK
                stop1 = std::chrono::high_resolution_clock::now();
#endif
            }

            // De-Initialization
            //--------------------------------------------------------------------------------------
            UnloadImageColors(mapPixels);   // Unload color array

            UnloadTexture(cubicmap);        // Unload cubicmap texture
            UnloadTexture(texture);         // Unload map texture
            UnloadModel(model);             // Unload map model
            UnloadModel(carModel);          // Unload car model

            session->closeConnection();
            g_gameRunning = false;
            wsUpdatedJsonQueue.push(""); // get out of deadlock
            for (auto& t : m_threadList) t.join();
        }
    }
    else
    {
        std::cout << "Error: couldn't login, http status code: " << statusCode << std::endl;
    }
    CloseWindow();
#ifdef TIMING_BENCHMARK
    timingReport.close();
#endif
    return 0;
}

