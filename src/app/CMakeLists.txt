add_executable(DroneFlightSim main.cpp)

# drone_frontend_terminal ya propaga drone_core (enlace PUBLIC).
target_link_libraries(DroneFlightSim
    PRIVATE drone_frontend_terminal drone_warnings
)
target_compile_definitions(DroneFlightSim
    PRIVATE DRONE_VERSION="${PROJECT_VERSION}"
)

install(TARGETS DroneFlightSim RUNTIME DESTINATION bin)
