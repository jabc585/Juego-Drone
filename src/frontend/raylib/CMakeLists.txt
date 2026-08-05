find_package(raylib QUIET)

if(NOT raylib_FOUND)
    message(STATUS "raylib not found — skipping drone_frontend_raylib")
    return()
endif()

add_library(drone_frontend_raylib STATIC
    RaylibInput.cpp
    RaylibRenderer.cpp
)

target_link_libraries(drone_frontend_raylib
    PUBLIC drone_core raylib
    PRIVATE drone_warnings
)
