#include "engine/render/buffer_types.hpp"

#include <unordered_set>

namespace render::rendering {
namespace {

std::uint16_t attribute_component_size(const AttributeType type) {
  switch (type) {
    case AttributeType::Uint8: return 1;
    case AttributeType::Int16: return 2;
    case AttributeType::Half: return 2;
    case AttributeType::Float: return 4;
    default: return 0;
  }
}

}  // namespace

VertexLayoutValidation validate_vertex_layout(const VertexLayoutDescription& desc) {
  if (desc.stride == 0) {
    return {.valid = false, .reason = "vertex layout stride must be greater than zero"};
  }
  if (desc.elements.empty()) {
    return {.valid = false, .reason = "vertex layout must define at least one element"};
  }

  std::unordered_set<std::uint8_t> semantics;
  std::uint16_t max_end = 0;

  for (const VertexElement& element : desc.elements) {
    if (element.components == 0 || element.components > 4) {
      return {.valid = false, .reason = "vertex element component count must be in [1,4]"};
    }
    const auto inserted = semantics.insert(static_cast<std::uint8_t>(element.attribute));
    if (!inserted.second) {
      return {.valid = false, .reason = "vertex layout contains duplicate attribute semantic"};
    }

    const std::uint16_t byte_size = static_cast<std::uint16_t>(attribute_component_size(element.type) * element.components);
    const std::uint16_t end = static_cast<std::uint16_t>(element.offset + byte_size);
    if (end > desc.stride) {
      return {.valid = false, .reason = "vertex element extends past stride"};
    }
    if (end > max_end) {
      max_end = end;
    }
  }

  if (max_end == 0) {
    return {.valid = false, .reason = "vertex layout has invalid effective size"};
  }

  return {.valid = true, .reason = {}};
}

CpuMeshValidation validate_cpu_mesh_data(const CpuMeshData& data) {
  const VertexLayoutValidation layout = validate_vertex_layout(data.layout);
  if (!layout.valid) {
    return {.valid = false, .reason = layout.reason};
  }

  if (data.vertex_count == 0) {
    return {.valid = false, .reason = "cpu mesh must have at least one vertex"};
  }

  const std::size_t required_vertex_bytes = static_cast<std::size_t>(data.layout.stride) * data.vertex_count;
  if (data.vertex_data.size() != required_vertex_bytes) {
    return {.valid = false, .reason = "cpu mesh vertex_data size does not match stride*vertex_count"};
  }

  if (data.index_count > 0) {
    const std::size_t index_size = data.index_type == IndexType::Uint16 ? 2U : 4U;
    if (data.index_data.size() != data.index_count * index_size) {
      return {.valid = false, .reason = "cpu mesh index_data size does not match index_count*index_size"};
    }
  }

  return {.valid = true, .reason = {}};
}

}  // namespace render::rendering
