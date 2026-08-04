add_library(drone_core STATIC
    Drone.cpp
    Environment.cpp
    EventBus.cpp
    GameController.cpp
    PhysicsEngine.cpp
    PlayerProgression.cpp
    World.cpp
)

# Un único root de includes: #include "core/..." y "frontend/...".
target_include_directories(drone_core PUBLIC ${CMAKE_SOURCE_DIR}/src)
target_compile_features(drone_core PUBLIC cxx_std_17)
target_link_libraries(drone_core PRIVATE drone_warnings)
