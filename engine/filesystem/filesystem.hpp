#pragma once

#include <cstddef>
#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace render::filesystem {

namespace fs = std::filesystem;

enum class PathCategory {
  InstallRoot,
  SourceData,
  WritableRoot,
  Cache,
  ShaderCache,
  GeneratedResources,
  Saves,
  Logs,
  Screenshots,
  CrashDumps,
  Config,
  Temp,
  TestOutput,
};

enum class WriteMode {
  Direct,
  AtomicReplace,
};

struct StorageConfig {
  std::string app_name{"render"};
  std::string org_name{"render"};

  fs::path executable_path{};
  fs::path working_directory{};

  std::optional<fs::path> platform_base_path{};
  std::optional<fs::path> platform_pref_path{};
  std::optional<fs::path> platform_temp_path{};

  std::optional<fs::path> source_root_override{};
  std::optional<fs::path> writable_root_override{};

  bool create_directories_on_init{true};
};

class FileSystemService {
public:
  [[nodiscard]] bool initialize(const StorageConfig& config);
  [[nodiscard]] bool initialized() const noexcept;

  [[nodiscard]] fs::path root(PathCategory category) const;
  [[nodiscard]] fs::path resolve(PathCategory category, std::string_view relative_path) const;

  [[nodiscard]] bool ensure_directory(PathCategory category, std::string_view relative_path = {});

  [[nodiscard]] bool exists(const fs::path& path) const;
  [[nodiscard]] bool is_directory(const fs::path& path) const;
  [[nodiscard]] bool is_regular_file(const fs::path& path) const;

  [[nodiscard]] bool read_text(PathCategory category, std::string_view relative_path, std::string& out_text) const;
  [[nodiscard]] bool read_bytes(PathCategory category, std::string_view relative_path, std::vector<std::byte>& out_bytes) const;

  [[nodiscard]] bool write_text(
    PathCategory category,
    std::string_view relative_path,
    std::string_view text,
    WriteMode mode = WriteMode::Direct);
  [[nodiscard]] bool write_bytes(
    PathCategory category,
    std::string_view relative_path,
    std::span<const std::byte> bytes,
    WriteMode mode = WriteMode::Direct);

  [[nodiscard]] bool remove(PathCategory category, std::string_view relative_path);
  [[nodiscard]] std::vector<fs::path> list(PathCategory category, std::string_view relative_path = {}) const;

  [[nodiscard]] fs::path make_cache_file_path(
    std::string_view cache_namespace,
    std::string_view deterministic_key,
    std::string_view extension = {}) const;

  [[nodiscard]] fs::path make_save_file_path(std::string_view slot_name, std::string_view extension = ".save") const;
  [[nodiscard]] fs::path make_log_file_path(std::string_view stem, bool timestamped = true) const;
  [[nodiscard]] fs::path make_screenshot_file_path(std::string_view stem = "screenshot") const;
  [[nodiscard]] fs::path make_crash_dump_file_path(std::string_view stem = "crash") const;

private:
  [[nodiscard]] static std::optional<fs::path> sanitize_relative_path(std::string_view raw_relative_path);
  [[nodiscard]] static std::string category_name(PathCategory category);
  [[nodiscard]] static std::string utc_timestamp_for_filename();

  [[nodiscard]] bool ensure_directory_at(const fs::path& path);
  [[nodiscard]] bool write_bytes_direct(const fs::path& path, std::span<const std::byte> bytes);
  [[nodiscard]] bool write_bytes_atomic(const fs::path& path, std::span<const std::byte> bytes);

  [[nodiscard]] fs::path discover_install_root(const StorageConfig& config) const;
  [[nodiscard]] fs::path discover_source_root(const StorageConfig& config, const fs::path& install_root) const;
  [[nodiscard]] std::optional<fs::path> discover_writable_root(const StorageConfig& config) const;

  bool initialized_{false};
  std::unordered_map<PathCategory, fs::path> roots_{};
};

}  // namespace render::filesystem
