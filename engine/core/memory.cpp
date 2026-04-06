#include "engine/core/memory.hpp"

#include <new>

namespace render::core {

void* MallocAllocator::allocate(const std::size_t size, const std::size_t alignment) {
  return ::operator new(size, std::align_val_t(alignment));
}

void MallocAllocator::deallocate(void* ptr, const std::size_t alignment) {
  ::operator delete(ptr, std::align_val_t(alignment));
}

IAllocator& default_allocator() noexcept {
  static MallocAllocator allocator;
  return allocator;
}

}  // namespace render::core
