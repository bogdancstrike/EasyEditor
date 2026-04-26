#include "log.h"

#include <cstdio>

namespace vx::log {

// Minimal Phase-0 implementation: stderr. Will be replaced with spdlog +
// platform sinks in Phase 1 once we wire spdlog as a third_party dep.
void log(Level level, std::string_view tag, std::string_view message) {
    const char* level_str = "INFO";
    switch (level) {
        case Level::Trace: level_str = "TRACE"; break;
        case Level::Debug: level_str = "DEBUG"; break;
        case Level::Info:  level_str = "INFO";  break;
        case Level::Warn:  level_str = "WARN";  break;
        case Level::Error: level_str = "ERROR"; break;
    }
    std::fprintf(stderr, "[%s] %.*s: %.*s\n", level_str,
                 static_cast<int>(tag.size()), tag.data(),
                 static_cast<int>(message.size()), message.data());
}

}  // namespace vx::log
