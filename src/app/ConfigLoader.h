#pragma once

#include <string>

#include "core/GameConfig.h"

namespace drone {

// Carga game.toml y devuelve una GameConfig con los defaults sobreescritos
// por los valores del fichero. Los valores fuera de rango se ignoran y se
// notifica por stderr. Si el fichero no existe o no se puede parsear,
// devuelve la configuracion por defecto.
GameConfig loadConfig(const std::string& path);

// Valida que los campos de la configuracion esten en rangos razonables
// y trunca los que no. Devuelve true si todos los valores eran validos.
bool validateConfig(GameConfig& cfg);

}  // namespace drone
