add_library(drone_frontend_terminal STATIC
    terminal/TerminalInput.cpp
    terminal/TerminalRenderer.cpp
)

target_link_libraries(drone_frontend_terminal
    PUBLIC drone_core
    PRIVATE drone_warnings
)

add_subdirectory(raylib)
