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

#include <array>
#include <cstdint>

using namespace tbf;

namespace {

constexpr DataTag TAG_VEC2 = "vec2";
constexpr DataTag TAG_VEC3 = "vec3";
constexpr DataTag TAG_VEC4 = "vec4";

}  // namespace

TEST(VectorsTest, Vec2f32ReadWrite) {
    Writer writer(true);
    auto& root = writer.RootObject();

    std::array<float, 2> vec = {1.0f, 2.0f};
    root.FieldVector<float, 2>(TAG_VEC2, vec);

    writer.Finish();

    Reader reader(writer.Data(), writer.Size(), true);
    const auto& read_root = reader.RootObject();

    ASSERT_TRUE(read_root.IsValid());

    auto read_vec = read_root.ReadVector<float, 2>(TAG_VEC2);
    ASSERT_TRUE(read_vec.has_value());
    EXPECT_FLOAT_EQ((*read_vec)[0], 1.0f);
    EXPECT_FLOAT_EQ((*read_vec)[1], 2.0f);
}

TEST(VectorsTest, Vec3f64ReadWrite) {
    Writer writer(true);
    auto& root = writer.RootObject();

    std::array<double, 3> vec = {1.0, 2.0, 3.0};
    root.FieldVector<double, 3>(TAG_VEC3, vec);

    writer.Finish();

    Reader reader(writer.Data(), writer.Size(), true);
    const auto& read_root = reader.RootObject();

    ASSERT_TRUE(read_root.IsValid());

    auto read_vec = read_root.ReadVector<double, 3>(TAG_VEC3);
    ASSERT_TRUE(read_vec.has_value());
    EXPECT_DOUBLE_EQ((*read_vec)[0], 1.0);
    EXPECT_DOUBLE_EQ((*read_vec)[1], 2.0);
    EXPECT_DOUBLE_EQ((*read_vec)[2], 3.0);
}

TEST(VectorsTest, Vec4i32ReadWrite) {
    Writer writer(true);
    auto& root = writer.RootObject();

    std::array<int32_t, 4> vec = {10, 20, 30, 40};
    root.FieldVector<int32_t, 4>(TAG_VEC4, vec);

    writer.Finish();

    Reader reader(writer.Data(), writer.Size(), true);
    const auto& read_root = reader.RootObject();

    ASSERT_TRUE(read_root.IsValid());

    auto read_vec = read_root.ReadVector<int32_t, 4>(TAG_VEC4);
    ASSERT_TRUE(read_vec.has_value());
    EXPECT_EQ((*read_vec)[0], 10);
    EXPECT_EQ((*read_vec)[1], 20);
    EXPECT_EQ((*read_vec)[2], 30);
    EXPECT_EQ((*read_vec)[3], 40);
}

TEST(VectorsTest, Vec2BoolReadWrite) {
    Writer writer(true);
    auto& root = writer.RootObject();

    std::array<bool, 2> vec = {true, false};
    root.FieldVector<bool, 2>(TAG_VEC2, vec);

    writer.Finish();

    Reader reader(writer.Data(), writer.Size(), true);
    const auto& read_root = reader.RootObject();

    ASSERT_TRUE(read_root.IsValid());

    auto read_vec = read_root.ReadVector<bool, 2>(TAG_VEC2);
    ASSERT_TRUE(read_vec.has_value());
    EXPECT_EQ((*read_vec)[0], true);
    EXPECT_EQ((*read_vec)[1], false);
}

TEST(VectorsTest, PointerAPIReadWrite) {
    Writer writer(true);
    auto& root = writer.RootObject();

    float vec2_data[] = {5.5f, 10.5f};
    root.FieldVector<float, 2>(TAG_VEC2, vec2_data);

    int32_t vec3_data[] = {100, 200, 300};
    root.FieldVector<int32_t, 3>(TAG_VEC3, vec3_data);

    writer.Finish();

    Reader reader(writer.Data(), writer.Size(), true);
    const auto& read_root = reader.RootObject();

    ASSERT_TRUE(read_root.IsValid());

    auto read_vec2 = read_root.ReadVector<float, 2>(TAG_VEC2);
    ASSERT_TRUE(read_vec2.has_value());
    EXPECT_FLOAT_EQ((*read_vec2)[0], 5.5f);
    EXPECT_FLOAT_EQ((*read_vec2)[1], 10.5f);

    auto read_vec3 = read_root.ReadVector<int32_t, 3>(TAG_VEC3);
    ASSERT_TRUE(read_vec3.has_value());
    EXPECT_EQ((*read_vec3)[0], 100);
    EXPECT_EQ((*read_vec3)[1], 200);
    EXPECT_EQ((*read_vec3)[2], 300);
}

TEST(VectorsTest, MultipleVectorTypesReadWrite) {
    Writer writer(true);
    auto& root = writer.RootObject();

    constexpr DataTag TAG_VEC2I = "vec2i";
    constexpr DataTag TAG_VEC3I = "vec3i";
    constexpr DataTag TAG_VEC4I = "vec4i";
    constexpr DataTag TAG_VEC2F = "vec2f";
    constexpr DataTag TAG_VEC3F = "vec3f";

    std::array<int32_t, 2> vec2i = {1, 2};
    std::array<int32_t, 3> vec3i = {10, 20, 30};
    std::array<int32_t, 4> vec4i = {100, 200, 300, 400};

    std::array<float, 2> vec2f = {1.1f, 2.2f};
    std::array<float, 3> vec3f = {1.11f, 2.22f, 3.33f};

    root.FieldVector<int32_t, 2>(TAG_VEC2I, vec2i);
    root.FieldVector<int32_t, 3>(TAG_VEC3I, vec3i);
    root.FieldVector<int32_t, 4>(TAG_VEC4I, vec4i);
    root.FieldVector<float, 2>(TAG_VEC2F, vec2f);
    root.FieldVector<float, 3>(TAG_VEC3F, vec3f);

    writer.Finish();

    Reader reader(writer.Data(), writer.Size(), true);
    const auto& read_root = reader.RootObject();

    ASSERT_TRUE(read_root.IsValid());

    auto read_vec2i = read_root.ReadVector<int32_t, 2>(TAG_VEC2I);
    ASSERT_TRUE(read_vec2i.has_value());
    EXPECT_EQ((*read_vec2i)[0], 1);
    EXPECT_EQ((*read_vec2i)[1], 2);

    auto read_vec3i = read_root.ReadVector<int32_t, 3>(TAG_VEC3I);
    ASSERT_TRUE(read_vec3i.has_value());
    EXPECT_EQ((*read_vec3i)[0], 10);
    EXPECT_EQ((*read_vec3i)[1], 20);
    EXPECT_EQ((*read_vec3i)[2], 30);

    auto read_vec4i = read_root.ReadVector<int32_t, 4>(TAG_VEC4I);
    ASSERT_TRUE(read_vec4i.has_value());
    EXPECT_EQ((*read_vec4i)[0], 100);
    EXPECT_EQ((*read_vec4i)[1], 200);
    EXPECT_EQ((*read_vec4i)[2], 300);
    EXPECT_EQ((*read_vec4i)[3], 400);

    auto read_vec2f = read_root.ReadVector<float, 2>(TAG_VEC2F);
    ASSERT_TRUE(read_vec2f.has_value());
    EXPECT_NEAR((*read_vec2f)[0], 1.1f, 0.0001f);
    EXPECT_NEAR((*read_vec2f)[1], 2.2f, 0.0001f);

    auto read_vec3f = read_root.ReadVector<float, 3>(TAG_VEC3F);
    ASSERT_TRUE(read_vec3f.has_value());
    EXPECT_NEAR((*read_vec3f)[0], 1.11f, 0.0001f);
    EXPECT_NEAR((*read_vec3f)[1], 2.22f, 0.0001f);
    EXPECT_NEAR((*read_vec3f)[2], 3.33f, 0.0001f);
}

TEST(VectorsTest, NonExistentVector) {
    Writer writer(true);
    auto& root = writer.RootObject();

    root.Field<int64_t>("dummy_data", 100);

    writer.Finish();

    Reader reader(writer.Data(), writer.Size(), true);
    const auto& read_root = reader.RootObject();

    ASSERT_TRUE(read_root.IsValid());

    auto vec = read_root.ReadVector<float, 2>(TAG_VEC2);
    EXPECT_FALSE(vec.has_value());
}