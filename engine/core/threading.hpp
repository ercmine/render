#pragma once

#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>
#include <string_view>
#include <utility>

namespace render::core {

using Mutex = std::mutex;
using LockGuard = std::lock_guard<Mutex>;
using UniqueLock = std::unique_lock<Mutex>;
using ConditionVariable = std::condition_variable;

class Thread {
public:
  Thread() = default;

  template <typename Fn, typename... Args>
  explicit Thread(Fn&& fn, Args&&... args) : thread_(std::forward<Fn>(fn), std::forward<Args>(args)...) {}

  Thread(Thread&&) noexcept = default;
  Thread& operator=(Thread&&) noexcept = default;

  Thread(const Thread&) = delete;
  Thread& operator=(const Thread&) = delete;

  ~Thread();

  [[nodiscard]] bool joinable() const noexcept { return thread_.joinable(); }
  void join();

private:
  std::thread thread_{};
};

void set_thread_name(std::string_view name);

}  // namespace render::core
