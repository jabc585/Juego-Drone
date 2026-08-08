add_library(drone_physics STATIC
    PhysicsManager.cpp
    PhysicsProfiler.cpp
)

# Sin dependencia de drone_core: drone_physics solo necesita la cabecera
# Vec3.h, que no tiene .cpp. Enlazarlo aquí crea un ciclo con el enlace
# PUBLIC del propio core (ld avisa de librerías duplicadas) e invierte la
# dirección de dependencia que fija ADR-009.
target_include_directories(drone_physics PUBLIC ${CMAKE_SOURCE_DIR}/src)
target_compile_features(drone_physics PUBLIC cxx_std_17)

# reactphysics3d es PRIVATE: ADR-009 exige que sus cabeceras no salgan del
# pimpl. Si algún día aparecen en la API pública, esto deja de compilar.
target_link_libraries(drone_physics PRIVATE reactphysics3d drone_warnings)
