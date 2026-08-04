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

        loadField(tbl, "physics", "gravity", cfg.gravity);
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

    clampF(cfg.gravity, 0.1f, 100.0f, "physics.gravity");
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

    return valid;
}

}  // namespace drone
