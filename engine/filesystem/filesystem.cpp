#include "engine/filesystem/filesystem.hpp"

#include "engine/core/hash.hpp"
#include "engine/core/log.hpp"

#include <algorithm>
#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace render::filesystem {

namespace {
constexpr const char* kLogCategory = "filesystem";

std::string sanitize_filename(std::string_view value, std::string_view fallback) {
  std::string out;
  out.reserve(value.size());

  for (const char c : value) {
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' || c == '-' || c == '.') {
      out.push_back(c);
    } else {
      out.push_back('_');
    }
  }

  if (out.empty()) {
    out = fallback;
  }
  return out;
}

bool has_repo_markers(const fs::path& candidate) {
  std::error_code ec;
  return fs::exists(candidate / "CMakeLists.txt", ec) && fs::exists(candidate / "engine", ec);
}

}  // namespace

bool FileSystemService::initialize(const StorageConfig& config) {
  roots_.clear();

  const fs::path install_root = discover_install_root(config);
  const fs::path source_root = discover_source_root(config, install_root);
  const std::optional<fs::path> writable_root = discover_writable_root(config);

  if (!writable_root.has_value()) {
    RENDER_LOG_ERROR(kLogCategory, "Writable root was not resolved. Provide platform_pref_path or writable_root_override.");
    initialized_ = false;
    return false;
  }

  roots_.emplace(PathCategory::InstallRoot, install_root);
  roots_.emplace(PathCategory::SourceData, source_root);
  roots_.emplace(PathCategory::WritableRoot, writable_root.value());
  roots_.emplace(PathCategory::Cache, writable_root.value() / "cache");
  roots_.emplace(PathCategory::ShaderCache, writable_root.value() / "cache" / "shaders");
  roots_.emplace(PathCategory::GeneratedResources, writable_root.value() / "cache" / "generated");
  roots_.emplace(PathCategory::Saves, writable_root.value() / "saves");
  roots_.emplace(PathCategory::Logs, writable_root.value() / "logs");
  roots_.emplace(PathCategory::Screenshots, writable_root.value() / "screenshots");
  roots_.emplace(PathCategory::CrashDumps, writable_root.value() / "crashes");
  roots_.emplace(PathCategory::Config, writable_root.value() / "config");
  roots_.emplace(PathCategory::Temp, writable_root.value() / "temp");
  roots_.emplace(PathCategory::TestOutput, writable_root.value() / "test_output");

  if (config.create_directories_on_init) {
    const PathCategory writable_categories[] = {
      PathCategory::WritableRoot,
      PathCategory::Cache,
      PathCategory::ShaderCache,
      PathCategory::GeneratedResources,
      PathCategory::Saves,
      PathCategory::Logs,
      PathCategory::Screenshots,
      PathCategory::CrashDumps,
      PathCategory::Config,
      PathCategory::Temp,
      PathCategory::TestOutput,
    };

    for (const PathCategory category : writable_categories) {
      if (!ensure_directory_at(root(category))) {
        initialized_ = false;
        return false;
      }
    }
  }

  initialized_ = true;

  for (const auto& [category, path] : roots_) {
    RENDER_LOG_INFO(kLogCategory, "Resolved ", category_name(category), " root: ", path.string());
  }

  if (!fs::exists(source_root)) {
    RENDER_LOG_WARN(kLogCategory, "Source data root does not exist yet: ", source_root.string());
  }

  return true;
}

bool FileSystemService::initialized() const noexcept { return initialized_; }

fs::path FileSystemService::root(const PathCategory category) const {
  if (const auto it = roots_.find(category); it != roots_.end()) {
    return it->second;
  }

  RENDER_LOG_ERROR(kLogCategory, "Requested unknown root category: ", category_name(category));
  return {};
}

fs::path FileSystemService::resolve(const PathCategory category, const std::string_view relative_path) const {
  const auto sanitized = sanitize_relative_path(relative_path);
  if (!sanitized.has_value()) {
    RENDER_LOG_ERROR(kLogCategory, "Refused to resolve unsafe relative path '", relative_path, "' in category ", category_name(category));
    return {};
  }

  return root(category) / sanitized.value();
}

bool FileSystemService::ensure_directory(const PathCategory category, const std::string_view relative_path) {
  const fs::path dir = resolve(category, relative_path);
  if (dir.empty()) {
    return false;
  }
  return ensure_directory_at(dir);
}

bool FileSystemService::exists(const fs::path& path) const {
  std::error_code ec;
  const bool result = fs::exists(path, ec);
  if (ec) {
    RENDER_LOG_WARN(kLogCategory, "exists() failed for path '", path.string(), "': ", ec.message());
    return false;
  }
  return result;
}

bool FileSystemService::is_directory(const fs::path& path) const {
  std::error_code ec;
  const bool result = fs::is_directory(path, ec);
  if (ec) {
    RENDER_LOG_WARN(kLogCategory, "is_directory() failed for path '", path.string(), "': ", ec.message());
    return false;
  }
  return result;
}

bool FileSystemService::is_regular_file(const fs::path& path) const {
  std::error_code ec;
  const bool result = fs::is_regular_file(path, ec);
  if (ec) {
    RENDER_LOG_WARN(kLogCategory, "is_regular_file() failed for path '", path.string(), "': ", ec.message());
    return false;
  }
  return result;
}

bool FileSystemService::read_text(const PathCategory category, const std::string_view relative_path, std::string& out_text) const {
  std::vector<std::byte> bytes;
  if (!read_bytes(category, relative_path, bytes)) {
    return false;
  }

  out_text.assign(reinterpret_cast<const char*>(bytes.data()), bytes.size());
  return true;
}

bool FileSystemService::read_bytes(
  const PathCategory category,
  const std::string_view relative_path,
  std::vector<std::byte>& out_bytes) const {
  const fs::path path = resolve(category, relative_path);
  if (path.empty()) {
    return false;
  }

  std::ifstream input(path, std::ios::binary | std::ios::ate);
  if (!input) {
    RENDER_LOG_ERROR(kLogCategory, "Failed to open file for reading: ", path.string());
    return false;
  }

  const auto end = input.tellg();
  if (end < 0) {
    RENDER_LOG_ERROR(kLogCategory, "Failed to query file size while reading: ", path.string());
    return false;
  }

  out_bytes.resize(static_cast<std::size_t>(end));
  input.seekg(0, std::ios::beg);

  if (!out_bytes.empty() && !input.read(reinterpret_cast<char*>(out_bytes.data()), static_cast<std::streamsize>(out_bytes.size()))) {
    RENDER_LOG_ERROR(kLogCategory, "Failed to read full file: ", path.string());
    return false;
  }

  return true;
}

bool FileSystemService::write_text(
  const PathCategory category,
  const std::string_view relative_path,
  const std::string_view text,
  const WriteMode mode) {
  return write_bytes(category, relative_path, std::as_bytes(std::span{text.data(), text.size()}), mode);
}

bool FileSystemService::write_bytes(
  const PathCategory category,
  const std::string_view relative_path,
  const std::span<const std::byte> bytes,
  const WriteMode mode) {
  const fs::path path = resolve(category, relative_path);
  if (path.empty()) {
    return false;
  }

  if (!ensure_directory_at(path.parent_path())) {
    return false;
  }

  switch (mode) {
    case WriteMode::Direct: return write_bytes_direct(path, bytes);
    case WriteMode::AtomicReplace: return write_bytes_atomic(path, bytes);
  }

  return false;
}

bool FileSystemService::remove(const PathCategory category, const std::string_view relative_path) {
  const fs::path path = resolve(category, relative_path);
  if (path.empty()) {
    return false;
  }

  std::error_code ec;
  const bool removed = fs::remove(path, ec);
  if (ec) {
    RENDER_LOG_ERROR(kLogCategory, "Failed to remove path '", path.string(), "': ", ec.message());
    return false;
  }
  return removed;
}

std::vector<fs::path> FileSystemService::list(const PathCategory category, const std::string_view relative_path) const {
  std::vector<fs::path> result;

  const fs::path dir = resolve(category, relative_path);
  if (dir.empty()) {
    return result;
  }

  std::error_code ec;
  if (!fs::exists(dir, ec)) {
    return result;
  }

  fs::directory_iterator iter(dir, ec);
  if (ec) {
    RENDER_LOG_WARN(kLogCategory, "Failed to list directory '", dir.string(), "': ", ec.message());
    return result;
  }

  for (const auto& entry : iter) {
    result.push_back(entry.path());
  }

  std::sort(result.begin(), result.end());
  return result;
}

fs::path FileSystemService::make_cache_file_path(
  const std::string_view cache_namespace,
  const std::string_view deterministic_key,
  const std::string_view extension) const {
  const std::string ns = sanitize_filename(cache_namespace, "default");
  const auto hash = static_cast<unsigned long long>(core::hash_string(deterministic_key));

  std::ostringstream filename;
  filename << std::hex << hash;
  if (!extension.empty()) {
    if (extension.front() != '.') {
      filename << '.';
    }
    filename << extension;
  }

  return resolve(PathCategory::Cache, (fs::path{ns} / filename.str()).generic_string());
}

fs::path FileSystemService::make_save_file_path(const std::string_view slot_name, const std::string_view extension) const {
  std::string filename = sanitize_filename(slot_name, "slot");
  if (!extension.empty()) {
    if (extension.front() != '.') {
      filename.push_back('.');
    }
    filename.append(extension);
  }
  return resolve(PathCategory::Saves, filename);
}

fs::path FileSystemService::make_log_file_path(const std::string_view stem, const bool timestamped) const {
  std::string filename = sanitize_filename(stem, "runtime");
  if (timestamped) {
    filename += "_" + utc_timestamp_for_filename();
  }
  filename += ".log";
  return resolve(PathCategory::Logs, filename);
}

fs::path FileSystemService::make_screenshot_file_path(const std::string_view stem) const {
  const std::string filename = sanitize_filename(stem, "screenshot") + "_" + utc_timestamp_for_filename() + ".png";
  return resolve(PathCategory::Screenshots, filename);
}

fs::path FileSystemService::make_crash_dump_file_path(const std::string_view stem) const {
  const std::string filename = sanitize_filename(stem, "crash") + "_" + utc_timestamp_for_filename() + ".dmp";
  return resolve(PathCategory::CrashDumps, filename);
}

std::optional<fs::path> FileSystemService::sanitize_relative_path(const std::string_view raw_relative_path) {
  fs::path path{raw_relative_path};
  if (path.empty()) {
    return fs::path{};
  }

  if (path.is_absolute() || path.has_root_name() || path.has_root_directory()) {
    return std::nullopt;
  }

  fs::path normalized;
  for (const auto& part : path) {
    if (part == ".") {
      continue;
    }
    if (part == "..") {
      return std::nullopt;
    }
    normalized /= part;
  }
  return normalized;
}

std::string FileSystemService::category_name(const PathCategory category) {
  switch (category) {
    case PathCategory::InstallRoot: return "install";
    case PathCategory::SourceData: return "source_data";
    case PathCategory::WritableRoot: return "writable";
    case PathCategory::Cache: return "cache";
    case PathCategory::ShaderCache: return "shader_cache";
    case PathCategory::GeneratedResources: return "generated_resources";
    case PathCategory::Saves: return "saves";
    case PathCategory::Logs: return "logs";
    case PathCategory::Screenshots: return "screenshots";
    case PathCategory::CrashDumps: return "crash_dumps";
    case PathCategory::Config: return "config";
    case PathCategory::Temp: return "temp";
    case PathCategory::TestOutput: return "test_output";
  }
  return "unknown";
}

std::string FileSystemService::utc_timestamp_for_filename() {
  const auto now = std::chrono::system_clock::now();
  const std::time_t now_tt = std::chrono::system_clock::to_time_t(now);

  std::tm utc{};
#if defined(_WIN32)
  gmtime_s(&utc, &now_tt);
#else
  gmtime_r(&now_tt, &utc);
#endif

  std::ostringstream oss;
  oss << std::put_time(&utc, "%Y%m%dT%H%M%SZ");
  return oss.str();
}

bool FileSystemService::ensure_directory_at(const fs::path& path) {
  if (path.empty()) {
    return false;
  }

  std::error_code ec;
  if (fs::exists(path, ec)) {
    if (ec) {
      RENDER_LOG_ERROR(kLogCategory, "Failed while checking directory '", path.string(), "': ", ec.message());
      return false;
    }

    if (!fs::is_directory(path, ec)) {
      RENDER_LOG_ERROR(kLogCategory, "Path exists but is not a directory: ", path.string());
      return false;
    }
    return true;
  }

  if (!fs::create_directories(path, ec)) {
    if (ec) {
      RENDER_LOG_ERROR(kLogCategory, "Failed to create directory '", path.string(), "': ", ec.message());
      return false;
    }
  }

  return true;
}

bool FileSystemService::write_bytes_direct(const fs::path& path, const std::span<const std::byte> bytes) {
  std::ofstream output(path, std::ios::binary | std::ios::trunc);
  if (!output) {
    RENDER_LOG_ERROR(kLogCategory, "Failed to open file for writing: ", path.string());
    return false;
  }

  if (!bytes.empty() && !output.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()))) {
    RENDER_LOG_ERROR(kLogCategory, "Failed while writing file: ", path.string());
    return false;
  }

  output.flush();
  if (!output) {
    RENDER_LOG_ERROR(kLogCategory, "Failed while flushing file: ", path.string());
    return false;
  }

  return true;
}

bool FileSystemService::write_bytes_atomic(const fs::path& path, const std::span<const std::byte> bytes) {
  const fs::path temp_path = path.string() + ".tmp";
  if (!write_bytes_direct(temp_path, bytes)) {
    return false;
  }

  std::error_code ec;
  fs::rename(temp_path, path, ec);
  if (!ec) {
    return true;
  }

  std::error_code remove_ec;
  fs::remove(path, remove_ec);
  fs::rename(temp_path, path, ec);
  if (ec) {
    RENDER_LOG_ERROR(kLogCategory, "Atomic rename failed from '", temp_path.string(), "' to '", path.string(), "': ", ec.message());
    std::error_code cleanup_ec;
    fs::remove(temp_path, cleanup_ec);
    return false;
  }

  return true;
}

fs::path FileSystemService::discover_install_root(const StorageConfig& config) const {
  if (config.platform_base_path.has_value() && !config.platform_base_path->empty()) {
    return config.platform_base_path.value();
  }

  if (!config.executable_path.empty()) {
    return config.executable_path.parent_path();
  }

  if (!config.working_directory.empty()) {
    return config.working_directory;
  }

  std::error_code ec;
  return fs::current_path(ec);
}

fs::path FileSystemService::discover_source_root(const StorageConfig& config, const fs::path& install_root) const {
  if (config.source_root_override.has_value()) {
    return config.source_root_override.value();
  }

  const std::vector<fs::path> candidates = {
    install_root / "assets",
    install_root / "assets_recipes",
    install_root / ".." / "assets",
    install_root / ".." / "assets_recipes",
    install_root / ".." / ".." / "assets",
    install_root / ".." / ".." / "assets_recipes",
  };

  for (const fs::path& candidate : candidates) {
    std::error_code ec;
    if (fs::exists(candidate, ec) && fs::is_directory(candidate, ec)) {
      return candidate.lexically_normal();
    }
  }

  fs::path probe = install_root;
  for (int i = 0; i < 8; ++i) {
    if (has_repo_markers(probe)) {
      const fs::path repo_assets = probe / "assets";
      const fs::path recipe_assets = probe / "assets_recipes";
      std::error_code ec;
      if (fs::exists(repo_assets, ec)) {
        return repo_assets;
      }
      if (fs::exists(recipe_assets, ec)) {
        return recipe_assets;
      }
      return repo_assets;
    }

    if (probe == probe.root_path()) {
      break;
    }
    probe = probe.parent_path();
  }

  return install_root / "assets";
}

std::optional<fs::path> FileSystemService::discover_writable_root(const StorageConfig& config) const {
  if (config.writable_root_override.has_value()) {
    return config.writable_root_override.value();
  }

  if (config.platform_pref_path.has_value() && !config.platform_pref_path->empty()) {
    return config.platform_pref_path.value();
  }

  return std::nullopt;
}

}  // namespace render::filesystem
