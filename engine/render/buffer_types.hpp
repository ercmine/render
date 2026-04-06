#pragma once

#include "engine/render/renderer_types.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace render::rendering {

enum class VertexAttribute : std::uint8_t {
  Position = 0,
  Normal,
  Tangent,
  Color0,
  TexCoord0,
};

enum class AttributeType : std::uint8_t {
  Uint8 = 0,
  Int16,
  Half,
  Float,
};

struct VertexElement {
  VertexAttribute attribute{VertexAttribute::Position};
  std::uint8_t components{3};
  AttributeType type{AttributeType::Float};
  bool normalized{false};
  bool as_int{false};
};

struct VertexLayoutDescription {
  std::vector<VertexElement> elements{};
};

struct VertexBufferDescription {
  VertexLayoutDescription layout{};
  std::span<const std::byte> data{};
};

struct IndexBufferDescription {
  std::span<const std::uint16_t> data{};
};

struct MeshHandle {
  VertexBufferHandle vertex_buffer{};
  IndexBufferHandle index_buffer{};
  std::uint32_t index_count{0};
};

struct MeshSubmission {
  MeshHandle mesh{};
  ProgramHandle program{};
  std::span<const float, 16> transform{};
  DrawState draw_state{};
};

}  // namespace render::rendering
