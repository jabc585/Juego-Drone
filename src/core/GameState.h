#pragma once

namespace drone {

// Máquina de estados de GameController (PLAN2.md §6.5).
enum class GameState { Booting, Playing, Paused, Settings, GameOver, ShuttingDown };

}  // namespace drone
