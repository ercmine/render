#include "engine/serialization/serialization.hpp"

#include <algorithm>
#include <array>
#include <cassert>

int main() {
  using namespace render;
  using namespace serialization;
  using namespace serialization::schemas;

  {
    BinaryWriter writer;
    writer.write_u16(0xBEEFU);
    writer.write_i32(-42);
    writer.write_f32(4.5F);
    writer.write_bool(true);
    writer.write_string("deterministic");
    const ByteBuffer blob{1, 2, 3, 4};
    writer.write_bytes(blob);

    BinaryReader reader(writer.bytes());
    core::u16 u16 = 0;
    core::i32 i32 = 0;
    core::f32 f32 = 0.0F;
    bool b = false;
    std::string text;
    ByteBuffer read_blob;
    assert(reader.read_u16(u16));
    assert(reader.read_i32(i32));
    assert(reader.read_f32(f32));
    assert(reader.read_bool(b));
    assert(reader.read_string(text));
    assert(reader.read_bytes(read_blob));
    assert(reader.remaining() == 0);
    assert(u16 == 0xBEEFU);
    assert(i32 == -42);
    assert(core::nearly_equal(f32, 4.5F));
    assert(b);
    assert(text == "deterministic");
    assert(read_blob == blob);
  }

  {
    const core::Uuid uuid = core::Uuid::generate_v4();
    const core::Seed seed{123456789ULL};
    BinaryWriter writer;
    write_uuid(writer, uuid);
    write_seed(writer, seed);

    BinaryReader reader(writer.bytes());
    core::Uuid uuid_out;
    core::Seed seed_out{};
    assert(read_uuid(reader, uuid_out));
    assert(read_seed(reader, seed_out));
    assert(uuid == uuid_out);
    assert(seed.value == seed_out.value);
  }

  {
    ItemPayload item{};
    item.item_id = core::Uuid::generate_v4();
    item.template_id = 77;
    item.seed = core::Seed{9988};
    item.serial_number = 0xABCDEF012345ULL;
    item.rarity = ItemRarity::Epic;
    item.affix_rolls = {{5, 10}, {2, -3}, {3, 7}};
    item.mint_source = "drop:boss";
    item.owner_id = core::Uuid::generate_v4();
    item.provenance_tags = {"s6", "deterministic"};

    const ByteBuffer encoded_a = serialize_item(item);
    const ByteBuffer encoded_b = serialize_item(item);
    assert(encoded_a == encoded_b);

    ItemPayload decoded{};
    Error error;
    assert(deserialize_item(encoded_a, decoded, error));
    assert(decoded.template_id == item.template_id);
    assert(decoded.seed.value == item.seed.value);
    assert(decoded.affix_rolls == item.affix_rolls);

    BinaryWriter ordered_writer;
    ordered_writer.write_map_sorted(decoded.affix_rolls,
                                    [](BinaryWriter& w, const core::u32 key, const core::i32 value) {
                                      w.write_u32(key);
                                      w.write_i32(value);
                                    });
    BinaryReader ordered_reader(ordered_writer.bytes());
    core::u32 count = 0;
    assert(ordered_reader.read_u32(count));
    assert(count == decoded.affix_rolls.size());
    std::vector<core::u32> keys;
    for (core::u32 i = 0; i < count; ++i) {
      core::u32 key = 0;
      core::i32 value = 0;
      assert(ordered_reader.read_u32(key));
      assert(ordered_reader.read_i32(value));
      keys.push_back(key);
    }
    assert(std::is_sorted(keys.begin(), keys.end()));
  }

  {
    RoomPayload room{};
    room.room_id = core::Uuid::generate_v4();
    room.room_type = 4;
    room.recipe_id = 12;
    room.transform.translation = {1.0F, 2.0F, 3.0F};
    room.upgrade_level = 2;
    room.visual_seed = core::Seed{42};
    room.state_hooks = {9, 10, 11};

    SavePayload save{};
    save.header.save_slot = 1;
    save.header.timestamp_unix_seconds = 1712000000ULL;
    save.header.world_seed = core::Seed{1234};
    save.header.room_count = 1;
    save.rooms = {room};
    save.inventory = {};

    const ByteBuffer encoded = serialize_save(save);
    SavePayload decoded{};
    Error error;
    assert(deserialize_save(encoded, decoded, error));
    assert(decoded.header.room_count == 1);
    assert(decoded.rooms.size() == 1);
    assert(decoded.rooms[0].recipe_id == room.recipe_id);
  }

  {
    RecipeBundle bundle{};
    bundle.recipe_bundle_id = core::Uuid::generate_v4();
    bundle.generator_seed = core::Seed{44};
    bundle.rooms.push_back(RoomRecipe{.room_type_id = 2,
                                      .layout_seed = core::Seed{99},
                                      .materials = {{.material_id = 8, .tier = 3, .seed = core::Seed{9}}},
                                      .bugs = {{.bug_type_id = 5, .behavior_seed = core::Seed{33}, .level = 7}},
                                      .items = {{.template_id = 9, .roll_seed = core::Seed{1}, .min_power = 2, .max_power = 6}}});

    const ByteBuffer encoded = serialize_recipe_bundle(bundle);
    RecipeBundle decoded{};
    Error error;
    assert(deserialize_recipe_bundle(encoded, decoded, error));
    assert(decoded.rooms.size() == 1);
    assert(decoded.rooms[0].items[0].max_power == 6);
  }

  {
    NetworkMessage message{};
    message.message_type_id = 0x42;
    message.message_version = 3;
    message.sequence = 987;
    message.payload = {8, 6, 7, 5, 3, 0, 9};

    const ByteBuffer encoded = serialize_network_message(message);
    NetworkMessage decoded{};
    Error error;
    assert(deserialize_network_message(encoded, decoded, error));
    assert(decoded.message_type_id == message.message_type_id);
    assert(decoded.payload == message.payload);

    ByteBuffer corrupted = encoded;
    corrupted.resize(corrupted.size() - 2);
    NetworkMessage rejected{};
    Error decode_error;
    assert(!deserialize_network_message(corrupted, rejected, decode_error));
    assert(decode_error.code != ErrorCode::None);
  }

  {
    // Schema/version mismatch coverage.
    BinaryWriter payload_writer;
    payload_writer.write_u32(123);
    const ByteBuffer bad_schema = pack_object(SchemaId{9999}, SchemaVersion{1}, payload_writer.bytes());
    SavePayload ignored{};
    Error error;
    assert(!deserialize_save(bad_schema, ignored, error));
    assert(error.code == ErrorCode::UnknownSchema);
  }

  return 0;
}
