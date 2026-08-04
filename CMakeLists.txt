cmake_minimum_required(VERSION 3.21)
project(DroneFlightSim VERSION 0.5.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(DRONE_WERROR "Treat warnings as errors" OFF)
option(BUILD_TESTS "Build unit tests" ON)

# Warnings compartidos por todos los targets propios (no globales: las
# dependencias de FetchContent compilan con sus propios flags).
add_library(drone_warnings INTERFACE)
if(MSVC)
    target_compile_options(drone_warnings INTERFACE /W4)
    if(DRONE_WERROR)
        target_compile_options(drone_warnings INTERFACE /WX)
    endif()
else()
    target_compile_options(drone_warnings INTERFACE -Wall -Wextra -Wpedantic)
    if(DRONE_WERROR)
        target_compile_options(drone_warnings INTERFACE -Werror)
    endif()
endif()

add_subdirectory(src/core)
add_subdirectory(src/frontend)
add_subdirectory(src/app)

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

set(CPACK_PACKAGE_NAME "DroneFlightSim")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_GENERATOR "TGZ;ZIP")
include(CPack)
