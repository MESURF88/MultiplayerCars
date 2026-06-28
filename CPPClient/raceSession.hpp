#pragma once

#include "courseMap.hpp"

#include <cstdint>

enum class RaceRunState
{
    STATE_WAITING,
    STATE_COUNTDOWN,
    STATE_RACING,
    STATE_FINISHED,
};

struct RaceSession
{
    RaceRunState state = RaceRunState::STATE_WAITING;
    int totalLaps = 0;
    int completedLaps = 0;
    std::int64_t startEpochMs = 0;
    float previousStartLineDistance = 0.0f;
    bool hasPreviousStartLineDistance = false;
    bool lapArmed = false;
};

std::int64_t currentEpochMilliseconds();
void resetRaceSession(RaceSession& session, const CourseMap& course);
void scheduleRaceStart(RaceSession& session, const CourseMap& course, std::int64_t startEpochMs, int totalLaps);
void updateRaceSession(RaceSession& session, const CourseMap& course, Vector2 carPosition, float carRadius, std::int64_t nowEpochMs);
bool raceSessionCanDrive(const RaceSession& session);
float raceSessionCountdownSeconds(const RaceSession& session, std::int64_t nowEpochMs);
int raceSessionDisplayLap(const RaceSession& session);
const char* raceSessionStateLabel(const RaceSession& session);
