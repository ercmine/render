#include "engine/core/threading.hpp"

namespace render::core {

Thread::~Thread() {
  if (thread_.joinable()) {
    thread_.join();
  }
}

void Thread::join() {
  if (thread_.joinable()) {
    thread_.join();
  }
}

void set_thread_name(std::string_view /*name*/) {
  // Intentionally no-op for now; platform-specific naming can be layered here.
}

}  // namespace render::core
