#pragma once

#include "engine/core/types.hpp"

#include <cstddef>

namespace render::core {

class IAllocator {
public:
  virtual ~IAllocator() = default;

  virtual void* allocate(std::size_t size, std::size_t alignment) = 0;
  virtual void deallocate(void* ptr, std::size_t alignment) = 0;
};

class MallocAllocator final : public IAllocator {
public:
  void* allocate(std::size_t size, std::size_t alignment) override;
  void deallocate(void* ptr, std::size_t alignment) override;
};

IAllocator& default_allocator() noexcept;

}  // namespace render::core
