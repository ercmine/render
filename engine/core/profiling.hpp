#pragma once

#include <chrono>
#include <functional>
#include <string_view>

namespace render::core {

using ProfileCallback = std::function<void(std::string_view name, std::chrono::nanoseconds duration)>;

void set_profile_callback(ProfileCallback callback);
void mark_profile_frame(std::string_view name = "frame");

class ProfileScope {
public:
  explicit ProfileScope(std::string_view name);
  ~ProfileScope();

private:
  std::string_view name_;
  std::chrono::steady_clock::time_point start_;
};

}  // namespace render::core

#if defined(RENDER_ENABLE_PROFILING)
#define RENDER_PROFILE_SCOPE(name) ::render::core::ProfileScope render_profile_scope_##__LINE__(name)
#define RENDER_PROFILE_FRAME(name) ::render::core::mark_profile_frame(name)
#else
#define RENDER_PROFILE_SCOPE(name) ((void)0)
#define RENDER_PROFILE_FRAME(name) ((void)0)
#endif
