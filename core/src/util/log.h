#ifndef VX_UTIL_LOG_H
#define VX_UTIL_LOG_H

// Thin logging facade. Real implementation will route to spdlog (desktop),
// __android_log_print (Android), and OSLog (iOS) without leaking those into
// callers. Domain code uses these macros only.

#include <string>
#include <string_view>

namespace vx::log {

enum class Level { Trace, Debug, Info, Warn, Error };

void log(Level level, std::string_view tag, std::string_view message);

}  // namespace vx::log

#define VX_LOG_INFO(tag, msg)  ::vx::log::log(::vx::log::Level::Info,  tag, msg)
#define VX_LOG_WARN(tag, msg)  ::vx::log::log(::vx::log::Level::Warn,  tag, msg)
#define VX_LOG_ERROR(tag, msg) ::vx::log::log(::vx::log::Level::Error, tag, msg)

#endif  // VX_UTIL_LOG_H
