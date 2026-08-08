include(FetchContent)
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.5.0
)
FetchContent_MakeAvailable(Catch2)

add_executable(test_core
    unit/TestVec3.cpp
    unit/TestDrone.cpp
    unit/TestHandlePool.cpp
    unit/TestFlightControl.cpp
    unit/TestEventBus.cpp
    unit/TestPhysics.cpp
    unit/TestProgression.cpp
    unit/TestGameController.cpp
    unit/TestTerminalInput.cpp
    unit/TestEnvironment.cpp
    unit/TestEdgeCases.cpp
    integration/TestWorldStep.cpp
)

# drone_frontend_terminal arrastra drone_core como PUBLIC; nombrarlo aqui otra
# vez hace que el enlazador avise de biblioteca duplicada.
target_link_libraries(test_core PRIVATE drone_frontend_terminal Catch2::Catch2WithMain)

# El frontend grafico es opcional: sin raylib el target no existe y estos
# tests no se compilan. ParticleSystem es logica pura, asi que se puede
# ejercitar sin abrir ninguna ventana.
if(TARGET drone_frontend_raylib)
    target_sources(test_core PRIVATE unit/TestParticleSystem.cpp)
    target_link_libraries(test_core PRIVATE drone_frontend_raylib)
endif()

list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(Catch)
catch_discover_tests(test_core)

# Humo E2E (PLAN2.md §11.5): el binario responde a --version/--help y una
# entrada agotada (EOF) termina limpiamente en vez de colgarse (B3).
add_test(NAME smoke_version COMMAND DroneFlightSim --version)
add_test(NAME smoke_help COMMAND DroneFlightSim --help)
if(UNIX)
    add_test(NAME smoke_eof_exits_cleanly
             COMMAND sh -c "printf '' | $<TARGET_FILE:DroneFlightSim>")
    add_test(NAME smoke_piped_commands
             COMMAND sh -c "printf 'wwqqx' | $<TARGET_FILE:DroneFlightSim>")
    set_tests_properties(smoke_eof_exits_cleanly smoke_piped_commands
                         PROPERTIES TIMEOUT 20)
endif()
