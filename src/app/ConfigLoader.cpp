#include "app/ConfigLoader.h"

#include <algorithm>
#include <cstdio>
#include <fstream>

#include "app/TomlSafe.h"

namespace drone {

namespace {

template <typename T>
void loadField(const toml::table& tbl, const char* section, const char* key, T& dest) {
    auto node = tbl.at_path(section);
    if (!node.is_table())
        return;
    auto field = node.as_table()->get(key);
    if (!field)
        return;
    if (auto val = field->value<T>()) {
        dest = *val;
    }
}

void warn(const char* msg, const char* key) {
    std::fprintf(stderr, "[config] %s: usando default para '%s'\n", msg, key);
}

}  // namespace

GameConfig loadConfig(const std::string& path) {
    GameConfig cfg;

    try {
        toml::table tbl = toml::parse_file(path);

        loadField(tbl, "physics", "drone_mass", cfg.droneMass);
        loadField(tbl, "physics", "max_thrust", cfg.maxThrust);
        loadField(tbl, "physics", "drag_coefficient", cfg.dragCoefficient);
        loadField(tbl, "physics", "drone_radius", cfg.droneRadius);
        loadField(tbl, "physics", "crash_speed", cfg.crashSpeed);

        loadField(tbl, "battery", "max", cfg.batteryMax);
        loadField(tbl, "battery", "per_newton", cfg.batteryPerNewton);
        loadField(tbl, "battery", "low_threshold", cfg.batteryLowThreshold);

        loadField(tbl, "loop", "fixed_timestep", cfg.fixedTimestep);
        loadField(tbl, "loop", "max_frame_time", cfg.maxFrameTime);
        loadField(tbl, "loop", "thrust_pulse_seconds", cfg.thrustPulseSeconds);

        loadField(tbl, "progression", "xp_per_level_base", cfg.xpPerLevelBase);
        loadField(tbl, "progression", "unlock_level", cfg.unlockLevel);
        loadField(tbl, "progression", "xp_per_second", cfg.xpPerSecond);

        loadField(tbl, "world", "half_extent", cfg.worldHalfExtent);
        loadField(tbl, "world", "max_altitude", cfg.maxAltitude);

        loadField(tbl, "environment", "wind_base_speed", cfg.windBaseSpeed);
        loadField(tbl, "environment", "wind_smoothing", cfg.windSmoothing);
        loadField(tbl, "environment", "gust_min_interval", cfg.gustMinInterval);
        loadField(tbl, "environment", "gust_max_interval", cfg.gustMaxInterval);
        loadField(tbl, "environment", "difficulty_ramp", cfg.difficultyRamp);
        loadField(tbl, "environment", "max_difficulty", cfg.maxDifficulty);

        loadField(tbl, "drone.frame", "arm_length", cfg.armLength);
        loadField(tbl, "drone.frame", "yaw_torque_factor", cfg.yawTorqueFactor);
        loadField(tbl, "drone.frame", "motor_time_constant", cfg.motorTimeConstant);
        loadField(tbl, "drone.frame", "inertia", cfg.droneInertia);
        loadField(tbl, "drone.frame", "max_tilt_deg", cfg.maxTiltDeg);

        loadField(tbl, "drone.pid", "roll_kp", cfg.pidRollKp);
        loadField(tbl, "drone.pid", "roll_ki", cfg.pidRollKi);
        loadField(tbl, "drone.pid", "roll_kd", cfg.pidRollKd);
        loadField(tbl, "drone.pid", "pitch_kp", cfg.pidPitchKp);
        loadField(tbl, "drone.pid", "pitch_ki", cfg.pidPitchKi);
        loadField(tbl, "drone.pid", "pitch_kd", cfg.pidPitchKd);
        loadField(tbl, "drone.pid", "yaw_kp", cfg.pidYawKp);
        loadField(tbl, "drone.pid", "yaw_ki", cfg.pidYawKi);
        loadField(tbl, "drone.pid", "yaw_ff", cfg.pidYawFF);

        loadField(tbl, "drone.assist", "altitude_hold_kp", cfg.altitudeHoldKp);
        loadField(tbl, "drone.assist", "yaw_hold_kp", cfg.yawHoldKp);
        loadField(tbl, "drone.assist", "failsafe_timeout", cfg.failsafeTimeout);
        loadField(tbl, "drone.assist", "failsafe_descent_per_s", cfg.failsafeDescentPerSecond);
        loadField(tbl, "drone.assist", "trim_step_deg", cfg.trimStepDeg);

        loadField(tbl, "materials", "drone_friction", cfg.droneFriction);
        loadField(tbl, "materials", "drone_bounciness", cfg.droneBounciness);
        loadField(tbl, "materials", "obstacle_friction", cfg.obstacleFriction);
        loadField(tbl, "materials", "obstacle_bounciness", cfg.obstacleBounciness);
        loadField(tbl, "materials", "ground_friction", cfg.groundFriction);
        loadField(tbl, "materials", "ground_bounciness", cfg.groundBounciness);
        loadField(tbl, "drone", "linear_damping", cfg.linearDamping);
        loadField(tbl, "drone", "angular_damping", cfg.angularDamping);

        validateConfig(cfg);
    } catch (const toml::parse_error& err) {
        std::fprintf(stderr, "[config] error parseando %s: %s\n", path.c_str(),
                     err.description().data());
    } catch (const std::exception& e) {
        std::fprintf(stderr, "[config] error leyendo %s: %s\n", path.c_str(), e.what());
    }

    return cfg;
}

bool validateConfig(GameConfig& cfg) {
    bool valid = true;
    const auto clampF = [&](float& v, float lo, float hi, const char* key) {
        if (v < lo || v > hi) {
            warn("fuera de rango", key);
            v = std::max(lo, std::min(hi, v));
            valid = false;
        }
    };
    const auto clampI = [&](int& v, int lo, int hi, const char* key) {
        if (v < lo || v > hi) {
            warn("fuera de rango", key);
            v = std::max(lo, std::min(hi, v));
            valid = false;
        }
    };

    clampF(cfg.droneMass, 0.01f, 100.0f, "physics.drone_mass");
    clampF(cfg.maxThrust, 0.1f, 1e6f, "physics.max_thrust");
    clampF(cfg.dragCoefficient, 0.0f, 100.0f, "physics.drag_coefficient");
    clampF(cfg.droneRadius, 0.01f, 10.0f, "physics.drone_radius");
    clampF(cfg.crashSpeed, 0.1f, 1e3f, "physics.crash_speed");
    clampF(cfg.batteryMax, 1.0f, 1e6f, "battery.max");
    clampF(cfg.batteryPerNewton, 0.0f, 100.0f, "battery.per_newton");
    clampF(cfg.batteryLowThreshold, 0.0f, cfg.batteryMax, "battery.low_threshold");
    clampF(cfg.fixedTimestep, 0.001f, 1.0f, "loop.fixed_timestep");
    clampF(cfg.maxFrameTime, 0.01f, 10.0f, "loop.max_frame_time");
    clampF(cfg.thrustPulseSeconds, 0.01f, 10.0f, "loop.thrust_pulse_seconds");
    clampF(cfg.worldHalfExtent, 1.0f, 1e6f, "world.half_extent");
    clampF(cfg.maxAltitude, 1.0f, 1e6f, "world.max_altitude");
    clampF(cfg.windBaseSpeed, 0.0f, 1e3f, "environment.wind_base_speed");
    clampF(cfg.windSmoothing, 0.01f, 100.0f, "environment.wind_smoothing");
    clampF(cfg.gustMinInterval, 0.1f, 3600.0f, "environment.gust_min_interval");
    clampF(cfg.gustMaxInterval, cfg.gustMinInterval, 3600.0f, "environment.gust_max_interval");
    clampF(cfg.difficultyRamp, 0.0f, 100.0f, "environment.difficulty_ramp");
    clampF(cfg.maxDifficulty, 1.0f, 1e6f, "environment.max_difficulty");
    clampI(cfg.xpPerLevelBase, 1, 1e6, "progression.xp_per_level_base");
    clampI(cfg.unlockLevel, 1, 1000, "progression.unlock_level");
    clampF(cfg.xpPerSecond, 0.0f, 1e6f, "progression.xp_per_second");

    clampF(cfg.armLength, 0.01f, 5.0f, "drone.frame.arm_length");
    clampF(cfg.yawTorqueFactor, 0.0f, 10.0f, "drone.frame.yaw_torque_factor");
    clampF(cfg.motorTimeConstant, 0.001f, 5.0f, "drone.frame.motor_time_constant");
    clampF(cfg.droneInertia, 1e-4f, 100.0f, "drone.frame.inertia");
    clampF(cfg.maxTiltDeg, 0.0f, 80.0f, "drone.frame.max_tilt_deg");
    clampF(cfg.altitudeHoldKp, 0.0f, 1e3f, "drone.assist.altitude_hold_kp");
    clampF(cfg.yawHoldKp, 0.0f, 1e3f, "drone.assist.yaw_hold_kp");
    clampF(cfg.failsafeTimeout, 0.1f, 3600.0f, "drone.assist.failsafe_timeout");
    clampF(cfg.failsafeDescentPerSecond, 0.0f, 10.0f, "drone.assist.failsafe_descent_per_s");
    clampF(cfg.trimStepDeg, 0.0f, 10.0f, "drone.assist.trim_step_deg");

    return valid;
}

physics::PhysicsSettings loadPhysicsConfig(const std::string& path) {
    physics::PhysicsSettings cfg;

    try {
        toml::table tbl = toml::parse_file(path);

        auto loadV3 = [&](const char* key, Vec3& v) {
            if (auto arr = tbl.at_path(key).as_array()) {
                if (arr->size() >= 3) {
                    v.x = arr->get(0)->value_or(v.x);
                    v.y = arr->get(1)->value_or(v.y);
                    v.z = arr->get(2)->value_or(v.z);
                }
            }
        };

        loadV3("physics.world.gravity", cfg.gravity);
        loadField(tbl, "physics.world", "persistent_contact_distance",
                  cfg.persistentContactDistance);
        loadField(tbl, "physics.world", "default_friction_coefficient",
                  cfg.defaultFrictionCoefficient);
        loadField(tbl, "physics.world", "default_bounciness", cfg.defaultBounciness);
        loadField(tbl, "physics.world", "restitution_velocity_threshold",
                  cfg.restitutionVelocityThreshold);
        loadField(tbl, "physics.world", "velocity_solver_iterations", cfg.velocitySolverIterations);
        loadField(tbl, "physics.world", "position_solver_iterations", cfg.positionSolverIterations);
        loadField(tbl, "physics.world", "cos_angle_similar_contact_manifold",
                  cfg.cosAngleSimilarContactManifold);

        loadField(tbl, "physics.sleeping", "enabled", cfg.sleepingEnabled);
        loadField(tbl, "physics.sleeping", "time_before_sleep", cfg.timeBeforeSleep);
        loadField(tbl, "physics.sleeping", "sleep_linear_velocity", cfg.sleepLinearVelocity);
        loadField(tbl, "physics.sleeping", "sleep_angular_velocity", cfg.sleepAngularVelocity);

        loadField(tbl, "physics.step", "fixed_timestep", cfg.fixedTimestep);
        loadField(tbl, "physics.step", "max_frame_time", cfg.maxFrameTime);
        loadField(tbl, "physics.step", "max_sub_steps", cfg.maxSubSteps);

        loadField(tbl, "physics.debug", "enabled", cfg.debugEnabled);

        validatePhysicsConfig(cfg);
    } catch (const toml::parse_error& err) {
        // Antes se tragaba cualquier excepción en silencio: un TOML mal
        // escrito arrancaba con los defaults sin decir una palabra.
        std::fprintf(stderr, "[config] error parseando %s: %s\n", path.c_str(),
                     err.description().data());
    } catch (const std::exception& e) {
        std::fprintf(stderr, "[config] error leyendo %s: %s\n", path.c_str(), e.what());
    }

    return cfg;
}

bool validatePhysicsConfig(physics::PhysicsSettings& cfg) {
    bool valid = true;
    const auto clampF = [&](float& v, float lo, float hi, const char* key) {
        if (v < lo || v > hi) {
            warn("fuera de rango", key);
            v = std::max(lo, std::min(hi, v));
            valid = false;
        }
    };
    const auto clampI = [&](int& v, int lo, int hi, const char* key) {
        if (v < lo || v > hi) {
            warn("fuera de rango", key);
            v = std::max(lo, std::min(hi, v));
            valid = false;
        }
    };

    clampF(cfg.persistentContactDistance, 0.0f, 1.0f, "physics.world.persistent_contact_distance");
    clampF(cfg.defaultFrictionCoefficient, 0.0f, 100.0f,
           "physics.world.default_friction_coefficient");
    clampF(cfg.defaultBounciness, 0.0f, 1.0f, "physics.world.default_bounciness");
    clampF(cfg.restitutionVelocityThreshold, 0.0f, 100.0f,
           "physics.world.restitution_velocity_threshold");
    clampF(cfg.cosAngleSimilarContactManifold, -1.0f, 1.0f,
           "physics.world.cos_angle_similar_contact_manifold");
    // rp3d guarda las iteraciones en un uint16: pasarse trunca en silencio.
    clampI(cfg.velocitySolverIterations, 1, 1000, "physics.world.velocity_solver_iterations");
    clampI(cfg.positionSolverIterations, 1, 1000, "physics.world.position_solver_iterations");
    clampF(cfg.timeBeforeSleep, 0.0f, 3600.0f, "physics.sleeping.time_before_sleep");
    clampF(cfg.sleepLinearVelocity, 0.0f, 100.0f, "physics.sleeping.sleep_linear_velocity");
    clampF(cfg.sleepAngularVelocity, 0.0f, 100.0f, "physics.sleeping.sleep_angular_velocity");
    clampF(cfg.fixedTimestep, 0.001f, 1.0f, "physics.step.fixed_timestep");
    clampF(cfg.maxFrameTime, 0.01f, 10.0f, "physics.step.max_frame_time");
    clampI(cfg.maxSubSteps, 1, 16, "physics.step.max_sub_steps");

    return valid;
}

}  // namespace drone
