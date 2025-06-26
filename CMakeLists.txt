cmake_minimum_required(VERSION 3.10)
project(DroneFlightSim)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Define the source directory (adjust if necessary)
set(SRC_DIR ${CMAKE_SOURCE_DIR})

# List of source files
set(SOURCES
    ${SRC_DIR}/main.cpp
    ${SRC_DIR}/Drone.cpp
    ${SRC_DIR}/Environment.cpp
    ${SRC_DIR}/PhysicsEngine.cpp
    ${SRC_DIR}/UIManager.cpp
    ${SRC_DIR}/Game.cpp
    ${SRC_DIR}/PlayerProgression.cpp
)

# Create the executable target
add_executable(DroneFlightSim ${SOURCES})

# Include the source directory for headers
target_include_directories(DroneFlightSim PUBLIC ${SRC_DIR})


# Si en el futuro deseas enlazar librerías externas (SDL2, OpenGL, Bullet, etc.),
# puedes agregar las siguientes líneas de ejemplo:
#
# find_package(SDL2 REQUIRED)
# include_directories(${SDL2_INCLUDE_DIRS})
# target_link_libraries(DroneFlightSim ${SDL2_LIBRARIES})
#
# find_package(OpenGL REQUIRED)
# target_link_libraries(DroneFlightSim ${OPENGL_LIBRARIES})

