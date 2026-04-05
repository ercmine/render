#include "engine/platform/platform_types.hpp"

#include <cassert>

int main() {
  using namespace render::platform;

  static_assert(to_index(Key::Unknown) == 0U);
  static_assert(to_index(MouseButton::Left) == 0U);
  static_assert(to_index(GamepadButton::South) == 0U);
  static_assert(to_index(GamepadAxis::LeftX) == 0U);

  InputFrame input{};
  assert(input.keyboard.keys[to_index(Key::A)].down == false);
  assert(input.mouse.buttons[to_index(MouseButton::Left)].pressed == false);

  GamepadState gamepad{};
  gamepad.connected = true;
  gamepad.active_this_frame = true;
  assert(gamepad.connected);
  assert(gamepad.active_this_frame);

  return 0;
}
