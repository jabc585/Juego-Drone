add_executable(DroneFlightSim
    main.cpp
    ConfigLoader.cpp
    GameLogger.cpp
    SaveManager.cpp
)

set(DRONE_LIBS drone_frontend_terminal drone_warnings tomlplusplus::tomlplusplus spdlog::spdlog)

if(TARGET drone_frontend_raylib)
    list(APPEND DRONE_LIBS drone_frontend_raylib)
    target_compile_definitions(DroneFlightSim PRIVATE DRONE_HAS_RAYLIB=1)
endif()

target_link_libraries(DroneFlightSim PRIVATE ${DRONE_LIBS})

target_compile_definitions(DroneFlightSim
    PRIVATE DRONE_VERSION="${PROJECT_VERSION}"
)

install(TARGETS DroneFlightSim RUNTIME DESTINATION bin)
