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
#include <vector>

using namespace tbf;

namespace {

constexpr DataTag TAG_INT_ARRAY = "int_array";
constexpr DataTag TAG_STRING_ARRAY = "string_array";
constexpr DataTag TAG_FLOAT_ARRAY = "float_array";
constexpr DataTag TAG_BINARY_ARRAY = "binary_array";

}  // namespace

TEST(ArraysTest, Int32ArrayReadWrite) {
    Writer writer(true);
    auto& root = writer.RootObject();

    int32_t int_data[] = {10, 20, 30, 40, 50};
    root.FieldArrayInt32(TAG_INT_ARRAY, int_data, 5);

    writer.Finish();

    Reader reader(writer.Data(), writer.Size(), true);
    const auto& read_root = reader.RootObject();

    ASSERT_TRUE(read_root.IsValid());

    auto int_array = read_root.ReadInt32Array(TAG_INT_ARRAY);
    ASSERT_FALSE(int_array.empty());
    ASSERT_EQ(int_array.size(), 5);

    std::vector<int32_t> expected = {10, 20, 30, 40, 50};
    for (size_t i = 0; i < int_array.size(); i++) {
        EXPECT_EQ(int_array[i], expected[i]);
    }
}

TEST(ArraysTest, StringArrayReadWrite) {
    Writer writer(true);
    auto& root = writer.RootObject();

    auto string_array = root.FieldStringArray(TAG_STRING_ARRAY);
    string_array.AddElement("first");
    string_array.AddElement("second");
    string_array.AddElement("third");
    string_array.Finish();

    writer.Finish();

    Reader reader(writer.Data(), writer.Size(), true);
    const auto& read_root = reader.RootObject();

    ASSERT_TRUE(read_root.IsValid());

    auto str_array = read_root.ReadStringArray(TAG_STRING_ARRAY);
    ASSERT_TRUE(str_array.has_value());

    std::vector<std::string_view> expected = {"first", "second", "third"};
    size_t index = 0;
    for (const auto& str : *str_array) {
        ASSERT_LT(index, expected.size());
        EXPECT_EQ(str, expected[index]);
        index++;
    }
    EXPECT_EQ(index, expected.size());
}

TEST(ArraysTest, Float32ArrayReadWrite) {
    Writer writer(true);
    auto& root = writer.RootObject();

    float float_data[] = {1.1f, 2.2f, 3.3f};
    root.FieldArrayFloat32(TAG_FLOAT_ARRAY, float_data, 3);

    writer.Finish();

    Reader reader(writer.Data(), writer.Size(), true);
    const auto& read_root = reader.RootObject();

    ASSERT_TRUE(read_root.IsValid());

    auto float_array = read_root.ReadFloat32Array(TAG_FLOAT_ARRAY);
    ASSERT_FALSE(float_array.empty());
    ASSERT_EQ(float_array.size(), 3);

    std::vector<float> expected = {1.1f, 2.2f, 3.3f};
    for (size_t i = 0; i < float_array.size(); i++) {
        EXPECT_NEAR(float_array[i], expected[i], 0.0001f);
    }
}

TEST(ArraysTest, BinaryArrayReadWrite) {
    Writer writer(true);
    auto& root = writer.RootObject();

    auto binary_array = root.FieldBinaryArray(TAG_BINARY_ARRAY);

    uint8_t data1[] = {0x01, 0x02, 0x03};
    binary_array.AddElement(data1, 3);

    uint8_t data2[] = {0xAA, 0xBB, 0xCC, 0xDD};
    binary_array.AddElement(data2, 4);

    uint8_t data3[] = {0xFF};
    binary_array.AddElement(data3, 1);

    binary_array.Finish();

    writer.Finish();

    Reader reader(writer.Data(), writer.Size(), true);
    const auto& read_root = reader.RootObject();

    ASSERT_TRUE(read_root.IsValid());

    auto bin_array = read_root.ReadBinaryArray(TAG_BINARY_ARRAY);
    ASSERT_TRUE(bin_array.has_value());
    ASSERT_EQ(bin_array->Size(), 3);

    std::vector<std::vector<uint8_t>> expected = {
        {0x01, 0x02, 0x03},
        {0xAA, 0xBB, 0xCC, 0xDD},
        {0xFF}
    };

    size_t index = 0;
    for (const auto& bin : *bin_array) {
        ASSERT_LT(index, expected.size());
        ASSERT_EQ(bin.size(), expected[index].size());
        for (size_t i = 0; i < bin.size(); i++) {
            EXPECT_EQ(bin[i], expected[index][i]);
        }
        index++;
    }
    EXPECT_EQ(index, expected.size());
}

TEST(ArraysTest, PointerAPIReadWrite) {
    Writer writer(true);
    auto& root = writer.RootObject();

    int32_t int_data[] = {100, 200, 300};
    root.FieldArrayInt32(TAG_INT_ARRAY, int_data, 3);

    float float_data[] = {1.5f, 2.5f};
    root.FieldArrayFloat32(TAG_FLOAT_ARRAY, float_data, 2);

    writer.Finish();

    Reader reader(writer.Data(), writer.Size(), true);
    const auto& read_root = reader.RootObject();

    ASSERT_TRUE(read_root.IsValid());

    // Test int32 array with pointer API
    uint32_t int_length;
    const int32_t* int_array_ptr = read_root.ReadInt32Array(TAG_INT_ARRAY, int_length);
    ASSERT_NE(int_array_ptr, nullptr);
    ASSERT_EQ(int_length, 3);
    EXPECT_EQ(int_array_ptr[0], 100);
    EXPECT_EQ(int_array_ptr[1], 200);
    EXPECT_EQ(int_array_ptr[2], 300);

    // Test float32 array with pointer API
    uint32_t float_length;
    const float* float_array_ptr = read_root.ReadFloat32Array(TAG_FLOAT_ARRAY, float_length);
    ASSERT_NE(float_array_ptr, nullptr);
    ASSERT_EQ(float_length, 2);
    EXPECT_NEAR(float_array_ptr[0], 1.5f, 0.0001f);
    EXPECT_NEAR(float_array_ptr[1], 2.5f, 0.0001f);

    // Test non-existent array
    uint32_t non_existent_length;
    const int32_t* non_existent_ptr = read_root.ReadInt32Array("non_existent_array", non_existent_length);
    EXPECT_EQ(non_existent_ptr, nullptr);
    EXPECT_EQ(non_existent_length, 0);
}

TEST(ArraysTest, EmptyArrays) {
    Writer writer(true);
    auto& root = writer.RootObject();

    // Empty int array
    int32_t empty_data[] = {0};  // Dummy data, size is set to 0
    root.FieldArrayInt32(TAG_INT_ARRAY, empty_data, 0);

    // Empty string array
    auto string_array = root.FieldStringArray(TAG_STRING_ARRAY);
    string_array.Finish();

    writer.Finish();

    Reader reader(writer.Data(), writer.Size(), true);
    const auto& read_root = reader.RootObject();

    ASSERT_TRUE(read_root.IsValid());

    auto int_array = read_root.ReadInt32Array(TAG_INT_ARRAY);
    EXPECT_TRUE(int_array.empty());

    auto str_array = read_root.ReadStringArray(TAG_STRING_ARRAY);
    ASSERT_TRUE(str_array.has_value());
    size_t count = 0;
    for ([[maybe_unused]] const auto& item : *str_array) {
        count++;
    }
    EXPECT_EQ(count, 0);
}

TEST(ArraysTest, NonExistentArray) {
    Writer writer(true);
    auto& root = writer.RootObject();

    root.FieldInt64("dummy_data", 100);

    writer.Finish();

    Reader reader(writer.Data(), writer.Size(), true);
    const auto& read_root = reader.RootObject();

    ASSERT_TRUE(read_root.IsValid());

    auto int_array = read_root.ReadInt32Array(TAG_INT_ARRAY);
    EXPECT_TRUE(int_array.empty());

    uint32_t length;
    const int32_t* int_array_ptr = read_root.ReadInt32Array(TAG_INT_ARRAY, length);
    EXPECT_EQ(int_array_ptr, nullptr);
    EXPECT_EQ(length, 0);

    auto str_array = read_root.ReadStringArray(TAG_STRING_ARRAY);
    EXPECT_FALSE(str_array.has_value());
}
