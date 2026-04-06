#include "engine/core/core.hpp"

#include <cassert>

int main() {
  using namespace render::core;

  const Vec3 v{1.0F, 2.0F, 3.0F};
  const Vec3 u{2.0F, 3.0F, 4.0F};
  const Vec3 sum = v + u;
  assert(nearly_equal(sum.x, 3.0F));

  Transform parent{};
  parent.translation = {1.0F, 0.0F, 0.0F};
  Transform child{};
  child.translation = {0.0F, 2.0F, 0.0F};
  const Transform combined = compose(parent, child);
  assert(nearly_equal(combined.translation.x, 1.0F));
  assert(nearly_equal(combined.translation.y, 2.0F));

  const ColorRGBA8 packed = to_rgba8(ColorRGBA::red());
  assert(packed.r == 255);
  assert(packed.g == 0);

  const Uuid id = Uuid::generate_v4();
  const auto parsed = Uuid::parse(id.to_string());
  assert(parsed.has_value());
  assert(parsed.value() == id);

  const u64 h1 = hash_string("statement5");
  const u64 h2 = hash_string("statement5");
  assert(h1 == h2);

  const Seed seed{1337ULL};
  Random rng_a(seed);
  Random rng_b(seed);
  assert(rng_a.next_u32() == rng_b.next_u32());
  assert(derive_seed(seed, "room-1").value == derive_seed(seed, "room-1").value);

  const Mat4 proj = make_perspective({});
  assert(proj.m[0] != 0.0F);

  bool callback_hit = false;
  set_profile_callback([&callback_hit](std::string_view, std::chrono::nanoseconds) { callback_hit = true; });
  {
    ProfileScope scope("test");
  }
  assert(callback_hit);

  RENDER_VERIFY(true, "verify should pass");

  return 0;
}
