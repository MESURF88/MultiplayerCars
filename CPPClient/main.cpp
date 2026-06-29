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
#include "courseMap.hpp"
#include "raceSession.hpp"
#include <functional>
#include <iostream>
#include <fstream>
#include <thread>
#include <mutex>
#include <string>
#include <chrono>
#include <vector>
#include <algorithm>
#include <cmath>
#include <simdjson.h>

// timing benchmark
//#define TIMING_BENCHMARK

#ifdef TIMING_BENCHMARK
std::chrono::time_point<std::chrono::high_resolution_clock> start;
std::chrono::time_point<std::chrono::high_resolution_clock> stop0;
std::chrono::time_point<std::chrono::high_resolution_clock> stop1;
std::chrono::time_point<std::chrono::high_resolution_clock> stop2;
#endif
#include <rcamera.h>
#include <rlgl.h>

// game states
enum class GameState {
    STATE_MENU,
    STATE_LOBBY,
    STATE_COUNTDOWN,
    STATE_RACING,
    STATE_RESULTS,
    STATE_GAME_OVER
};

// cursor states
enum class CursorState {
    STATE_CURSOR_ENABLED,
    STATE_CURSOR_DISABLED,
};

// accel states
enum class DriveState {
    STATE_DRIVE_IDLE,
    STATE_DRIVE_FORWARD,
    STATE_DRIVE_BACKWARD,
};

// Globals
static constexpr float SCALEFACTOR = 1000.0f;
static constexpr float CAR_ANGLE_ADJUSTMENT = 90.0f;
static constexpr float CAMERA_MOUSE_MOVE_SENSITIVITY = 4.5f;
static constexpr float CAMERA_KEY_LOOK_SPEED = 90.0f;
static constexpr float CAMERA_MAX_MOUSE_DELTA = 80.0f;
static constexpr float CAMERA_MOUSE_DEADZONE = 0.05f;
static constexpr float INIT_CAR_SPEED = 2.0f;
static constexpr float INIT_CAR_TURN_SPEED = 80.0f;
static constexpr float THRESHOLD_CAR_SPEED1 = 4.0f;
static constexpr float THRESHOLD_CAR_SPEED2 = 7.0f;
static constexpr float MAX_CAR_SPEED = 8.0f;
static constexpr float DRS_MAX_CAR_SPEED = 15.0f;
static constexpr float DRS_MIN_READY_SPEED = 7.0f;
static constexpr float DISPLAY_TOP_SPEED_KMH = 200.0f;
static constexpr float DRS_DURATION_SECONDS = 0.45f;
static constexpr float DRS_ACCELERATION_MULTIPLIER = 1.35f;
static constexpr float OVERSPEED_DECELERATION = 4.0f;
static constexpr float MAX_REVERSE_CAR_SPEED = 4.0f;
static constexpr float CAR_FORWARD_ACCELERATION = 5.5f;
static constexpr float CAR_REVERSE_ACCELERATION = 3.5f;
static constexpr float CAR_BRAKE_DECELERATION = 10.0f;
static constexpr float CAR_COAST_DECELERATION = 3.0f;
static constexpr float CAR_STOP_EPSILON = 0.05f;
static constexpr float MIN_CAR_TURN_SPEED = 50.0f;
static constexpr float TURN_SPEED_DROP_START = 3.0f;
static constexpr float CAMERA_TURN_RESISTANCE_SPEED_START = 6.5f;
static constexpr float CAMERA_TURN_RESISTANCE_START_DEGREES = 20.0f;
static constexpr float CAMERA_TURN_RESISTANCE_FULL_DEGREES = 120.0f;
static constexpr float CAMERA_TURN_RESISTANCE_MIN_FACTOR = 0.25f;
static constexpr bool ENABLE_COURSE_WALLS = true;
static constexpr bool ENABLE_MAP_COLLISION = false;
static constexpr float GROUND_PLANE_SIZE = 8192.0f;
static constexpr float GROUND_TEXTURE_REPEATS = 1536.0f;
static constexpr float CAMERA_FOLLOW_DISTANCE = 6.0f;
static constexpr float CAMERA_TARGET_HEIGHT = 0.55f;
static constexpr float CAMERA_MIN_PITCH = 8.0f;
static constexpr float CAMERA_MAX_PITCH = 55.0f;
static constexpr float CAMERA_START_PITCH = 20.0f;
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
GameState currentGameState = GameState::STATE_LOBBY;
CursorState currentCursorState = CursorState::STATE_CURSOR_ENABLED;
DriveState currentDriveState = DriveState::STATE_DRIVE_IDLE;
DriveState prevDriveState = DriveState::STATE_DRIVE_IDLE;

static std::string jsonSnippet(const std::string& jsonText)
{
    static constexpr std::size_t MAX_JSON_LOG_CHARS = 500;
    if (jsonText.size() <= MAX_JSON_LOG_CHARS)
    {
        return jsonText;
    }
    return jsonText.substr(0, MAX_JSON_LOG_CHARS) + "...";
}

static void logJsonPayloadError(const std::string& sourceName, const std::string& jsonText, const std::string& details)
{
    std::cout << "JSON parse error in " << sourceName << std::endl;
    std::cout << "  details: " << details << std::endl;
    std::cout << "  payload: " << jsonSnippet(jsonText) << std::endl;
}

static float shortestAngleDifference(float fromDegrees, float toDegrees)
{
    float difference = fmodf((toDegrees - fromDegrees) + 540.0f, 360.0f) - 180.0f;
    return difference;
}

static float normalizeAngleDegrees(float angle)
{
    angle = fmodf(angle, 360.0f);
    if (angle < 0.0f) angle += 360.0f;
    return angle;
}

static float smoothstep(float edge0, float edge1, float value)
{
    if (edge0 == edge1) return (value < edge0) ? 0.0f : 1.0f;

    const float t = std::clamp((value - edge0) / (edge1 - edge0), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

static Vector2 directionFromAngle(float angleDegrees)
{
    const float angleRadians = angleDegrees * DEG2RAD;
    return { cosf(angleRadians), sinf(angleRadians) };
}

static void updateChaseCamera(Camera& camera, Vector2 carPosition, float cameraAngle, float cameraPitch)
{
    const Vector2 cameraDirection = directionFromAngle(cameraAngle);
    const float pitchRadians = cameraPitch * DEG2RAD;
    const float horizontalDistance = CAMERA_FOLLOW_DISTANCE * cosf(pitchRadians);

    camera.target = { carPosition.x, CAMERA_TARGET_HEIGHT, carPosition.y };
    camera.position = {
        carPosition.x - cameraDirection.x * horizontalDistance,
        CAMERA_TARGET_HEIGHT + sinf(pitchRadians) * CAMERA_FOLLOW_DISTANCE,
        carPosition.y - cameraDirection.y * horizontalDistance
    };
}

static bool colorMatches(Color actual, Color expected, int tolerance)
{
    return (std::abs(static_cast<int>(actual.r) - static_cast<int>(expected.r)) <= tolerance) &&
        (std::abs(static_cast<int>(actual.g) - static_cast<int>(expected.g)) <= tolerance) &&
        (std::abs(static_cast<int>(actual.b) - static_cast<int>(expected.b)) <= tolerance);
}

static int findCarPaintMaterialIndex(const Model& model)
{
    const Color originalPaintBlue = { 105, 122, 216, 255 };
    for (int i = 0; i < model.materialCount; ++i)
    {
        const Color diffuseColor = model.materials[i].maps[MATERIAL_MAP_DIFFUSE].color;
        if (colorMatches(diffuseColor, originalPaintBlue, 4))
        {
            return i;
        }
    }

    return 0;
}

static Color carPaintColorFromHexString(const std::string& colorHex)
{
    return GetColor(colorHexToString(colorHex));
}

static void applyCarPaintMaterial(Model& model, int paintMaterialIndex, const std::string& colorHex)
{
    if ((paintMaterialIndex < 0) || (paintMaterialIndex >= model.materialCount))
    {
        return;
    }

    model.materials[paintMaterialIndex].maps[MATERIAL_MAP_DIFFUSE].color = carPaintColorFromHexString(colorHex);
}

static void drawCarModelWithPaint(Model& model, int paintMaterialIndex, const std::string& colorHex, Vector3 position, float angle)
{
    applyCarPaintMaterial(model, paintMaterialIndex, colorHex);
    DrawModelEx(model, position, { 0.0f, 1.0f, 0.0f }, angle, { 0.5f, 0.5f, 0.5f }, WHITE);
}

static char pathSeparator()
{
#if defined(WIN32)
    return '\\';
#else
    return '/';
#endif
}

static bool fileExists(const std::string& path)
{
    std::ifstream file(path);
    return file.good();
}

static std::string trimTrailingSeparators(std::string path)
{
    while (!path.empty() && (path.back() == '/' || path.back() == '\\'))
    {
        path.pop_back();
    }
    return path;
}

static std::string directoryName(const std::string& path)
{
    const std::size_t separator = path.find_last_of("/\\");
    if (separator == std::string::npos)
    {
        return "";
    }
    return path.substr(0, separator);
}

static std::string joinPath(const std::string& left, const std::string& right)
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

static std::string appDirectory()
{
    const char* appDir = GetApplicationDirectory();
    if (appDir == nullptr)
    {
        return "";
    }
    return trimTrailingSeparators(std::string(appDir));
}

static std::string findResourcePath(const std::string& fileName)
{
    const std::string appDir = appDirectory();
    const std::string parentDir = directoryName(appDir);

    std::vector<std::string> candidates = {
        joinPath("CPPClient/resources", fileName),
        joinPath("resources", fileName)
    };

    if (!appDir.empty())
    {
        candidates.push_back(joinPath(joinPath(appDir, "resources"), fileName));
    }
    if (!parentDir.empty())
    {
        candidates.push_back(joinPath(joinPath(parentDir, "resources"), fileName));
    }

    for (const std::string& candidate : candidates)
    {
        if (fileExists(candidate))
        {
            std::cout << "Using resource " << candidate << std::endl;
            return candidate;
        }
    }

    std::cout << "error: could not find resource " << fileName << std::endl;
    return joinPath("resources", fileName);
}

static Texture2D loadTextureResource(const std::string& fileName)
{
    const std::string path = findResourcePath(fileName);
    Texture2D texture = LoadTexture(path.c_str());
    if (texture.id == 0)
    {
        std::cout << "error: failed to load texture resource " << path << std::endl;
    }
    return texture;
}

static Model loadModelResource(const std::string& fileName)
{
    const std::string path = findResourcePath(fileName);
    Model model = LoadModel(path.c_str());
    if (model.meshCount == 0)
    {
        std::cout << "error: failed to load model resource " << path << std::endl;
    }
    return model;
}

static void drawCenteredOutlinedText(const char* text, int centerY, int fontSize, Color color)
{
    const int textWidth = MeasureText(text, fontSize);
    const int x = (windowScreenWidth() - textWidth) / 2;
    const int y = centerY - (fontSize / 2);
    const int shadowOffset = std::max(2, fontSize / 18);

    DrawText(text, x + shadowOffset, y + shadowOffset, fontSize, Fade(BLACK, 0.75f));
    DrawText(text, x - 1, y, fontSize, Fade(BLACK, 0.65f));
    DrawText(text, x + 1, y, fontSize, Fade(BLACK, 0.65f));
    DrawText(text, x, y - 1, fontSize, Fade(BLACK, 0.65f));
    DrawText(text, x, y + 1, fontSize, Fade(BLACK, 0.65f));
    DrawText(text, x, y, fontSize, color);
}

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
            std::string jsondata = wsUpdatedJsonQueue.front();
            if (jsondata.length() > 0)
            {
                try
                {
                    auto tmpJson = simdjson::padded_string(jsondata);
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
                        logJsonPayloadError("websocket relay Type field", jsondata, simdjson::error_message(error));
                    }
                }
                catch (const simdjson::simdjson_error& e)
                {
                    logJsonPayloadError("websocket relay payload", jsondata, e.what());
                }
                catch (const std::exception& e)
                {
                    logJsonPayloadError("websocket relay payload", jsondata, e.what());
                }
            }
            wsUpdatedJsonQueue.pop();
        } while (g_gameRunning);
    }

    void relayBatchHandlerThread() {
        std::this_thread::sleep_for(std::chrono::milliseconds(PERIODIC_POSITION_BATCH_HANDLING_MS));
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
        m_Angle = 0.0;
    }
    int m_X;
    int m_Y;
    double m_Angle;
};

class CarContext
{
public:
    CarContext() {
        m_color = "ffffff";
    }
    CarContext(int X, int Y, double Angle, std::string color)
    {
        m_coords.m_X = X;
        m_coords.m_Y = Y;
        m_coords.m_Angle = Angle;
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
    std::string host = "multiplayercars.onrender.com";
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
            bool mouseLookEnabled = false;
            bool skipMouseLookFrame = true;
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
            float g_Angle = 0.0f;
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
            float playerRadius = 0.1f;  // Collision radius (player is modelled as a cilinder for collision)
            CourseMap raceCourse = loadCourseOrDefault(findResourcePath("courses/simple_circuit.json"));
            Vector2 carPosition = raceCourse.startPosition;
            Vector2 playerPos = { 0 };
            Vector3 rotation = { 0 };
            Vector2 mousePositionDelta = { 0 };
            float cameraAngle = raceCourse.startAngle;
            float cameraPitch = CAMERA_START_PITCH;
            float carAngle = raceCourse.startAngle;
            float carVelocity = 0.0f;
            float carTurnSpeed = INIT_CAR_TURN_SPEED;
            float drsTimer = 0.0f;
            RaceSession raceSession;
            resetRaceSession(raceSession, raceCourse);
            bool localRaceReady = false;
            int raceReadyCount = 0;
            int racePlayerCount = 0;
            playerPos = carPosition;
            updateChaseCamera(camera, carPosition, cameraAngle, cameraPitch);

            Image imMap = LoadImage(findResourcePath("cubicmap.png").c_str());      // Load cubicmap image (RAM)
            if (imMap.data == nullptr)
            {
                std::cout << "error: cubicmap.png failed to load; using empty fallback collision map" << std::endl;
                imMap = GenImageColor(32, 16, BLACK);
            }
            Texture2D cubicmap = LoadTextureFromImage(imMap);       // Convert image to texture to display (VRAM)
            Mesh mesh = GenMeshCubicmap(imMap, { 1.0f, 1.0f, 1.0f });
            Model model = LoadModelFromMesh(mesh);

            // NOTE: By default each cube is mapped to one part of texture atlas
            Texture2D texture = loadTextureResource("cubicmap_atlas.png");    // Load map texture
            model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texture;    // Set map diffuse texture

            // Get map image data to be used for collision detection
            Color* mapPixels = LoadImageColors(imMap);
            UnloadImage(imMap);             // Unload image from RAM

            // Create a RenderTexture2D to be used for render to texture
            RenderTexture2D target3DArea = LoadRenderTexture(windowScreenWidth(), windowYBoundary());

            Texture2D groundTexture = loadTextureResource("ground_texture.png");
            SetTextureWrap(groundTexture, TEXTURE_WRAP_REPEAT);
            Mesh groundMesh = GenMeshPlane(GROUND_PLANE_SIZE, GROUND_PLANE_SIZE, 64, 64);
            if (groundMesh.texcoords != nullptr)
            {
                for (int i = 0; i < groundMesh.vertexCount * 2; ++i)
                {
                    groundMesh.texcoords[i] *= GROUND_TEXTURE_REPEATS;
                }
            }
            Model groundModel = LoadModelFromMesh(groundMesh);
            groundModel.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = groundTexture;

            Model skyboxModel = LoadModelFromMesh(GenMeshCube(1.0f, 1.0f, 1.0f));
            Shader skyboxShader = LoadShader(
                findResourcePath("shaders/glsl330/skybox.vs").c_str(),
                findResourcePath("shaders/glsl330/skybox.fs").c_str());
            skyboxModel.materials[0].shader = skyboxShader;

            int environmentMap = MATERIAL_MAP_CUBEMAP;
            int doGamma = 0;
            int vflipped = 0;
            SetShaderValue(skyboxShader, GetShaderLocation(skyboxShader, "environmentMap"), &environmentMap, SHADER_UNIFORM_INT);
            SetShaderValue(skyboxShader, GetShaderLocation(skyboxShader, "doGamma"), &doGamma, SHADER_UNIFORM_INT);
            SetShaderValue(skyboxShader, GetShaderLocation(skyboxShader, "vflipped"), &vflipped, SHADER_UNIFORM_INT);

            Image skyboxImage = LoadImage(findResourcePath("skybox_texture.png").c_str());
            TextureCubemap skyboxCubemap = { 0 };
            if (skyboxImage.data != nullptr)
            {
                skyboxCubemap = LoadTextureCubemap(skyboxImage, CUBEMAP_LAYOUT_AUTO_DETECT);
                UnloadImage(skyboxImage);
            }

            if (skyboxCubemap.id == 0)
            {
                std::cout << "warning: skybox_texture.png is not a supported cubemap layout; using fallback sky color" << std::endl;
                Image fallbackSkybox = GenImageColor(1536, 256, SKYBLUE);
                skyboxCubemap = LoadTextureCubemap(fallbackSkybox, CUBEMAP_LAYOUT_LINE_HORIZONTAL);
                UnloadImage(fallbackSkybox);
            }
            skyboxModel.materials[0].maps[MATERIAL_MAP_CUBEMAP].texture = skyboxCubemap;

            Model carModel = loadModelResource("raceFuture.obj");
            int carPaintMaterialIndex = findCarPaintMaterialIndex(carModel);

            Vector3 mapPosition = { -16.0f, 0.0f, -8.0f };  // Set model position
            auto resetCarToCourseStart = [&]() {
                carPosition = raceCourse.startPosition;
                playerPos = carPosition;
                cameraAngle = raceCourse.startAngle;
                cameraPitch = CAMERA_START_PITCH;
                carAngle = raceCourse.startAngle;
                carVelocity = 0.0f;
                drsTimer = 0.0f;
                carTurnSpeed = INIT_CAR_TURN_SPEED;
                currentDriveState = DriveState::STATE_DRIVE_IDLE;
                g_X = static_cast<int>(carPosition.x * SCALEFACTOR);
                g_Y = static_cast<int>(carPosition.y * SCALEFACTOR);
                g_Angle = -carAngle + CAR_ANGLE_ADJUSTMENT;
                updateChaseCamera(camera, carPosition, cameraAngle, cameraPitch);
            };
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
                                    gui_externalplayers.emplace(uuidOfPlayerCar, CarContext(parsedJson["X"].get_int64(), parsedJson["Y"].get_int64(), parsedJson["Angle"].get_double(), std::string{ parsedJson["Color"].get_string().value() }));
                                    // need to send new player our own position, because client owns position
                                    move = true;
                                }
                                else
                                {
                                    gui_externalplayers.at(uuidOfPlayerCar).m_coords.m_X = parsedJson["X"].get_int64();
                                    gui_externalplayers.at(uuidOfPlayerCar).m_coords.m_Y = parsedJson["Y"].get_int64();
                                    gui_externalplayers.at(uuidOfPlayerCar).m_coords.m_Angle = parsedJson["Angle"].get_double();
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
                                    gui_externalplayers.emplace(uuidOfPlayerCar, CarContext(parsedJson["X"].get_int64(), parsedJson["Y"].get_int64(), parsedJson["Angle"].get_double(), std::string{ parsedJson["Color"].get_string().value() }));
                                    // need to send new player our own position, because client owns position
                                    move = true;
                                }
                                else
                                {
                                    gui_externalplayers.at(uuidOfPlayerCar).m_coords.m_X = parsedJson["X"].get_int64();
                                    gui_externalplayers.at(uuidOfPlayerCar).m_coords.m_Y = parsedJson["Y"].get_int64();
                                    gui_externalplayers.at(uuidOfPlayerCar).m_coords.m_Angle = parsedJson["Angle"].get_double();
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
                            std::cout << "  payload: " << jsonSnippet(positionJsonQueue.front()) << std::endl;
                        }
                        catch (const simdjson::simdjson_error& e)
                        {
                            logJsonPayloadError("positionJsonQueue field read", positionJsonQueue.front(), e.what());
                        }
                        catch (const std::exception& e)
                        {
                            logJsonPayloadError("positionJsonQueue field read", positionJsonQueue.front(), e.what());
                        }
                    }
                    else
                    {
                        logJsonPayloadError("positionJsonQueue", positionJsonQueue.front(), simdjson::error_message(error));
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
                            case BEventType::BEventRaceStartMessage:
                            {
                                const std::string courseId = std::string{ parsedJson["CourseID"].get_string().value() };
                                const int laps = static_cast<int>(parsedJson["Laps"].get_int64().value());
                                const std::int64_t startEpochMs = parsedJson["StartEpochMs"].get_int64().value();
                                if (courseId == raceCourse.courseId)
                                {
                                    resetCarToCourseStart();
                                    localRaceReady = false;
                                    raceReadyCount = 0;
                                    racePlayerCount = 0;
                                    scheduleRaceStart(raceSession, raceCourse, startEpochMs, laps);
                                    currentGameState = GameState::STATE_RACING;
                                    currentCursorState = CursorState::STATE_CURSOR_ENABLED;
                                    mouseLookEnabled = false;
                                    skipMouseLookFrame = true;
                                    g_in_state_transition = true;
                                }
                            }
                            break;
                            case BEventType::BEventRaceReadyMessage:
                            {
                                const std::string courseId = std::string{ parsedJson["CourseID"].get_string().value() };
                                if (courseId == raceCourse.courseId)
                                {
                                    const std::string readyUUID = std::string{ parsedJson["UUID"].get_string().value() };
                                    raceReadyCount = static_cast<int>(parsedJson["ReadyCount"].get_int64().value());
                                    racePlayerCount = static_cast<int>(parsedJson["PlayerCount"].get_int64().value());
                                    if (readyUUID == session->getClientUUID())
                                    {
                                        localRaceReady = parsedJson["Ready"].get_bool().value();
                                    }
                                }
                            }
                            break;
                            }
                        }
                        catch (std::out_of_range& e)
                        {
                            std::cout << "guiJsonQueue Update out of range: " << e.what() << std::endl;
                            std::cout << "  payload: " << jsonSnippet(guiJsonQueue.front()) << std::endl;
                        }
                        catch (const simdjson::simdjson_error& e)
                        {
                            logJsonPayloadError("guiJsonQueue field read", guiJsonQueue.front(), e.what());
                        }
                        catch (const std::exception& e)
                        {
                            logJsonPayloadError("guiJsonQueue field read", guiJsonQueue.front(), e.what());
                        }
                    }
                    else
                    {
                        logJsonPayloadError("guiJsonQueue", guiJsonQueue.front(), simdjson::error_message(error));
                    }
                    guiJsonQueue.pop();
                }

                //----------------------------------------------------------------------------------
                // End Update

                // Update and input handling
                //----------------------------------------------------------------------------------
                switch (currentGameState)
                {
                    case GameState::STATE_RACING:
                    {
                        const float frameTime = GetFrameTime();
                        const std::int64_t raceNowEpochMs = currentEpochMilliseconds();
                        updateRaceSession(raceSession, raceCourse, carPosition, playerRadius, raceNowEpochMs);
                        const bool raceDrivingEnabled = raceSessionCanDrive(raceSession);
                        const int throttleInput = raceDrivingEnabled ? (IsKeyDown(KEY_W) - IsKeyDown(KEY_S)) : 0;
                        if (!mouseOnText &&
                            (raceSession.state == RaceRunState::STATE_WAITING) &&
                            !localRaceReady &&
                            IsKeyPressed(KEY_SPACE))
                        {
                            localRaceReady = true;
                            raceReadyCount = std::max(raceReadyCount, 1);
                            racePlayerCount = std::max(racePlayerCount, static_cast<int>(gui_externalplayers.size()) + 1);
                            session->sendRaceReadyUpdate(raceCourse.courseId, true, true, raceCourse.lapCount);
                        }
                        if (raceDrivingEnabled && (carVelocity >= DRS_MIN_READY_SPEED) && IsKeyPressed(KEY_SPACE))
                        {
                            drsTimer = DRS_DURATION_SECONDS;
                        }

                        if (raceDrivingEnabled && (drsTimer > 0.0f))
                        {
                            drsTimer = std::max(0.0f, drsTimer - frameTime);
                        }
                        else if (!raceDrivingEnabled)
                        {
                            carVelocity = 0.0f;
                            drsTimer = 0.0f;
                            currentDriveState = DriveState::STATE_DRIVE_IDLE;
                        }

                        const bool drsActive = drsTimer > 0.0f;
                        const float maxForwardSpeed = drsActive ? DRS_MAX_CAR_SPEED : MAX_CAR_SPEED;

                        if (throttleInput > 0)
                        {
                            currentDriveState = DriveState::STATE_DRIVE_FORWARD;
                            if (carVelocity < 0.0f)
                            {
                                carVelocity += CAR_BRAKE_DECELERATION * frameTime;
                                if (carVelocity > 0.0f) carVelocity = 0.0f;
                            }
                            else
                            {
                                const float accelerationMultiplier = drsActive ? DRS_ACCELERATION_MULTIPLIER : 1.0f;
                                carVelocity += CAR_FORWARD_ACCELERATION * accelerationMultiplier * frameTime;
                            }
                        }
                        else if (throttleInput < 0)
                        {
                            currentDriveState = DriveState::STATE_DRIVE_BACKWARD;
                            if (carVelocity > 0.0f)
                            {
                                carVelocity -= CAR_BRAKE_DECELERATION * frameTime;
                                if (carVelocity < 0.0f) carVelocity = 0.0f;
                            }
                            else
                            {
                                carVelocity -= CAR_REVERSE_ACCELERATION * frameTime;
                            }
                        }
                        else
                        {
                            currentDriveState = DriveState::STATE_DRIVE_IDLE;
                            if (carVelocity > 0.0f)
                            {
                                carVelocity -= CAR_COAST_DECELERATION * frameTime;
                                if (carVelocity < 0.0f) carVelocity = 0.0f;
                            }
                            else if (carVelocity < 0.0f)
                            {
                                carVelocity += CAR_COAST_DECELERATION * frameTime;
                                if (carVelocity > 0.0f) carVelocity = 0.0f;
                            }
                        }

                        if (!drsActive && (carVelocity > MAX_CAR_SPEED))
                        {
                            carVelocity -= OVERSPEED_DECELERATION * frameTime;
                            if (carVelocity < MAX_CAR_SPEED) carVelocity = MAX_CAR_SPEED;
                        }

                        carVelocity = std::clamp(carVelocity, -MAX_REVERSE_CAR_SPEED, maxForwardSpeed);
                        if (std::abs(carVelocity) < CAR_STOP_EPSILON)
                        {
                            carVelocity = 0.0f;
                        }

                        const float absVelocity = std::abs(carVelocity);
                        const float turnSpeedDrop = smoothstep(TURN_SPEED_DROP_START, maxForwardSpeed, absVelocity);
                        carTurnSpeed = INIT_CAR_TURN_SPEED - ((INIT_CAR_TURN_SPEED - MIN_CAR_TURN_SPEED) * turnSpeedDrop);

                        rotation = { 0 };
                        mousePositionDelta = GetMouseDelta();
                        if (mouseLookEnabled && (CursorState::STATE_CURSOR_DISABLED == currentCursorState))
                        {
                            if (skipMouseLookFrame)
                            {
                                mousePositionDelta = { 0.0f, 0.0f };
                                skipMouseLookFrame = false;
                            }

                            mousePositionDelta.x = std::clamp(mousePositionDelta.x, -CAMERA_MAX_MOUSE_DELTA, CAMERA_MAX_MOUSE_DELTA);
                            mousePositionDelta.y = std::clamp(mousePositionDelta.y, -CAMERA_MAX_MOUSE_DELTA, CAMERA_MAX_MOUSE_DELTA);

                            if (std::abs(mousePositionDelta.x) > CAMERA_MOUSE_DEADZONE)
                            {
                                rotation.x += mousePositionDelta.x * CAMERA_MOUSE_MOVE_SENSITIVITY * GetFrameTime();
                            }
                            if (std::abs(mousePositionDelta.y) > CAMERA_MOUSE_DEADZONE)
                            {
                                rotation.y += mousePositionDelta.y * CAMERA_MOUSE_MOVE_SENSITIVITY * GetFrameTime();
                            }
                        }

                        rotation.x += (IsKeyDown(KEY_RIGHT) - IsKeyDown(KEY_LEFT)) * CAMERA_KEY_LOOK_SPEED * GetFrameTime();
                        rotation.y += (IsKeyDown(KEY_DOWN) - IsKeyDown(KEY_UP)) * CAMERA_KEY_LOOK_SPEED * GetFrameTime();

                        cameraAngle = normalizeAngleDegrees(cameraAngle + rotation.x);
                        cameraPitch = std::clamp(cameraPitch + rotation.y, CAMERA_MIN_PITCH, CAMERA_MAX_PITCH);

                        const float oldCarAngle = carAngle;
                        const Vector2 oldCarPosition = carPosition;
                        if (std::abs(carVelocity) > CAR_STOP_EPSILON)
                        {
                            const float steeringDirection = (carVelocity < 0.0f) ? -1.0f : 1.0f;
                            const float cameraOffset = std::abs(shortestAngleDifference(carAngle, cameraAngle));
                            const float resistanceRange = CAMERA_TURN_RESISTANCE_FULL_DEGREES - CAMERA_TURN_RESISTANCE_START_DEGREES;
                            const float cameraOffsetResistance = std::clamp((cameraOffset - CAMERA_TURN_RESISTANCE_START_DEGREES) / resistanceRange, 0.0f, 1.0f);
                            const float speedResistance = smoothstep(CAMERA_TURN_RESISTANCE_SPEED_START, maxForwardSpeed, absVelocity);
                            const float resistanceAmount = cameraOffsetResistance * speedResistance;
                            const float cameraTurnFactor = 1.0f - (resistanceAmount * (1.0f - CAMERA_TURN_RESISTANCE_MIN_FACTOR));
                            carAngle += (IsKeyDown(KEY_D) - IsKeyDown(KEY_A)) * carTurnSpeed * cameraTurnFactor * frameTime * steeringDirection;
                            carAngle = normalizeAngleDegrees(carAngle);
                        }

                        if (carVelocity > 0.0f) prevDriveState = DriveState::STATE_DRIVE_FORWARD;
                        else if (carVelocity < 0.0f) prevDriveState = DriveState::STATE_DRIVE_BACKWARD;

                        const Vector2 carDirection = directionFromAngle(carAngle);
                        carPosition.x += carDirection.x * carVelocity * frameTime;
                        carPosition.y += carDirection.y * carVelocity * frameTime;
                        playerPos = carPosition;
                        g_Angle = -carAngle + CAR_ANGLE_ADJUSTMENT;
                        updateRaceSession(raceSession, raceCourse, carPosition, playerRadius, raceNowEpochMs);

                        if ((oldCarPosition.x != carPosition.x) || (oldCarPosition.y != carPosition.y) || (oldCarAngle != carAngle))
                        {
                            move = true;
                        }

                        g_SafetyX = (int)(playerPos.x - mapPosition.x + 0.5f);
                        g_SafetyY = (int)(playerPos.y - mapPosition.z + 0.5f);

                        // Out-of-limits security check
                        if (g_SafetyX < 0) g_SafetyX = 0;
                        else if (g_SafetyX >= cubicmap.width) g_SafetyX = cubicmap.width - 1;

                        if (g_SafetyY < 0) g_SafetyY = 0;
                        else if (g_SafetyY >= cubicmap.height) g_SafetyY = cubicmap.height - 1;

                        g_X = (playerPos.x)* SCALEFACTOR;
                        g_Y = (playerPos.y)* SCALEFACTOR;

                        bool collisionDetected = false;
                        if (ENABLE_COURSE_WALLS && courseCollidesWithWalls(raceCourse, playerPos, playerRadius))
                        {
                            carPosition = oldCarPosition;
                            carVelocity = 0.0f;
                            drsTimer = 0.0f;
                            carTurnSpeed = INIT_CAR_TURN_SPEED;
                            currentDriveState = DriveState::STATE_DRIVE_IDLE;
                            collisionDetected = true;
                        }
                        if (ENABLE_MAP_COLLISION)
                        {
                            // Check map collisions using image data and player position.
                            // Disabled while using the open ground/skybox scene.
                            for (int y = 0; y < cubicmap.height; y++)
                            {
                                for (int x = 0; x < cubicmap.width; x++)
                                {
                                    if ((mapPixels[y * cubicmap.width + x].r == 255) &&
                                        (CheckCollisionCircleRec(playerPos, playerRadius,
                                            {
                                        mapPosition.x - 0.5f + x * 1.0f, mapPosition.z - 0.5f + y * 1.0f, 1.0f, 1.0f
                                            })))
                                    {
                                        carPosition = oldCarPosition;
                                        carVelocity = 0.0f;
                                        drsTimer = 0.0f;
                                        carTurnSpeed = INIT_CAR_TURN_SPEED;
                                        currentDriveState = DriveState::STATE_DRIVE_IDLE;
                                        collisionDetected = true;
                                    }
                                }
                            }
                        }
                        if (collisionDetected)
                        {
                            carAngle = oldCarAngle;
                            g_Angle = -carAngle + CAR_ANGLE_ADJUSTMENT;
                            playerPos = carPosition;
                            g_X = (playerPos.x) * SCALEFACTOR;
                            g_Y = (playerPos.y) * SCALEFACTOR;
                            move = true;
                        }
                        updateChaseCamera(camera, carPosition, cameraAngle, cameraPitch);
                        if (!mouseOnText && (IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT)) && IsKeyPressed(KEY_C))
                        {
                            if (CursorState::STATE_CURSOR_DISABLED == currentCursorState)
                            {
                                EnableCursor();
                                currentCursorState = CursorState::STATE_CURSOR_ENABLED;
                                mouseLookEnabled = false;
                            }
                            else
                            {
                                DisableCursor();
                                currentCursorState = CursorState::STATE_CURSOR_DISABLED;
                                mouseLookEnabled = true;
                                skipMouseLookFrame = true;
                            }
                        }
                        if (!mouseOnText && windowIsKeyOnlyPressed(KEY_LEFT_SHIFT))
                        {
                            if (raceSession.state == RaceRunState::STATE_WAITING)
                            {
                                session->sendRaceReadyUpdate(raceCourse.courseId, false, false, raceCourse.lapCount);
                            }
                            localRaceReady = false;
                            raceReadyCount = 0;
                            racePlayerCount = 0;
                            resetRaceSession(raceSession, raceCourse);
                            g_in_state_transition = true;
                            currentGameState = GameState::STATE_LOBBY;
                            currentCursorState = CursorState::STATE_CURSOR_ENABLED;
                            mouseLookEnabled = false;
                            skipMouseLookFrame = true;
                            g_X = 0;
                            g_Y = 125;
                        }
                    }
                        break;
                    case GameState::STATE_LOBBY:
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
                            resetCarToCourseStart();
                            resetRaceSession(raceSession, raceCourse);
                            localRaceReady = false;
                            raceReadyCount = 0;
                            racePlayerCount = std::max(1, static_cast<int>(gui_externalplayers.size()) + 1);
                            currentGameState = GameState::STATE_RACING;
                            currentCursorState = CursorState::STATE_CURSOR_ENABLED;
                            mouseLookEnabled = false;
                            skipMouseLookFrame = true;
                            g_in_state_transition = true;
                            session->sendRaceReadyUpdate(raceCourse.courseId, false, true, raceCourse.lapCount);
                        }
                        break;
                }
                if ((currentGameState == GameState::STATE_LOBBY) || ((currentCursorState == CursorState::STATE_CURSOR_ENABLED) && (currentGameState == GameState::STATE_RACING)))
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
                    session->sendPosition(g_X, g_Y, g_Angle);
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
                    if (CursorState::STATE_CURSOR_ENABLED == currentCursorState)
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
                    case GameState::STATE_RACING:
                    {
                        BeginTextureMode(target3DArea);       // Enable drawing to texture
                        ClearBackground(RAYWHITE);
                        BeginMode3D(camera);
                        rlDisableBackfaceCulling();
                        rlDisableDepthMask();
                        DrawModel(skyboxModel, { 0.0f, 0.0f, 0.0f }, 1.0f, WHITE);
                        rlEnableBackfaceCulling();
                        rlEnableDepthMask();
                        DrawModel(groundModel, { 0.0f, -0.03f, 0.0f }, 1.0f, WHITE);
                        // DrawModel(model, mapPosition, 1.0f, WHITE);                  // Draw collision map
                        if (ENABLE_COURSE_WALLS)
                        {
                            drawCourseWalls(raceCourse);
                        }
                        drawCourseStartLine(raceCourse);
                        drawCourseCheckpoint(raceCourse);
                        for (auto coords = gui_externalplayers.begin(); coords != gui_externalplayers.end(); coords++)
                        {
                            DrawCylinder({ static_cast<float>(coords->second.m_coords.m_X)/SCALEFACTOR, 0.0f, static_cast<float>(coords->second.m_coords.m_Y)/ SCALEFACTOR }, 0.15f, 0.15f, 0.3f, 5, GetColor(colorHexToString(coords->second.m_color)));
                            drawCarModelWithPaint(carModel, carPaintMaterialIndex, coords->second.m_color, { static_cast<float>(coords->second.m_coords.m_X) / SCALEFACTOR, 0.0f, static_cast<float>(coords->second.m_coords.m_Y) / SCALEFACTOR }, coords->second.m_coords.m_Angle);
                        }
                        drawCarModelWithPaint(carModel, carPaintMaterialIndex, getCarColorString(), { playerPos.x, -0.1f, playerPos.y }, g_Angle);

                        EndMode3D();
                        EndTextureMode();

                        BeginDrawing();

                        ClearBackground(RAYWHITE);
                        DrawFPS(10, 10);
                        DrawTextureRec(target3DArea.texture, { 0, 0, (float)target3DArea.texture.width, (float)-target3DArea.texture.height }, { 0, 0 }, WHITE);

                        const bool drsHudActive = drsTimer > 0.0f;
                        const bool drsHudReady = !drsHudActive && (carVelocity >= DRS_MIN_READY_SPEED);
                        const char* drsHudText = drsHudActive ? "ACTIVE" : (drsHudReady ? "READY (SPACE)" : "NEED SPEED");
                        const Color drsHudColor = drsHudActive ? GOLD : (drsHudReady ? LIME : GRAY);
                        const float displaySpeedKmh = std::abs(carVelocity) * DISPLAY_TOP_SPEED_KMH / DRS_MAX_CAR_SPEED;
                        const std::int64_t raceHudNowEpochMs = currentEpochMilliseconds();
                        const float raceCountdownSeconds = raceSessionCountdownSeconds(raceSession, raceHudNowEpochMs);
                        const int raceHudPlayerCount = std::max(1, racePlayerCount);
                        const int raceHudReadyCount = std::clamp(raceReadyCount, 0, raceHudPlayerCount);

                        if (raceSession.state == RaceRunState::STATE_WAITING)
                        {
                            const int stagingOverlayY = windowYBoundary() / 3;
                            drawCenteredOutlinedText(localRaceReady ? "READY" : "READY UP", stagingOverlayY - 36, 52, localRaceReady ? LIME : GOLD);
                            drawCenteredOutlinedText(localRaceReady ? "Waiting for players" : "Press SPACE", stagingOverlayY + 14, 28, RAYWHITE);
                            drawCenteredOutlinedText(TextFormat("%d/%d ready", raceHudReadyCount, raceHudPlayerCount), stagingOverlayY + 48, 24, RAYWHITE);
                        }
                        else if (raceSession.state == RaceRunState::STATE_COUNTDOWN)
                        {
                            const int countdownOverlayY = windowYBoundary() / 2;
                            const int countdownNumber = static_cast<int>(std::ceil(raceCountdownSeconds));
                            drawCenteredOutlinedText("RACE START", countdownOverlayY - 84, 36, RAYWHITE);
                            drawCenteredOutlinedText(countdownNumber > 0 ? TextFormat("%d", countdownNumber) : "GO", countdownOverlayY, 124, countdownNumber > 0 ? GOLD : LIME);
                        }

                        DrawRectangle(1350, 5, 245, 235, Fade(SKYBLUE, 0.45f));
                        DrawRectangleLines(1350, 5, 245, 235, DARKBLUE);

                        // draw camera player status
                        DrawText("Controls:", 1360, 15, 12, BLACK);
                        DrawText("W/S: accelerate / brake", 1360, 30, 12, BLACK);
                        DrawText("A/D: steer", 1360, 45, 12, BLACK);
                        DrawText("Arrow keys: look around", 1360, 60, 12, BLACK);
                        DrawText("Alt+C: toggle mouse look", 1360, 75, 12, BLACK);
                        DrawText("Left Shift: return to lobby", 1360, 90, 12, BLACK);
                        DrawText(TextFormat("- Projection: %s", (camera.projection == CAMERA_PERSPECTIVE) ? "PERSPECTIVE" :
                            (camera.projection == CAMERA_ORTHOGRAPHIC) ? "ORTHOGRAPHIC" : "CUSTOM"), 1360, 110, 12, BLACK);
                        DrawText(TextFormat("- Speed: %03.0f km/h", displaySpeedKmh), 1360, 130, 12, BLACK);
                        DrawText(TextFormat("- Race: %s", raceSessionStateLabel(raceSession)), 1360, 145, 12, BLACK);
                        DrawText(TextFormat("- Ready: %d/%d", raceHudReadyCount, raceHudPlayerCount), 1360, 160, 12, localRaceReady ? LIME : BLACK);
                        DrawText(TextFormat("- Start: %.1f", raceCountdownSeconds), 1360, 175, 12, BLACK);
                        DrawText(TextFormat("- Lap: %d/%d", raceSessionDisplayLap(raceSession), raceSession.totalLaps), 1360, 190, 12, BLACK);
                        DrawText(TextFormat("- DRS: %s", drsHudText), 1360, 220, 12, drsHudColor);

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
                    }
                        break;
                    case GameState::STATE_LOBBY:
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
            UnloadModel(groundModel);       // Unload ground model
            UnloadTexture(groundTexture);   // Unload ground texture
            UnloadShader(skyboxShader);     // Unload skybox shader
            UnloadTexture(skyboxCubemap);   // Unload skybox cubemap texture
            UnloadModel(skyboxModel);       // Unload skybox model
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

