cmake_minimum_required(VERSION 3.21)
project(DroneFlightSim VERSION 0.5.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(DRONE_WERROR "Treat warnings as errors" OFF)
option(BUILD_TESTS "Build unit tests" ON)

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

# toml++ para carga de configuracion en runtime (PLAN3.md P0-2)
# spdlog para logging estructurado (PLAN3.md P0-1)
include(FetchContent)
# SYSTEM: los warnings de los headers de dependencias no rompen nuestro -Werror.
FetchContent_Declare(
    tomlplusplus
    GIT_REPOSITORY https://github.com/marzer/tomlplusplus.git
    GIT_TAG        v3.4.0
    SYSTEM
)
# v1.14.1: v1.13 no compila con -Werror en libc++ moderno (char_traits
# deprecado en el fmt embebido).
FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG        v1.14.1
    SYSTEM
)
FetchContent_MakeAvailable(tomlplusplus spdlog)

# El fmt embebido de spdlog usa char_traits<char8_type>, deprecado en libc++.
# Es código de la dependencia: se silencia para no ahogar los avisos propios.
if(TARGET spdlog AND NOT MSVC)
    target_compile_options(spdlog PRIVATE -Wno-deprecated-declarations)
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
