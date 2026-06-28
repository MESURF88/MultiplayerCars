#pragma once

#include <raylib.h>
#include <string>
#include <vector>

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

struct CourseCheckpoint
{
    Rectangle bounds;
};

struct CourseMap
{
    std::string courseId;
    Vector2 startPosition;
    float startAngle;
    int lapCount;
    CourseStartLine startLine;
    CourseCheckpoint lapCheckpoint;
    std::vector<CourseWall> walls;
};

CourseWall makeCourseWall(float x, float z, float width, float depth, Color color);
CourseStartLine makeCourseStartLine(Vector2 center, float width, float depth, float forwardAngle, int checkerCount);
CourseCheckpoint makeCourseCheckpoint(Vector2 center, float width, float depth);
CourseMap createSimpleCircuitCourse();
CourseMap loadCourseOrDefault(const std::string& path);
bool courseCollidesWithWalls(const CourseMap& course, Vector2 position, float radius);
bool courseOverlapsStartLine(const CourseMap& course, Vector2 position, float radius);
bool courseOverlapsLapCheckpoint(const CourseMap& course, Vector2 position, float radius);
float courseStartLineSignedDistance(const CourseMap& course, Vector2 position);
bool courseCrossedStartLineForward(const CourseMap& course, Vector2 position, float previousDistance, float currentDistance, float radius);
void drawCourseWalls(const CourseMap& course);
void drawCourseStartLine(const CourseMap& course);
void drawCourseCheckpoint(const CourseMap& course);
