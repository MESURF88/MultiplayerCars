#pragma once

#include <array>
#include <raylib.h>

struct CourseWall
{
    Rectangle bounds;
    Color color;
};

struct CourseMap
{
    Vector2 startPosition;
    float startAngle;
    std::array<CourseWall, 8> walls;
};

CourseMap createSimpleCircuitCourse();
bool courseCollidesWithWalls(const CourseMap& course, Vector2 position, float radius);
void drawCourseWalls(const CourseMap& course);
