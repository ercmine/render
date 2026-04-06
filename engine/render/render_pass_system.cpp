#include "engine/render/render_pass_system.hpp"

#include <algorithm>
#include <chrono>
#include <sstream>
#include <unordered_set>

namespace render::rendering {
namespace {

bool is_read_access(const RenderResourceAccess access) {
  return access == RenderResourceAccess::Read || access == RenderResourceAccess::ReadWrite;
}

bool is_write_access(const RenderResourceAccess access) {
  return access == RenderResourceAccess::Write || access == RenderResourceAccess::ReadWrite;
}

}  // namespace

void RenderPassRegistry::reset() {
  resources_.clear();
  resource_index_.clear();
  passes_.clear();
  pass_index_.clear();
  execution_order_.clear();
  resource_producers_.clear();
  diagnostics_ = {};
  built_ = false;
}

bool RenderPassRegistry::declare_resource(const RenderResourceDesc& resource, std::string* error) {
  if (resource.name.empty()) {
    if (error != nullptr) {
      *error = "resource name must not be empty";
    }
    return false;
  }
  if (resource_index_.contains(resource.name)) {
    if (error != nullptr) {
      *error = "resource already declared: " + resource.name;
    }
    return false;
  }

  resource_index_.emplace(resource.name, resources_.size());
  resources_.push_back(resource);
  return true;
}

bool RenderPassRegistry::add_pass(RenderPassDefinition pass, std::string* error) {
  if (pass.name.empty()) {
    if (error != nullptr) {
      *error = "pass name must not be empty";
    }
    return false;
  }
  if (pass_index_.contains(pass.name)) {
    if (error != nullptr) {
      *error = "duplicate pass name: " + pass.name;
    }
    return false;
  }
  if (!pass.execute) {
    if (error != nullptr) {
      *error = "pass execute callback missing: " + pass.name;
    }
    return false;
  }

  pass_index_.emplace(pass.name, passes_.size());
  passes_.push_back(PassNode{.definition = std::move(pass)});
  return true;
}

RenderGraphBuildResult RenderPassRegistry::build() {
  std::string error;
  if (!validate(&error) || !resolve_execution_order(&error)) {
    built_ = false;
    return RenderGraphBuildResult{.ok = false, .error = error};
  }
  built_ = true;
  return RenderGraphBuildResult{.ok = true, .error = {}};
}

bool RenderPassRegistry::execute(const RenderPassExecutionContext& context, std::string* error) {
  if (!built_) {
    if (error != nullptr) {
      *error = "render pass registry execute called before successful build";
    }
    return false;
  }

  diagnostics_.active_passes.clear();
  diagnostics_.inactive_passes.clear();

  for (const std::string& pass_name : execution_order_) {
    const auto pass_it = pass_index_.find(pass_name);
    if (pass_it == pass_index_.end()) {
      continue;
    }

    const RenderPassDefinition& pass = passes_[pass_it->second].definition;
    const bool enabled = pass.enabled ? pass.enabled(context) : true;
    if (!enabled) {
      diagnostics_.inactive_passes.push_back(pass_name);
      continue;
    }

    diagnostics_.active_passes.push_back(pass_name);

    const auto pass_begin = std::chrono::steady_clock::now();
    pass.execute(context);
    const auto pass_end = std::chrono::steady_clock::now();

    if (context.renderer != nullptr) {
      const float cpu_ms = static_cast<float>(std::chrono::duration<double, std::milli>(pass_end - pass_begin).count());
      context.renderer->add_debug_pass_timing(RendererPassTiming{.pass_name = pass.name, .cpu_ms = cpu_ms, .gpu_ms = std::nullopt});
    }
  }

  return true;
}

std::optional<RenderResourceDesc> RenderPassRegistry::find_resource(const std::string_view name) const {
  const auto it = resource_index_.find(std::string{name});
  if (it == resource_index_.end()) {
    return std::nullopt;
  }
  return resources_[it->second];
}

std::span<const RenderResourceDesc> RenderPassRegistry::resources() const noexcept {
  return resources_;
}

std::span<const std::string> RenderPassRegistry::ordered_passes() const noexcept {
  return execution_order_;
}

const RenderPassDiagnostics& RenderPassRegistry::diagnostics() const noexcept {
  return diagnostics_;
}

std::string RenderPassRegistry::dump_graph() const {
  std::ostringstream out;
  out << "render-pass-graph\n";
  out << "resources:\n";
  for (const auto& resource : resources_) {
    out << "  - " << resource.name << " [" << static_cast<std::uint32_t>(resource.kind) << "] "
        << resource.width << "x" << resource.height
        << (resource.transient ? " transient" : " persistent")
        << (resource.imported ? " imported" : "") << "\n";
  }

  out << "passes:\n";
  for (const auto& pass_name : execution_order_) {
    const auto pass_it = pass_index_.find(pass_name);
    if (pass_it == pass_index_.end()) {
      continue;
    }
    const auto& pass = passes_[pass_it->second].definition;
    out << "  - " << pass.name;
    if (!pass.depends_on.empty()) {
      out << " depends=[";
      for (std::size_t i = 0; i < pass.depends_on.size(); ++i) {
        out << pass.depends_on[i];
        if (i + 1 < pass.depends_on.size()) {
          out << ", ";
        }
      }
      out << "]";
    }
    out << " resources=[";
    for (std::size_t i = 0; i < pass.resources.size(); ++i) {
      out << pass.resources[i].resource << ":" << static_cast<std::uint32_t>(pass.resources[i].access);
      if (i + 1 < pass.resources.size()) {
        out << ", ";
      }
    }
    out << "]\n";
  }

  return out.str();
}

bool RenderPassRegistry::validate(std::string* error) {
  diagnostics_.validation_messages.clear();
  resource_producers_.clear();

  for (const PassNode& node : passes_) {
    const RenderPassDefinition& pass = node.definition;

    for (const std::string& dep : pass.depends_on) {
      if (!pass_index_.contains(dep)) {
        const std::string msg = "pass '" + pass.name + "' depends on missing pass '" + dep + "'";
        diagnostics_.validation_messages.push_back(msg);
        if (error != nullptr) {
          *error = msg;
        }
        return false;
      }
    }

    for (const RenderPassResourceUsage& usage : pass.resources) {
      if (!resource_index_.contains(usage.resource)) {
        const std::string msg = "pass '" + pass.name + "' references undeclared resource '" + usage.resource + "'";
        diagnostics_.validation_messages.push_back(msg);
        if (error != nullptr) {
          *error = msg;
        }
        return false;
      }

      if (is_write_access(usage.access)) {
        const auto producer_it = resource_producers_.find(usage.resource);
        if (producer_it != resource_producers_.end()) {
          const std::string msg = "resource '" + usage.resource + "' already has producer '" + producer_it->second
            + "', cannot also be written by pass '" + pass.name + "'";
          diagnostics_.validation_messages.push_back(msg);
          if (error != nullptr) {
            *error = msg;
          }
          return false;
        }
        resource_producers_.emplace(usage.resource, pass.name);
      }
    }
  }

  for (const PassNode& node : passes_) {
    const RenderPassDefinition& pass = node.definition;
    for (const RenderPassResourceUsage& usage : pass.resources) {
      if (!is_read_access(usage.access)) {
        continue;
      }

      const auto resource_it = resource_index_.find(usage.resource);
      if (resource_it == resource_index_.end()) {
        continue;
      }
      const RenderResourceDesc& resource = resources_[resource_it->second];
      if (resource.imported) {
        continue;
      }

      if (!resource_producers_.contains(usage.resource)) {
        const std::string msg = "pass '" + pass.name + "' reads resource '" + usage.resource + "' with no producing pass";
        diagnostics_.validation_messages.push_back(msg);
        if (error != nullptr) {
          *error = msg;
        }
        return false;
      }
    }
  }

  return true;
}

bool RenderPassRegistry::resolve_execution_order(std::string* error) {
  enum class VisitState : std::uint8_t { Unvisited = 0, Visiting, Visited };

  std::unordered_map<std::string, std::vector<std::string>> dependency_graph;
  dependency_graph.reserve(passes_.size());

  for (const PassNode& node : passes_) {
    const RenderPassDefinition& pass = node.definition;
    auto& deps = dependency_graph[pass.name];
    deps = pass.depends_on;

    for (const RenderPassResourceUsage& usage : pass.resources) {
      if (!is_read_access(usage.access)) {
        continue;
      }
      const auto producer_it = resource_producers_.find(usage.resource);
      if (producer_it == resource_producers_.end()) {
        continue;
      }
      if (producer_it->second != pass.name) {
        deps.push_back(producer_it->second);
      }
    }

    std::sort(deps.begin(), deps.end());
    deps.erase(std::unique(deps.begin(), deps.end()), deps.end());
  }

  std::unordered_map<std::string, VisitState> state;
  state.reserve(passes_.size());
  execution_order_.clear();
  execution_order_.reserve(passes_.size());

  std::function<bool(const std::string&)> visit = [&](const std::string& pass_name) -> bool {
    const VisitState current = state[pass_name];
    if (current == VisitState::Visited) {
      return true;
    }
    if (current == VisitState::Visiting) {
      const std::string msg = "dependency cycle detected at pass '" + pass_name + "'";
      diagnostics_.validation_messages.push_back(msg);
      if (error != nullptr) {
        *error = msg;
      }
      return false;
    }

    state[pass_name] = VisitState::Visiting;
    for (const std::string& dep : dependency_graph[pass_name]) {
      if (!visit(dep)) {
        return false;
      }
    }

    state[pass_name] = VisitState::Visited;
    execution_order_.push_back(pass_name);
    return true;
  };

  for (const PassNode& node : passes_) {
    if (!visit(node.definition.name)) {
      return false;
    }
  }

  diagnostics_.execution_order = execution_order_;
  return true;
}

}  // namespace render::rendering
