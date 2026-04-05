#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace render::platform {

enum class Key : std::uint16_t {
  Unknown = 0,
  A,
  B,
  C,
  D,
  E,
  F,
  G,
  H,
  I,
  J,
  K,
  L,
  M,
  N,
  O,
  P,
  Q,
  R,
  S,
  T,
  U,
  V,
  W,
  X,
  Y,
  Z,
  Num0,
  Num1,
  Num2,
  Num3,
  Num4,
  Num5,
  Num6,
  Num7,
  Num8,
  Num9,
  Escape,
  Tab,
  Space,
  Enter,
  Backspace,
  Left,
  Right,
  Up,
  Down,
  LeftShift,
  RightShift,
  LeftCtrl,
  RightCtrl,
  LeftAlt,
  RightAlt,
  Count
};

enum class MouseButton : std::uint8_t {
  Left = 0,
  Middle,
  Right,
  X1,
  X2,
  Count
};

enum class GamepadButton : std::uint8_t {
  South = 0,
  East,
  West,
  North,
  Back,
  Guide,
  Start,
  LeftStick,
  RightStick,
  LeftShoulder,
  RightShoulder,
  DpadUp,
  DpadDown,
  DpadLeft,
  DpadRight,
  Count
};

enum class GamepadAxis : std::uint8_t {
  LeftX = 0,
  LeftY,
  RightX,
  RightY,
  LeftTrigger,
  RightTrigger,
  Count
};

struct ButtonState {
  bool down{false};
  bool pressed{false};
  bool released{false};
};

struct MouseState {
  std::array<ButtonState, static_cast<std::size_t>(MouseButton::Count)> buttons{};
  float x{0.0F};
  float y{0.0F};
  float wheel_x{0.0F};
  float wheel_y{0.0F};
};

struct KeyboardState {
  std::array<ButtonState, static_cast<std::size_t>(Key::Count)> keys{};
  std::string text_input{};
};

struct InputFrame {
  KeyboardState keyboard{};
  MouseState mouse{};
};

struct GamepadState {
  std::int32_t id{-1};
  std::string name{};
  bool connected{false};
  bool active_this_frame{false};
  std::array<ButtonState, static_cast<std::size_t>(GamepadButton::Count)> buttons{};
  std::array<float, static_cast<std::size_t>(GamepadAxis::Count)> axes{};
};

struct TimingState {
  std::uint64_t frame_index{0};
  std::uint64_t ticks{0};
  double total_seconds{0.0};
  double delta_seconds{0.0};
};

struct Paths {
  std::string base_path{};
  std::string pref_path{};
  std::string temp_path{};
};

struct WindowConfig {
  std::string title{"render"};
  std::uint32_t width{1280};
  std::uint32_t height{720};
  bool resizable{true};
};

struct WindowState {
  std::uint32_t width{0};
  std::uint32_t height{0};
  bool close_requested{false};
  bool resized_this_frame{false};
};

struct RuntimeConfig {
  std::string app_name{"render"};
  std::string org_name{"render"};
  WindowConfig window{};
  bool enable_audio{true};
};

constexpr std::size_t to_index(const Key key) noexcept { return static_cast<std::size_t>(key); }
constexpr std::size_t to_index(const MouseButton button) noexcept { return static_cast<std::size_t>(button); }
constexpr std::size_t to_index(const GamepadButton button) noexcept { return static_cast<std::size_t>(button); }
constexpr std::size_t to_index(const GamepadAxis axis) noexcept { return static_cast<std::size_t>(axis); }

}  // namespace render::platform
