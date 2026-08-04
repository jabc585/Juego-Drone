add_executable(DroneFlightSim
    main.cpp
    ConfigLoader.cpp
    GameLogger.cpp
    SaveManager.cpp
)

target_link_libraries(DroneFlightSim
    PRIVATE drone_frontend_terminal drone_warnings
    tomlplusplus::tomlplusplus spdlog::spdlog
)
target_compile_definitions(DroneFlightSim
    PRIVATE DRONE_VERSION="${PROJECT_VERSION}"
)

install(TARGETS DroneFlightSim RUNTIME DESTINATION bin)
