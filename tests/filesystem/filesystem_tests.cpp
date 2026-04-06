#include "engine/filesystem/filesystem.hpp"

#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <vector>

namespace fs = std::filesystem;

int main() {
  using namespace render::filesystem;

  const fs::path sandbox = fs::temp_directory_path() / "render_statement7_fs_tests";
  std::error_code ec;
  fs::remove_all(sandbox, ec);
  fs::create_directories(sandbox / "source_data", ec);

  StorageConfig config{};
  config.executable_path = sandbox / "bin" / "render_shell";
  config.source_root_override = sandbox / "source_data";
  config.writable_root_override = sandbox / "user";

  FileSystemService file_system;
  assert(file_system.initialize(config));
  assert(file_system.initialized());

  assert(file_system.root(PathCategory::SourceData) == sandbox / "source_data");
  assert(file_system.root(PathCategory::Saves) == sandbox / "user" / "saves");
  assert(file_system.root(PathCategory::Logs) == sandbox / "user" / "logs");
  assert(file_system.root(PathCategory::Cache) == sandbox / "user" / "cache");

  assert(file_system.is_directory(file_system.root(PathCategory::Saves)));
  assert(file_system.is_directory(file_system.root(PathCategory::Logs)));

  assert(file_system.resolve(PathCategory::Saves, "slot1/save.bin") == (sandbox / "user" / "saves" / "slot1" / "save.bin"));
  assert(file_system.resolve(PathCategory::Saves, "../escape.txt").empty());

  assert(file_system.write_text(PathCategory::Logs, "runtime/test.log", "hello logs"));
  std::string read_back;
  assert(file_system.read_text(PathCategory::Logs, "runtime/test.log", read_back));
  assert(read_back == "hello logs");

  std::vector<std::byte> binary_payload = {
    std::byte{0x00},
    std::byte{0x11},
    std::byte{0x22},
    std::byte{0x33},
  };
  assert(file_system.write_bytes(PathCategory::Cache, "ns/data.bin", binary_payload, WriteMode::AtomicReplace));

  std::vector<std::byte> read_binary;
  assert(file_system.read_bytes(PathCategory::Cache, "ns/data.bin", read_binary));
  assert(read_binary == binary_payload);

  assert(file_system.write_text(PathCategory::Saves, "slot1/profile.save", "save_v1", WriteMode::AtomicReplace));
  assert(file_system.write_text(PathCategory::Saves, "slot1/profile.save", "save_v2", WriteMode::AtomicReplace));
  std::string save_text;
  assert(file_system.read_text(PathCategory::Saves, "slot1/profile.save", save_text));
  assert(save_text == "save_v2");

  const fs::path cache_file_a = file_system.make_cache_file_path("shaders", "backend=vulkan;shader=vs_statement4", "bin");
  const fs::path cache_file_b = file_system.make_cache_file_path("shaders", "backend=vulkan;shader=vs_statement4", "bin");
  assert(cache_file_a == cache_file_b);
  assert(cache_file_a.string().find((sandbox / "user" / "cache" / "shaders").string()) != std::string::npos);

  const fs::path save_file = file_system.make_save_file_path("slot:main", "save");
  const fs::path log_file = file_system.make_log_file_path("engine", false);
  const fs::path screenshot_file = file_system.make_screenshot_file_path("frame");
  const fs::path crash_file = file_system.make_crash_dump_file_path("crash");

  assert(save_file.parent_path() == file_system.root(PathCategory::Saves));
  assert(log_file.parent_path() == file_system.root(PathCategory::Logs));
  assert(screenshot_file.parent_path() == file_system.root(PathCategory::Screenshots));
  assert(crash_file.parent_path() == file_system.root(PathCategory::CrashDumps));

  const auto listed_logs = file_system.list(PathCategory::Logs, "runtime");
  assert(!listed_logs.empty());

  fs::remove_all(sandbox, ec);
  return 0;
}
