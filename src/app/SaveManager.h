#pragma once

#include <string>

#include "core/SaveData.h"
#include "core/WorldState.h"

namespace drone {

bool saveGame(const std::string& path, const SaveData& data);
SaveData loadGame(const std::string& path);
std::string saveDirectory();
std::string saveFilePath();
SaveData buildSaveData(const WorldState& state);

}  // namespace drone
