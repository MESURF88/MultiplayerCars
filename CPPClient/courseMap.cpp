#include "courseMap.hpp"

#include <cmath>

namespace
{
constexpr float WALL_HEIGHT = 0.45f;
constexpr float WALL_THICKNESS = 1.0f;
constexpr float WALL_BASE_Y = -0.03f;
constexpr float START_LINE_HEIGHT = 0.025f;
constexpr float OUTER_MIN_X = -34.0f;
constexpr float OUTER_MAX_X = 34.0f;
constexpr float OUTER_MIN_Z = -22.0f;
constexpr float OUTER_MAX_Z = 22.0f;
constexpr float INNER_MIN_X = -16.0f;
constexpr float INNER_MAX_X = 16.0f;
constexpr float INNER_MIN_Z = -8.0f;
constexpr float INNER_MAX_Z = 8.0f;
constexpr float START_X = 2.0f;
constexpr float START_Z = -14.0f;
constexpr float START_ANGLE = 0.0f;
constexpr int COURSE_LAPS = 10;
constexpr int START_LINE_CHECKERS = 14;

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
}

CourseWall makeCourseWall(float x, float z, float width, float depth, Color color)
{
    return { { x, z, width, depth }, color };
}

CourseStartLine makeCourseStartLine(Vector2 center, float width, float depth, float forwardAngle, int checkerCount)
{
    return { { center.x - width * 0.5f, center.y - depth * 0.5f, width, depth }, forwardAngle, checkerCount };
}

CourseMap createSimpleCircuitCourse()
{
    const Color wallColor = GRAY;
    return {
        "simple-circuit",
        { START_X, START_Z },
        START_ANGLE,
        COURSE_LAPS,
        makeCourseStartLine({ 0.0f, START_Z }, 1.0f, 12.0f, START_ANGLE, START_LINE_CHECKERS),
        {
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
        }
    };
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

bool courseCrossedStartLineForward(const CourseMap& course, float previousDistance, float currentDistance, float radius)
{
    return (previousDistance < -radius) && (currentDistance >= radius);
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
