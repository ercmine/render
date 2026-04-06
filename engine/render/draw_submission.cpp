#include "engine/render/draw_submission.hpp"

#include <algorithm>
#include <unordered_map>

namespace render::rendering {

BatchKey make_batch_key(const DrawSubmission& submission) {
  return BatchKey{
    .view = submission.view.value,
    .mesh_buffer = submission.mesh.vertex_buffer.idx,
    .index_buffer = submission.mesh.index_buffer.idx,
    .program = submission.material.program.idx,
    .state_flags = submission.draw_state.state_flags,
    .parameter_hash = submission.material.parameter_hash,
  };
}

std::vector<DrawBatch> build_draw_batches(
  const std::span<const DrawSubmission> submissions,
  const BatchPolicy& policy,
  SubmissionDiagnostics* diagnostics) {
  std::vector<DrawBatch> batches;
  if (diagnostics != nullptr) {
    *diagnostics = {};
    diagnostics->submitted_draws = static_cast<std::uint32_t>(submissions.size());
  }

  std::unordered_map<std::uint64_t, std::vector<const DrawSubmission*>> grouped;
  grouped.reserve(submissions.size());

  for (const DrawSubmission& submission : submissions) {
    const BatchKey key = make_batch_key(submission);
    const std::uint64_t packed =
      (static_cast<std::uint64_t>(key.view) << 48U) ^ (static_cast<std::uint64_t>(key.mesh_buffer) << 32U)
      ^ (static_cast<std::uint64_t>(key.index_buffer) << 16U) ^ static_cast<std::uint64_t>(key.program)
      ^ key.state_flags ^ key.parameter_hash;
    grouped[packed].push_back(&submission);
  }

  batches.reserve(grouped.size());

  for (auto& [_, group] : grouped) {
    if (group.empty()) {
      continue;
    }

    std::sort(group.begin(), group.end(), [](const DrawSubmission* a, const DrawSubmission* b) {
      return a->sort_key < b->sort_key;
    });

    const DrawSubmission& first = *group.front();
    const bool use_instancing = group.size() >= policy.min_instanced_count;
    if (use_instancing) {
      std::size_t offset = 0;
      while (offset < group.size()) {
        const std::size_t draw_count = std::min<std::size_t>(policy.max_instances_per_draw, group.size() - offset);
        DrawBatch batch{};
        batch.key = make_batch_key(first);
        batch.mode = SubmissionMode::Instanced;
        batch.mesh = first.mesh;
        batch.material = first.material;
        batch.draw_state = first.draw_state;
        batch.transforms.reserve(draw_count * 16U);

        for (std::size_t i = 0; i < draw_count; ++i) {
          const DrawSubmission& draw = *group[offset + i];
          batch.transforms.insert(batch.transforms.end(), draw.transform.begin(), draw.transform.end());
        }

        if (diagnostics != nullptr) {
          diagnostics->instanced_draws += 1;
          diagnostics->submitted_instances += static_cast<std::uint32_t>(draw_count);
        }
        batches.push_back(std::move(batch));
        offset += draw_count;
      }
      continue;
    }

    for (const DrawSubmission* draw : group) {
      DrawBatch batch{};
      batch.key = make_batch_key(*draw);
      batch.mode = SubmissionMode::Unique;
      batch.mesh = draw->mesh;
      batch.material = draw->material;
      batch.draw_state = draw->draw_state;
      batch.unique_draws.push_back(*draw);
      if (diagnostics != nullptr) {
        diagnostics->unique_draws += 1;
        diagnostics->submitted_instances += 1;
      }
      batches.push_back(std::move(batch));
    }
  }

  if (diagnostics != nullptr) {
    diagnostics->batch_count = static_cast<std::uint32_t>(batches.size());
  }

  return batches;
}

}  // namespace render::rendering
