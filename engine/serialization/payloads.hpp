#pragma once

#include "engine/serialization/core_types.hpp"
#include "engine/serialization/schema.hpp"

#include <optional>
#include <unordered_map>

namespace render::serialization::schemas {

enum class SaveSchema : core::u32 { Envelope = 0x1001 };
enum class RecipeSchema : core::u32 { Bundle = 0x2001 };
enum class ItemSchema : core::u32 { ItemPayload = 0x3001 };
enum class RoomSchema : core::u32 { RoomPayload = 0x4001 };
enum class NetworkSchema : core::u32 { MessageEnvelope = 0x5001 };

struct SaveHeader {
  core::u32 save_slot{0};
  core::u64 timestamp_unix_seconds{0};
  core::Seed world_seed{};
  core::u32 room_count{0};
};

struct MaterialRecipe {
  core::u32 material_id{0};
  core::u32 tier{0};
  core::Seed seed{};
};

struct BugRecipe {
  core::u32 bug_type_id{0};
  core::Seed behavior_seed{};
  core::u32 level{1};
};

struct ItemRecipe {
  core::u32 template_id{0};
  core::Seed roll_seed{};
  core::u32 min_power{0};
  core::u32 max_power{0};
};

struct RoomRecipe {
  core::u32 room_type_id{0};
  core::Seed layout_seed{};
  std::vector<MaterialRecipe> materials{};
  std::vector<BugRecipe> bugs{};
  std::vector<ItemRecipe> items{};
};

struct RecipeBundle {
  core::Uuid recipe_bundle_id{};
  core::Seed generator_seed{};
  std::vector<RoomRecipe> rooms{};
};

enum class ItemRarity : core::u32 { Common = 0, Uncommon = 1, Rare = 2, Epic = 3, Legendary = 4 };

struct ItemPayload {
  core::Uuid item_id{};
  core::u32 template_id{0};
  core::Seed seed{};
  core::u64 serial_number{0};
  ItemRarity rarity{ItemRarity::Common};
  std::unordered_map<core::u32, core::i32> affix_rolls{};
  std::string mint_source;
  std::optional<core::Uuid> owner_id;
  std::vector<std::string> provenance_tags{};
};

struct RoomPayload {
  core::Uuid room_id{};
  core::u32 room_type{0};
  core::u32 recipe_id{0};
  core::Transform transform{};
  core::u32 upgrade_level{0};
  core::Seed visual_seed{};
  std::vector<core::u32> state_hooks{};
};

struct SavePayload {
  SaveHeader header{};
  std::vector<ItemPayload> inventory{};
  std::vector<RoomPayload> rooms{};
};

struct NetworkMessage {
  core::u32 message_type_id{0};
  core::u16 message_version{1};
  core::u32 sequence{0};
  ByteBuffer payload{};
};

void write_save_payload(BinaryWriter& writer, const SavePayload& payload);
bool read_save_payload(BinaryReader& reader, SavePayload& payload);

void write_recipe_bundle(BinaryWriter& writer, const RecipeBundle& payload);
bool read_recipe_bundle(BinaryReader& reader, RecipeBundle& payload);

void write_item_payload(BinaryWriter& writer, const ItemPayload& payload);
bool read_item_payload(BinaryReader& reader, ItemPayload& payload);

void write_room_payload(BinaryWriter& writer, const RoomPayload& payload);
bool read_room_payload(BinaryReader& reader, RoomPayload& payload);

void write_network_message(BinaryWriter& writer, const NetworkMessage& payload);
bool read_network_message(BinaryReader& reader, NetworkMessage& payload);

ByteBuffer serialize_save(const SavePayload& payload);
bool deserialize_save(std::span<const core::u8> bytes, SavePayload& payload, Error& error);

ByteBuffer serialize_recipe_bundle(const RecipeBundle& payload);
bool deserialize_recipe_bundle(std::span<const core::u8> bytes, RecipeBundle& payload, Error& error);

ByteBuffer serialize_item(const ItemPayload& payload);
bool deserialize_item(std::span<const core::u8> bytes, ItemPayload& payload, Error& error);

ByteBuffer serialize_room(const RoomPayload& payload);
bool deserialize_room(std::span<const core::u8> bytes, RoomPayload& payload, Error& error);

ByteBuffer serialize_network_message(const NetworkMessage& payload);
bool deserialize_network_message(std::span<const core::u8> bytes, NetworkMessage& payload, Error& error);

}  // namespace render::serialization::schemas
