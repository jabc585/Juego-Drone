add_library(drone_core STATIC
    Drone.cpp
    Environment.cpp
    EventBus.cpp
    GameController.cpp
    PlayerProgression.cpp
    World.cpp
    XFrameMixer.cpp
    DronePID.cpp
    AltitudeYawHold.cpp
    FailsafeTrim.cpp
)

target_include_directories(drone_core PUBLIC ${CMAKE_SOURCE_DIR}/src)
target_compile_features(drone_core PUBLIC cxx_std_17)
target_link_libraries(drone_core PUBLIC drone_physics PRIVATE drone_warnings)
