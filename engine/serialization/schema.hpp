#pragma once

#include "engine/serialization/binary_reader.hpp"
#include "engine/serialization/binary_writer.hpp"
#include "engine/serialization/errors.hpp"

#include <functional>
#include <unordered_map>

namespace render::serialization {

struct SchemaId {
  core::u32 value{0};
  friend bool operator==(const SchemaId&, const SchemaId&) noexcept = default;
};

struct SchemaVersion {
  core::u16 value{1};
  friend bool operator==(const SchemaVersion&, const SchemaVersion&) noexcept = default;
};

struct ObjectEnvelope {
  core::u32 magic{0x4A42534FU};  // 'OSBJ' little endian on wire
  core::u16 format_version{1};
  SchemaId schema_id{};
  SchemaVersion schema_version{};
  core::u32 flags{0};
  core::u32 payload_size{0};
};

using MigrationFn = std::function<bool(const ByteBuffer&, ByteBuffer&, Error&)>;

class MigrationRegistry {
public:
  void register_migration(SchemaId schema_id, SchemaVersion from, SchemaVersion to, MigrationFn migration);
  [[nodiscard]] bool migrate(SchemaId schema_id,
                             SchemaVersion from,
                             SchemaVersion to,
                             const ByteBuffer& in,
                             ByteBuffer& out,
                             Error& error) const;

private:
  struct Key {
    core::u32 schema_id{0};
    core::u16 from{0};
    core::u16 to{0};

    friend bool operator==(const Key&, const Key&) noexcept = default;
  };

  struct KeyHash {
    std::size_t operator()(const Key& key) const noexcept;
  };

  std::unordered_map<Key, MigrationFn, KeyHash> migrations_{};
};

void write_envelope(BinaryWriter& writer, const ObjectEnvelope& envelope);
[[nodiscard]] bool read_envelope(BinaryReader& reader, ObjectEnvelope& envelope, Error& error);

[[nodiscard]] ByteBuffer pack_object(SchemaId schema_id, SchemaVersion schema_version, const ByteBuffer& payload);
[[nodiscard]] bool unpack_object(std::span<const core::u8> bytes,
                                 SchemaId expected_schema,
                                 SchemaVersion min_version,
                                 SchemaVersion max_version,
                                 ByteBuffer& payload,
                                 ObjectEnvelope& envelope,
                                 Error& error);

}  // namespace render::serialization
