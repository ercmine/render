#pragma once

#include "engine/render/renderer_types.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace render::rendering {

enum class VertexAttribute : std::uint8_t {
  Position = 0,
  Normal,
  Tangent,
  Bitangent,
  Color0,
  TexCoord0,
  TexCoord1,
  InstanceRow0,
  InstanceRow1,
  InstanceRow2,
  InstanceRow3,
  InstanceColor,
  InstanceData0,
};

enum class AttributeType : std::uint8_t {
  Uint8 = 0,
  Int16,
  Half,
  Float,
};

enum class BufferUsage : std::uint8_t {
  Immutable = 0,
  Dynamic,
};

enum class IndexType : std::uint8_t {
  Uint16 = 0,
  Uint32,
};

struct VertexElement {
  VertexAttribute attribute{VertexAttribute::Position};
  std::uint16_t offset{0};
  std::uint8_t components{3};
  AttributeType type{AttributeType::Float};
  bool normalized{false};
  bool as_int{false};
};

struct VertexLayoutDescription {
  std::uint16_t stride{0};
  std::vector<VertexElement> elements{};
};

struct VertexLayoutValidation {
  bool valid{false};
  std::string reason{};
};

[[nodiscard]] VertexLayoutValidation validate_vertex_layout(const VertexLayoutDescription& desc);

struct MeshBufferDescription {
  VertexLayoutDescription layout{};
  std::span<const std::byte> data{};
  std::uint32_t vertex_count{0};
  BufferUsage usage{BufferUsage::Immutable};
  std::string debug_name{};
};

struct MeshBufferUpdate {
  std::span<const std::byte> data{};
  std::uint32_t vertex_count{0};
};

struct IndexBufferDescription {
  std::span<const std::byte> data{};
  std::uint32_t index_count{0};
  IndexType index_type{IndexType::Uint16};
  BufferUsage usage{BufferUsage::Immutable};
  std::string debug_name{};
};

struct IndexBufferUpdate {
  std::span<const std::byte> data{};
  std::uint32_t index_count{0};
};

struct InstanceBufferDescription {
  std::uint16_t stride{64};
  std::uint32_t capacity{0};
  std::span<const std::byte> initial_data{};
  std::uint32_t initial_count{0};
  std::string debug_name{};
};

struct InstanceBufferUpdate {
  std::span<const std::byte> data{};
  std::uint16_t stride{64};
  std::uint32_t count{0};
};

struct MeshBufferHandle {
  std::uint16_t idx{kInvalidHandle};
};

struct InstanceBufferHandle {
  std::uint16_t idx{kInvalidHandle};
};

struct MeshHandle {
  MeshBufferHandle vertex_buffer{};
  IndexBufferHandle index_buffer{};
  std::uint32_t vertex_count{0};
  std::uint32_t index_count{0};
  IndexType index_type{IndexType::Uint16};
};

struct CpuMeshData {
  VertexLayoutDescription layout{};
  std::vector<std::byte> vertex_data{};
  std::uint32_t vertex_count{0};
  std::vector<std::byte> index_data{};
  std::uint32_t index_count{0};
  IndexType index_type{IndexType::Uint16};

  [[nodiscard]] bool indexed() const noexcept { return index_count > 0; }
};

struct CpuMeshValidation {
  bool valid{false};
  std::string reason{};
};

[[nodiscard]] CpuMeshValidation validate_cpu_mesh_data(const CpuMeshData& data);

struct InstanceDataView {
  std::span<const std::byte> data{};
  std::uint16_t stride{64};
  std::uint32_t count{0};
};

struct MaterialBinding {
  ProgramHandle program{};
  std::array<TextureHandle, 4> textures{};
  std::uint64_t parameter_hash{0};

  [[nodiscard]] bool valid() const noexcept { return program.idx != kInvalidHandle; }
};

}  // namespace render::rendering
