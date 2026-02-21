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

using namespace tbf;

namespace {

constexpr DataTag TAG_INT8 = "int8";
constexpr DataTag TAG_INT16 = "int16";
constexpr DataTag TAG_INT32 = "int32";
constexpr DataTag TAG_INT64 = "int64";
constexpr DataTag TAG_UINT8 = "uint8";
constexpr DataTag TAG_UINT16 = "uint16";
constexpr DataTag TAG_UINT32 = "uint32";
constexpr DataTag TAG_UINT64 = "uint64";
constexpr DataTag TAG_FLOAT = "float";
constexpr DataTag TAG_DOUBLE = "double";
constexpr DataTag TAG_BOOL = "bool";
constexpr DataTag TAG_STRING = "string";

}  // namespace

TEST(BasicTypesTest, Int8ReadWrite) {
    Writer writer(true);
    auto& root = writer.RootObject();
    root.FieldInt8(TAG_INT8, -100);

    writer.Finish();

    Reader reader(writer.Data(), writer.Size(), true);
    const auto& read_root = reader.RootObject();

    ASSERT_TRUE(read_root.IsValid());
    auto value = read_root.ReadInt8(TAG_INT8);
    ASSERT_TRUE(value.has_value());
    EXPECT_EQ(value.value(), -100);
}

TEST(BasicTypesTest, Int16ReadWrite) {
    Writer writer(true);
    auto& root = writer.RootObject();
    root.FieldInt16(TAG_INT16, -12345);

    writer.Finish();

    Reader reader(writer.Data(), writer.Size(), true);
    const auto& read_root = reader.RootObject();

    ASSERT_TRUE(read_root.IsValid());
    auto value = read_root.ReadInt16(TAG_INT16);
    ASSERT_TRUE(value.has_value());
    EXPECT_EQ(value.value(), -12345);
}

TEST(BasicTypesTest, Int32ReadWrite) {
    Writer writer(true);
    auto& root = writer.RootObject();
    root.FieldInt32(TAG_INT32, -123456789);

    writer.Finish();

    Reader reader(writer.Data(), writer.Size(), true);
    const auto& read_root = reader.RootObject();

    ASSERT_TRUE(read_root.IsValid());
    auto value = read_root.ReadInt32(TAG_INT32);
    ASSERT_TRUE(value.has_value());
    EXPECT_EQ(value.value(), -123456789);
}

TEST(BasicTypesTest, Int64ReadWrite) {
    Writer writer(true);
    auto& root = writer.RootObject();
    root.FieldInt64(TAG_INT64, -1234567890123456789LL);

    writer.Finish();

    Reader reader(writer.Data(), writer.Size(), true);
    const auto& read_root = reader.RootObject();

    ASSERT_TRUE(read_root.IsValid());
    auto value = read_root.ReadInt64(TAG_INT64);
    ASSERT_TRUE(value.has_value());
    EXPECT_EQ(value.value(), -1234567890123456789LL);
}

TEST(BasicTypesTest, UInt8ReadWrite) {
    Writer writer(true);
    auto& root = writer.RootObject();
    root.FieldUInt8(TAG_UINT8, 200);

    writer.Finish();

    Reader reader(writer.Data(), writer.Size(), true);
    const auto& read_root = reader.RootObject();

    ASSERT_TRUE(read_root.IsValid());
    auto value = read_root.ReadUInt8(TAG_UINT8);
    ASSERT_TRUE(value.has_value());
    EXPECT_EQ(value.value(), 200);
}

TEST(BasicTypesTest, UInt16ReadWrite) {
    Writer writer(true);
    auto& root = writer.RootObject();
    root.FieldUInt16(TAG_UINT16, 54321);

    writer.Finish();

    Reader reader(writer.Data(), writer.Size(), true);
    const auto& read_root = reader.RootObject();

    ASSERT_TRUE(read_root.IsValid());
    auto value = read_root.ReadUInt16(TAG_UINT16);
    ASSERT_TRUE(value.has_value());
    EXPECT_EQ(value.value(), 54321);
}

TEST(BasicTypesTest, UInt32ReadWrite) {
    Writer writer(true);
    auto& root = writer.RootObject();
    root.FieldUInt32(TAG_UINT32, 987654321);

    writer.Finish();

    Reader reader(writer.Data(), writer.Size(), true);
    const auto& read_root = reader.RootObject();

    ASSERT_TRUE(read_root.IsValid());
    auto value = read_root.ReadUInt32(TAG_UINT32);
    ASSERT_TRUE(value.has_value());
    EXPECT_EQ(value.value(), 987654321);
}

TEST(BasicTypesTest, UInt64ReadWrite) {
    Writer writer(true);
    auto& root = writer.RootObject();
    root.FieldUInt64(TAG_UINT64, 12345678901234567890ULL);

    writer.Finish();

    Reader reader(writer.Data(), writer.Size(), true);
    const auto& read_root = reader.RootObject();

    ASSERT_TRUE(read_root.IsValid());
    auto value = read_root.ReadUInt64(TAG_UINT64);
    ASSERT_TRUE(value.has_value());
    EXPECT_EQ(value.value(), 12345678901234567890ULL);
}

TEST(BasicTypesTest, Float32ReadWrite) {
    Writer writer(true);
    auto& root = writer.RootObject();
    root.FieldFloat32(TAG_FLOAT, 3.14159f);

    writer.Finish();

    Reader reader(writer.Data(), writer.Size(), true);
    const auto& read_root = reader.RootObject();

    ASSERT_TRUE(read_root.IsValid());
    auto value = read_root.ReadFloat32(TAG_FLOAT);
    ASSERT_TRUE(value.has_value());
    EXPECT_NEAR(value.value(), 3.14159f, 0.00001f);
}

TEST(BasicTypesTest, Float64ReadWrite) {
    Writer writer(true);
    auto& root = writer.RootObject();
    root.FieldFloat64(TAG_DOUBLE, 2.718281828459045);

    writer.Finish();

    Reader reader(writer.Data(), writer.Size(), true);
    const auto& read_root = reader.RootObject();

    ASSERT_TRUE(read_root.IsValid());
    auto value = read_root.ReadFloat64(TAG_DOUBLE);
    ASSERT_TRUE(value.has_value());
    EXPECT_NEAR(value.value(), 2.718281828459045, 0.000000001);
}

TEST(BasicTypesTest, BooleanReadWrite) {
    Writer writer(true);
    auto& root = writer.RootObject();
    root.FieldBoolean(TAG_BOOL, true);

    writer.Finish();

    Reader reader(writer.Data(), writer.Size(), true);
    const auto& read_root = reader.RootObject();

    ASSERT_TRUE(read_root.IsValid());
    auto value = read_root.ReadBoolean(TAG_BOOL);
    ASSERT_TRUE(value.has_value());
    EXPECT_TRUE(value.value());
}

TEST(BasicTypesTest, StringReadWrite) {
    Writer writer(true);
    auto& root = writer.RootObject();
    root.FieldString(TAG_STRING, "Hello, TBF!");

    writer.Finish();

    Reader reader(writer.Data(), writer.Size(), true);
    const auto& read_root = reader.RootObject();

    ASSERT_TRUE(read_root.IsValid());
    auto value = read_root.ReadString(TAG_STRING);
    ASSERT_TRUE(value.has_value());
    EXPECT_EQ(value.value(), "Hello, TBF!");
}

TEST(BasicTypesTest, AllTypesReadWrite) {
    Writer writer(true);
    auto& root = writer.RootObject();

    root.FieldInt8(TAG_INT8, -100);
    root.FieldInt16(TAG_INT16, -12345);
    root.FieldInt32(TAG_INT32, -123456789);
    root.FieldInt64(TAG_INT64, -1234567890123456789LL);
    root.FieldUInt8(TAG_UINT8, 200);
    root.FieldUInt16(TAG_UINT16, 54321);
    root.FieldUInt32(TAG_UINT32, 987654321);
    root.FieldUInt64(TAG_UINT64, 12345678901234567890ULL);
    root.FieldFloat32(TAG_FLOAT, 3.14159f);
    root.FieldFloat64(TAG_DOUBLE, 2.718281828459045);
    root.FieldBoolean(TAG_BOOL, true);
    root.FieldString(TAG_STRING, "Hello, TBF!");

    writer.Finish();

    Reader reader(writer.Data(), writer.Size(), true);
    const auto& read_root = reader.RootObject();

    ASSERT_TRUE(read_root.IsValid());

    auto int8_val = read_root.ReadInt8(TAG_INT8);
    ASSERT_TRUE(int8_val.has_value());
    EXPECT_EQ(int8_val.value(), -100);

    auto int16_val = read_root.ReadInt16(TAG_INT16);
    ASSERT_TRUE(int16_val.has_value());
    EXPECT_EQ(int16_val.value(), -12345);

    auto int32_val = read_root.ReadInt32(TAG_INT32);
    ASSERT_TRUE(int32_val.has_value());
    EXPECT_EQ(int32_val.value(), -123456789);

    auto int64_val = read_root.ReadInt64(TAG_INT64);
    ASSERT_TRUE(int64_val.has_value());
    EXPECT_EQ(int64_val.value(), -1234567890123456789LL);

    auto uint8_val = read_root.ReadUInt8(TAG_UINT8);
    ASSERT_TRUE(uint8_val.has_value());
    EXPECT_EQ(uint8_val.value(), 200);

    auto uint16_val = read_root.ReadUInt16(TAG_UINT16);
    ASSERT_TRUE(uint16_val.has_value());
    EXPECT_EQ(uint16_val.value(), 54321);

    auto uint32_val = read_root.ReadUInt32(TAG_UINT32);
    ASSERT_TRUE(uint32_val.has_value());
    EXPECT_EQ(uint32_val.value(), 987654321);

    auto uint64_val = read_root.ReadUInt64(TAG_UINT64);
    ASSERT_TRUE(uint64_val.has_value());
    EXPECT_EQ(uint64_val.value(), 12345678901234567890ULL);

    auto float_val = read_root.ReadFloat32(TAG_FLOAT);
    ASSERT_TRUE(float_val.has_value());
    EXPECT_NEAR(float_val.value(), 3.14159f, 0.00001f);

    auto double_val = read_root.ReadFloat64(TAG_DOUBLE);
    ASSERT_TRUE(double_val.has_value());
    EXPECT_NEAR(double_val.value(), 2.718281828459045, 0.000000001);

    auto bool_val = read_root.ReadBoolean(TAG_BOOL);
    ASSERT_TRUE(bool_val.has_value());
    EXPECT_TRUE(bool_val.value());

    auto string_val = read_root.ReadString(TAG_STRING);
    ASSERT_TRUE(string_val.has_value());
    EXPECT_EQ(string_val.value(), "Hello, TBF!");
}
