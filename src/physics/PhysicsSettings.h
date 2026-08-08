#pragma once

#include "core/math/Vec3.h"

namespace drone::physics {

struct PhysicsSettings {
    // World
    Vec3 gravity{0.0f, -9.81f, 0.0f};
    float persistentContactDistance = 0.03f;
    float defaultFrictionCoefficient = 0.3f;
    float defaultBounciness = 0.5f;
    float restitutionVelocityThreshold = 0.5f;
    int velocitySolverIterations = 6;
    int positionSolverIterations = 3;
    float cosAngleSimilarContactManifold = 0.95f;

    // Sleeping
    bool sleepingEnabled = true;
    float timeBeforeSleep = 1.0f;          // rp3d: defaultTimeBeforeSleep
    float sleepLinearVelocity = 0.02f;     // rp3d: defaultSleepLinearVelocity
    float sleepAngularVelocity = 0.0523f;  // rp3d: defaultSleepAngularVelocity

    // Step
    float fixedTimestep = 1.0f / 60.0f;
    float maxFrameTime = 0.25f;
    int maxSubSteps = 4;

    // Debug
    bool debugEnabled = false;
};

}  // namespace drone::physics
