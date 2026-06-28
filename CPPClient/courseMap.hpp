#pragma once

#include <array>
#include <raylib.h>
#include <string>

struct CourseWall
{
    Rectangle bounds;
    Color color;
};

struct CourseStartLine
{
    Rectangle bounds;
    float forwardAngle;
    int checkerCount;
};

struct CourseMap
{
    std::string courseId;
    Vector2 startPosition;
    float startAngle;
    int lapCount;
    CourseStartLine startLine;
    std::array<CourseWall, 8> walls;
};

CourseWall makeCourseWall(float x, float z, float width, float depth, Color color);
CourseStartLine makeCourseStartLine(Vector2 center, float width, float depth, float forwardAngle, int checkerCount);
CourseMap createSimpleCircuitCourse();
bool courseCollidesWithWalls(const CourseMap& course, Vector2 position, float radius);
float courseStartLineSignedDistance(const CourseMap& course, Vector2 position);
bool courseCrossedStartLineForward(const CourseMap& course, float previousDistance, float currentDistance, float radius);
void drawCourseWalls(const CourseMap& course);
void drawCourseStartLine(const CourseMap& course);
