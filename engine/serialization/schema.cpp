#include "engine/serialization/schema.hpp"

#include "engine/core/hash.hpp"

namespace render::serialization {

void MigrationRegistry::register_migration(const SchemaId schema_id,
                                           const SchemaVersion from,
                                           const SchemaVersion to,
                                           MigrationFn migration) {
  migrations_[Key{schema_id.value, from.value, to.value}] = std::move(migration);
}

bool MigrationRegistry::migrate(const SchemaId schema_id,
                                const SchemaVersion from,
                                const SchemaVersion to,
                                const ByteBuffer& in,
                                ByteBuffer& out,
                                Error& error) const {
  if (from == to) {
    out = in;
    return true;
  }

  const auto it = migrations_.find(Key{schema_id.value, from.value, to.value});
  if (it == migrations_.end()) {
    error = {ErrorCode::MigrationUnavailable, "no migration path registered", 0};
    return false;
  }
  return it->second(in, out, error);
}

std::size_t MigrationRegistry::KeyHash::operator()(const Key& key) const noexcept {
  core::u64 hash = key.schema_id;
  hash = core::hash_combine(hash, key.from);
  hash = core::hash_combine(hash, key.to);
  return static_cast<std::size_t>(hash);
}

void write_envelope(BinaryWriter& writer, const ObjectEnvelope& envelope) {
  writer.write_u32(envelope.magic);
  writer.write_u16(envelope.format_version);
  writer.write_u32(envelope.schema_id.value);
  writer.write_u16(envelope.schema_version.value);
  writer.write_u32(envelope.flags);
  writer.write_u32(envelope.payload_size);
}

bool read_envelope(BinaryReader& reader, ObjectEnvelope& envelope, Error& error) {
  if (!reader.read_u32(envelope.magic) || !reader.read_u16(envelope.format_version) ||
      !reader.read_u32(envelope.schema_id.value) || !reader.read_u16(envelope.schema_version.value) ||
      !reader.read_u32(envelope.flags) || !reader.read_u32(envelope.payload_size)) {
    error = reader.error();
    return false;
  }

  if (envelope.magic != 0x4A42534FU || envelope.format_version != 1U) {
    error = {ErrorCode::IncompatibleEnvelope, "unsupported envelope magic or format", reader.offset()};
    return false;
  }

  if (envelope.payload_size > kMaxContainerLength) {
    error = {ErrorCode::InvalidLength, "payload exceeds canonical maximum", reader.offset()};
    return false;
  }

  return true;
}

ByteBuffer pack_object(const SchemaId schema_id, const SchemaVersion schema_version, const ByteBuffer& payload) {
  BinaryWriter writer;
  write_envelope(writer,
                 ObjectEnvelope{.schema_id = schema_id,
                                .schema_version = schema_version,
                                .payload_size = static_cast<core::u32>(payload.size())});
  writer.write_u32(static_cast<core::u32>(payload.size()));
  writer.write_raw_bytes(payload);
  return writer.bytes();
}

bool unpack_object(const std::span<const core::u8> bytes,
                   const SchemaId expected_schema,
                   const SchemaVersion min_version,
                   const SchemaVersion max_version,
                   ByteBuffer& payload,
                   ObjectEnvelope& envelope,
                   Error& error) {
  BinaryReader reader(bytes);
  if (!read_envelope(reader, envelope, error)) {
    return false;
  }

  if (envelope.schema_id != expected_schema) {
    error = {ErrorCode::UnknownSchema, "unexpected schema id", reader.offset()};
    return false;
  }

  if (envelope.schema_version.value < min_version.value || envelope.schema_version.value > max_version.value) {
    error = {ErrorCode::UnsupportedVersion, "schema version out of supported range", reader.offset()};
    return false;
  }

  core::u32 payload_size_marker = 0;
  if (!reader.read_u32(payload_size_marker)) {
    error = reader.error();
    return false;
  }

  if (payload_size_marker != envelope.payload_size) {
    error = {ErrorCode::InvalidData, "payload size marker mismatch", reader.offset()};
    return false;
  }

  if (reader.remaining() != payload_size_marker) {
    error = {ErrorCode::InvalidData, "payload body length mismatch", reader.offset()};
    return false;
  }

  payload.assign(bytes.begin() + static_cast<std::ptrdiff_t>(reader.offset()), bytes.end());
  return true;
}

}  // namespace render::serialization
