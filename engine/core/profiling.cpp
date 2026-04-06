#include "engine/core/profiling.hpp"

#include <mutex>

namespace render::core {

namespace {
std::mutex g_profile_mutex;
ProfileCallback g_profile_callback;
}  // namespace

void set_profile_callback(ProfileCallback callback) {
  std::lock_guard lock(g_profile_mutex);
  g_profile_callback = std::move(callback);
}

void mark_profile_frame(const std::string_view name) {
  std::lock_guard lock(g_profile_mutex);
  if (g_profile_callback) {
    g_profile_callback(name, std::chrono::nanoseconds::zero());
  }
}

ProfileScope::ProfileScope(const std::string_view name) : name_(name), start_(std::chrono::steady_clock::now()) {}

ProfileScope::~ProfileScope() {
  const auto end = std::chrono::steady_clock::now();
  const auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start_);
  std::lock_guard lock(g_profile_mutex);
  if (g_profile_callback) {
    g_profile_callback(name_, duration);
  }
}

}  // namespace render::core
