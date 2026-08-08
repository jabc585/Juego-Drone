#include "app/SaveManager.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>

#include "app/TomlSafe.h"

namespace drone {

std::string saveDirectory() {
    const char* home = nullptr;
#if defined(_WIN32)
    home = std::getenv("APPDATA");
    if (!home)
        home = std::getenv("USERPROFILE");
#elif defined(__APPLE__)
    home = std::getenv("HOME");
    if (home) {
        std::string dir = std::string(home) + "/Library/Application Support/Juego-Drone";
        return dir;
    }
#else
    home = std::getenv("XDG_DATA_HOME");
    if (!home) {
        home = std::getenv("HOME");
        if (home) {
            std::string dir = std::string(home) + "/.local/share/Juego-Drone";
            return dir;
        }
    } else {
        std::string dir = std::string(home) + "/Juego-Drone";
        return dir;
    }
#endif
    if (!home)
        return "./saves";
    return std::string(home) + "/Juego-Drone";
}

std::string saveFilePath() {
    return saveDirectory() + "/save.toml";
}

SaveData buildSaveData(const WorldState& state) {
    SaveData d;
    d.dronePosX = state.dronePosition.x;
    d.dronePosY = state.dronePosition.y;
    d.dronePosZ = state.dronePosition.z;
    d.droneVelX = state.droneVelocity.x;
    d.droneVelY = state.droneVelocity.y;
    d.droneVelZ = state.droneVelocity.z;
    d.battery = state.battery;
    d.level = state.level;
    d.experience = state.experience;
    d.difficulty = state.difficulty;
    d.simTime = state.simTime;
    d.droneQx = state.droneQx;
    d.droneQy = state.droneQy;
    d.droneQz = state.droneQz;
    d.droneQw = state.droneQw;
    d.state = state.state;
    return d;
}

bool saveGame(const std::string& path, const SaveData& data) {
    toml::table tbl{
        {"version", data.version},
        {"drone_position",
         toml::table{{"x", data.dronePosX}, {"y", data.dronePosY}, {"z", data.dronePosZ}}},
        {"drone_velocity",
         toml::table{{"x", data.droneVelX}, {"y", data.droneVelY}, {"z", data.droneVelZ}}},
        {"battery", data.battery},
        {"level", data.level},
        {"experience", data.experience},
        {"difficulty", data.difficulty},
        {"sim_time", data.simTime},
        {"drone_orientation", toml::table{{"qx", data.droneQx},
                                          {"qy", data.droneQy},
                                          {"qz", data.droneQz},
                                          {"qw", data.droneQw}}},
        {"game_state", static_cast<int>(data.state)},
    };

    try {
        // El directorio de guardado no existe en la primera ejecución.
        std::error_code ec;
        std::filesystem::create_directories(std::filesystem::path(path).parent_path(), ec);
        if (ec)
            return false;

        std::ofstream file(path);
        if (!file)
            return false;
        file << tbl;
        return file.good();
    } catch (...) {
        return false;
    }
}

SaveData loadGame(const std::string& path) {
    SaveData d;
    d.version = -1;

    try {
        toml::table tbl = toml::parse_file(path);

        auto ver = tbl["version"].value<int>();
        if (!ver || *ver < 1)
            return d;
        d.version = *ver;

        if (auto posTbl = tbl["drone_position"].as_table()) {
            d.dronePosX = posTbl->get("x")->value_or(0.0f);
            d.dronePosY = posTbl->get("y")->value_or(0.0f);
            d.dronePosZ = posTbl->get("z")->value_or(0.0f);
        }

        if (auto velTbl = tbl["drone_velocity"].as_table()) {
            d.droneVelX = velTbl->get("x")->value_or(0.0f);
            d.droneVelY = velTbl->get("y")->value_or(0.0f);
            d.droneVelZ = velTbl->get("z")->value_or(0.0f);
        }

        d.battery = tbl["battery"].value_or(100.0f);
        d.level = tbl["level"].value_or(1);
        d.experience = tbl["experience"].value_or(0);
        d.difficulty = tbl["difficulty"].value_or(1.0f);
        d.simTime = tbl["sim_time"].value_or(0.0f);
        if (auto orientTbl = tbl["drone_orientation"].as_table()) {
            d.droneQx = orientTbl->get("qx")->value_or(0.0f);
            d.droneQy = orientTbl->get("qy")->value_or(0.0f);
            d.droneQz = orientTbl->get("qz")->value_or(0.0f);
            d.droneQw = orientTbl->get("qw")->value_or(1.0f);
        }
        int gs = tbl["game_state"].value_or(0);
        d.state = (gs >= 0 && gs <= 5) ? static_cast<GameState>(gs) : GameState::Playing;

        // Un save editado o corrupto con NaN/inf contaminaría toda la
        // simulación: se rechaza entero.
        const float floats[] = {d.dronePosX, d.dronePosY,  d.dronePosZ, d.droneVelX, d.droneVelY,
                                d.droneVelZ, d.droneQx,    d.droneQy,   d.droneQz,   d.droneQw,
                                d.battery,   d.difficulty, d.simTime};
        for (float f : floats) {
            if (!std::isfinite(f)) {
                d.version = -1;
                return d;
            }
        }
    } catch (...) {
        d.version = -1;
    }

    return d;
}

void applySaveData(WorldState& state, const SaveData& data) {
    state.dronePosition = {data.dronePosX, data.dronePosY, data.dronePosZ};
    state.droneVelocity = {data.droneVelX, data.droneVelY, data.droneVelZ};
    state.battery = data.battery;
    state.level = data.level;
    state.experience = data.experience;
    state.difficulty = data.difficulty;
    state.simTime = data.simTime;
    state.state = data.state;
}

}  // namespace drone
