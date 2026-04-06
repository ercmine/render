#pragma once

#include "engine/render/renderer.hpp"

#include <cstdint>
#include <functional>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace render::rendering {

enum class RenderResourceKind : std::uint8_t {
  RenderTarget = 0,
  DepthTarget,
  ShadowMap,
  PostProcessTexture,
  UiTarget,
  Backbuffer,
};

enum class RenderResourceAccess : std::uint8_t {
  Read = 0,
  Write,
  ReadWrite,
};

struct RenderResourceDesc {
  std::string name{};
  RenderResourceKind kind{RenderResourceKind::RenderTarget};
  std::uint16_t width{0};
  std::uint16_t height{0};
  bool transient{true};
  bool imported{false};
};

struct RenderPassResourceUsage {
  std::string resource{};
  RenderResourceAccess access{RenderResourceAccess::Read};
  bool clear_before_use{false};
};

struct RenderFrameInfo {
  std::uint64_t frame_index{0};
  std::uint16_t backbuffer_width{0};
  std::uint16_t backbuffer_height{0};
  float delta_seconds{0.0F};
};

class RenderPassRegistry;

struct RenderPassExecutionContext {
  RenderFrameInfo frame{};
  Renderer* renderer{nullptr};
  const RenderPassRegistry* registry{nullptr};
  void* user_data{nullptr};
};

using RenderPassExecuteFn = std::function<void(const RenderPassExecutionContext&)>;
using RenderPassEnabledFn = std::function<bool(const RenderPassExecutionContext&)>;

struct RenderPassDefinition {
  std::string name{};
  std::vector<std::string> depends_on{};
  std::vector<RenderPassResourceUsage> resources{};
  RenderPassExecuteFn execute{};
  RenderPassEnabledFn enabled{};
};

struct RenderPassDiagnostics {
  std::vector<std::string> execution_order{};
  std::vector<std::string> active_passes{};
  std::vector<std::string> inactive_passes{};
  std::vector<std::string> validation_messages{};
};

struct RenderGraphBuildResult {
  bool ok{false};
  std::string error{};
};

class RenderPassRegistry {
public:
  void reset();

  [[nodiscard]] bool declare_resource(const RenderResourceDesc& resource, std::string* error = nullptr);
  [[nodiscard]] bool add_pass(RenderPassDefinition pass, std::string* error = nullptr);

  [[nodiscard]] RenderGraphBuildResult build();
  [[nodiscard]] bool execute(const RenderPassExecutionContext& context, std::string* error = nullptr);

  [[nodiscard]] std::optional<RenderResourceDesc> find_resource(std::string_view name) const;
  [[nodiscard]] std::span<const RenderResourceDesc> resources() const noexcept;
  [[nodiscard]] std::span<const std::string> ordered_passes() const noexcept;
  [[nodiscard]] const RenderPassDiagnostics& diagnostics() const noexcept;
  [[nodiscard]] std::string dump_graph() const;

private:
  struct PassNode {
    RenderPassDefinition definition{};
  };

  [[nodiscard]] bool validate(std::string* error);
  [[nodiscard]] bool resolve_execution_order(std::string* error);

  std::vector<RenderResourceDesc> resources_{};
  std::unordered_map<std::string, std::size_t> resource_index_{};

  std::vector<PassNode> passes_{};
  std::unordered_map<std::string, std::size_t> pass_index_{};

  std::vector<std::string> execution_order_{};
  std::unordered_map<std::string, std::string> resource_producers_{};
  RenderPassDiagnostics diagnostics_{};
  bool built_{false};
};

}  // namespace render::rendering
