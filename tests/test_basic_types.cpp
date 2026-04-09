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

constexpr DataTag TAG_INT8 = "int8";
constexpr DataTag TAG_INT16 = "int16";
constexpr DataTag TAG_INT32 = "int32";
constexpr DataTag TAG_INT64 = "int64";
constexpr DataTag TAG_FLOAT = "float";
constexpr DataTag TAG_DOUBLE = "double";
constexpr DataTag TAG_BOOL = "bool";
constexpr DataTag TAG_STRING = "string";
constexpr DataTag TAG_BINARY = "binary";
constexpr DataTag TAG_UUID = "uuid";

}  // namespace

TEST(BasicTypesTest, Int8ReadWrite) {
    Writer writer(true);
    auto& root = writer.RootObject();
    root.Field<int8_t>(TAG_INT8, -100);

    writer.Finish();

    Reader reader(writer.Data(), writer.Size(), true);
    const auto& read_root = reader.RootObject();

    ASSERT_TRUE(read_root.IsValid());
    auto value = read_root.Read<int8_t>(TAG_INT8);
    ASSERT_TRUE(value.has_value());
    EXPECT_EQ(value.value(), -100);
}

TEST(BasicTypesTest, Int16ReadWrite) {
    Writer writer(true);
    auto& root = writer.RootObject();
    root.Field<int16_t>(TAG_INT16, -12345);

    writer.Finish();

    Reader reader(writer.Data(), writer.Size(), true);
    const auto& read_root = reader.RootObject();

    ASSERT_TRUE(read_root.IsValid());
    auto value = read_root.Read<int16_t>(TAG_INT16);
    ASSERT_TRUE(value.has_value());
    EXPECT_EQ(value.value(), -12345);
}

TEST(BasicTypesTest, Int32ReadWrite) {
    Writer writer(true);
    auto& root = writer.RootObject();
    root.Field<int32_t>(TAG_INT32, -123456789);

    writer.Finish();

    Reader reader(writer.Data(), writer.Size(), true);
    const auto& read_root = reader.RootObject();

    ASSERT_TRUE(read_root.IsValid());
    auto value = read_root.Read<int32_t>(TAG_INT32);
    ASSERT_TRUE(value.has_value());
    EXPECT_EQ(value.value(), -123456789);
}

TEST(BasicTypesTest, Int64ReadWrite) {
    Writer writer(true);
    auto& root = writer.RootObject();
    root.Field<int64_t>(TAG_INT64, -1234567890123456789LL);

    writer.Finish();

    Reader reader(writer.Data(), writer.Size(), true);
    const auto& read_root = reader.RootObject();

    ASSERT_TRUE(read_root.IsValid());
    auto value = read_root.Read<int64_t>(TAG_INT64);
    ASSERT_TRUE(value.has_value());
    EXPECT_EQ(value.value(), -1234567890123456789LL);
}

TEST(BasicTypesTest, Float32ReadWrite) {
    Writer writer(true);
    auto& root = writer.RootObject();
    root.Field<float>(TAG_FLOAT, 3.14159f);

    writer.Finish();

    Reader reader(writer.Data(), writer.Size(), true);
    const auto& read_root = reader.RootObject();

    ASSERT_TRUE(read_root.IsValid());
    auto value = read_root.Read<float>(TAG_FLOAT);
    ASSERT_TRUE(value.has_value());
    EXPECT_NEAR(value.value(), 3.14159f, 0.00001f);
}

TEST(BasicTypesTest, Float64ReadWrite) {
    Writer writer(true);
    auto& root = writer.RootObject();
    root.Field<double>(TAG_DOUBLE, 2.718281828459045);

    writer.Finish();

    Reader reader(writer.Data(), writer.Size(), true);
    const auto& read_root = reader.RootObject();

    ASSERT_TRUE(read_root.IsValid());
    auto value = read_root.Read<double>(TAG_DOUBLE);
    ASSERT_TRUE(value.has_value());
    EXPECT_NEAR(value.value(), 2.718281828459045, 0.000000001);
}

TEST(BasicTypesTest, BooleanReadWrite) {
    Writer writer(true);
    auto& root = writer.RootObject();
    root.Field<bool>(TAG_BOOL, true);

    writer.Finish();

    Reader reader(writer.Data(), writer.Size(), true);
    const auto& read_root = reader.RootObject();

    ASSERT_TRUE(read_root.IsValid());
    auto value = read_root.Read<bool>(TAG_BOOL);
    ASSERT_TRUE(value.has_value());
    EXPECT_TRUE(value.value());
}

TEST(BasicTypesTest, StringReadWrite) {
    Writer writer(true);
    auto& root = writer.RootObject();
    root.Field(TAG_STRING, "Hello, TBF!");

    writer.Finish();

    Reader reader(writer.Data(), writer.Size(), true);
    const auto& read_root = reader.RootObject();

    ASSERT_TRUE(read_root.IsValid());
    auto value = read_root.ReadString(TAG_STRING);
    ASSERT_TRUE(value.has_value());
    EXPECT_EQ(value.value(), "Hello, TBF!");
}

TEST(BasicTypesTest, BinaryReadWrite) {
    Writer writer(true);
    auto& root = writer.RootObject();

    uint8_t binary_data[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE, 0xBA, 0xBE};
    root.Field(TAG_BINARY, std::span<const uint8_t>(binary_data, sizeof(binary_data)));

    writer.Finish();

    Reader reader(writer.Data(), writer.Size(), true);
    const auto& read_root = reader.RootObject();

    ASSERT_TRUE(read_root.IsValid());

    auto binary_span = read_root.ReadBinary(TAG_BINARY);
    ASSERT_TRUE(binary_span.data() != nullptr);
    ASSERT_EQ(binary_span.size(), sizeof(binary_data));

    for (size_t i = 0; i < sizeof(binary_data); i++) {
        EXPECT_EQ(binary_span[i], binary_data[i]);
    }
}

TEST(BasicTypesTest, UUIDReadWrite) {
    Writer writer(true);
    auto& root = writer.RootObject();

    // UUID v3 for www.electrodiux.com
    // 2b0978d7-0dc9-37ea-94e8-ddd399ffc6c2
    uint8_t uuid_data[] = {0x2b, 0x09, 0x78, 0xd7, 0x0d, 0xc9, 0x37, 0xea, 0x94, 0xe8, 0xdd, 0xd3, 0x99, 0xff, 0xc6, 0xc2};
    root.FieldUUID(TAG_UUID, uuid_data);

    writer.Finish();

    Reader reader(writer.Data(), writer.Size(), true);
    const auto& read_root = reader.RootObject();

    ASSERT_TRUE(read_root.IsValid());

    const uint8_t* read_uuid = static_cast<const uint8_t*>(read_root.ReadUUID(TAG_UUID));
    ASSERT_TRUE(read_uuid != nullptr);

    for (size_t i = 0; i < 16; i++) {
        EXPECT_EQ(read_uuid[i], uuid_data[i]);
    }
}

TEST(BasicTypesTest, AllTypesReadWrite) {
    Writer writer(true);
    auto& root = writer.RootObject();

    root.Field<int8_t>(TAG_INT8, -100);
    root.Field<int16_t>(TAG_INT16, -12345);
    root.Field<int32_t>(TAG_INT32, -123456789);
    root.Field<int64_t>(TAG_INT64, -1234567890123456789LL);
    root.Field<float>(TAG_FLOAT, 3.14159f);
    root.Field<double>(TAG_DOUBLE, 2.718281828459045);
    root.Field<bool>(TAG_BOOL, true);
    root.Field(TAG_STRING, "Hello, TBF!");

    writer.Finish();

    Reader reader(writer.Data(), writer.Size(), true);
    const auto& read_root = reader.RootObject();

    ASSERT_TRUE(read_root.IsValid());

    auto int8_val = read_root.Read<int8_t>(TAG_INT8);
    ASSERT_TRUE(int8_val.has_value());
    EXPECT_EQ(int8_val.value(), -100);

    auto int16_val = read_root.Read<int16_t>(TAG_INT16);
    ASSERT_TRUE(int16_val.has_value());
    EXPECT_EQ(int16_val.value(), -12345);

    auto int32_val = read_root.Read<int32_t>(TAG_INT32);
    ASSERT_TRUE(int32_val.has_value());
    EXPECT_EQ(int32_val.value(), -123456789);

    auto int64_val = read_root.Read<int64_t>(TAG_INT64);
    ASSERT_TRUE(int64_val.has_value());
    EXPECT_EQ(int64_val.value(), -1234567890123456789LL);

    auto float_val = read_root.Read<float>(TAG_FLOAT);
    ASSERT_TRUE(float_val.has_value());
    EXPECT_NEAR(float_val.value(), 3.14159f, 0.00001f);

    auto double_val = read_root.Read<double>(TAG_DOUBLE);
    ASSERT_TRUE(double_val.has_value());
    EXPECT_NEAR(double_val.value(), 2.718281828459045, 0.000000001);

    auto bool_val = read_root.Read<bool>(TAG_BOOL);
    ASSERT_TRUE(bool_val.has_value());
    EXPECT_TRUE(bool_val.value());

    auto string_val = read_root.ReadString(TAG_STRING);
    ASSERT_TRUE(string_val.has_value());
    EXPECT_EQ(string_val.value(), "Hello, TBF!");
}

TEST(BasicTypesTest, OutParamRead) {
    Writer writer(true);
    auto& root = writer.RootObject();
    root.Field<int32_t>(TAG_INT32, 42);

    writer.Finish();

    Reader reader(writer.Data(), writer.Size(), true);
    const auto& read_root = reader.RootObject();

    ASSERT_TRUE(read_root.IsValid());

    int32_t value = 0;
    bool success = read_root.Read<int32_t>(TAG_INT32, value);
    ASSERT_TRUE(success);
    EXPECT_EQ(value, 42);

    // Test non-existent tag
    int32_t dummy = 0;
    bool fail = read_root.Read<int32_t>("nonexistent", dummy);
    EXPECT_FALSE(fail);
}
