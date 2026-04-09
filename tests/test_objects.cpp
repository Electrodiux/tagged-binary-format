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

#include "tbf/Reader.hpp"
#include "tbf/Writer.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <string_view>

using namespace tbf;

namespace {

constexpr DataTag TAG_INNER_OBJECT = "inner";
constexpr DataTag TAG_OUTER_VALUE = "outer_value";
constexpr DataTag TAG_INNER_VALUE = "inner_value";
constexpr DataTag TAG_OBJECT_ARRAY = "object_array";
constexpr DataTag TAG_NAME = "name";
constexpr DataTag TAG_ID = "id";

}  // namespace

TEST(ObjectsTest, SimpleObjectReadWrite) {
    Writer writer(true);
    auto& root = writer.RootObject();

    root.Field<int32_t>(TAG_OUTER_VALUE, 42);

    {
        auto inner = root.FieldObject(TAG_INNER_OBJECT);
        inner.Field<int32_t>(TAG_INNER_VALUE, 100);
        inner.Finish();
    }

    writer.Finish();

    Reader reader(writer.Data(), writer.Size(), true);
    const auto& read_root = reader.RootObject();

    ASSERT_TRUE(read_root.IsValid());

    auto outer_val = read_root.Read<int32_t>(TAG_OUTER_VALUE);
    ASSERT_TRUE(outer_val.has_value());
    EXPECT_EQ(outer_val.value(), 42);

    auto inner_obj = read_root.ReadObject(TAG_INNER_OBJECT);
    ASSERT_TRUE(inner_obj.has_value());
    ASSERT_TRUE(inner_obj->IsValid());

    auto inner_val = inner_obj->Read<int32_t>(TAG_INNER_VALUE);
    ASSERT_TRUE(inner_val.has_value());
    EXPECT_EQ(inner_val.value(), 100);
}

TEST(ObjectsTest, NestedObjectReadWrite) {
    Writer writer(true);
    auto& root = writer.RootObject();

    root.Field(TAG_NAME, "root");

    {
        auto level1 = root.FieldObject("level1");
        level1.Field(TAG_NAME, "level1_object");

        {
            auto level2 = level1.FieldObject("level2");
            level2.Field(TAG_NAME, "level2_object");
            level2.Finish();
        }

        level1.Finish();
    }

    writer.Finish();

    Reader reader(writer.Data(), writer.Size(), true);
    const auto& read_root = reader.RootObject();

    ASSERT_TRUE(read_root.IsValid());

    auto root_name = read_root.ReadString(TAG_NAME);
    ASSERT_TRUE(root_name.has_value());
    EXPECT_EQ(root_name.value(), "root");

    auto level1_obj = read_root.ReadObject("level1");
    ASSERT_TRUE(level1_obj.has_value());
    ASSERT_TRUE(level1_obj->IsValid());

    auto level1_name = level1_obj->ReadString(TAG_NAME);
    ASSERT_TRUE(level1_name.has_value());
    EXPECT_EQ(level1_name.value(), "level1_object");

    auto level2_obj = level1_obj->ReadObject("level2");
    ASSERT_TRUE(level2_obj.has_value());
    ASSERT_TRUE(level2_obj->IsValid());

    auto level2_name = level2_obj->ReadString(TAG_NAME);
    ASSERT_TRUE(level2_name.has_value());
    EXPECT_EQ(level2_name.value(), "level2_object");
}

TEST(ObjectsTest, ObjectArrayReadWrite) {
    Writer writer(true);
    auto& root = writer.RootObject();

    auto obj_array = root.FieldObjectArray(TAG_OBJECT_ARRAY);

    for (int i = 0; i < 3; i++) {
        auto obj = obj_array.CreateElement();
        obj.Field<int32_t>(TAG_ID, i);
        obj.Finish();
    }

    obj_array.Finish();
    writer.Finish();

    Reader reader(writer.Data(), writer.Size(), true);
    const auto& read_root = reader.RootObject();

    ASSERT_TRUE(read_root.IsValid());

    auto read_array = read_root.ReadObjectArray(TAG_OBJECT_ARRAY);
    ASSERT_TRUE(read_array.has_value());
    ASSERT_EQ(read_array->Size(), 3);

    int index = 0;
    for (const auto& obj : *read_array) {
        ASSERT_TRUE(obj.IsValid());

        auto id = obj.Read<int32_t>(TAG_ID);
        ASSERT_TRUE(id.has_value());
        EXPECT_EQ(id.value(), index);

        index++;
    }
    EXPECT_EQ(index, 3);
}

TEST(ObjectsTest, ObjectWithMultipleFields) {
    Writer writer(true);
    auto& root = writer.RootObject();

    root.Field<int32_t>(TAG_ID, 12345);
    root.Field(TAG_NAME, "Test Object");
    root.Field<float>("score", 98.6f);
    root.Field<bool>("active", true);

    writer.Finish();

    Reader reader(writer.Data(), writer.Size(), true);
    const auto& read_root = reader.RootObject();

    ASSERT_TRUE(read_root.IsValid());

    auto id = read_root.Read<int32_t>(TAG_ID);
    ASSERT_TRUE(id.has_value());
    EXPECT_EQ(id.value(), 12345);

    auto name = read_root.ReadString(TAG_NAME);
    ASSERT_TRUE(name.has_value());
    EXPECT_EQ(name.value(), "Test Object");

    auto score = read_root.Read<float>("score");
    ASSERT_TRUE(score.has_value());
    EXPECT_NEAR(score.value(), 98.6f, 0.0001f);

    auto active = read_root.Read<bool>("active");
    ASSERT_TRUE(active.has_value());
    EXPECT_TRUE(active.value());
}

TEST(ObjectsTest, NonExistentObject) {
    Writer writer(true);
    auto& root = writer.RootObject();

    root.Field<int64_t>(TAG_OUTER_VALUE, 999);

    writer.Finish();

    Reader reader(writer.Data(), writer.Size(), true);
    const auto& read_root = reader.RootObject();

    ASSERT_TRUE(read_root.IsValid());

    auto non_existent = read_root.ReadObject(TAG_INNER_OBJECT);
    EXPECT_FALSE(non_existent.has_value());
}

TEST(ObjectsTest, EmptyObject) {
    Writer writer(true);
    auto& root = writer.RootObject();

    {
        auto empty_obj = root.FieldObject(TAG_INNER_OBJECT);
        // Object with no fields
        empty_obj.Finish();
    }

    writer.Finish();

    Reader reader(writer.Data(), writer.Size(), true);
    const auto& read_root = reader.RootObject();

    ASSERT_TRUE(read_root.IsValid());

    auto empty_read = read_root.ReadObject(TAG_INNER_OBJECT);
    ASSERT_TRUE(empty_read.has_value());
    ASSERT_TRUE(empty_read->IsValid());

    // Empty object should have no tags
    auto all_tags = empty_read->GetAllTags();
    EXPECT_TRUE(all_tags.empty());
}
