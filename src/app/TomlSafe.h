#pragma once

// toml++ usa TOML_ASSERT (assert de C) para invariantes internos, y algunos
// caminos de entrada malformada lo disparan en builds Debug, abortando el
// proceso (p. ej. un fichero que empieza por "[[["). Se desactiva para que
// TODA entrada inválida termine en toml::parse_error — el mismo
// comportamiento que un build Release con NDEBUG. Incluir siempre este
// header en lugar de <toml++/toml.hpp>.
#define TOML_ASSERT(...) static_cast<void>(0)
#include <toml++/toml.hpp>
