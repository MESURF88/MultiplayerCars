#include "courseMap.hpp"
#include "generatedCourses.hpp"

#include <cmath>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <stdexcept>

namespace
{
constexpr float WALL_HEIGHT = 0.45f;
constexpr float WALL_THICKNESS = 1.0f;
constexpr float WALL_BASE_Y = -0.03f;
constexpr float START_LINE_HEIGHT = 0.025f;
constexpr float CHECKPOINT_LINE_HEIGHT = 0.018f;
constexpr float OUTER_MIN_X = -34.0f;
constexpr float OUTER_MAX_X = 34.0f;
constexpr float OUTER_MIN_Z = -22.0f;
constexpr float OUTER_MAX_Z = 22.0f;
constexpr float INNER_MIN_X = -16.0f;
constexpr float INNER_MAX_X = 16.0f;
constexpr float INNER_MIN_Z = -8.0f;
constexpr float INNER_MAX_Z = 8.0f;
constexpr int DEFAULT_START_LINE_CHECKERS = 14;

Vector2 directionFromDegrees(float angle)
{
    const float radians = angle * DEG2RAD;
    return { cosf(radians), sinf(radians) };
}

void drawWall(const CourseWall& wall)
{
    const Vector3 center = {
        wall.bounds.x + wall.bounds.width * 0.5f,
        WALL_BASE_Y + WALL_HEIGHT * 0.5f,
        wall.bounds.y + wall.bounds.height * 0.5f
    };
    const Vector3 size = { wall.bounds.width, WALL_HEIGHT, wall.bounds.height };

    DrawCubeV(center, size, wall.color);
}

float requiredFloat(const nlohmann::json& json, const char* key)
{
    if (!json.contains(key) || !json.at(key).is_number())
    {
        throw std::runtime_error(std::string("missing numeric field '") + key + "'");
    }
    return json.at(key).get<float>();
}

int optionalInt(const nlohmann::json& json, const char* key, int fallback)
{
    if (!json.contains(key))
    {
        return fallback;
    }
    if (!json.at(key).is_number_integer())
    {
        throw std::runtime_error(std::string("field '") + key + "' must be an integer");
    }
    return json.at(key).get<int>();
}

std::string optionalString(const nlohmann::json& json, const char* key, const std::string& fallback)
{
    if (!json.contains(key))
    {
        return fallback;
    }
    if (!json.at(key).is_string())
    {
        throw std::runtime_error(std::string("field '") + key + "' must be a string");
    }
    return json.at(key).get<std::string>();
}

Vector2 requiredPoint(const nlohmann::json& json, const char* key)
{
    if (!json.contains(key) || !json.at(key).is_object())
    {
        throw std::runtime_error(std::string("missing point object '") + key + "'");
    }
    const nlohmann::json& point = json.at(key);
    return { requiredFloat(point, "x"), requiredFloat(point, "z") };
}

Color colorFromHexString(const std::string& hex)
{
    std::string cleanHex = hex;
    if (!cleanHex.empty() && cleanHex.front() == '#')
    {
        cleanHex.erase(cleanHex.begin());
    }
    if (cleanHex.size() == 6)
    {
        cleanHex += "FF";
    }
    if (cleanHex.size() != 8)
    {
        throw std::runtime_error("color must be RRGGBB or RRGGBBAA hex");
    }

    const unsigned long value = std::stoul(cleanHex, nullptr, 16);
    return {
        static_cast<unsigned char>((value >> 24) & 0xff),
        static_cast<unsigned char>((value >> 16) & 0xff),
        static_cast<unsigned char>((value >> 8) & 0xff),
        static_cast<unsigned char>(value & 0xff)
    };
}

void validateRectangleDimensions(float width, float depth, const char* objectName)
{
    if (width <= 0.0f || depth <= 0.0f)
    {
        throw std::runtime_error(std::string(objectName) + " width/depth must be positive");
    }
}

CourseMap courseFromJson(const nlohmann::json& json)
{
    CourseMap course;
    course.courseId = optionalString(json, "courseId", "simple-circuit");
    course.lapCount = optionalInt(json, "lapCount", 10);

    if (!json.contains("start") || !json.at("start").is_object())
    {
        throw std::runtime_error("missing start object");
    }
    const nlohmann::json& start = json.at("start");
    course.startPosition = requiredPoint(start, "position");
    course.startAngle = requiredFloat(start, "angle");

    if (!json.contains("startLine") || !json.at("startLine").is_object())
    {
        throw std::runtime_error("missing startLine object");
    }
    const nlohmann::json& startLine = json.at("startLine");
    const float startLineWidth = requiredFloat(startLine, "width");
    const float startLineDepth = requiredFloat(startLine, "depth");
    const int checkerCount = optionalInt(startLine, "checkerCount", DEFAULT_START_LINE_CHECKERS);
    validateRectangleDimensions(startLineWidth, startLineDepth, "startLine");
    if (checkerCount <= 0)
    {
        throw std::runtime_error("startLine checkerCount must be positive");
    }
    course.startLine = makeCourseStartLine(
        requiredPoint(startLine, "center"),
        startLineWidth,
        startLineDepth,
        requiredFloat(startLine, "forwardAngle"),
        checkerCount);

    if (!json.contains("lapCheckpoint") || !json.at("lapCheckpoint").is_object())
    {
        throw std::runtime_error("missing lapCheckpoint object");
    }
    const nlohmann::json& lapCheckpoint = json.at("lapCheckpoint");
    const float checkpointWidth = requiredFloat(lapCheckpoint, "width");
    const float checkpointDepth = requiredFloat(lapCheckpoint, "depth");
    validateRectangleDimensions(checkpointWidth, checkpointDepth, "lapCheckpoint");
    course.lapCheckpoint = makeCourseCheckpoint(
        requiredPoint(lapCheckpoint, "center"),
        checkpointWidth,
        checkpointDepth);

    if (!json.contains("walls") || !json.at("walls").is_array())
    {
        throw std::runtime_error("missing walls array");
    }
    for (const nlohmann::json& wall : json.at("walls"))
    {
        const float wallWidth = requiredFloat(wall, "width");
        const float wallDepth = requiredFloat(wall, "depth");
        validateRectangleDimensions(wallWidth, wallDepth, "wall");
        course.walls.push_back(makeCourseWall(
            requiredFloat(wall, "x"),
            requiredFloat(wall, "z"),
            wallWidth,
            wallDepth,
            colorFromHexString(optionalString(wall, "color", "808080"))));
    }

    if (course.walls.empty())
    {
        throw std::runtime_error("course must include at least one wall");
    }

    return course;
}

CourseMap courseFromJsonText(const std::string& jsonText)
{
    return courseFromJson(nlohmann::json::parse(jsonText));
}
}

CourseWall makeCourseWall(float x, float z, float width, float depth, Color color)
{
    return { { x, z, width, depth }, color };
}

CourseStartLine makeCourseStartLine(Vector2 center, float width, float depth, float forwardAngle, int checkerCount)
{
    return { { center.x - width * 0.5f, center.y - depth * 0.5f, width, depth }, forwardAngle, checkerCount };
}

CourseCheckpoint makeCourseCheckpoint(Vector2 center, float width, float depth)
{
    return { { center.x - width * 0.5f, center.y - depth * 0.5f, width, depth } };
}

CourseMap createSimpleCircuitCourse()
{
    try
    {
        return courseFromJsonText(defaultCourseJson());
    }
    catch (const std::exception& e)
    {
        std::cout << "error: baked course JSON failed to parse: " << e.what() << std::endl;
        const Color wallColor = GRAY;
        CourseMap course;
        course.courseId = "simple-circuit";
        course.startPosition = { 2.0f, -14.0f };
        course.startAngle = 0.0f;
        course.lapCount = 10;
        course.startLine = makeCourseStartLine({ 0.0f, -14.0f }, 1.0f, 12.0f, 0.0f, DEFAULT_START_LINE_CHECKERS);
        course.lapCheckpoint = makeCourseCheckpoint({ 0.0f, 14.0f }, 1.0f, 12.0f);
        course.walls = {
            makeCourseWall(OUTER_MIN_X - WALL_THICKNESS, OUTER_MIN_Z - WALL_THICKNESS,
                (OUTER_MAX_X - OUTER_MIN_X) + WALL_THICKNESS * 2.0f, WALL_THICKNESS, wallColor),
            makeCourseWall(OUTER_MIN_X - WALL_THICKNESS, OUTER_MAX_Z,
                (OUTER_MAX_X - OUTER_MIN_X) + WALL_THICKNESS * 2.0f, WALL_THICKNESS, wallColor),
            makeCourseWall(OUTER_MIN_X - WALL_THICKNESS, OUTER_MIN_Z,
                WALL_THICKNESS, OUTER_MAX_Z - OUTER_MIN_Z, wallColor),
            makeCourseWall(OUTER_MAX_X, OUTER_MIN_Z,
                WALL_THICKNESS, OUTER_MAX_Z - OUTER_MIN_Z, wallColor),
            makeCourseWall(INNER_MIN_X, INNER_MIN_Z,
                INNER_MAX_X - INNER_MIN_X, WALL_THICKNESS, wallColor),
            makeCourseWall(INNER_MIN_X, INNER_MAX_Z - WALL_THICKNESS,
                INNER_MAX_X - INNER_MIN_X, WALL_THICKNESS, wallColor),
            makeCourseWall(INNER_MIN_X, INNER_MIN_Z,
                WALL_THICKNESS, INNER_MAX_Z - INNER_MIN_Z, wallColor),
            makeCourseWall(INNER_MAX_X - WALL_THICKNESS, INNER_MIN_Z,
                WALL_THICKNESS, INNER_MAX_Z - INNER_MIN_Z, wallColor)
        };
        return course;
    }
}

CourseMap loadCourseOrDefault(const std::string& path)
{
    std::ifstream file(path);
    if (file.good())
    {
        try
        {
            CourseMap course = courseFromJson(nlohmann::json::parse(file));
            std::cout << "Loaded course " << course.courseId << " from " << path << " with " << course.walls.size() << " walls" << std::endl;
            return course;
        }
        catch (const std::exception& e)
        {
            std::cout << "error: failed to load course " << path << ": " << e.what() << std::endl;
        }
    }
    else
    {
        std::cout << "warning: course file not found, using baked course: " << path << std::endl;
    }

    return createSimpleCircuitCourse();
}

bool courseCollidesWithWalls(const CourseMap& course, Vector2 position, float radius)
{
    for (const CourseWall& wall : course.walls)
    {
        if (CheckCollisionCircleRec(position, radius, wall.bounds))
        {
            return true;
        }
    }

    return false;
}

float courseStartLineSignedDistance(const CourseMap& course, Vector2 position)
{
    const Vector2 center = {
        course.startLine.bounds.x + course.startLine.bounds.width * 0.5f,
        course.startLine.bounds.y + course.startLine.bounds.height * 0.5f
    };
    const Vector2 forward = directionFromDegrees(course.startLine.forwardAngle);
    return ((position.x - center.x) * forward.x) + ((position.y - center.y) * forward.y);
}

bool courseOverlapsStartLine(const CourseMap& course, Vector2 position, float radius)
{
    return CheckCollisionCircleRec(position, radius, course.startLine.bounds);
}

bool courseOverlapsLapCheckpoint(const CourseMap& course, Vector2 position, float radius)
{
    return CheckCollisionCircleRec(position, radius, course.lapCheckpoint.bounds);
}

bool courseCrossedStartLineForward(const CourseMap& course, Vector2 position, float previousDistance, float currentDistance, float radius)
{
    return (previousDistance < 0.0f) &&
        (currentDistance >= 0.0f) &&
        courseOverlapsStartLine(course, position, radius);
}

void drawCourseWalls(const CourseMap& course)
{
    for (const CourseWall& wall : course.walls)
    {
        drawWall(wall);
    }
}

void drawCourseStartLine(const CourseMap& course)
{
    const int rows = course.startLine.checkerCount;
    const int columns = 2;
    const float cellWidth = course.startLine.bounds.width / static_cast<float>(columns);
    const float cellDepth = course.startLine.bounds.height / static_cast<float>(rows);

    for (int row = 0; row < rows; ++row)
    {
        for (int column = 0; column < columns; ++column)
        {
            const bool blackCell = ((row + column) % 2) == 0;
            const Color cellColor = blackCell ? BLACK : RAYWHITE;
            const Vector3 center = {
                course.startLine.bounds.x + (static_cast<float>(column) + 0.5f) * cellWidth,
                WALL_BASE_Y + START_LINE_HEIGHT,
                course.startLine.bounds.y + (static_cast<float>(row) + 0.5f) * cellDepth
            };
            const Vector3 size = { cellWidth, START_LINE_HEIGHT, cellDepth };
            DrawCubeV(center, size, cellColor);
        }
    }
}

void drawCourseCheckpoint(const CourseMap& course)
{
    const Vector3 center = {
        course.lapCheckpoint.bounds.x + course.lapCheckpoint.bounds.width * 0.5f,
        WALL_BASE_Y + CHECKPOINT_LINE_HEIGHT,
        course.lapCheckpoint.bounds.y + course.lapCheckpoint.bounds.height * 0.5f
    };
    const Vector3 size = { course.lapCheckpoint.bounds.width, CHECKPOINT_LINE_HEIGHT, course.lapCheckpoint.bounds.height };
    DrawCubeV(center, size, Fade(LIME, 0.55f));
}
