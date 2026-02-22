/*  ==============================================================================
 *  Tagged Binary Format (TBF) - www.electrodiux.com
 *  ------------------------------------------------------------------------------
 *  Copyright (c) 2026 Electrodiux. All rights reserved.
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in
 *  all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *  SOFTWARE.
 *  ==============================================================================
 */

#include "tbf/DataTag.hpp"
#include "tbf/Reader.hpp"
#include "tbf/Writer.hpp"

#include <gtest/gtest.h>

#include <cstdint>

using namespace tbf;

namespace {

constexpr DataTag TAG_VEC2_I8 = "vec2_i8";
constexpr DataTag TAG_VEC2_I16 = "vec2_i16";
constexpr DataTag TAG_VEC2_I32 = "vec2_i32";
constexpr DataTag TAG_VEC2_I64 = "vec2_i64";
constexpr DataTag TAG_VEC2_BOOL = "vec2_bool";
constexpr DataTag TAG_VEC2_F32 = "vec2_f32";
constexpr DataTag TAG_VEC2_F64 = "vec2_f64";

constexpr DataTag TAG_VEC3_I8 = "vec3_i8";
constexpr DataTag TAG_VEC3_I16 = "vec3_i16";
constexpr DataTag TAG_VEC3_I32 = "vec3_i32";
constexpr DataTag TAG_VEC3_I64 = "vec3_i64";
constexpr DataTag TAG_VEC3_BOOL = "vec3_bool";
constexpr DataTag TAG_VEC3_F32 = "vec3_f32";
constexpr DataTag TAG_VEC3_F64 = "vec3_f64";

constexpr DataTag TAG_VEC4_I8 = "vec4_i8";
constexpr DataTag TAG_VEC4_I16 = "vec4_i16";
constexpr DataTag TAG_VEC4_I32 = "vec4_i32";
constexpr DataTag TAG_VEC4_I64 = "vec4_i64";
constexpr DataTag TAG_VEC4_BOOL = "vec4_bool";
constexpr DataTag TAG_VEC4_F32 = "vec4_f32";
constexpr DataTag TAG_VEC4_F64 = "vec4_f64";

}  // namespace

TEST(VectorsTest, Vector2Types) {
    Writer writer(true);
    auto& root = writer.RootObject();

    int8_t vec2_i8[2] = {-10, 20};
    int16_t vec2_i16[2] = {-1000, 2000};
    int32_t vec2_i32[2] = {-100000, 200000};
    int64_t vec2_i64[2] = {-1000000000LL, 2000000000LL};
    bool vec2_bool[2] = {true, false};
    float vec2_f32[2] = {1.5f, -2.5f};
    double vec2_f64[2] = {10.123, -20.456};

    root.FieldVector2i8(TAG_VEC2_I8, vec2_i8);
    root.FieldVector2i16(TAG_VEC2_I16, vec2_i16);
    root.FieldVector2i32(TAG_VEC2_I32, vec2_i32);
    root.FieldVector2i64(TAG_VEC2_I64, vec2_i64);
    root.FieldVector2b(TAG_VEC2_BOOL, vec2_bool);
    root.FieldVector2f32(TAG_VEC2_F32, vec2_f32);
    root.FieldVector2f64(TAG_VEC2_F64, vec2_f64);

    writer.Finish();

    Reader reader(writer.Data(), writer.Size(), true);
    const auto& read_root = reader.RootObject();

    ASSERT_TRUE(read_root.IsValid());

    // Test i8
    int8_t* read_i8 = read_root.ReadVector2i8(TAG_VEC2_I8);
    ASSERT_NE(read_i8, nullptr);
    EXPECT_EQ(read_i8[0], vec2_i8[0]);
    EXPECT_EQ(read_i8[1], vec2_i8[1]);

    // Test i16
    int16_t* read_i16 = read_root.ReadVector2i16(TAG_VEC2_I16);
    ASSERT_NE(read_i16, nullptr);
    EXPECT_EQ(read_i16[0], vec2_i16[0]);
    EXPECT_EQ(read_i16[1], vec2_i16[1]);

    // Test i32
    int32_t* read_i32 = read_root.ReadVector2i32(TAG_VEC2_I32);
    ASSERT_NE(read_i32, nullptr);
    EXPECT_EQ(read_i32[0], vec2_i32[0]);
    EXPECT_EQ(read_i32[1], vec2_i32[1]);

    // Test i64
    int64_t* read_i64 = read_root.ReadVector2i64(TAG_VEC2_I64);
    ASSERT_NE(read_i64, nullptr);
    EXPECT_EQ(read_i64[0], vec2_i64[0]);
    EXPECT_EQ(read_i64[1], vec2_i64[1]);

    // Test bool
    bool* read_bool = read_root.ReadVector2b(TAG_VEC2_BOOL);
    ASSERT_NE(read_bool, nullptr);
    EXPECT_EQ(read_bool[0], vec2_bool[0]);
    EXPECT_EQ(read_bool[1], vec2_bool[1]);

    // Test f32
    float* read_f32 = read_root.ReadVector2f32(TAG_VEC2_F32);
    ASSERT_NE(read_f32, nullptr);
    EXPECT_FLOAT_EQ(read_f32[0], vec2_f32[0]);
    EXPECT_FLOAT_EQ(read_f32[1], vec2_f32[1]);

    // Test f64
    double* read_f64 = read_root.ReadVector2f64(TAG_VEC2_F64);
    ASSERT_NE(read_f64, nullptr);
    EXPECT_DOUBLE_EQ(read_f64[0], vec2_f64[0]);
    EXPECT_DOUBLE_EQ(read_f64[1], vec2_f64[1]);
}

TEST(VectorsTest, Vector3Types) {
    Writer writer(true);
    auto& root = writer.RootObject();

    int8_t vec3_i8[3] = {-5, 10, -15};
    int16_t vec3_i16[3] = {-500, 1000, -1500};
    int32_t vec3_i32[3] = {-50000, 100000, -150000};
    int64_t vec3_i64[3] = {-500000000LL, 1000000000LL, -1500000000LL};
    bool vec3_bool[3] = {false, true, false};
    float vec3_f32[3] = {1.1f, 2.2f, 3.3f};
    double vec3_f64[3] = {11.111, 22.222, 33.333};

    root.FieldVector3i8(TAG_VEC3_I8, vec3_i8);
    root.FieldVector3i16(TAG_VEC3_I16, vec3_i16);
    root.FieldVector3i32(TAG_VEC3_I32, vec3_i32);
    root.FieldVector3i64(TAG_VEC3_I64, vec3_i64);
    root.FieldVector3b(TAG_VEC3_BOOL, vec3_bool);
    root.FieldVector3f32(TAG_VEC3_F32, vec3_f32);
    root.FieldVector3f64(TAG_VEC3_F64, vec3_f64);

    writer.Finish();

    Reader reader(writer.Data(), writer.Size(), true);
    const auto& read_root = reader.RootObject();

    ASSERT_TRUE(read_root.IsValid());

    // Test i8
    int8_t* read_i8 = read_root.ReadVector3i8(TAG_VEC3_I8);
    ASSERT_NE(read_i8, nullptr);
    EXPECT_EQ(read_i8[0], vec3_i8[0]);
    EXPECT_EQ(read_i8[1], vec3_i8[1]);
    EXPECT_EQ(read_i8[2], vec3_i8[2]);

    // Test i16
    int16_t* read_i16 = read_root.ReadVector3i16(TAG_VEC3_I16);
    ASSERT_NE(read_i16, nullptr);
    EXPECT_EQ(read_i16[0], vec3_i16[0]);
    EXPECT_EQ(read_i16[1], vec3_i16[1]);
    EXPECT_EQ(read_i16[2], vec3_i16[2]);

    // Test i32
    int32_t* read_i32 = read_root.ReadVector3i32(TAG_VEC3_I32);
    ASSERT_NE(read_i32, nullptr);
    EXPECT_EQ(read_i32[0], vec3_i32[0]);
    EXPECT_EQ(read_i32[1], vec3_i32[1]);
    EXPECT_EQ(read_i32[2], vec3_i32[2]);

    // Test i64
    int64_t* read_i64 = read_root.ReadVector3i64(TAG_VEC3_I64);
    ASSERT_NE(read_i64, nullptr);
    EXPECT_EQ(read_i64[0], vec3_i64[0]);
    EXPECT_EQ(read_i64[1], vec3_i64[1]);
    EXPECT_EQ(read_i64[2], vec3_i64[2]);

    // Test bool
    bool* read_bool = read_root.ReadVector3b(TAG_VEC3_BOOL);
    ASSERT_NE(read_bool, nullptr);
    EXPECT_EQ(read_bool[0], vec3_bool[0]);
    EXPECT_EQ(read_bool[1], vec3_bool[1]);
    EXPECT_EQ(read_bool[2], vec3_bool[2]);

    // Test f32
    float* read_f32 = read_root.ReadVector3f32(TAG_VEC3_F32);
    ASSERT_NE(read_f32, nullptr);
    EXPECT_FLOAT_EQ(read_f32[0], vec3_f32[0]);
    EXPECT_FLOAT_EQ(read_f32[1], vec3_f32[1]);
    EXPECT_FLOAT_EQ(read_f32[2], vec3_f32[2]);

    // Test f64
    double* read_f64 = read_root.ReadVector3f64(TAG_VEC3_F64);
    ASSERT_NE(read_f64, nullptr);
    EXPECT_DOUBLE_EQ(read_f64[0], vec3_f64[0]);
    EXPECT_DOUBLE_EQ(read_f64[1], vec3_f64[1]);
    EXPECT_DOUBLE_EQ(read_f64[2], vec3_f64[2]);
}

TEST(VectorsTest, Vector4Types) {
    Writer writer(true);
    auto& root = writer.RootObject();

    int8_t vec4_i8[4] = {-1, 2, -3, 4};
    int16_t vec4_i16[4] = {-100, 200, -300, 400};
    int32_t vec4_i32[4] = {-10000, 20000, -30000, 40000};
    int64_t vec4_i64[4] = {-100000000LL, 200000000LL, -300000000LL, 400000000LL};
    bool vec4_bool[4] = {true, false, true, false};
    float vec4_f32[4] = {0.1f, 0.2f, 0.3f, 0.4f};
    double vec4_f64[4] = {10.01, 20.02, 30.03, 40.04};

    root.FieldVector4i8(TAG_VEC4_I8, vec4_i8);
    root.FieldVector4i16(TAG_VEC4_I16, vec4_i16);
    root.FieldVector4i32(TAG_VEC4_I32, vec4_i32);
    root.FieldVector4i64(TAG_VEC4_I64, vec4_i64);
    root.FieldVector4b(TAG_VEC4_BOOL, vec4_bool);
    root.FieldVector4f32(TAG_VEC4_F32, vec4_f32);
    root.FieldVector4f64(TAG_VEC4_F64, vec4_f64);

    writer.Finish();

    Reader reader(writer.Data(), writer.Size(), true);
    const auto& read_root = reader.RootObject();

    ASSERT_TRUE(read_root.IsValid());

    // Test i8
    int8_t* read_i8 = read_root.ReadVector4i8(TAG_VEC4_I8);
    ASSERT_NE(read_i8, nullptr);
    EXPECT_EQ(read_i8[0], vec4_i8[0]);
    EXPECT_EQ(read_i8[1], vec4_i8[1]);
    EXPECT_EQ(read_i8[2], vec4_i8[2]);
    EXPECT_EQ(read_i8[3], vec4_i8[3]);

    // Test i16
    int16_t* read_i16 = read_root.ReadVector4i16(TAG_VEC4_I16);
    ASSERT_NE(read_i16, nullptr);
    EXPECT_EQ(read_i16[0], vec4_i16[0]);
    EXPECT_EQ(read_i16[1], vec4_i16[1]);
    EXPECT_EQ(read_i16[2], vec4_i16[2]);
    EXPECT_EQ(read_i16[3], vec4_i16[3]);

    // Test i32
    int32_t* read_i32 = read_root.ReadVector4i32(TAG_VEC4_I32);
    ASSERT_NE(read_i32, nullptr);
    EXPECT_EQ(read_i32[0], vec4_i32[0]);
    EXPECT_EQ(read_i32[1], vec4_i32[1]);
    EXPECT_EQ(read_i32[2], vec4_i32[2]);
    EXPECT_EQ(read_i32[3], vec4_i32[3]);

    // Test i64
    int64_t* read_i64 = read_root.ReadVector4i64(TAG_VEC4_I64);
    ASSERT_NE(read_i64, nullptr);
    EXPECT_EQ(read_i64[0], vec4_i64[0]);
    EXPECT_EQ(read_i64[1], vec4_i64[1]);
    EXPECT_EQ(read_i64[2], vec4_i64[2]);
    EXPECT_EQ(read_i64[3], vec4_i64[3]);

    // Test bool
    bool* read_bool = read_root.ReadVector4b(TAG_VEC4_BOOL);
    ASSERT_NE(read_bool, nullptr);
    EXPECT_EQ(read_bool[0], vec4_bool[0]);
    EXPECT_EQ(read_bool[1], vec4_bool[1]);
    EXPECT_EQ(read_bool[2], vec4_bool[2]);
    EXPECT_EQ(read_bool[3], vec4_bool[3]);

    // Test f32
    float* read_f32 = read_root.ReadVector4f32(TAG_VEC4_F32);
    ASSERT_NE(read_f32, nullptr);
    EXPECT_FLOAT_EQ(read_f32[0], vec4_f32[0]);
    EXPECT_FLOAT_EQ(read_f32[1], vec4_f32[1]);
    EXPECT_FLOAT_EQ(read_f32[2], vec4_f32[2]);
    EXPECT_FLOAT_EQ(read_f32[3], vec4_f32[3]);

    // Test f64
    double* read_f64 = read_root.ReadVector4f64(TAG_VEC4_F64);
    ASSERT_NE(read_f64, nullptr);
    EXPECT_DOUBLE_EQ(read_f64[0], vec4_f64[0]);
    EXPECT_DOUBLE_EQ(read_f64[1], vec4_f64[1]);
    EXPECT_DOUBLE_EQ(read_f64[2], vec4_f64[2]);
    EXPECT_DOUBLE_EQ(read_f64[3], vec4_f64[3]);
}

TEST(VectorsTest, UnsignedIntVectorReadWrite) {
    Writer writer(true);
    auto& root = writer.RootObject();

    // Write unsigned vectors
    uint8_t vec2_u8[2] = {0xFF, 0x7F};
    uint16_t vec2_u16[2] = {0xFFFF, 0x7FFF};
    uint32_t vec2_u32[2] = {0xFFFFFFFF, 0x7FFFFFFF};
    uint64_t vec2_u64[2] = {0xFFFFFFFFFFFFFFFFULL, 0x7FFFFFFFFFFFFFFFULL};

    root.FieldVector2i8(TAG_VEC2_I8, vec2_u8);
    root.FieldVector2i16(TAG_VEC2_I16, vec2_u16);
    root.FieldVector2i32(TAG_VEC2_I32, vec2_u32);
    root.FieldVector2i64(TAG_VEC2_I64, vec2_u64);

    writer.Finish();

    Reader reader(writer.Data(), writer.Size(), true);
    const auto& read_root = reader.RootObject();

    ASSERT_TRUE(read_root.IsValid());

    // Read as signed and compare binary representation
    int8_t* read_i8 = read_root.ReadVector2i8(TAG_VEC2_I8);
    ASSERT_NE(read_i8, nullptr);
    EXPECT_EQ(*reinterpret_cast<const uint8_t*>(&read_i8[0]), vec2_u8[0]);
    EXPECT_EQ(*reinterpret_cast<const uint8_t*>(&read_i8[1]), vec2_u8[1]);

    int16_t* read_i16 = read_root.ReadVector2i16(TAG_VEC2_I16);
    ASSERT_NE(read_i16, nullptr);
    EXPECT_EQ(*reinterpret_cast<const uint16_t*>(&read_i16[0]), vec2_u16[0]);
    EXPECT_EQ(*reinterpret_cast<const uint16_t*>(&read_i16[1]), vec2_u16[1]);

    int32_t* read_i32 = read_root.ReadVector2i32(TAG_VEC2_I32);
    ASSERT_NE(read_i32, nullptr);
    EXPECT_EQ(*reinterpret_cast<const uint32_t*>(&read_i32[0]), vec2_u32[0]);
    EXPECT_EQ(*reinterpret_cast<const uint32_t*>(&read_i32[1]), vec2_u32[1]);

    int64_t* read_i64 = read_root.ReadVector2i64(TAG_VEC2_I64);
    ASSERT_NE(read_i64, nullptr);
    EXPECT_EQ(*reinterpret_cast<const uint64_t*>(&read_i64[0]), vec2_u64[0]);
    EXPECT_EQ(*reinterpret_cast<const uint64_t*>(&read_i64[1]), vec2_u64[1]);
}

TEST(VectorsTest, InvalidVectorRead) {
    Writer writer(true);
    auto& root = writer.RootObject();

    int32_t vec2_i32[2] = {100, 200};
    root.FieldVector2i32(TAG_VEC2_I32, vec2_i32);

    writer.Finish();

    Reader reader(writer.Data(), writer.Size(), true);
    const auto& read_root = reader.RootObject();

    ASSERT_TRUE(read_root.IsValid());

    // Try to read with wrong dimension
    int32_t* wrong_dim = read_root.ReadVector3i32(TAG_VEC2_I32);
    EXPECT_EQ(wrong_dim, nullptr);

    // Try to read with wrong type
    float* wrong_type = read_root.ReadVector2f32(TAG_VEC2_I32);
    EXPECT_EQ(wrong_type, nullptr);

    // Try to read non-existent tag
    int32_t* non_existent = read_root.ReadVector2i32("non_existent");
    EXPECT_EQ(non_existent, nullptr);
}