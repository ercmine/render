#include "engine/render/buffer_types.hpp"
#include "engine/render/draw_submission.hpp"

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <vector>

int main() {
  using namespace render::rendering;

  VertexLayoutDescription layout{};
  layout.stride = 16;
  layout.elements = {
    {VertexAttribute::Position, 0, 3, AttributeType::Float, false, false},
    {VertexAttribute::Color0, 12, 4, AttributeType::Uint8, true, false},
  };
  const auto valid_layout = validate_vertex_layout(layout);
  assert(valid_layout.valid);

  CpuMeshData mesh{};
  mesh.layout = layout;
  mesh.vertex_count = 3;
  mesh.vertex_data.resize(48);
  mesh.index_type = IndexType::Uint16;
  mesh.index_count = 3;
  mesh.index_data.resize(6);
  const auto valid_mesh = validate_cpu_mesh_data(mesh);
  assert(valid_mesh.valid);

  std::array<float, 16> identity{};
  identity[0] = identity[5] = identity[10] = identity[15] = 1.0F;

  DrawSubmission draw{};
  draw.view = ViewId{0};
  draw.mesh.vertex_buffer = MeshBufferHandle{1};
  draw.mesh.index_buffer = IndexBufferHandle{2};
  draw.mesh.index_count = 3;
  draw.material.program = ProgramHandle{3};
  draw.transform = identity;

  std::vector<DrawSubmission> draws(8, draw);
  SubmissionDiagnostics diagnostics{};
  const auto batches = build_draw_batches(draws, BatchPolicy{.min_instanced_count = 4, .max_instances_per_draw = 64}, &diagnostics);
  assert(!batches.empty());
  assert(diagnostics.instanced_draws >= 1);
  assert(diagnostics.submitted_instances == 8);

  return 0;
}
