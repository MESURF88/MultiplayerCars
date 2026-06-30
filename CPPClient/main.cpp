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
#include <deque>
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
    STATE_TRAFFIC_SIM,
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
static constexpr int LOBBY_MOVE_SPEED = 5;
static constexpr int TRAFFIC_LANE_COUNT = 4;
static constexpr float TRAFFIC_LANE_WIDTH = 3.7f;
static constexpr float TRAFFIC_ROAD_HALF_WIDTH = (TRAFFIC_LANE_COUNT * TRAFFIC_LANE_WIDTH) * 0.5f;
static constexpr float TRAFFIC_EYE_HEIGHT = 1.22f;
static constexpr float TRAFFIC_HEAD_MAX_YAW = 88.0f;
static constexpr float TRAFFIC_HEAD_MIN_PITCH = -28.0f;
static constexpr float TRAFFIC_HEAD_MAX_PITCH = 32.0f;
static constexpr float TRAFFIC_MOUSE_SENSITIVITY = 0.11f;
static constexpr float TRAFFIC_KEY_LOOK_SPEED = 90.0f;
static constexpr float TRAFFIC_MAX_SPEED = 38.0f;
static constexpr float TRAFFIC_MIN_SPEED = 0.0f;
static constexpr float TRAFFIC_ACCELERATION = 10.0f;
static constexpr float TRAFFIC_BRAKE_DECELERATION = 18.0f;
static constexpr float TRAFFIC_COAST_DECELERATION = 3.0f;
static constexpr float TRAFFIC_STEER_SPEED = 9.0f;
static constexpr float TRAFFIC_SEGMENT_LENGTH = 80.0f;
static constexpr int TRAFFIC_SEGMENT_COUNT = 8;
static constexpr int TRAFFIC_VEHICLE_COUNT = 18;
static constexpr float TRAFFIC_MPS_TO_MPH = 2.23693629f;
static constexpr float TRAFFIC_METERS_PER_MILE = 1609.344f;
static constexpr float TRAFFIC_EGO_LENGTH = 4.7f;
static constexpr float TRAFFIC_EGO_WIDTH = 2.0f;
static constexpr float TRAFFIC_IMPACT_COOLDOWN_SECONDS = 0.8f;
static constexpr float TRAFFIC_STEERING_WHEEL_MAX_ANGLE = 55.0f;
static constexpr float TRAFFIC_STEERING_WHEEL_RETURN_SPEED = 8.0f;
static constexpr float TRAFFIC_BANNER_SLIDE_SECONDS = 0.32f;
static constexpr float TRAFFIC_BANNER_HOLD_SECONDS = 3.0f;
static constexpr float TRAFFIC_CLEAN_MILE_INTERVAL = 5.0f;
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

static std::string courseResourceNameForId(const std::string& courseId)
{
    (void)courseId;
    return "courses/simple_circuit.json";
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

struct TrafficVehicle
{
    int lane = 0;
    int targetLane = 0;
    float lateral = 0.0f;
    float relativeZ = 0.0f;
    float speed = 20.0f;
    float desiredSpeed = 20.0f;
    float laneChangeCooldown = 1.0f;
    float length = 4.4f;
    float width = 1.9f;
    bool braking = false;
    bool bigRig = false;
    bool merging = false;
    bool speedRacer = false;
    int mergeSide = 0;
    Color color = BLUE;
};

struct TrafficImpactResult
{
    bool hit = false;
    const char* side = "NONE";
};

struct TrafficUpdateEvents
{
    int passedCars = 0;
    int dodgedSpeedRacers = 0;
};

struct TrafficBanner
{
    std::string title;
    std::string detail;
    Color accent = GOLD;
    float age = 0.0f;
};

static float trafficLaneCenter(int lane)
{
    const float centerOffset = (static_cast<float>(TRAFFIC_LANE_COUNT) - 1.0f) * 0.5f;
    return (static_cast<float>(lane) - centerOffset) * TRAFFIC_LANE_WIDTH;
}

static float trafficRandom01(float seed)
{
    const float value = sinf(seed) * 43758.5453f;
    return value - floorf(value);
}

static TrafficVehicle makeTrafficVehicle(int index)
{
    TrafficVehicle vehicle;
    vehicle.lane = index % TRAFFIC_LANE_COUNT;
    vehicle.targetLane = vehicle.lane;
    vehicle.lateral = trafficLaneCenter(vehicle.lane);
    vehicle.relativeZ = 36.0f + static_cast<float>(index * 21);
    vehicle.bigRig = (index % 6) == 4;
    vehicle.length = vehicle.bigRig ? 11.0f : 4.6f;
    vehicle.width = vehicle.bigRig ? 2.35f : 1.95f;
    vehicle.speed = 15.0f + static_cast<float>((index * 7) % 12);
    vehicle.desiredSpeed = vehicle.speed;
    vehicle.laneChangeCooldown = 1.5f + static_cast<float>((index * 5) % 8);
    const Color palette[] = {
        GetColor(0x0079F1FF),
        GetColor(0xE62937FF),
        GetColor(0xC8C8C8FF),
        GetColor(0xFFA100FF),
        GetColor(0x2E363CFF),
        GetColor(0x25FDCBFF)
    };
    vehicle.color = palette[index % 6];
    return vehicle;
}

static void startTrafficMerge(TrafficVehicle& vehicle, std::size_t index, float elapsedSeconds)
{
    const bool fromLeft = ((static_cast<int>(index) + static_cast<int>(elapsedSeconds)) % 2) == 0;
    vehicle.merging = true;
    vehicle.mergeSide = fromLeft ? -1 : 1;
    vehicle.lane = fromLeft ? 0 : TRAFFIC_LANE_COUNT - 1;
    vehicle.targetLane = vehicle.lane;
    vehicle.lateral = static_cast<float>(vehicle.mergeSide) * (TRAFFIC_ROAD_HALF_WIDTH + 7.5f);
    vehicle.relativeZ = 95.0f + static_cast<float>((static_cast<int>(index) * 31) % 150);
    vehicle.speed = 12.0f + static_cast<float>((static_cast<int>(index) * 5 + static_cast<int>(elapsedSeconds)) % 7);
    vehicle.desiredSpeed = 18.0f;
    vehicle.laneChangeCooldown = 5.0f;
}

static void startTrafficSpeedRacer(TrafficVehicle& vehicle, std::size_t index, float elapsedSeconds)
{
    vehicle.speedRacer = true;
    vehicle.merging = false;
    vehicle.mergeSide = 0;
    vehicle.bigRig = false;
    vehicle.length = 4.4f;
    vehicle.width = 1.9f;
    vehicle.lane = static_cast<int>((index + static_cast<int>(elapsedSeconds)) % TRAFFIC_LANE_COUNT);
    vehicle.targetLane = vehicle.lane;
    vehicle.lateral = trafficLaneCenter(vehicle.lane);
    vehicle.relativeZ = -92.0f - static_cast<float>((static_cast<int>(index) * 9) % 24);
    vehicle.speed = 48.0f + static_cast<float>((static_cast<int>(index) * 7 + static_cast<int>(elapsedSeconds)) % 11);
    vehicle.desiredSpeed = vehicle.speed;
    vehicle.laneChangeCooldown = 1.2f + static_cast<float>((static_cast<int>(index) * 3) % 4);
    vehicle.color = GetColor(0xFDF900FF);
}

static void resetTrafficVehicles(std::vector<TrafficVehicle>& trafficVehicles)
{
    trafficVehicles.clear();
    trafficVehicles.reserve(TRAFFIC_VEHICLE_COUNT);
    for (int index = 0; index < TRAFFIC_VEHICLE_COUNT; ++index)
    {
        trafficVehicles.push_back(makeTrafficVehicle(index));
    }
}

static Vector3 trafficLookDirection(float yawDegrees, float pitchDegrees)
{
    const float yawRadians = yawDegrees * DEG2RAD;
    const float pitchRadians = pitchDegrees * DEG2RAD;
    const float horizontal = cosf(pitchRadians);
    return { sinf(yawRadians) * horizontal, sinf(pitchRadians), cosf(yawRadians) * horizontal };
}

static void updateTrafficCamera(Camera& camera, float egoLateral, float headYaw, float headPitch)
{
    const Vector3 eye = { egoLateral, TRAFFIC_EYE_HEIGHT, 0.0f };
    const Vector3 direction = trafficLookDirection(headYaw, headPitch);
    camera.position = eye;
    camera.target = { eye.x + direction.x, eye.y + direction.y, eye.z + direction.z };
    camera.up = { 0.0f, 1.0f, 0.0f };
    camera.fovy = 74.0f;
    camera.projection = CAMERA_PERSPECTIVE;
}

static Camera makeTrafficRearViewCamera(float egoLateral)
{
    Camera mirrorCamera = { 0 };
    mirrorCamera.position = { egoLateral, 1.72f, -0.35f };
    mirrorCamera.target = { egoLateral, 1.48f, -28.0f };
    mirrorCamera.up = { 0.0f, 1.0f, 0.0f };
    mirrorCamera.fovy = 58.0f;
    mirrorCamera.projection = CAMERA_PERSPECTIVE;
    return mirrorCamera;
}

static TrafficImpactResult detectTrafficImpact(const std::vector<TrafficVehicle>& trafficVehicles, float egoLateral)
{
    TrafficImpactResult result;
    const float egoHalfWidth = TRAFFIC_EGO_WIDTH * 0.5f;
    const float egoHalfLength = TRAFFIC_EGO_LENGTH * 0.5f;

    for (const TrafficVehicle& vehicle : trafficVehicles)
    {
        const float lateralGap = std::abs(vehicle.lateral - egoLateral);
        const float longitudinalGap = std::abs(vehicle.relativeZ);
        const float combinedHalfWidth = egoHalfWidth + vehicle.width * 0.5f;
        const float combinedHalfLength = egoHalfLength + vehicle.length * 0.5f;
        if (lateralGap < combinedHalfWidth && longitudinalGap < combinedHalfLength)
        {
            const float lateralPenetration = combinedHalfWidth - lateralGap;
            const float longitudinalPenetration = combinedHalfLength - longitudinalGap;
            result.hit = true;
            if (longitudinalPenetration < lateralPenetration)
            {
                result.side = vehicle.relativeZ >= 0.0f ? "FRONT" : "REAR";
            }
            else
            {
                result.side = vehicle.lateral >= egoLateral ? "RIGHT" : "LEFT";
            }
            return result;
        }
    }

    return result;
}

static bool trafficIsOutOfLane(float egoLateral)
{
    const int nearestLane = std::clamp(static_cast<int>(std::round((egoLateral / TRAFFIC_LANE_WIDTH) + ((static_cast<float>(TRAFFIC_LANE_COUNT) - 1.0f) * 0.5f))), 0, TRAFFIC_LANE_COUNT - 1);
    const float laneCenter = trafficLaneCenter(nearestLane);
    const float fullyInsideLaneOffset = (TRAFFIC_LANE_WIDTH - TRAFFIC_EGO_WIDTH) * 0.5f;
    return std::abs(egoLateral - laneCenter) > fullyInsideLaneOffset;
}

static TrafficUpdateEvents updateTrafficVehicles(std::vector<TrafficVehicle>& trafficVehicles, float egoSpeed, float elapsedSeconds, float frameTime)
{
    TrafficUpdateEvents events;
    const float recycleDistance = 430.0f;
    for (std::size_t index = 0; index < trafficVehicles.size(); ++index)
    {
        TrafficVehicle& vehicle = trafficVehicles[index];
        vehicle.laneChangeCooldown -= frameTime;
        const float previousRelativeZ = vehicle.relativeZ;

        const float pattern = sinf(elapsedSeconds * 0.31f + static_cast<float>(index) * 1.73f);
        if (!vehicle.merging && vehicle.laneChangeCooldown <= 0.0f && pattern > (vehicle.speedRacer ? 0.34f : 0.58f))
        {
            const int direction = (sinf(elapsedSeconds * 0.47f + static_cast<float>(index) * 2.21f) > 0.0f) ? 1 : -1;
            vehicle.targetLane = std::clamp(vehicle.lane + direction, 0, TRAFFIC_LANE_COUNT - 1);
            vehicle.laneChangeCooldown = vehicle.speedRacer ? 0.8f + static_cast<float>((static_cast<int>(index) * 2) % 3) : 3.0f + static_cast<float>((static_cast<int>(index) * 3) % 6);
        }

        const float targetLateral = trafficLaneCenter(vehicle.targetLane);
        const float lateralDelta = targetLateral - vehicle.lateral;
        const float laneStep = (vehicle.merging ? 2.7f : 1.65f) * frameTime;
        if (std::abs(lateralDelta) <= laneStep)
        {
            vehicle.lateral = targetLateral;
            vehicle.lane = vehicle.targetLane;
            vehicle.merging = false;
            vehicle.mergeSide = 0;
        }
        else
        {
            vehicle.lateral += (lateralDelta > 0.0f ? 1.0f : -1.0f) * laneStep;
        }

        bool trafficAheadClose = false;
        for (const TrafficVehicle& other : trafficVehicles)
        {
            if (&other == &vehicle) continue;
            if (std::abs(other.lateral - vehicle.lateral) < 1.25f)
            {
                const float gap = other.relativeZ - vehicle.relativeZ;
                if (gap > 0.0f && gap < 15.0f)
                {
                    trafficAheadClose = true;
                    break;
                }
            }
        }

        const bool randomBrake = sinf(elapsedSeconds * 0.91f + static_cast<float>(index) * 4.31f) > 0.965f;
        vehicle.braking = trafficAheadClose || (!vehicle.speedRacer && randomBrake);
        if (vehicle.speedRacer)
        {
            vehicle.desiredSpeed = vehicle.braking ? std::max(34.0f, vehicle.speed - 13.0f) : (48.0f + static_cast<float>((static_cast<int>(index) * 7) % 12));
        }
        else
        {
            vehicle.desiredSpeed = vehicle.braking ? std::max(8.0f, vehicle.speed - 9.0f) : (14.0f + static_cast<float>((static_cast<int>(index) * 5) % 15));
        }
        const float speedStep = (vehicle.desiredSpeed < vehicle.speed ? (vehicle.speedRacer ? 14.0f : 8.0f) : (vehicle.speedRacer ? 6.0f : 3.0f)) * frameTime;
        if (std::abs(vehicle.desiredSpeed - vehicle.speed) <= speedStep)
        {
            vehicle.speed = vehicle.desiredSpeed;
        }
        else
        {
            vehicle.speed += (vehicle.desiredSpeed > vehicle.speed ? 1.0f : -1.0f) * speedStep;
        }

        vehicle.relativeZ += (vehicle.speed - egoSpeed) * frameTime;
        if (!vehicle.speedRacer && previousRelativeZ > 0.0f && vehicle.relativeZ <= 0.0f)
        {
            events.passedCars++;
        }
        else if (vehicle.speedRacer && previousRelativeZ < 0.0f && vehicle.relativeZ >= 0.0f)
        {
            events.dodgedSpeedRacers++;
        }
        if (vehicle.relativeZ < -95.0f)
        {
            vehicle.relativeZ += recycleDistance;
            vehicle.speedRacer = false;
            const float eventRoll = trafficRandom01(elapsedSeconds * 0.37f + static_cast<float>(index) * 19.173f);
            const bool shouldSpeedRace = eventRoll > 0.965f;
            const bool shouldMerge = eventRoll > 0.765f && eventRoll <= 0.965f;
            if (shouldSpeedRace)
            {
                startTrafficSpeedRacer(vehicle, index, elapsedSeconds);
            }
            else if (shouldMerge)
            {
                startTrafficMerge(vehicle, index, elapsedSeconds);
            }
            else
            {
                vehicle.merging = false;
                vehicle.mergeSide = 0;
                vehicle.lane = (vehicle.lane + 1 + static_cast<int>(index)) % TRAFFIC_LANE_COUNT;
                vehicle.targetLane = vehicle.lane;
                vehicle.lateral = trafficLaneCenter(vehicle.lane);
                vehicle.bigRig = (static_cast<int>(index) % 6) == 4;
                vehicle.length = vehicle.bigRig ? 11.0f : 4.6f;
                vehicle.width = vehicle.bigRig ? 2.35f : 1.95f;
                vehicle.speed = 15.0f + static_cast<float>((static_cast<int>(index) * 7 + static_cast<int>(elapsedSeconds)) % 13);
                vehicle.color = makeTrafficVehicle(static_cast<int>(index)).color;
            }
        }
        else if (vehicle.relativeZ > recycleDistance - 20.0f)
        {
            vehicle.relativeZ -= recycleDistance;
            vehicle.merging = false;
            vehicle.speedRacer = false;
            vehicle.mergeSide = 0;
        }
    }
    return events;
}

static void drawTrafficRoad(float trafficDistance)
{
    const float roadWidth = TRAFFIC_ROAD_HALF_WIDTH * 2.0f + 2.0f;
    const float segmentScroll = fmodf(trafficDistance, TRAFFIC_SEGMENT_LENGTH);
    const float firstSegmentZ = -TRAFFIC_SEGMENT_LENGTH - segmentScroll;
    for (int segment = 0; segment < TRAFFIC_SEGMENT_COUNT; ++segment)
    {
        const float segmentZ = firstSegmentZ + static_cast<float>(segment) * TRAFFIC_SEGMENT_LENGTH;
        DrawCubeV({ 0.0f, -0.055f, segmentZ + TRAFFIC_SEGMENT_LENGTH * 0.5f }, { roadWidth, 0.05f, TRAFFIC_SEGMENT_LENGTH }, GetColor(0x2B333AFF));
        DrawCubeV({ -TRAFFIC_ROAD_HALF_WIDTH - 1.0f, 0.08f, segmentZ + TRAFFIC_SEGMENT_LENGTH * 0.5f }, { 0.18f, 0.25f, TRAFFIC_SEGMENT_LENGTH }, GetColor(0xB5BCC2FF));
        DrawCubeV({ TRAFFIC_ROAD_HALF_WIDTH + 1.0f, 0.08f, segmentZ + TRAFFIC_SEGMENT_LENGTH * 0.5f }, { 0.18f, 0.25f, TRAFFIC_SEGMENT_LENGTH }, GetColor(0xB5BCC2FF));
    }

    const float rampSpacing = 170.0f;
    const float rampPhase = fmodf(trafficDistance, rampSpacing);
    for (int ramp = -1; ramp < 5; ++ramp)
    {
        const float rampStartZ = -rampPhase + static_cast<float>(ramp) * rampSpacing + 62.0f;
        const int side = (ramp % 2 == 0) ? -1 : 1;
        for (int step = 0; step < 12; ++step)
        {
            const float t = static_cast<float>(step) / 11.0f;
            const float rampX = static_cast<float>(side) * (TRAFFIC_ROAD_HALF_WIDTH + 8.2f - t * 5.5f);
            const float rampZ = rampStartZ + t * 58.0f;
            DrawCubeV({ rampX, -0.035f, rampZ }, { 3.2f, 0.055f, 6.2f }, GetColor(0x343D45FF));
            DrawCubeV({ rampX - static_cast<float>(side) * 1.55f, 0.005f, rampZ }, { 0.08f, 0.025f, 4.4f }, GetColor(0xE9E4C9FF));
        }
    }

    for (int lane = 1; lane < TRAFFIC_LANE_COUNT; ++lane)
    {
        const float laneX = -TRAFFIC_ROAD_HALF_WIDTH + static_cast<float>(lane) * TRAFFIC_LANE_WIDTH;
        for (int dash = -5; dash < 28; ++dash)
        {
            const float dashZ = -fmodf(trafficDistance, 11.0f) + static_cast<float>(dash) * 11.0f;
            DrawCubeV({ laneX, 0.01f, dashZ }, { 0.12f, 0.025f, 5.8f }, GetColor(0xE9E4C9FF));
        }
    }
}

static void drawTrafficVehicle(const TrafficVehicle& vehicle)
{
    const float y = 0.42f;
    const Vector3 base = { vehicle.lateral, y, vehicle.relativeZ };

    // TODO: replace procedural placeholder with a refined traffic car model.
    if (vehicle.speedRacer)
    {
        DrawCubeV(base, { vehicle.width, 0.62f, vehicle.length }, vehicle.color);
        DrawCubeV({ base.x, base.y + 0.36f, base.z + 0.10f }, { vehicle.width * 0.66f, 0.34f, vehicle.length * 0.36f }, GetColor(0x11171DFF));
        DrawCubeV({ base.x, base.y - 0.31f, base.z + vehicle.length * 0.30f }, { vehicle.width * 0.94f, 0.12f, 0.35f }, GetColor(0xFF4A2BFF));
        DrawSphere({ vehicle.lateral - vehicle.width * 0.34f, 0.62f, vehicle.relativeZ + vehicle.length * 0.5f + 0.08f }, 0.12f, Fade(WHITE, 0.85f));
        DrawSphere({ vehicle.lateral + vehicle.width * 0.34f, 0.62f, vehicle.relativeZ + vehicle.length * 0.5f + 0.08f }, 0.12f, Fade(WHITE, 0.85f));
    }
    else if (!vehicle.bigRig)
    {
        DrawCubeV(base, { vehicle.width, 0.82f, vehicle.length }, vehicle.color);
        DrawCubeV({ base.x, base.y + 0.48f, base.z + 0.15f }, { vehicle.width * 0.72f, 0.56f, vehicle.length * 0.44f }, GetColor(0xBFEAFFFF));
        DrawCubeV({ base.x, base.y - 0.37f, base.z + vehicle.length * 0.22f }, { vehicle.width * 0.92f, 0.16f, 0.25f }, GetColor(0x11171DFF));
    }
    else
    {
        // TODO: replace procedural placeholder with a refined big-rig model.
        DrawCubeV({ base.x, base.y + 0.18f, base.z - 1.4f }, { vehicle.width, 1.2f, vehicle.length * 0.68f }, GetColor(0xD8DDE2FF));
        DrawCubeV({ base.x, base.y + 0.08f, base.z + vehicle.length * 0.28f }, { vehicle.width, 1.05f, vehicle.length * 0.25f }, vehicle.color);
        DrawCubeV({ base.x, base.y + 0.64f, base.z + vehicle.length * 0.36f }, { vehicle.width * 0.72f, 0.38f, vehicle.length * 0.12f }, GetColor(0xBFEAFFFF));
    }

    const float rearZ = vehicle.relativeZ - vehicle.length * 0.5f - 0.08f;
    const Color tailColor = vehicle.braking ? RED : Fade(RED, 0.55f);
    DrawSphere({ vehicle.lateral - vehicle.width * 0.36f, 0.52f, rearZ }, vehicle.braking ? 0.18f : 0.10f, tailColor);
    DrawSphere({ vehicle.lateral + vehicle.width * 0.36f, 0.52f, rearZ }, vehicle.braking ? 0.18f : 0.10f, tailColor);
    if (vehicle.braking)
    {
        DrawSphere({ vehicle.lateral - vehicle.width * 0.36f, 0.52f, rearZ - 0.05f }, 0.32f, Fade(RED, 0.25f));
        DrawSphere({ vehicle.lateral + vehicle.width * 0.36f, 0.52f, rearZ - 0.05f }, 0.32f, Fade(RED, 0.25f));
    }
}

static void drawTrafficScene(const std::vector<TrafficVehicle>& trafficVehicles, float trafficDistance)
{
    drawTrafficRoad(trafficDistance);
    for (const TrafficVehicle& vehicle : trafficVehicles)
    {
        if (vehicle.relativeZ > -120.0f && vehicle.relativeZ < 360.0f)
        {
            drawTrafficVehicle(vehicle);
        }
    }
}

static Vector3 trafficWheelPoint(Vector3 center, float x, float y, float angleDegrees)
{
    const float angleRadians = angleDegrees * DEG2RAD;
    const float rotatedX = x * cosf(angleRadians) - y * sinf(angleRadians);
    const float rotatedY = x * sinf(angleRadians) + y * cosf(angleRadians);
    return { center.x + rotatedX, center.y + rotatedY, center.z - 0.10f };
}

static void drawTrafficDashBobblehead(float egoLateral, Color playerColor)
{
    const float time = static_cast<float>(GetTime());
    const float bobble = sinf(time * 6.4f) * 0.025f;
    const float sway = sinf(time * 4.1f) * 0.035f;
    const Vector3 base = { egoLateral + 0.54f, 0.63f, 1.07f };
    const Vector3 springBottom = { base.x, base.y + 0.04f, base.z };
    const Vector3 springTop = { base.x + sway * 0.35f, base.y + 0.18f + bobble, base.z - 0.02f };
    const Vector3 body = { springTop.x, springTop.y + 0.08f, springTop.z };
    const Vector3 head = { springTop.x + sway, springTop.y + 0.25f + bobble, springTop.z - 0.01f };

    DrawCylinderEx({ base.x, base.y, base.z }, { base.x, base.y + 0.035f, base.z }, 0.16f, 0.13f, 18, GetColor(0x151C22FF));
    DrawCylinderEx({ base.x, base.y + 0.038f, base.z }, { base.x, base.y + 0.052f, base.z }, 0.13f, 0.11f, 18, playerColor);
    DrawSphere({ base.x, base.y + 0.055f, base.z }, 0.18f, Fade(playerColor, 0.20f));
    DrawCylinderEx(springBottom, springTop, 0.018f, 0.014f, 10, GetColor(0xC8CDD1FF));
    DrawSphere(body, 0.095f, playerColor);
    DrawSphere({ body.x, body.y + 0.018f, body.z - 0.075f }, 0.032f, Fade(RAYWHITE, 0.75f));
    DrawSphere(head, 0.105f, GetColor(0xF0C08DFF));
    DrawSphere({ head.x - 0.034f, head.y + 0.022f, head.z + 0.082f }, 0.012f, GetColor(0x11171DFF));
    DrawSphere({ head.x + 0.034f, head.y + 0.022f, head.z + 0.082f }, 0.012f, GetColor(0x11171DFF));
    DrawCubeV({ head.x, head.y + 0.092f, head.z }, { 0.16f, 0.042f, 0.15f }, playerColor);
}

static void drawTrafficCockpitPanelTextures(RenderTexture2D& leftPanelTarget, RenderTexture2D& rightPanelTarget, float egoSpeed, float headYaw, float trafficDistance)
{
    (void)headYaw;
    const float milesDriven = trafficDistance / TRAFFIC_METERS_PER_MILE;
    const int fiveMileMarker = static_cast<int>(milesDriven / 5.0f) * 5;
    const float mph = egoSpeed * TRAFFIC_MPS_TO_MPH;
    const int rpm = static_cast<int>(900.0f + std::clamp(egoSpeed / TRAFFIC_MAX_SPEED, 0.0f, 1.0f) * 6700.0f);

    BeginTextureMode(leftPanelTarget);
    ClearBackground(BLANK);
    DrawRectangleRounded({ 0.0f, 0.0f, static_cast<float>(leftPanelTarget.texture.width), static_cast<float>(leftPanelTarget.texture.height) }, 0.16f, 8, GetColor(0x1B232AFF));
    DrawRectangleRoundedLinesEx({ 2.0f, 2.0f, static_cast<float>(leftPanelTarget.texture.width - 4), static_cast<float>(leftPanelTarget.texture.height - 4) }, 0.16f, 8, 3.0f, GetColor(0x4F626FFF));
    DrawText(TextFormat("%03.0f mph", mph), 26, 18, 34, RAYWHITE);
    DrawText(TextFormat("RPM %d", rpm), 28, 58, 20, GetColor(0xB8C0C7FF));
    EndTextureMode();

    BeginTextureMode(rightPanelTarget);
    ClearBackground(BLANK);
    DrawRectangleRounded({ 0.0f, 0.0f, static_cast<float>(rightPanelTarget.texture.width), static_cast<float>(rightPanelTarget.texture.height) }, 0.16f, 8, GetColor(0x1B232AFF));
    DrawRectangleRoundedLinesEx({ 2.0f, 2.0f, static_cast<float>(rightPanelTarget.texture.width - 4), static_cast<float>(rightPanelTarget.texture.height - 4) }, 0.16f, 8, 3.0f, GetColor(0x4F626FFF));
    DrawText(TextFormat("Miles %.1f", milesDriven), 26, 18, 28, RAYWHITE);
    DrawText(TextFormat("Next mile marker %d", fiveMileMarker + 5), 26, 56, 20, GetColor(0xD7DEE5FF));
    EndTextureMode();
}

static void drawTrafficCockpit3D(Camera camera, Model& steeringWheelModel, float egoLateral, float steeringWheelAngle, Texture2D leftPanelTexture, Texture2D rightPanelTexture)
{
    const Vector3 wheelCenter = { egoLateral, 0.43f, 0.82f };
    const Rectangle leftPanelSource = { 0.0f, 0.0f, static_cast<float>(leftPanelTexture.width), static_cast<float>(-leftPanelTexture.height) };
    const Rectangle rightPanelSource = { 0.0f, 0.0f, static_cast<float>(rightPanelTexture.width), static_cast<float>(-rightPanelTexture.height) };
    const Color frameColor = GetColor(0x12191FFF);
    const Color frameHighlight = GetColor(0x2D3942FF);

    DrawCubeV({ egoLateral, 0.25f, 0.82f }, { 4.05f, 0.48f, 0.74f }, GetColor(0x0E1419FF));
    DrawCubeV({ egoLateral, 0.36f, 1.20f }, { 3.85f, 0.24f, 0.38f }, GetColor(0x11171DFF));
    DrawCubeV({ egoLateral, 0.57f, 1.02f }, { 3.60f, 0.15f, 0.18f }, frameColor);
    DrawCubeV({ egoLateral, 0.10f, 0.24f }, { 4.15f, 0.38f, 0.95f }, GetColor(0x0B1015FF));
    DrawBillboardPro(camera, leftPanelTexture, leftPanelSource, { egoLateral - 0.92f, 0.76f, 1.08f }, { 0.0f, 1.0f, 0.0f }, { 0.82f, 0.30f }, { 0.41f, 0.15f }, 0.0f, WHITE);
    DrawBillboardPro(camera, rightPanelTexture, rightPanelSource, { egoLateral + 0.95f, 0.76f, 1.08f }, { 0.0f, 1.0f, 0.0f }, { 0.98f, 0.30f }, { 0.49f, 0.15f }, 0.0f, WHITE);
    drawTrafficDashBobblehead(egoLateral, GetColor(getCarColorHexValue()));

    DrawCubeV({ egoLateral - 1.78f, 0.58f, -0.18f }, { 0.16f, 0.76f, 1.96f }, frameColor);
    DrawCubeV({ egoLateral + 1.78f, 0.58f, -0.18f }, { 0.16f, 0.76f, 1.96f }, frameColor);
    DrawCubeV({ egoLateral - 1.66f, 0.74f, -0.08f }, { 0.12f, 0.14f, 1.18f }, frameHighlight);
    DrawCubeV({ egoLateral + 1.66f, 0.74f, -0.08f }, { 0.12f, 0.14f, 1.18f }, frameHighlight);
    DrawCubeV({ egoLateral - 1.66f, 0.44f, 0.08f }, { 0.10f, 0.20f, 0.42f }, GetColor(0x0B1015FF));
    DrawCubeV({ egoLateral + 1.66f, 0.44f, 0.08f }, { 0.10f, 0.20f, 0.42f }, GetColor(0x0B1015FF));

    DrawCylinderEx({ egoLateral - 1.72f, 0.48f, 1.08f }, { egoLateral - 1.12f, 1.83f, 0.50f }, 0.07f, 0.10f, 12, frameColor);
    DrawCylinderEx({ egoLateral + 1.72f, 0.48f, 1.08f }, { egoLateral + 1.12f, 1.83f, 0.50f }, 0.07f, 0.10f, 12, frameColor);
    DrawCylinderEx({ egoLateral - 1.12f, 1.83f, 0.50f }, { egoLateral + 1.12f, 1.83f, 0.50f }, 0.08f, 0.08f, 12, frameColor);
    DrawCylinderEx({ egoLateral - 1.10f, 1.82f, 0.48f }, { egoLateral - 1.12f, 1.76f, -1.40f }, 0.07f, 0.08f, 12, frameColor);
    DrawCylinderEx({ egoLateral + 1.10f, 1.82f, 0.48f }, { egoLateral + 1.12f, 1.76f, -1.40f }, 0.07f, 0.08f, 12, frameColor);
    DrawCylinderEx({ egoLateral - 1.16f, 1.74f, -0.72f }, { egoLateral - 1.25f, 0.62f, -0.72f }, 0.075f, 0.095f, 12, frameHighlight);
    DrawCylinderEx({ egoLateral + 1.16f, 1.74f, -0.72f }, { egoLateral + 1.25f, 0.62f, -0.72f }, 0.075f, 0.095f, 12, frameHighlight);
    DrawCubeV({ egoLateral, 1.84f, -0.46f }, { 2.32f, 0.08f, 0.12f }, frameHighlight);

    DrawModelEx(steeringWheelModel, wheelCenter, { 0.0f, 0.0f, 1.0f }, 0.0f, { 0.72f, 0.72f, 0.72f }, GetColor(0x10161BFF));
    DrawSphere({ wheelCenter.x, wheelCenter.y, wheelCenter.z - 0.10f }, 0.08f, GetColor(0x252E35FF));
    DrawCylinderEx({ wheelCenter.x, wheelCenter.y, wheelCenter.z - 0.10f }, trafficWheelPoint(wheelCenter, 0.0f, 0.24f, steeringWheelAngle), 0.022f, 0.016f, 12, GetColor(0x20282EFF));
    DrawCylinderEx({ wheelCenter.x, wheelCenter.y, wheelCenter.z - 0.10f }, trafficWheelPoint(wheelCenter, -0.21f, -0.15f, steeringWheelAngle), 0.022f, 0.016f, 12, GetColor(0x20282EFF));
    DrawCylinderEx({ wheelCenter.x, wheelCenter.y, wheelCenter.z - 0.10f }, trafficWheelPoint(wheelCenter, 0.21f, -0.15f, steeringWheelAngle), 0.022f, 0.016f, 12, GetColor(0x20282EFF));
    DrawSphere(trafficWheelPoint(wheelCenter, 0.0f, 0.30f, steeringWheelAngle), 0.03f, ORANGE);
}

static void drawTrafficImpactOverlay(int impactCount, const std::string& lastImpactSide, bool outOfLane)
{
    DrawRectangle(windowScreenWidth() - 244, 18, 220, 58, Fade(GetColor(0x10161BFF), 0.58f));
    DrawRectangleLines(windowScreenWidth() - 244, 18, 220, 58, ORANGE);
    DrawText(TextFormat("IMPACTS: %d", impactCount), windowScreenWidth() - 224, 30, 20, ORANGE);
    DrawText(TextFormat("LAST: %s", lastImpactSide.c_str()), windowScreenWidth() - 224, 54, 14, GetColor(0xFFC066FF));

    const Color laneColor = outOfLane ? ORANGE : GetColor(0x25FDCBFF);
    const Color laneFill = outOfLane ? Fade(GetColor(0xD66B00FF), 0.72f) : Fade(GetColor(0x0D2B2DFF), 0.72f);
    DrawRectangle(windowScreenWidth() - 244, 84, 220, 44, laneFill);
    DrawRectangleLines(windowScreenWidth() - 244, 84, 220, 44, laneColor);
    DrawText(outOfLane ? "LANE WARNING" : "LANE OK", windowScreenWidth() - 224, 96, 20, laneColor);
}

static int nextTrafficPassMilestone(int passedCars)
{
    if (passedCars < 50) return 50;
    if (passedCars < 100) return 100;
    if (passedCars < 200) return 200;
    return ((passedCars / 200) + 1) * 200;
}

static void queueTrafficBanner(std::deque<TrafficBanner>& banners, const std::string& title, const std::string& detail, Color accent)
{
    TrafficBanner banner;
    banner.title = title;
    banner.detail = detail;
    banner.accent = accent;
    banners.push_back(banner);
    while (banners.size() > 4)
    {
        banners.pop_back();
    }
}

static void updateTrafficBannerQueue(std::deque<TrafficBanner>& banners, float frameTime)
{
    if (banners.empty()) return;

    TrafficBanner& banner = banners.front();
    banner.age += frameTime;
    const float lifetime = TRAFFIC_BANNER_SLIDE_SECONDS * 2.0f + TRAFFIC_BANNER_HOLD_SECONDS;
    if (banner.age >= lifetime)
    {
        banners.pop_front();
    }
}

static void drawTrafficBannerQueue(const std::deque<TrafficBanner>& banners)
{
    if (banners.empty()) return;

    const TrafficBanner& banner = banners.front();
    const float lifetime = TRAFFIC_BANNER_SLIDE_SECONDS * 2.0f + TRAFFIC_BANNER_HOLD_SECONDS;
    const float exitStart = TRAFFIC_BANNER_SLIDE_SECONDS + TRAFFIC_BANNER_HOLD_SECONDS;
    float visible = 1.0f;
    if (banner.age < TRAFFIC_BANNER_SLIDE_SECONDS)
    {
        visible = banner.age / TRAFFIC_BANNER_SLIDE_SECONDS;
    }
    else if (banner.age > exitStart)
    {
        visible = std::max(0.0f, (lifetime - banner.age) / TRAFFIC_BANNER_SLIDE_SECONDS);
    }

    visible = visible * visible * (3.0f - 2.0f * visible);
    const float width = 430.0f;
    const float height = 68.0f;
    const float x = static_cast<float>((windowScreenWidth() - static_cast<int>(width)) / 2);
    const float y = 140.0f - (1.0f - visible) * 110.0f;
    DrawRectangleRounded({ x, y, width, height }, 0.14f, 8, Fade(GetColor(0x10161BFF), 0.84f));
    DrawRectangleRoundedLinesEx({ x, y, width, height }, 0.14f, 8, 3.0f, banner.accent);
    DrawRectangle(static_cast<int>(x), static_cast<int>(y), 8, static_cast<int>(height), banner.accent);
    DrawText(banner.title.c_str(), static_cast<int>(x + 24.0f), static_cast<int>(y + 12.0f), 23, RAYWHITE);
    DrawText(banner.detail.c_str(), static_cast<int>(x + 25.0f), static_cast<int>(y + 42.0f), 15, GetColor(0xD7DEE5FF));
}

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

    int statusCode = 0;     // get status code by reference
    std::string otp;
#if DEBUG_CLIENT
    const std::string loginUrl = "https://" + host + ":" + port + "/login";
#else
    const std::string loginUrl = "https://" + host + "/login";
#endif

    // The io_context is required for all I/O must be declared so that it is placed in memory
    net::io_context ioc;

    // The SSL context is required, and holds certificates
    ssl::context ctx{ ssl::context::tlsv12_client };

    std::shared_ptr<WebsocketSession> session;
    try
    {
        otp = PostRequestPassword(loginUrl, statusCode);
        if ((statusCode == 200) && (!otp.empty()))
        {
            session = WebsocketConn(ioc, ctx, host, port, otp, stdf_message, getCarColorString());
        }
    }
    catch (const std::exception& e)
    {
        std::cout << "warning: multiplayer connection setup failed: " << e.what() << std::endl;
    }

    const bool multiplayerConnected = (session != nullptr);
    if (!multiplayerConnected)
    {
        std::cout << "warning: multiplayer server unavailable; continuing in offline mode";
        if (statusCode != 200)
        {
            std::cout << " (login status " << statusCode << ")";
        }
        std::cout << std::endl;
    }

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
            std::string playerRacePortalCourseId;
            bool playerInTrafficPortal = false;
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
            CourseMap raceCourse = loadCourseOrDefault(findResourcePath(courseResourceNameForId("simple-circuit")));
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
            float trafficEgoLateral = trafficLaneCenter(1);
            float trafficEgoSpeed = 0.0f;
            float trafficHeadYaw = 0.0f;
            float trafficHeadPitch = 0.0f;
            float trafficDistance = 0.0f;
            float trafficElapsedSeconds = 0.0f;
            float trafficSteeringWheelAngle = 0.0f;
            int trafficImpactCount = 0;
            bool trafficImpactActive = false;
            float trafficImpactCooldown = 0.0f;
            int trafficPassedCars = 0;
            int trafficDodgedSpeedRacers = 0;
            int nextPassedCarsBanner = 50;
            int nextDodgedSpeedRacerBanner = 3;
            float cleanDistanceSinceImpact = 0.0f;
            float nextCleanMileBanner = TRAFFIC_CLEAN_MILE_INTERVAL;
            std::string trafficLastImpactSide = "NONE";
            std::deque<TrafficBanner> trafficBanners;
            std::vector<TrafficVehicle> trafficVehicles;
            resetTrafficVehicles(trafficVehicles);
            playerPos = carPosition;
            updateChaseCamera(camera, carPosition, cameraAngle, cameraPitch);

            // Create a RenderTexture2D to be used for render to texture
            RenderTexture2D target3DArea = LoadRenderTexture(windowScreenWidth(), windowYBoundary());
            RenderTexture2D trafficMirrorTarget = LoadRenderTexture(420, 116);
            RenderTexture2D trafficLeftPanelTarget = LoadRenderTexture(260, 96);
            RenderTexture2D trafficRightPanelTarget = LoadRenderTexture(320, 96);
            Model trafficSteeringWheelModel = LoadModelFromMesh(GenMeshTorus(0.18f, 0.9f, 36, 12));

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
            auto resetTrafficSimulator = [&]() {
                trafficEgoLateral = trafficLaneCenter(1);
                trafficEgoSpeed = 18.0f;
                trafficHeadYaw = 0.0f;
                trafficHeadPitch = 0.0f;
                trafficDistance = 0.0f;
                trafficElapsedSeconds = 0.0f;
                trafficSteeringWheelAngle = 0.0f;
                trafficImpactCount = 0;
                trafficImpactActive = false;
                trafficImpactCooldown = 0.0f;
                trafficPassedCars = 0;
                trafficDodgedSpeedRacers = 0;
                nextPassedCarsBanner = 50;
                nextDodgedSpeedRacerBanner = 3;
                cleanDistanceSinceImpact = 0.0f;
                nextCleanMileBanner = TRAFFIC_CLEAN_MILE_INTERVAL;
                trafficLastImpactSide = "NONE";
                trafficBanners.clear();
                resetTrafficVehicles(trafficVehicles);
                g_X = static_cast<int>(trafficEgoLateral * SCALEFACTOR);
                g_Y = 0;
                g_Angle = 0.0f;
                updateTrafficCamera(camera, trafficEgoLateral, trafficHeadYaw, trafficHeadPitch);
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
                                if (!session || (uuidOfSender != session->getClientUUID()))
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
                                    if (session && (readyUUID == session->getClientUUID()))
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
                            if (session)
                            {
                                session->sendRaceReadyUpdate(raceCourse.courseId, true, true, raceCourse.lapCount);
                            }
                            else
                            {
                                scheduleRaceStart(raceSession, raceCourse, currentEpochMilliseconds() + 5000, raceCourse.lapCount);
                            }
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
                                if (session)
                                {
                                    session->sendRaceReadyUpdate(raceCourse.courseId, false, false, raceCourse.lapCount);
                                }
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
                    case GameState::STATE_TRAFFIC_SIM:
                    {
                        const float frameTime = GetFrameTime();
                        trafficElapsedSeconds += frameTime;

                        mousePositionDelta = GetMouseDelta();
                        if (skipMouseLookFrame)
                        {
                            mousePositionDelta = { 0.0f, 0.0f };
                            skipMouseLookFrame = false;
                        }
                        mousePositionDelta.x = std::clamp(mousePositionDelta.x, -CAMERA_MAX_MOUSE_DELTA, CAMERA_MAX_MOUSE_DELTA);
                        mousePositionDelta.y = std::clamp(mousePositionDelta.y, -CAMERA_MAX_MOUSE_DELTA, CAMERA_MAX_MOUSE_DELTA);
                        trafficHeadYaw = std::clamp(trafficHeadYaw - mousePositionDelta.x * TRAFFIC_MOUSE_SENSITIVITY, -TRAFFIC_HEAD_MAX_YAW, TRAFFIC_HEAD_MAX_YAW);
                        trafficHeadPitch = std::clamp(trafficHeadPitch - mousePositionDelta.y * TRAFFIC_MOUSE_SENSITIVITY, TRAFFIC_HEAD_MIN_PITCH, TRAFFIC_HEAD_MAX_PITCH);
                        trafficHeadYaw = std::clamp(trafficHeadYaw + (IsKeyDown(KEY_LEFT) - IsKeyDown(KEY_RIGHT)) * TRAFFIC_KEY_LOOK_SPEED * frameTime, -TRAFFIC_HEAD_MAX_YAW, TRAFFIC_HEAD_MAX_YAW);
                        trafficHeadPitch = std::clamp(trafficHeadPitch + (IsKeyDown(KEY_UP) - IsKeyDown(KEY_DOWN)) * TRAFFIC_KEY_LOOK_SPEED * frameTime, TRAFFIC_HEAD_MIN_PITCH, TRAFFIC_HEAD_MAX_PITCH);

                        if (IsKeyDown(KEY_W))
                        {
                            trafficEgoSpeed += TRAFFIC_ACCELERATION * frameTime;
                        }
                        else if (IsKeyDown(KEY_S))
                        {
                            trafficEgoSpeed -= TRAFFIC_BRAKE_DECELERATION * frameTime;
                        }
                        else
                        {
                            trafficEgoSpeed -= TRAFFIC_COAST_DECELERATION * frameTime;
                        }
                        trafficEgoSpeed = std::clamp(trafficEgoSpeed, TRAFFIC_MIN_SPEED, TRAFFIC_MAX_SPEED);

                        const float trafficSteerInput = static_cast<float>(IsKeyDown(KEY_A) - IsKeyDown(KEY_D));
                        trafficEgoLateral += trafficSteerInput * TRAFFIC_STEER_SPEED * frameTime;
                        trafficEgoLateral = std::clamp(trafficEgoLateral, -TRAFFIC_ROAD_HALF_WIDTH + 1.15f, TRAFFIC_ROAD_HALF_WIDTH - 1.15f);
                        const float targetSteeringWheelAngle = -trafficSteerInput * TRAFFIC_STEERING_WHEEL_MAX_ANGLE;
                        trafficSteeringWheelAngle += (targetSteeringWheelAngle - trafficSteeringWheelAngle) * std::min(1.0f, TRAFFIC_STEERING_WHEEL_RETURN_SPEED * frameTime);
                        trafficDistance += trafficEgoSpeed * frameTime;
                        cleanDistanceSinceImpact += trafficEgoSpeed * frameTime;
                        const TrafficUpdateEvents trafficEvents = updateTrafficVehicles(trafficVehicles, trafficEgoSpeed, trafficElapsedSeconds, frameTime);
                        trafficPassedCars += trafficEvents.passedCars;
                        trafficDodgedSpeedRacers += trafficEvents.dodgedSpeedRacers;
                        while (trafficPassedCars >= nextPassedCarsBanner)
                        {
                            queueTrafficBanner(trafficBanners, TextFormat("%d traffic passes", nextPassedCarsBanner), "Stay smooth through the pack", GetColor(0x25FDCBFF));
                            nextPassedCarsBanner = nextTrafficPassMilestone(nextPassedCarsBanner);
                        }
                        while (trafficDodgedSpeedRacers >= nextDodgedSpeedRacerBanner)
                        {
                            queueTrafficBanner(trafficBanners, TextFormat("%d speed racers dodged", nextDodgedSpeedRacerBanner), "Mirror checks are paying off", GOLD);
                            nextDodgedSpeedRacerBanner += 3;
                        }
                        while ((cleanDistanceSinceImpact / TRAFFIC_METERS_PER_MILE) >= nextCleanMileBanner)
                        {
                            queueTrafficBanner(trafficBanners, TextFormat("%.0f clean miles", nextCleanMileBanner), "No impacts in this streak", GetColor(0x00E430FF));
                            nextCleanMileBanner += TRAFFIC_CLEAN_MILE_INTERVAL;
                        }
                        updateTrafficBannerQueue(trafficBanners, frameTime);
                        trafficImpactCooldown = std::max(0.0f, trafficImpactCooldown - frameTime);
                        const TrafficImpactResult trafficImpact = detectTrafficImpact(trafficVehicles, trafficEgoLateral);
                        if (trafficImpact.hit)
                        {
                            if (!trafficImpactActive && trafficImpactCooldown <= 0.0f)
                            {
                                trafficImpactCount++;
                                trafficLastImpactSide = trafficImpact.side;
                                trafficImpactActive = true;
                                trafficImpactCooldown = TRAFFIC_IMPACT_COOLDOWN_SECONDS;
                                cleanDistanceSinceImpact = 0.0f;
                                nextCleanMileBanner = TRAFFIC_CLEAN_MILE_INTERVAL;
                            }
                        }
                        else
                        {
                            trafficImpactActive = false;
                        }
                        updateTrafficCamera(camera, trafficEgoLateral, trafficHeadYaw, trafficHeadPitch);

                        if (windowIsKeyOnlyPressed(KEY_LEFT_SHIFT) || windowIsKeyOnlyPressed(KEY_RIGHT_SHIFT))
                        {
                            g_in_state_transition = true;
                            currentGameState = GameState::STATE_LOBBY;
                            currentCursorState = CursorState::STATE_CURSOR_ENABLED;
                            mouseLookEnabled = false;
                            skipMouseLookFrame = true;
                            g_X = 0;
                            g_Y = 125;
                            move = true;
                        }
                    }
                        break;
                    case GameState::STATE_LOBBY:
                        if (windowIsKeyPressedUp() || windowIsKeyPressed(KEY_W))
                        {
                            if (g_Y > 1)
                            {
                                g_Y = std::max(1, g_Y - LOBBY_MOVE_SPEED);
                                move = true;
                            }
                        }
                        if (windowIsKeyPressedDown() || windowIsKeyPressed(KEY_S))
                        {
                            if (g_Y < (windowYBoundary() - getCarHeight() - 1))
                            {
                                g_Y = std::min(windowYBoundary() - getCarHeight() - 1, g_Y + LOBBY_MOVE_SPEED);
                                move = true;
                            }
                        }
                        if (windowIsKeyPressedLeft() || windowIsKeyPressed(KEY_A))
                        {
                            if (g_X > 1)
                            {
                                g_X = std::max(1, g_X - LOBBY_MOVE_SPEED);
                                move = true;
                            }
                        }
                        if (windowIsKeyPressedRight() || windowIsKeyPressed(KEY_D))
                        {
                            if (g_X < (windowScreenWidth() - getCarWidth() - 1))
                            {
                                g_X = std::min(windowScreenWidth() - getCarWidth() - 1, g_X + LOBBY_MOVE_SPEED);
                                move = true;
                            }
                        }
                        
                        // portal handling
                        playerInTrafficPortal = windowIsPlayerCollidesTrafficPortal(g_X, g_Y);
                        if (!mouseOnText && playerInTrafficPortal && windowIsKeyOnlyPressed(KEY_E))
                        {
                            resetTrafficSimulator();
                            currentGameState = GameState::STATE_TRAFFIC_SIM;
                            currentCursorState = CursorState::STATE_CURSOR_DISABLED;
                            mouseLookEnabled = true;
                            skipMouseLookFrame = true;
                            g_in_state_transition = true;
                            move = false;
                        }

                        playerRacePortalCourseId = windowGetPlayerRacePortalCourseId(g_X, g_Y);
                        if (!mouseOnText && !playerRacePortalCourseId.empty() && windowIsKeyOnlyPressed(KEY_E))
                        {
                            if (raceCourse.courseId != playerRacePortalCourseId)
                            {
                                raceCourse = loadCourseOrDefault(findResourcePath(courseResourceNameForId(playerRacePortalCourseId)));
                            }
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
                            if (session)
                            {
                                session->sendRaceReadyUpdate(raceCourse.courseId, false, true, raceCourse.lapCount);
                            }
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
                                if (session)
                                {
                                    session->sendColorUpdate(windowGetColorSelectionMap().at(colorSelection).hexString);
                                }
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
                        if (session)
                        {
                            session->sendTextMessage("", "", std::string(textmessage)); // global is true, default current color
                        }
                        text = false;
                    }
                }
                if (move)
                {
#ifdef TIMING_BENCHMARK
                    start = std::chrono::high_resolution_clock::now();
#endif
                    if (session)
                    {
                        session->sendPosition(g_X, g_Y, g_Angle);
                    }
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
                    case GameState::STATE_TRAFFIC_SIM:
                    {
                        drawTrafficCockpitPanelTextures(trafficLeftPanelTarget, trafficRightPanelTarget, trafficEgoSpeed, trafficHeadYaw, trafficDistance);

                        BeginTextureMode(target3DArea);
                        ClearBackground(RAYWHITE);
                        BeginMode3D(camera);
                        rlDisableBackfaceCulling();
                        rlDisableDepthMask();
                        DrawModel(skyboxModel, { 0.0f, 0.0f, 0.0f }, 1.0f, WHITE);
                        rlEnableBackfaceCulling();
                        rlEnableDepthMask();
                        DrawModel(groundModel, { 0.0f, -0.09f, 0.0f }, 1.0f, WHITE);
                        drawTrafficScene(trafficVehicles, trafficDistance);
                        drawTrafficCockpit3D(camera, trafficSteeringWheelModel, trafficEgoLateral, trafficSteeringWheelAngle, trafficLeftPanelTarget.texture, trafficRightPanelTarget.texture);
                        EndMode3D();
                        EndTextureMode();

                        BeginTextureMode(trafficMirrorTarget);
                        ClearBackground(GetColor(0x9ED8FFFF));
                        Camera mirrorCamera = makeTrafficRearViewCamera(trafficEgoLateral);
                        BeginMode3D(mirrorCamera);
                        rlDisableBackfaceCulling();
                        rlDisableDepthMask();
                        DrawModel(skyboxModel, { 0.0f, 0.0f, 0.0f }, 1.0f, WHITE);
                        rlEnableBackfaceCulling();
                        rlEnableDepthMask();
                        DrawModel(groundModel, { 0.0f, -0.09f, 0.0f }, 1.0f, WHITE);
                        drawTrafficScene(trafficVehicles, trafficDistance);
                        EndMode3D();
                        EndTextureMode();

                        BeginDrawing();
                        ClearBackground(RAYWHITE);
                        DrawFPS(10, 10);
                        DrawTextureRec(target3DArea.texture, { 0, 0, (float)target3DArea.texture.width, (float)-target3DArea.texture.height }, { 0, 0 }, WHITE);

                        const Rectangle mirrorDest = { static_cast<float>((windowScreenWidth() - 420) / 2), 18.0f, 420.0f, 116.0f };
                        DrawRectangleRounded({ mirrorDest.x - 8.0f, mirrorDest.y - 8.0f, mirrorDest.width + 16.0f, mirrorDest.height + 16.0f }, 0.12f, 8, Fade(BLACK, 0.62f));
                        DrawTextureRec(trafficMirrorTarget.texture, { (float)trafficMirrorTarget.texture.width, 0, (float)-trafficMirrorTarget.texture.width, (float)-trafficMirrorTarget.texture.height }, { mirrorDest.x, mirrorDest.y }, WHITE);
                        DrawRectangleRoundedLinesEx({ mirrorDest.x - 2.0f, mirrorDest.y - 2.0f, mirrorDest.width + 4.0f, mirrorDest.height + 4.0f }, 0.08f, 8, 3.0f, GetColor(0x11171DFF));
                        DrawText("REAR VIEW", static_cast<int>(mirrorDest.x + 14), static_cast<int>(mirrorDest.y + 8), 14, RAYWHITE);

                        drawTrafficBannerQueue(trafficBanners);
                        drawTrafficImpactOverlay(trafficImpactCount, trafficLastImpactSide, trafficIsOutOfLane(trafficEgoLateral));
                        DrawRectangle(24, 18, 348, 96, Fade(GetColor(0x10161BFF), 0.58f));
                        DrawRectangleLines(24, 18, 348, 96, GetColor(0xD7DEE5FF));
                        DrawText("Traffic Trainer", 42, 30, 24, RAYWHITE);
                        DrawText("W/S accelerate and brake", 42, 60, 14, GetColor(0xD7DEE5FF));
                        DrawText("A/D steer   Mouse/Arrows look", 42, 78, 14, GetColor(0xD7DEE5FF));
                        DrawText("Shift exits to lobby", 42, 96, 14, GOLD);
                        EndDrawing();
                    }
                        break;
                    case GameState::STATE_LOBBY:
                        BeginDrawing();
                        windowDrawBackground();
                        drawLobbyStreetScene();
                        drawPortalRectangles(g_X, g_Y);
                        for (auto coords = gui_externalplayers.begin(); coords != gui_externalplayers.end(); coords++)
                        {
                            drawLobbyDriver(coords->second.m_coords.m_X, coords->second.m_coords.m_Y, colorHexToString(coords->second.m_color));
                        }
                        drawLobbyDriver(g_X, g_Y);
                        
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
                        drawPortalRaceInfoPane(playerRacePortalCourseId, windowCoursePortalDisplayName(playerRacePortalCourseId));
                        drawTrafficSimInfoPane(playerInTrafficPortal);
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
            UnloadRenderTexture(trafficRightPanelTarget); // Unload traffic cockpit right panel texture
            UnloadRenderTexture(trafficLeftPanelTarget);  // Unload traffic cockpit left panel texture
            UnloadRenderTexture(trafficMirrorTarget); // Unload traffic rear-view mirror texture
            UnloadRenderTexture(target3DArea);        // Unload 3D render target
            UnloadModel(trafficSteeringWheelModel);   // Unload generated traffic steering wheel model
            UnloadModel(groundModel);       // Unload ground model
            UnloadTexture(groundTexture);   // Unload ground texture
            UnloadShader(skyboxShader);     // Unload skybox shader
            UnloadTexture(skyboxCubemap);   // Unload skybox cubemap texture
            UnloadModel(skyboxModel);       // Unload skybox model
            UnloadModel(carModel);          // Unload car model

            if (session)
            {
                session->closeConnection();
            }
            g_gameRunning = false;
            wsUpdatedJsonQueue.push(""); // get out of deadlock
            for (auto& t : m_threadList) t.join();
    CloseWindow();
#ifdef TIMING_BENCHMARK
    timingReport.close();
#endif
    return 0;
}

