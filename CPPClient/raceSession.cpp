#include "raceSession.hpp"

#include <algorithm>
#include <chrono>

std::int64_t currentEpochMilliseconds()
{
    const auto now = std::chrono::system_clock::now().time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
}

void resetRaceSession(RaceSession& session, const CourseMap& course)
{
    session.state = RaceRunState::STATE_WAITING;
    session.totalLaps = course.lapCount;
    session.completedLaps = 0;
    session.startEpochMs = 0;
    session.previousStartLineDistance = courseStartLineSignedDistance(course, course.startPosition);
    session.hasPreviousStartLineDistance = true;
    session.lapArmed = false;
}

void scheduleRaceStart(RaceSession& session, const CourseMap& course, std::int64_t startEpochMs, int totalLaps)
{
    session.state = RaceRunState::STATE_COUNTDOWN;
    session.totalLaps = std::max(1, totalLaps);
    session.completedLaps = 0;
    session.startEpochMs = startEpochMs;
    session.previousStartLineDistance = courseStartLineSignedDistance(course, course.startPosition);
    session.hasPreviousStartLineDistance = true;
    session.lapArmed = false;
}

void updateRaceSession(RaceSession& session, const CourseMap& course, Vector2 carPosition, float carRadius, std::int64_t nowEpochMs)
{
    if ((session.state == RaceRunState::STATE_COUNTDOWN) && (nowEpochMs >= session.startEpochMs))
    {
        session.state = RaceRunState::STATE_RACING;
    }

    const float currentDistance = courseStartLineSignedDistance(course, carPosition);
    if (!session.hasPreviousStartLineDistance)
    {
        session.previousStartLineDistance = currentDistance;
        session.hasPreviousStartLineDistance = true;
        return;
    }

    if ((session.state == RaceRunState::STATE_RACING) &&
        !session.lapArmed &&
        courseOverlapsLapCheckpoint(course, carPosition, carRadius))
    {
        session.lapArmed = true;
    }

    if ((session.state == RaceRunState::STATE_RACING) &&
        session.lapArmed &&
        courseCrossedStartLineForward(course, carPosition, session.previousStartLineDistance, currentDistance, carRadius))
    {
        session.completedLaps++;
        session.lapArmed = false;
        if (session.completedLaps >= session.totalLaps)
        {
            session.state = RaceRunState::STATE_FINISHED;
        }
    }

    session.previousStartLineDistance = currentDistance;
}

bool raceSessionCanDrive(const RaceSession& session)
{
    return session.state == RaceRunState::STATE_RACING;
}

float raceSessionCountdownSeconds(const RaceSession& session, std::int64_t nowEpochMs)
{
    if (session.state != RaceRunState::STATE_COUNTDOWN)
    {
        return 0.0f;
    }

    return std::max(0.0f, static_cast<float>(session.startEpochMs - nowEpochMs) / 1000.0f);
}

int raceSessionDisplayLap(const RaceSession& session)
{
    if (session.state == RaceRunState::STATE_FINISHED)
    {
        return session.totalLaps;
    }

    return std::clamp(session.completedLaps + 1, 1, std::max(1, session.totalLaps));
}

const char* raceSessionStateLabel(const RaceSession& session)
{
    switch (session.state)
    {
    case RaceRunState::STATE_WAITING:
        return "WAITING";
    case RaceRunState::STATE_COUNTDOWN:
        return "COUNTDOWN";
    case RaceRunState::STATE_RACING:
        return "RACING";
    case RaceRunState::STATE_FINISHED:
        return "FINISHED";
    default:
        return "UNKNOWN";
    }
}
