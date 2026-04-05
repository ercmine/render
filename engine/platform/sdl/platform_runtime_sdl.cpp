#include "engine/platform/platform_runtime.hpp"

#include "engine/platform/platform_log.hpp"

#include <SDL3/SDL.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>

namespace render::platform {

namespace {

std::string sdl_error() {
  const char* message = SDL_GetError();
  return message != nullptr ? std::string{message} : std::string{"unknown SDL error"};
}

Key map_scancode(const SDL_Scancode scancode) {
  switch (scancode) {
    case SDL_SCANCODE_A: return Key::A;
    case SDL_SCANCODE_B: return Key::B;
    case SDL_SCANCODE_C: return Key::C;
    case SDL_SCANCODE_D: return Key::D;
    case SDL_SCANCODE_E: return Key::E;
    case SDL_SCANCODE_F: return Key::F;
    case SDL_SCANCODE_G: return Key::G;
    case SDL_SCANCODE_H: return Key::H;
    case SDL_SCANCODE_I: return Key::I;
    case SDL_SCANCODE_J: return Key::J;
    case SDL_SCANCODE_K: return Key::K;
    case SDL_SCANCODE_L: return Key::L;
    case SDL_SCANCODE_M: return Key::M;
    case SDL_SCANCODE_N: return Key::N;
    case SDL_SCANCODE_O: return Key::O;
    case SDL_SCANCODE_P: return Key::P;
    case SDL_SCANCODE_Q: return Key::Q;
    case SDL_SCANCODE_R: return Key::R;
    case SDL_SCANCODE_S: return Key::S;
    case SDL_SCANCODE_T: return Key::T;
    case SDL_SCANCODE_U: return Key::U;
    case SDL_SCANCODE_V: return Key::V;
    case SDL_SCANCODE_W: return Key::W;
    case SDL_SCANCODE_X: return Key::X;
    case SDL_SCANCODE_Y: return Key::Y;
    case SDL_SCANCODE_Z: return Key::Z;
    case SDL_SCANCODE_0: return Key::Num0;
    case SDL_SCANCODE_1: return Key::Num1;
    case SDL_SCANCODE_2: return Key::Num2;
    case SDL_SCANCODE_3: return Key::Num3;
    case SDL_SCANCODE_4: return Key::Num4;
    case SDL_SCANCODE_5: return Key::Num5;
    case SDL_SCANCODE_6: return Key::Num6;
    case SDL_SCANCODE_7: return Key::Num7;
    case SDL_SCANCODE_8: return Key::Num8;
    case SDL_SCANCODE_9: return Key::Num9;
    case SDL_SCANCODE_ESCAPE: return Key::Escape;
    case SDL_SCANCODE_TAB: return Key::Tab;
    case SDL_SCANCODE_SPACE: return Key::Space;
    case SDL_SCANCODE_RETURN: return Key::Enter;
    case SDL_SCANCODE_BACKSPACE: return Key::Backspace;
    case SDL_SCANCODE_LEFT: return Key::Left;
    case SDL_SCANCODE_RIGHT: return Key::Right;
    case SDL_SCANCODE_UP: return Key::Up;
    case SDL_SCANCODE_DOWN: return Key::Down;
    case SDL_SCANCODE_LSHIFT: return Key::LeftShift;
    case SDL_SCANCODE_RSHIFT: return Key::RightShift;
    case SDL_SCANCODE_LCTRL: return Key::LeftCtrl;
    case SDL_SCANCODE_RCTRL: return Key::RightCtrl;
    case SDL_SCANCODE_LALT: return Key::LeftAlt;
    case SDL_SCANCODE_RALT: return Key::RightAlt;
    default: return Key::Unknown;
  }
}

std::optional<MouseButton> map_mouse_button(const uint8_t button) {
  switch (button) {
    case SDL_BUTTON_LEFT: return MouseButton::Left;
    case SDL_BUTTON_MIDDLE: return MouseButton::Middle;
    case SDL_BUTTON_RIGHT: return MouseButton::Right;
    case SDL_BUTTON_X1: return MouseButton::X1;
    case SDL_BUTTON_X2: return MouseButton::X2;
    default: return std::nullopt;
  }
}

std::optional<GamepadButton> map_gamepad_button(const SDL_GamepadButton button) {
  switch (button) {
    case SDL_GAMEPAD_BUTTON_SOUTH: return GamepadButton::South;
    case SDL_GAMEPAD_BUTTON_EAST: return GamepadButton::East;
    case SDL_GAMEPAD_BUTTON_WEST: return GamepadButton::West;
    case SDL_GAMEPAD_BUTTON_NORTH: return GamepadButton::North;
    case SDL_GAMEPAD_BUTTON_BACK: return GamepadButton::Back;
    case SDL_GAMEPAD_BUTTON_GUIDE: return GamepadButton::Guide;
    case SDL_GAMEPAD_BUTTON_START: return GamepadButton::Start;
    case SDL_GAMEPAD_BUTTON_LEFT_STICK: return GamepadButton::LeftStick;
    case SDL_GAMEPAD_BUTTON_RIGHT_STICK: return GamepadButton::RightStick;
    case SDL_GAMEPAD_BUTTON_LEFT_SHOULDER: return GamepadButton::LeftShoulder;
    case SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER: return GamepadButton::RightShoulder;
    case SDL_GAMEPAD_BUTTON_DPAD_UP: return GamepadButton::DpadUp;
    case SDL_GAMEPAD_BUTTON_DPAD_DOWN: return GamepadButton::DpadDown;
    case SDL_GAMEPAD_BUTTON_DPAD_LEFT: return GamepadButton::DpadLeft;
    case SDL_GAMEPAD_BUTTON_DPAD_RIGHT: return GamepadButton::DpadRight;
    default: return std::nullopt;
  }
}

std::optional<GamepadAxis> map_gamepad_axis(const SDL_GamepadAxis axis) {
  switch (axis) {
    case SDL_GAMEPAD_AXIS_LEFTX: return GamepadAxis::LeftX;
    case SDL_GAMEPAD_AXIS_LEFTY: return GamepadAxis::LeftY;
    case SDL_GAMEPAD_AXIS_RIGHTX: return GamepadAxis::RightX;
    case SDL_GAMEPAD_AXIS_RIGHTY: return GamepadAxis::RightY;
    case SDL_GAMEPAD_AXIS_LEFT_TRIGGER: return GamepadAxis::LeftTrigger;
    case SDL_GAMEPAD_AXIS_RIGHT_TRIGGER: return GamepadAxis::RightTrigger;
    default: return std::nullopt;
  }
}

void set_button_state(ButtonState& state, const bool is_down) {
  if (is_down != state.down) {
    state.down = is_down;
    state.pressed = is_down;
    state.released = !is_down;
  }
}

void clear_transitions(InputFrame& input) {
  for (auto& key : input.keyboard.keys) {
    key.pressed = false;
    key.released = false;
  }
  for (auto& button : input.mouse.buttons) {
    button.pressed = false;
    button.released = false;
  }
  input.keyboard.text_input.clear();
  input.mouse.wheel_x = 0.0F;
  input.mouse.wheel_y = 0.0F;
}

void clear_gamepad_transitions(std::vector<GamepadState>& gamepads) {
  for (auto& gamepad : gamepads) {
    gamepad.active_this_frame = false;
    for (auto& button : gamepad.buttons) {
      button.pressed = false;
      button.released = false;
    }
  }
}

float normalize_axis(const Sint16 axis_value) {
  constexpr float kMaxAxisMagnitude = 32767.0F;
  return std::clamp(static_cast<float>(axis_value) / kMaxAxisMagnitude, -1.0F, 1.0F);
}

}  // namespace

struct PlatformRuntime::Impl {
  RuntimeConfig config{};
  InputFrame input{};
  std::vector<GamepadState> gamepads{};
  std::unordered_map<SDL_JoystickID, SDL_Gamepad*> handles{};
  std::unordered_map<SDL_JoystickID, std::size_t> index_by_id{};
  TimingState timing{};
  Paths paths{};
  WindowState window{};
  AudioService audio{};
  SDL_Window* sdl_window{nullptr};
  bool quit_requested{false};
  bool initialized{false};
  std::uint64_t perf_frequency{0};
  std::uint64_t startup_counter{0};
  std::uint64_t last_frame_counter{0};

  void rebuild_gamepad_lookup() {
    index_by_id.clear();
    for (std::size_t i = 0; i < gamepads.size(); ++i) {
      index_by_id.emplace(static_cast<SDL_JoystickID>(gamepads[i].id), i);
    }
  }

  void log_sdl_runtime_info() {
    const int version = SDL_GetVersion();
    std::ostringstream stream;
    stream << "SDL runtime version code " << version << " on " << SDL_GetPlatform();
    log::info(stream.str());
  }

  void query_paths() {
    if (const char* base = SDL_GetBasePath()) {
      paths.base_path = base;
      log::info(std::string{"Base path: "} + paths.base_path);
    } else {
      log::warn(std::string{"Failed to resolve base path: "} + sdl_error());
    }

    if (const char* pref = SDL_GetPrefPath(config.org_name.c_str(), config.app_name.c_str())) {
      paths.pref_path = pref;
      log::info(std::string{"Pref path: "} + paths.pref_path);
    } else {
      log::warn(std::string{"Failed to resolve pref path: "} + sdl_error());
    }

    if (const char* home = SDL_GetUserFolder(SDL_FOLDER_HOME)) {
      paths.temp_path = std::string{home} + ".cache/";
    } else {
      log::warn(std::string{"Failed to resolve user folder for cache placeholder: "} + sdl_error());
    }
  }

  bool open_default_audio() {
    if (!config.enable_audio) {
      log::info("Audio initialization disabled by runtime config");
      return true;
    }

    constexpr SDL_AudioSpec kDesired{
      SDL_AUDIO_F32,
      2,
      48000,
    };

    audio.stream_ = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &kDesired, nullptr, nullptr);
    if (audio.stream_ == nullptr) {
      log::warn(std::string{"Audio stream initialization failed: "} + sdl_error());
      audio.initialized_ = false;
      return false;
    }

    if (!SDL_ResumeAudioStreamDevice(static_cast<SDL_AudioStream*>(audio.stream_))) {
      log::warn(std::string{"Failed to resume audio stream: "} + sdl_error());
      SDL_DestroyAudioStream(static_cast<SDL_AudioStream*>(audio.stream_));
      audio.stream_ = nullptr;
      audio.initialized_ = false;
      return false;
    }

    audio.initialized_ = true;
    log::info("Audio service initialized");
    return true;
  }

  void close_audio() {
    if (audio.stream_ != nullptr) {
      SDL_DestroyAudioStream(static_cast<SDL_AudioStream*>(audio.stream_));
      audio.stream_ = nullptr;
    }
    audio.initialized_ = false;
  }

  void attach_gamepad(const SDL_JoystickID id) {
    SDL_Gamepad* gamepad = SDL_OpenGamepad(id);
    if (gamepad == nullptr) {
      log::warn(std::string{"Could not open gamepad: "} + sdl_error());
      return;
    }

    GamepadState state{};
    state.id = id;
    state.connected = true;
    if (const char* name = SDL_GetGamepadName(gamepad)) {
      state.name = name;
    }

    handles.emplace(id, gamepad);
    gamepads.emplace_back(std::move(state));
    rebuild_gamepad_lookup();
    log::info(std::string{"Gamepad connected: "} + gamepads.back().name);
  }

  void detach_gamepad(const SDL_JoystickID id) {
    if (auto handle_it = handles.find(id); handle_it != handles.end()) {
      SDL_CloseGamepad(handle_it->second);
      handles.erase(handle_it);
    }

    gamepads.erase(
      std::remove_if(gamepads.begin(), gamepads.end(), [id](const GamepadState& state) {
        return state.id == static_cast<std::int32_t>(id);
      }),
      gamepads.end());
    rebuild_gamepad_lookup();
    log::info("Gamepad disconnected");
  }
};

bool AudioService::is_initialized() const noexcept { return initialized_; }

bool AudioService::queue_interleaved_float_samples(const float* samples, const std::size_t sample_count) noexcept {
  if (!initialized_ || stream_ == nullptr || samples == nullptr || sample_count == 0U) {
    return false;
  }

  const auto byte_count = static_cast<int>(sample_count * sizeof(float));
  return SDL_PutAudioStreamData(static_cast<SDL_AudioStream*>(stream_), samples, byte_count);
}

PlatformRuntime::PlatformRuntime() : impl_(new Impl{}) {}

PlatformRuntime::~PlatformRuntime() {
  shutdown();
  delete impl_;
  impl_ = nullptr;
}

bool PlatformRuntime::initialize(const RuntimeConfig& config) {
  impl_->config = config;

  if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD | SDL_INIT_AUDIO | SDL_INIT_EVENTS)) {
    log::error(std::string{"SDL_Init failed: "} + sdl_error());
    return false;
  }

  impl_->log_sdl_runtime_info();
  impl_->query_paths();

  Uint64 perf_frequency = SDL_GetPerformanceFrequency();
  if (perf_frequency == 0U) {
    log::error("SDL_GetPerformanceFrequency returned 0");
    return false;
  }

  impl_->perf_frequency = perf_frequency;
  impl_->startup_counter = SDL_GetPerformanceCounter();
  impl_->last_frame_counter = impl_->startup_counter;

  Uint32 window_flags = SDL_WINDOW_HIGH_PIXEL_DENSITY;
  if (config.window.resizable) {
    window_flags |= SDL_WINDOW_RESIZABLE;
  }

  impl_->sdl_window = SDL_CreateWindow(
    config.window.title.c_str(),
    static_cast<int>(config.window.width),
    static_cast<int>(config.window.height),
    window_flags);

  if (impl_->sdl_window == nullptr) {
    log::error(std::string{"Failed to create SDL window: "} + sdl_error());
    return false;
  }

  int width = 0;
  int height = 0;
  SDL_GetWindowSize(impl_->sdl_window, &width, &height);
  impl_->window.width = static_cast<std::uint32_t>(width);
  impl_->window.height = static_cast<std::uint32_t>(height);

  log::info("Window created successfully");
  impl_->open_default_audio();

  int joystick_count = 0;
  if (SDL_JoystickID* joysticks = SDL_GetJoysticks(&joystick_count)) {
    for (int i = 0; i < joystick_count; ++i) {
      const SDL_JoystickID id = joysticks[i];
      if (SDL_IsGamepad(id)) {
        impl_->attach_gamepad(id);
      }
    }
    SDL_free(joysticks);
  }

  impl_->initialized = true;
  return true;
}

void PlatformRuntime::shutdown() {
  if (impl_ == nullptr || !impl_->initialized) {
    return;
  }

  impl_->close_audio();

  for (auto& [_, handle] : impl_->handles) {
    SDL_CloseGamepad(handle);
  }
  impl_->handles.clear();
  impl_->gamepads.clear();
  impl_->index_by_id.clear();

  if (impl_->sdl_window != nullptr) {
    SDL_DestroyWindow(impl_->sdl_window);
    impl_->sdl_window = nullptr;
  }

  SDL_Quit();
  impl_->initialized = false;
}

void PlatformRuntime::begin_frame() {
  clear_transitions(impl_->input);
  clear_gamepad_transitions(impl_->gamepads);
  impl_->window.resized_this_frame = false;
}

void PlatformRuntime::pump_events() {
  SDL_Event event{};
  while (SDL_PollEvent(&event)) {
    switch (event.type) {
      case SDL_EVENT_QUIT:
        impl_->quit_requested = true;
        impl_->window.close_requested = true;
        break;
      case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
        impl_->quit_requested = true;
        impl_->window.close_requested = true;
        break;
      case SDL_EVENT_WINDOW_RESIZED:
      case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
        impl_->window.width = static_cast<std::uint32_t>(event.window.data1);
        impl_->window.height = static_cast<std::uint32_t>(event.window.data2);
        impl_->window.resized_this_frame = true;
        break;
      case SDL_EVENT_KEY_DOWN:
      case SDL_EVENT_KEY_UP: {
        const Key key = map_scancode(event.key.scancode);
        if (key != Key::Unknown) {
          set_button_state(impl_->input.keyboard.keys[to_index(key)], event.key.down);
        }
      } break;
      case SDL_EVENT_TEXT_INPUT:
        impl_->input.keyboard.text_input += event.text.text;
        break;
      case SDL_EVENT_MOUSE_BUTTON_DOWN:
      case SDL_EVENT_MOUSE_BUTTON_UP:
        if (const auto button = map_mouse_button(event.button.button)) {
          set_button_state(impl_->input.mouse.buttons[to_index(*button)], event.button.down);
        }
        break;
      case SDL_EVENT_MOUSE_MOTION:
        impl_->input.mouse.x = event.motion.x;
        impl_->input.mouse.y = event.motion.y;
        break;
      case SDL_EVENT_MOUSE_WHEEL:
        impl_->input.mouse.wheel_x += event.wheel.x;
        impl_->input.mouse.wheel_y += event.wheel.y;
        break;
      case SDL_EVENT_GAMEPAD_ADDED:
        impl_->attach_gamepad(event.gdevice.which);
        break;
      case SDL_EVENT_GAMEPAD_REMOVED:
        impl_->detach_gamepad(event.gdevice.which);
        break;
      case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
      case SDL_EVENT_GAMEPAD_BUTTON_UP:
        if (auto it = impl_->index_by_id.find(event.gbutton.which); it != impl_->index_by_id.end()) {
          GamepadState& gamepad = impl_->gamepads[it->second];
          gamepad.active_this_frame = true;
          if (const auto button = map_gamepad_button(static_cast<SDL_GamepadButton>(event.gbutton.button))) {
            set_button_state(gamepad.buttons[to_index(*button)], event.gbutton.down);
          }
        }
        break;
      case SDL_EVENT_GAMEPAD_AXIS_MOTION:
        if (auto it = impl_->index_by_id.find(event.gaxis.which); it != impl_->index_by_id.end()) {
          GamepadState& gamepad = impl_->gamepads[it->second];
          gamepad.active_this_frame = true;
          if (const auto axis = map_gamepad_axis(static_cast<SDL_GamepadAxis>(event.gaxis.axis))) {
            gamepad.axes[to_index(*axis)] = normalize_axis(event.gaxis.value);
          }
        }
        break;
      default:
        break;
    }
  }
}

void PlatformRuntime::end_frame() {
  const std::uint64_t now_counter = SDL_GetPerformanceCounter();
  const std::uint64_t frame_ticks = now_counter - impl_->last_frame_counter;

  impl_->timing.frame_index += 1;
  impl_->timing.ticks = now_counter - impl_->startup_counter;
  impl_->timing.total_seconds = static_cast<double>(impl_->timing.ticks) / static_cast<double>(impl_->perf_frequency);
  impl_->timing.delta_seconds = static_cast<double>(frame_ticks) / static_cast<double>(impl_->perf_frequency);

  impl_->last_frame_counter = now_counter;
}

bool PlatformRuntime::should_quit() const noexcept { return impl_->quit_requested; }

const InputFrame& PlatformRuntime::input() const noexcept { return impl_->input; }

const std::vector<GamepadState>& PlatformRuntime::gamepads() const noexcept { return impl_->gamepads; }

const TimingState& PlatformRuntime::timing() const noexcept { return impl_->timing; }

const Paths& PlatformRuntime::paths() const noexcept { return impl_->paths; }

const WindowState& PlatformRuntime::window_state() const noexcept { return impl_->window; }

AudioService& PlatformRuntime::audio() noexcept { return impl_->audio; }

void* PlatformRuntime::native_window_handle() const noexcept { return impl_->sdl_window; }

void PlatformRuntime::sleep_for_milliseconds(const std::uint32_t milliseconds) noexcept {
  SDL_Delay(milliseconds);
}

}  // namespace render::platform
