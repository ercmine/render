#include "engine/render/render_pass_system.hpp"

#include <cassert>
#include <cstdint>
#include <string>
#include <vector>

int main() {
  using namespace render::rendering;

  RenderPassRegistry registry;
  std::string error;

  assert(registry.declare_resource({.name = "shadow", .kind = RenderResourceKind::ShadowMap}, &error));
  assert(registry.declare_resource({.name = "scene", .kind = RenderResourceKind::RenderTarget}, &error));
  assert(registry.declare_resource({.name = "post", .kind = RenderResourceKind::PostProcessTexture}, &error));
  assert(registry.declare_resource({.name = "backbuffer", .kind = RenderResourceKind::Backbuffer, .imported = true, .transient = false}, &error));

  std::vector<std::string> executed;

  assert(registry.add_pass(RenderPassDefinition{
    .name = "shadow-pass",
    .resources = {{.resource = "shadow", .access = RenderResourceAccess::Write}},
    .execute = [&](const RenderPassExecutionContext&) { executed.push_back("shadow"); },
  }, &error));

  assert(registry.add_pass(RenderPassDefinition{
    .name = "main-pass",
    .depends_on = {"shadow-pass"},
    .resources = {
      {.resource = "shadow", .access = RenderResourceAccess::Read},
      {.resource = "scene", .access = RenderResourceAccess::Write},
    },
    .execute = [&](const RenderPassExecutionContext&) { executed.push_back("main"); },
  }, &error));

  bool post_enabled = false;
  assert(registry.add_pass(RenderPassDefinition{
    .name = "post-pass",
    .depends_on = {"main-pass"},
    .resources = {
      {.resource = "scene", .access = RenderResourceAccess::Read},
      {.resource = "post", .access = RenderResourceAccess::Write},
    },
    .execute = [&](const RenderPassExecutionContext&) { executed.push_back("post"); },
    .enabled = [&](const RenderPassExecutionContext&) { return post_enabled; },
  }, &error));

  assert(registry.add_pass(RenderPassDefinition{
    .name = "present-pass",
    .depends_on = {"post-pass"},
    .resources = {
      {.resource = "post", .access = RenderResourceAccess::Read},
      {.resource = "backbuffer", .access = RenderResourceAccess::Write},
    },
    .execute = [&](const RenderPassExecutionContext&) { executed.push_back("present"); },
  }, &error));

  const auto build = registry.build();
  assert(build.ok);
  assert(registry.ordered_passes().size() == 4);

  RenderPassExecutionContext context{};
  context.frame.frame_index = 4;
  assert(registry.execute(context, &error));
  assert(executed.size() == 3);
  assert(executed[0] == "shadow");
  assert(executed[1] == "main");
  assert(executed[2] == "present");

  const auto diagnostics = registry.diagnostics();
  assert(diagnostics.inactive_passes.size() == 1);
  assert(diagnostics.inactive_passes[0] == "post-pass");

  RenderPassRegistry invalid;
  assert(invalid.declare_resource({.name = "scene", .kind = RenderResourceKind::RenderTarget}, &error));
  assert(invalid.add_pass(RenderPassDefinition{
    .name = "present",
    .resources = {{.resource = "scene", .access = RenderResourceAccess::Read}},
    .execute = [](const RenderPassExecutionContext&) {},
  }, &error));
  const auto invalid_build = invalid.build();
  assert(!invalid_build.ok);

  RenderPassRegistry cycle;
  assert(cycle.declare_resource({.name = "a", .kind = RenderResourceKind::RenderTarget}, &error));
  assert(cycle.declare_resource({.name = "b", .kind = RenderResourceKind::RenderTarget}, &error));
  assert(cycle.add_pass(RenderPassDefinition{
    .name = "A",
    .depends_on = {"B"},
    .resources = {{.resource = "a", .access = RenderResourceAccess::Write}},
    .execute = [](const RenderPassExecutionContext&) {},
  }, &error));
  assert(cycle.add_pass(RenderPassDefinition{
    .name = "B",
    .depends_on = {"A"},
    .resources = {{.resource = "b", .access = RenderResourceAccess::Write}},
    .execute = [](const RenderPassExecutionContext&) {},
  }, &error));
  const auto cycle_build = cycle.build();
  assert(!cycle_build.ok);

  return 0;
}
