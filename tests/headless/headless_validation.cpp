#include "engine/filesystem/filesystem.hpp"
#include "engine/serialization/serialization.hpp"

#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

int main() {
  using namespace render;

  std::cout << "[headless] starting smoke validation\n";

  const fs::path sandbox = fs::temp_directory_path() / "render_headless_validation";
  std::error_code ec;
  fs::remove_all(sandbox, ec);

  filesystem::StorageConfig config{};
  config.executable_path = sandbox / "bin" / "render_shell";
  config.source_root_override = sandbox / "source_data";
  config.writable_root_override = sandbox / "user";

  fs::create_directories(*config.source_root_override, ec);

  filesystem::FileSystemService file_system;
  if (!file_system.initialize(config)) {
    std::cerr << "[headless] filesystem initialization failed\n";
    return 1;
  }

  if (!file_system.write_text(filesystem::PathCategory::Logs, "headless/startup.log", "ok")) {
    std::cerr << "[headless] failed to write startup log\n";
    return 2;
  }

  serialization::SavePayload payload{};
  payload.header.save_slot = 7;
  payload.header.room_count = 0;
  payload.header.timestamp_unix_seconds = 1700000000ULL;

  const serialization::ByteBuffer encoded = serialization::serialize_save(payload);
  serialization::SavePayload decoded{};
  serialization::Error error{};
  if (!serialization::deserialize_save(encoded, decoded, error)) {
    std::cerr << "[headless] serialization roundtrip failed\n";
    return 3;
  }

  if (decoded.header.save_slot != payload.header.save_slot) {
    std::cerr << "[headless] decoded payload mismatch\n";
    return 4;
  }

  std::cout << "[headless] smoke validation complete\n";
  fs::remove_all(sandbox, ec);
  return 0;
}
