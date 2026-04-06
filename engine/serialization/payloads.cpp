#include "engine/serialization/payloads.hpp"

namespace render::serialization::schemas {

namespace {

constexpr SchemaVersion kV1{1};

void write_save_header(BinaryWriter& writer, const SaveHeader& value) {
  writer.write_u32(value.save_slot);
  writer.write_u64(value.timestamp_unix_seconds);
  write_seed(writer, value.world_seed);
  writer.write_u32(value.room_count);
}

bool read_save_header(BinaryReader& reader, SaveHeader& value) {
  return reader.read_u32(value.save_slot) && reader.read_u64(value.timestamp_unix_seconds) &&
         read_seed(reader, value.world_seed) && reader.read_u32(value.room_count);
}

void write_material_recipe(BinaryWriter& writer, const MaterialRecipe& value) {
  writer.write_u32(value.material_id);
  writer.write_u32(value.tier);
  write_seed(writer, value.seed);
}

bool read_material_recipe(BinaryReader& reader, MaterialRecipe& value) {
  return reader.read_u32(value.material_id) && reader.read_u32(value.tier) && read_seed(reader, value.seed);
}

void write_bug_recipe(BinaryWriter& writer, const BugRecipe& value) {
  writer.write_u32(value.bug_type_id);
  write_seed(writer, value.behavior_seed);
  writer.write_u32(value.level);
}

bool read_bug_recipe(BinaryReader& reader, BugRecipe& value) {
  return reader.read_u32(value.bug_type_id) && read_seed(reader, value.behavior_seed) && reader.read_u32(value.level);
}

void write_item_recipe(BinaryWriter& writer, const ItemRecipe& value) {
  writer.write_u32(value.template_id);
  write_seed(writer, value.roll_seed);
  writer.write_u32(value.min_power);
  writer.write_u32(value.max_power);
}

bool read_item_recipe(BinaryReader& reader, ItemRecipe& value) {
  return reader.read_u32(value.template_id) && read_seed(reader, value.roll_seed) && reader.read_u32(value.min_power) &&
         reader.read_u32(value.max_power);
}

void write_room_recipe(BinaryWriter& writer, const RoomRecipe& value) {
  writer.write_u32(value.room_type_id);
  write_seed(writer, value.layout_seed);
  writer.write_vector(value.materials, [](BinaryWriter& w, const MaterialRecipe& recipe) { write_material_recipe(w, recipe); });
  writer.write_vector(value.bugs, [](BinaryWriter& w, const BugRecipe& recipe) { write_bug_recipe(w, recipe); });
  writer.write_vector(value.items, [](BinaryWriter& w, const ItemRecipe& recipe) { write_item_recipe(w, recipe); });
}

bool read_room_recipe(BinaryReader& reader, RoomRecipe& value) {
  return reader.read_u32(value.room_type_id) && read_seed(reader, value.layout_seed) &&
         reader.read_vector(value.materials,
                            [](BinaryReader& r, MaterialRecipe& recipe) { return read_material_recipe(r, recipe); }) &&
         reader.read_vector(value.bugs, [](BinaryReader& r, BugRecipe& recipe) { return read_bug_recipe(r, recipe); }) &&
         reader.read_vector(value.items, [](BinaryReader& r, ItemRecipe& recipe) { return read_item_recipe(r, recipe); });
}

}  // namespace

void write_save_payload(BinaryWriter& writer, const SavePayload& payload) {
  write_save_header(writer, payload.header);
  writer.write_vector(payload.inventory, [](BinaryWriter& w, const ItemPayload& item) { write_item_payload(w, item); });
  writer.write_vector(payload.rooms, [](BinaryWriter& w, const RoomPayload& room) { write_room_payload(w, room); });
}

bool read_save_payload(BinaryReader& reader, SavePayload& payload) {
  return read_save_header(reader, payload.header) &&
         reader.read_vector(payload.inventory, [](BinaryReader& r, ItemPayload& item) { return read_item_payload(r, item); }) &&
         reader.read_vector(payload.rooms, [](BinaryReader& r, RoomPayload& room) { return read_room_payload(r, room); });
}

void write_recipe_bundle(BinaryWriter& writer, const RecipeBundle& payload) {
  write_uuid(writer, payload.recipe_bundle_id);
  write_seed(writer, payload.generator_seed);
  writer.write_vector(payload.rooms, [](BinaryWriter& w, const RoomRecipe& recipe) { write_room_recipe(w, recipe); });
}

bool read_recipe_bundle(BinaryReader& reader, RecipeBundle& payload) {
  return read_uuid(reader, payload.recipe_bundle_id) && read_seed(reader, payload.generator_seed) &&
         reader.read_vector(payload.rooms, [](BinaryReader& r, RoomRecipe& recipe) { return read_room_recipe(r, recipe); });
}

void write_item_payload(BinaryWriter& writer, const ItemPayload& payload) {
  write_uuid(writer, payload.item_id);
  writer.write_u32(payload.template_id);
  write_seed(writer, payload.seed);
  writer.write_u64(payload.serial_number);
  writer.write_enum_u32(payload.rarity);
  writer.write_map_sorted(payload.affix_rolls,
                          [](BinaryWriter& w, const core::u32 key, const core::i32 value) { w.write_u32(key); w.write_i32(value); });
  writer.write_string(payload.mint_source);
  writer.write_optional(payload.owner_id, [](BinaryWriter& w, const core::Uuid& owner) { write_uuid(w, owner); });
  writer.write_vector(payload.provenance_tags, [](BinaryWriter& w, const std::string& tag) { w.write_string(tag); });
}

bool read_item_payload(BinaryReader& reader, ItemPayload& payload) {
  std::vector<std::pair<core::u32, core::i32>> ordered_affixes;
  if (!read_uuid(reader, payload.item_id) || !reader.read_u32(payload.template_id) || !read_seed(reader, payload.seed) ||
      !reader.read_u64(payload.serial_number) || !reader.read_enum_u32(payload.rarity) ||
      !reader.read_vector(ordered_affixes, [](BinaryReader& r, std::pair<core::u32, core::i32>& value) {
        return r.read_u32(value.first) && r.read_i32(value.second);
      }) ||
      !reader.read_string(payload.mint_source) ||
      !reader.read_optional(payload.owner_id, [](BinaryReader& r, core::Uuid& owner) { return read_uuid(r, owner); }) ||
      !reader.read_vector(payload.provenance_tags, [](BinaryReader& r, std::string& tag) { return r.read_string(tag); })) {
    return false;
  }

  payload.affix_rolls.clear();
  for (const auto& [key, value] : ordered_affixes) {
    payload.affix_rolls[key] = value;
  }
  return true;
}

void write_room_payload(BinaryWriter& writer, const RoomPayload& payload) {
  write_uuid(writer, payload.room_id);
  writer.write_u32(payload.room_type);
  writer.write_u32(payload.recipe_id);
  write_transform(writer, payload.transform);
  writer.write_u32(payload.upgrade_level);
  write_seed(writer, payload.visual_seed);
  writer.write_vector(payload.state_hooks, [](BinaryWriter& w, const core::u32 value) { w.write_u32(value); });
}

bool read_room_payload(BinaryReader& reader, RoomPayload& payload) {
  return read_uuid(reader, payload.room_id) && reader.read_u32(payload.room_type) && reader.read_u32(payload.recipe_id) &&
         read_transform(reader, payload.transform) && reader.read_u32(payload.upgrade_level) &&
         read_seed(reader, payload.visual_seed) &&
         reader.read_vector(payload.state_hooks, [](BinaryReader& r, core::u32& value) { return r.read_u32(value); });
}

void write_network_message(BinaryWriter& writer, const NetworkMessage& payload) {
  writer.write_u32(payload.message_type_id);
  writer.write_u16(payload.message_version);
  writer.write_u32(payload.sequence);
  writer.write_bytes(payload.payload);
}

bool read_network_message(BinaryReader& reader, NetworkMessage& payload) {
  return reader.read_u32(payload.message_type_id) && reader.read_u16(payload.message_version) &&
         reader.read_u32(payload.sequence) && reader.read_bytes(payload.payload);
}

ByteBuffer serialize_save(const SavePayload& payload) {
  BinaryWriter payload_writer;
  write_save_payload(payload_writer, payload);
  return pack_object(SchemaId{static_cast<core::u32>(SaveSchema::Envelope)}, kV1, payload_writer.bytes());
}

bool deserialize_save(const std::span<const core::u8> bytes, SavePayload& payload, Error& error) {
  ByteBuffer encoded_payload;
  ObjectEnvelope envelope;
  if (!unpack_object(bytes,
                     SchemaId{static_cast<core::u32>(SaveSchema::Envelope)},
                     kV1,
                     kV1,
                     encoded_payload,
                     envelope,
                     error)) {
    return false;
  }
  BinaryReader reader(encoded_payload);
  if (!read_save_payload(reader, payload) || reader.remaining() != 0) {
    error = reader.error();
    return false;
  }
  return true;
}

ByteBuffer serialize_recipe_bundle(const RecipeBundle& payload) {
  BinaryWriter payload_writer;
  write_recipe_bundle(payload_writer, payload);
  return pack_object(SchemaId{static_cast<core::u32>(RecipeSchema::Bundle)}, kV1, payload_writer.bytes());
}

bool deserialize_recipe_bundle(const std::span<const core::u8> bytes, RecipeBundle& payload, Error& error) {
  ByteBuffer encoded_payload;
  ObjectEnvelope envelope;
  if (!unpack_object(bytes,
                     SchemaId{static_cast<core::u32>(RecipeSchema::Bundle)},
                     kV1,
                     kV1,
                     encoded_payload,
                     envelope,
                     error)) {
    return false;
  }
  BinaryReader reader(encoded_payload);
  if (!read_recipe_bundle(reader, payload) || reader.remaining() != 0) {
    error = reader.error();
    return false;
  }
  return true;
}

ByteBuffer serialize_item(const ItemPayload& payload) {
  BinaryWriter payload_writer;
  write_item_payload(payload_writer, payload);
  return pack_object(SchemaId{static_cast<core::u32>(ItemSchema::ItemPayload)}, kV1, payload_writer.bytes());
}

bool deserialize_item(const std::span<const core::u8> bytes, ItemPayload& payload, Error& error) {
  ByteBuffer encoded_payload;
  ObjectEnvelope envelope;
  if (!unpack_object(bytes,
                     SchemaId{static_cast<core::u32>(ItemSchema::ItemPayload)},
                     kV1,
                     kV1,
                     encoded_payload,
                     envelope,
                     error)) {
    return false;
  }
  BinaryReader reader(encoded_payload);
  if (!read_item_payload(reader, payload) || reader.remaining() != 0) {
    error = reader.error();
    return false;
  }
  return true;
}

ByteBuffer serialize_room(const RoomPayload& payload) {
  BinaryWriter payload_writer;
  write_room_payload(payload_writer, payload);
  return pack_object(SchemaId{static_cast<core::u32>(RoomSchema::RoomPayload)}, kV1, payload_writer.bytes());
}

bool deserialize_room(const std::span<const core::u8> bytes, RoomPayload& payload, Error& error) {
  ByteBuffer encoded_payload;
  ObjectEnvelope envelope;
  if (!unpack_object(bytes,
                     SchemaId{static_cast<core::u32>(RoomSchema::RoomPayload)},
                     kV1,
                     kV1,
                     encoded_payload,
                     envelope,
                     error)) {
    return false;
  }
  BinaryReader reader(encoded_payload);
  if (!read_room_payload(reader, payload) || reader.remaining() != 0) {
    error = reader.error();
    return false;
  }
  return true;
}

ByteBuffer serialize_network_message(const NetworkMessage& payload) {
  BinaryWriter payload_writer;
  write_network_message(payload_writer, payload);
  return pack_object(SchemaId{static_cast<core::u32>(NetworkSchema::MessageEnvelope)}, kV1, payload_writer.bytes());
}

bool deserialize_network_message(const std::span<const core::u8> bytes, NetworkMessage& payload, Error& error) {
  ByteBuffer encoded_payload;
  ObjectEnvelope envelope;
  if (!unpack_object(bytes,
                     SchemaId{static_cast<core::u32>(NetworkSchema::MessageEnvelope)},
                     kV1,
                     kV1,
                     encoded_payload,
                     envelope,
                     error)) {
    return false;
  }
  BinaryReader reader(encoded_payload);
  if (!read_network_message(reader, payload) || reader.remaining() != 0) {
    error = reader.error();
    return false;
  }
  return true;
}

}  // namespace render::serialization::schemas
