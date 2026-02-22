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

#pragma once

#include <cstdint>
#include <stdexcept>
#include <string_view>

namespace tbf {

constexpr uint32_t MAX_TAG_NAME_LENGTH = 0xFF;  // 255 characters

inline consteval bool IsValidTagChar(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_';
}

inline consteval bool IsTagNameValid(std::string_view name) {
    if (name.empty() || name.size() > MAX_TAG_NAME_LENGTH) {
        return false;
    }

    for (char c : name) {
        if (!IsValidTagChar(c)) {
            return false;
        }
    }

    return true;
}

consteval uint32_t TagNameHash(std::string_view str) {
    // FNV-1a hash optimized for snake_case identifiers (a-z, A-Z, 0-9, _)
    // Using 32-bit FNV-1a constants
    uint32_t hash = 2166136261u;
    for (char c : str) {
        uint8_t mapped;

        if (c >= 'a' && c <= 'z') {
            mapped = c - 'a' + 1;  // Map 'a'-'z' to 1-26
        } else if (c >= 'A' && c <= 'Z') {
            mapped = c - 'A' + 1;  // Map 'A'-'Z' to 1-26
        } else if (c >= '0' && c <= '9') {
            mapped = c - '0' + 27;  // Map '0'-'9' to 27-36
        } else if (c == '_') {
            mapped = 37;  // Map '_' to 37
        } else {
            mapped = 0;  // Invalid character, map to 0
        }

        hash ^= mapped;
        hash *= 16777619u;
    }

    return hash;
}

class DataTag {
   public:
    using Id = uint16_t;
    using NameSize = uint8_t;

   public:
    static constexpr Id INVALID_ID = 0;

   private:
    Id id;
    std::string_view name;

   private:
    void consteval Validate() const {
        if (!IsTagNameValid(name)) {
            throw std::invalid_argument("Invalid tag name");
        }

        if (id == INVALID_ID) {
            throw std::invalid_argument("Tag ID cannot be zero");
        }
    }

   public:
    consteval DataTag(const char* name) : id(static_cast<Id>(TagNameHash(name))), name(name) {
        Validate();
    }

    consteval DataTag(Id id, const char* name) : id(id), name(name) {
        Validate();
    }

    Id GetId() const noexcept { return id; }
    std::string_view GetName() const noexcept { return name; }
    bool HasId() const noexcept { return id != INVALID_ID; }

    explicit DataTag(Id id) noexcept : id(id), name() {}
    explicit DataTag(std::string_view name) noexcept : id(INVALID_ID), name(name) {}

    bool operator==(const DataTag& other) const noexcept {
        if (HasId() && other.HasId()) {
            return id == other.id;
        }
        return name == other.name;
    }

    bool operator!=(const DataTag& other) const noexcept {
        return !(*this == other);
    }
};

}  // namespace tbf