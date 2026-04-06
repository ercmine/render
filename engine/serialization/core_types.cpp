#include "engine/serialization/core_types.hpp"

namespace render::serialization {

void write_uuid(BinaryWriter& writer, const core::Uuid& uuid) {
  writer.write_array(uuid.bytes(), [](BinaryWriter& w, const core::u8 value) { w.write_u8(value); });
}

bool read_uuid(BinaryReader& reader, core::Uuid& uuid) {
  std::array<core::u8, 16> bytes{};
  if (!reader.read_array(bytes, [](BinaryReader& r, core::u8& value) { return r.read_u8(value); })) {
    return false;
  }
  uuid = core::Uuid(bytes);
  return true;
}

}  // namespace render::serialization
