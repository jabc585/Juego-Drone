# Contribuir a Juego-Drone

## Flujo de trabajo

1. Crea una rama desde `main`: `feature/<nombre>` o `fix/<nombre>`
2. Implementa los cambios siguiendo la guía de estilo (`docs/style.md`)
3. Añade tests unitarios para nueva funcionalidad
4. Ejecuta tests localmente: `ctest --test-dir build --output-on-failure`
5. Abre un Pull Request a `main`

## Commits

Usa [Conventional Commits](https://www.conventionalcommits.org/):

- `feat:` nueva funcionalidad
- `fix:` corrección de bug
- `refactor:` refactorización sin cambio funcional
- `test:` tests
- `docs:` documentación
- `ci:` configuración de CI/CD
- `chore:` mantenimiento

## Tests

Framework: Catch2 v3.

```bash
cmake -B build -DBUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

## Reglas

- CI debe pasar antes de merge
- Una revisión requerida
- No se suben binarios ni artefactos de build
- Cero `std::cout`/`std::cin` en `src/core/`
