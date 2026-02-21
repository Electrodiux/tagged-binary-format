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

#include <vector>

using namespace tbf;

namespace {

constexpr DataTag TAG_USER = "user";
constexpr DataTag TAG_ID = "id";
constexpr DataTag TAG_NAME = "name";
constexpr DataTag TAG_SETTINGS = "settings";
constexpr DataTag TAG_THEME = "theme";
constexpr DataTag TAG_NOTIFICATIONS = "notifications";
constexpr DataTag TAG_USERS_ARRAY = "users";

}  // namespace

TEST(ObjectsTest, SimpleObjectReadWrite) {
    Writer writer(true);
    auto& root = writer.RootObject();

    auto user = root.FieldObject(TAG_USER);
    user.FieldInt32(TAG_ID, 12345);
    user.FieldString(TAG_NAME, "John Doe");
    user.Finish();

    writer.Finish();

    Reader reader(writer.Data(), writer.Size(), true);
    const auto& read_root = reader.RootObject();

    ASSERT_TRUE(read_root.IsValid());

    auto user_obj = read_root.ReadObject(TAG_USER);
    ASSERT_TRUE(user_obj.has_value());

    auto id = user_obj->ReadInt32(TAG_ID);
    ASSERT_TRUE(id.has_value());
    EXPECT_EQ(id.value(), 12345);

    auto name = user_obj->ReadString(TAG_NAME);
    ASSERT_TRUE(name.has_value());
    EXPECT_EQ(name.value(), "John Doe");
}

TEST(ObjectsTest, NestedObjectReadWrite) {
    Writer writer(true);
    auto& root = writer.RootObject();

    auto user = root.FieldObject(TAG_USER);
    user.FieldInt32(TAG_ID, 12345);
    user.FieldString(TAG_NAME, "John Doe");

    auto settings = user.FieldObject(TAG_SETTINGS);
    settings.FieldString(TAG_THEME, "dark");
    settings.FieldBoolean(TAG_NOTIFICATIONS, true);
    settings.Finish();

    user.Finish();

    writer.Finish();

    Reader reader(writer.Data(), writer.Size(), true);
    const auto& read_root = reader.RootObject();

    ASSERT_TRUE(read_root.IsValid());

    auto user_obj = read_root.ReadObject(TAG_USER);
    ASSERT_TRUE(user_obj.has_value());

    auto id = user_obj->ReadInt32(TAG_ID);
    ASSERT_TRUE(id.has_value());
    EXPECT_EQ(id.value(), 12345);

    auto name = user_obj->ReadString(TAG_NAME);
    ASSERT_TRUE(name.has_value());
    EXPECT_EQ(name.value(), "John Doe");

    auto settings_obj = user_obj->ReadObject(TAG_SETTINGS);
    ASSERT_TRUE(settings_obj.has_value());

    auto theme = settings_obj->ReadString(TAG_THEME);
    ASSERT_TRUE(theme.has_value());
    EXPECT_EQ(theme.value(), "dark");

    auto notifications = settings_obj->ReadBoolean(TAG_NOTIFICATIONS);
    ASSERT_TRUE(notifications.has_value());
    EXPECT_TRUE(notifications.value());
}

TEST(ObjectsTest, ObjectArrayReadWrite) {
    Writer writer(true);
    auto& root = writer.RootObject();

    auto users_array = root.FieldObjectArray(TAG_USERS_ARRAY);

    auto user1 = users_array.CreateElement();
    user1.FieldInt32(TAG_ID, 1);
    user1.FieldString(TAG_NAME, "Alice");
    user1.Finish();

    auto user2 = users_array.CreateElement();
    user2.FieldInt32(TAG_ID, 2);
    user2.FieldString(TAG_NAME, "Bob");
    user2.Finish();

    auto user3 = users_array.CreateElement();
    user3.FieldInt32(TAG_ID, 3);
    user3.FieldString(TAG_NAME, "Charlie");
    user3.Finish();

    users_array.Finish();

    writer.Finish();

    Reader reader(writer.Data(), writer.Size(), true);
    const auto& read_root = reader.RootObject();

    ASSERT_TRUE(read_root.IsValid());

    auto users_array_read = read_root.ReadObjectArray(TAG_USERS_ARRAY);
    ASSERT_TRUE(users_array_read.has_value());

    std::vector<int32_t> expected_ids = {1, 2, 3};
    std::vector<std::string> expected_names = {"Alice", "Bob", "Charlie"};

    size_t count = 0;
    for (const auto& user : *users_array_read) {
        ASSERT_LT(count, expected_ids.size());

        auto id = user.ReadInt32(TAG_ID);
        ASSERT_TRUE(id.has_value());
        EXPECT_EQ(id.value(), expected_ids[count]);

        auto name = user.ReadString(TAG_NAME);
        ASSERT_TRUE(name.has_value());
        EXPECT_EQ(name.value(), expected_names[count]);

        count++;
    }

    EXPECT_EQ(count, expected_ids.size());
}

TEST(ObjectsTest, EmptyObjectArray) {
    Writer writer(true);
    auto& root = writer.RootObject();

    auto users_array = root.FieldObjectArray(TAG_USERS_ARRAY);
    users_array.Finish();

    writer.Finish();

    Reader reader(writer.Data(), writer.Size(), true);
    const auto& read_root = reader.RootObject();

    ASSERT_TRUE(read_root.IsValid());

    auto users_array_read = read_root.ReadObjectArray(TAG_USERS_ARRAY);
    ASSERT_TRUE(users_array_read.has_value());

    size_t count = 0;
    for ([[maybe_unused]] const auto& user : *users_array_read) {
        count++;
    }

    EXPECT_EQ(count, 0);
}
