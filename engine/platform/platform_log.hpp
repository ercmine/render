#pragma once

#include <iostream>
#include <string_view>

namespace render::platform::log {

inline void info(const std::string_view message) { std::cout << "[render][info] " << message << '\n'; }
inline void warn(const std::string_view message) { std::cout << "[render][warn] " << message << '\n'; }
inline void error(const std::string_view message) { std::cerr << "[render][error] " << message << '\n'; }

}  // namespace render::platform::log
