#include "courseMap.hpp"

namespace
{
constexpr float WALL_HEIGHT = 0.45f;
constexpr float WALL_THICKNESS = 1.0f;
constexpr float WALL_BASE_Y = -0.03f;
constexpr float OUTER_MIN_X = -34.0f;
constexpr float OUTER_MAX_X = 34.0f;
constexpr float OUTER_MIN_Z = -22.0f;
constexpr float OUTER_MAX_Z = 22.0f;
constexpr float INNER_MIN_X = -16.0f;
constexpr float INNER_MAX_X = 16.0f;
constexpr float INNER_MIN_Z = -8.0f;
constexpr float INNER_MAX_Z = 8.0f;
constexpr float START_X = 0.0f;
constexpr float START_Z = -14.0f;
constexpr float START_ANGLE = 0.0f;

CourseWall makeWall(float x, float z, float width, float depth, Color color)
{
    return { { x, z, width, depth }, color };
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

CourseMap createSimpleCircuitCourse()
{
    const Color wallColor = GRAY;
    return {
        { START_X, START_Z },
        START_ANGLE,
        {
            makeWall(OUTER_MIN_X - WALL_THICKNESS, OUTER_MIN_Z - WALL_THICKNESS,
                (OUTER_MAX_X - OUTER_MIN_X) + WALL_THICKNESS * 2.0f, WALL_THICKNESS, wallColor),
            makeWall(OUTER_MIN_X - WALL_THICKNESS, OUTER_MAX_Z,
                (OUTER_MAX_X - OUTER_MIN_X) + WALL_THICKNESS * 2.0f, WALL_THICKNESS, wallColor),
            makeWall(OUTER_MIN_X - WALL_THICKNESS, OUTER_MIN_Z,
                WALL_THICKNESS, OUTER_MAX_Z - OUTER_MIN_Z, wallColor),
            makeWall(OUTER_MAX_X, OUTER_MIN_Z,
                WALL_THICKNESS, OUTER_MAX_Z - OUTER_MIN_Z, wallColor),

            makeWall(INNER_MIN_X, INNER_MIN_Z,
                INNER_MAX_X - INNER_MIN_X, WALL_THICKNESS, wallColor),
            makeWall(INNER_MIN_X, INNER_MAX_Z - WALL_THICKNESS,
                INNER_MAX_X - INNER_MIN_X, WALL_THICKNESS, wallColor),
            makeWall(INNER_MIN_X, INNER_MIN_Z,
                WALL_THICKNESS, INNER_MAX_Z - INNER_MIN_Z, wallColor),
            makeWall(INNER_MAX_X - WALL_THICKNESS, INNER_MIN_Z,
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

void drawCourseWalls(const CourseMap& course)
{
    for (const CourseWall& wall : course.walls)
    {
        drawWall(wall);
    }
}
