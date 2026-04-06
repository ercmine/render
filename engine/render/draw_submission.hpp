#pragma once

#include "engine/render/buffer_types.hpp"
#include "engine/render/renderer_types.hpp"

#include <array>
#include <cstdint>
#include <span>
#include <vector>

namespace render::rendering {

enum class SubmissionMode : std::uint8_t {
  Unique = 0,
  Instanced,
};

struct DrawSubmission {
  ViewId view{};
  MeshHandle mesh{};
  MaterialBinding material{};
  std::array<float, 16> transform{};
  DrawState draw_state{};
  std::uint32_t sort_key{0};
};

struct BatchKey {
  std::uint16_t view{0};
  std::uint16_t mesh_buffer{kInvalidHandle};
  std::uint16_t index_buffer{kInvalidHandle};
  std::uint16_t program{kInvalidHandle};
  std::uint64_t state_flags{0};
  std::uint64_t parameter_hash{0};

  friend bool operator==(const BatchKey&, const BatchKey&) = default;
};

struct BatchPolicy {
  std::uint32_t min_instanced_count{4};
  std::uint32_t max_instances_per_draw{1024};
};

struct DrawBatch {
  BatchKey key{};
  SubmissionMode mode{SubmissionMode::Unique};
  MeshHandle mesh{};
  MaterialBinding material{};
  DrawState draw_state{};
  std::vector<float> transforms{};  // 16 floats per instance
  std::vector<DrawSubmission> unique_draws{};
};

struct SubmissionDiagnostics {
  std::uint32_t submitted_draws{0};
  std::uint32_t instanced_draws{0};
  std::uint32_t unique_draws{0};
  std::uint32_t submitted_instances{0};
  std::uint32_t batch_count{0};
};

[[nodiscard]] BatchKey make_batch_key(const DrawSubmission& submission);
[[nodiscard]] std::vector<DrawBatch> build_draw_batches(
  std::span<const DrawSubmission> submissions,
  const BatchPolicy& policy,
  SubmissionDiagnostics* diagnostics = nullptr);

}  // namespace render::rendering
