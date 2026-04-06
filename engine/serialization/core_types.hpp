#pragma once

#include "engine/core/camera.hpp"
#include "engine/core/color.hpp"
#include "engine/core/random.hpp"
#include "engine/core/transform.hpp"
#include "engine/core/uuid.hpp"
#include "engine/serialization/binary_reader.hpp"
#include "engine/serialization/binary_writer.hpp"

namespace render::serialization {

void write_uuid(BinaryWriter& writer, const core::Uuid& uuid);
bool read_uuid(BinaryReader& reader, core::Uuid& uuid);

inline void write_seed(BinaryWriter& writer, const core::Seed seed) { writer.write_u64(seed.value); }
inline bool read_seed(BinaryReader& reader, core::Seed& seed) {
  return reader.read_u64(seed.value);
}

inline void write_vec3(BinaryWriter& writer, const core::Vec3& value) {
  writer.write_f32(value.x);
  writer.write_f32(value.y);
  writer.write_f32(value.z);
}
inline bool read_vec3(BinaryReader& reader, core::Vec3& value) {
  return reader.read_f32(value.x) && reader.read_f32(value.y) && reader.read_f32(value.z);
}

inline void write_quaternion(BinaryWriter& writer, const core::Quaternion& value) {
  writer.write_f32(value.x);
  writer.write_f32(value.y);
  writer.write_f32(value.z);
  writer.write_f32(value.w);
}
inline bool read_quaternion(BinaryReader& reader, core::Quaternion& value) {
  return reader.read_f32(value.x) && reader.read_f32(value.y) && reader.read_f32(value.z) && reader.read_f32(value.w);
}

inline void write_transform(BinaryWriter& writer, const core::Transform& value) {
  write_vec3(writer, value.translation);
  write_quaternion(writer, value.rotation);
  write_vec3(writer, value.scale);
}
inline bool read_transform(BinaryReader& reader, core::Transform& value) {
  return read_vec3(reader, value.translation) && read_quaternion(reader, value.rotation) && read_vec3(reader, value.scale);
}

inline void write_color_rgba8(BinaryWriter& writer, const core::ColorRGBA8& value) {
  writer.write_u8(value.r);
  writer.write_u8(value.g);
  writer.write_u8(value.b);
  writer.write_u8(value.a);
}
inline bool read_color_rgba8(BinaryReader& reader, core::ColorRGBA8& value) {
  return reader.read_u8(value.r) && reader.read_u8(value.g) && reader.read_u8(value.b) && reader.read_u8(value.a);
}

inline void write_perspective_camera(BinaryWriter& writer, const core::PerspectiveCameraParams& value) {
  writer.write_f32(value.vertical_fov_radians);
  writer.write_f32(value.aspect_ratio);
  writer.write_f32(value.near_plane);
  writer.write_f32(value.far_plane);
}

inline bool read_perspective_camera(BinaryReader& reader, core::PerspectiveCameraParams& value) {
  return reader.read_f32(value.vertical_fov_radians) && reader.read_f32(value.aspect_ratio) &&
         reader.read_f32(value.near_plane) && reader.read_f32(value.far_plane);
}

}  // namespace render::serialization
