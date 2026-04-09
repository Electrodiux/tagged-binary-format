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

#include "tbf/DataTag.hpp"
#include "tbf/DataType.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <optional>
#include <span>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>

namespace tbf {

class Reader;
class ObjectReader;

class ObjectArrayReader;
class StringArrayReader;
class BinaryArrayReader;

class ObjectReader {
   private:
    friend class Reader;

    friend class ObjectArrayReader;
    friend class StringArrayReader;
    friend class BinaryArrayReader;

   private:
    union CacheValue {
        const void* ptr;

        int8_t v_int8;
        int16_t v_int16;
        int32_t v_int32;
        int64_t v_int64;

        bool v_bool;
        uint16_t v_float16;
        float v_float32;
        double v_float64;
    };

    struct CacheEntry {
        DataType type;
        CacheValue value;
    };

   private:
    static constexpr uint32_t INITIAL_CACHE_SIZE = 100;

   private:
    const void* m_buffer;
    FieldSize m_size;

    bool m_name_based;

    // Reader cache for quick tag lookup
    using IdCache = std::unordered_map<DataTag::Id, CacheEntry>;
    using NameCache = std::unordered_map<std::string_view, CacheEntry>;
    mutable std::variant<IdCache, NameCache> m_cache;

    mutable bool m_cache_built = false;
    mutable bool m_is_valid = false;

    // ---------------------------------
    // Constructors & Destructor
    // ---------------------------------

   public:
    ObjectReader(const void* buffer, size_t size, bool name_based) noexcept;
    ObjectReader(const void* buffer, bool name_based) noexcept;

   public:
    ObjectReader(const ObjectReader&) noexcept = delete;
    ObjectReader& operator=(const ObjectReader&) noexcept = delete;
    ObjectReader(ObjectReader&&) noexcept = default;
    ObjectReader& operator=(ObjectReader&&) noexcept = default;

    ~ObjectReader() noexcept = default;

    // ---------------------------------
    // Methods
    // ---------------------------------

   public:
    inline bool IsValid() const noexcept {
        if (!m_cache_built) {
            CreateCache();
        }
        return m_is_valid;
    }

    bool ContainsTag(const DataTag& tag) const noexcept {
        CacheEntry entry;
        return FindTag(tag, entry);
    }

    bool AssertTag(const DataTag& tag, DataType expected_type) const noexcept {
        CacheEntry entry;
        return FindTag(tag, entry) && entry.type == expected_type;
    }

    std::optional<DataType> GetTagType(const DataTag& tag) const noexcept {
        CacheEntry entry;
        if (FindTag(tag, entry)) {
            return entry.type;
        }
        return std::nullopt;
    }

    std::vector<DataTag> GetAllTags() const noexcept;

    // ---------------------------------
    // Cache management
    // ---------------------------------

   public:
    void CreateCache(uint32_t initial_size = INITIAL_CACHE_SIZE) const noexcept;

   private:
    bool FindTag(const DataTag& tag, CacheEntry& out_entry) const noexcept;

    void Invalidate() noexcept {
        m_cache_built = false;
        m_is_valid = false;
    }

    // ---------------------------------
    // Read methods - Template-based
    // ---------------------------------

   private:
    template <Primitive T>
    bool ReadPrimitive(const DataTag& tag, T& out_value) const noexcept;

    const void* ReadPointerData(const DataTag& tag, DataType expected_type, FieldSize& out_size) const noexcept;

   public:
    // Primitives - returns optional
    template <Primitive T>
    [[nodiscard]] std::optional<T> Read(const DataTag& tag) const noexcept;

    // Primitives - out-param style
    template <Primitive T>
    bool Read(const DataTag& tag, T& out_value) const noexcept;

    // Enum
    template <typename Enum>
        requires std::is_enum_v<Enum>
    [[nodiscard]] std::optional<Enum> ReadEnum(const DataTag& tag) const noexcept;

    // String
    [[nodiscard]] std::optional<std::string_view> ReadString(const DataTag& tag) const noexcept;

    // Binary
    [[nodiscard]] std::span<const uint8_t> ReadBinary(const DataTag& tag) const noexcept;

    // UUID
    [[nodiscard]] const void* ReadUUID(const DataTag& tag) const noexcept;

    // Object
    [[nodiscard]] std::optional<ObjectReader> ReadObject(const DataTag& tag) const noexcept;

    // ---------------------------------
    // Read arrays - Template-based
    // ---------------------------------

    // Fixed arrays - span return
    template <ArrayElement T>
    [[nodiscard]] std::span<const T> ReadArray(const DataTag& tag) const noexcept;

    // Fixed arrays - pointer + length return
    template <ArrayElement T>
    [[nodiscard]] const T* ReadArray(const DataTag& tag, uint32_t& out_length) const noexcept;

    // Variable arrays
    [[nodiscard]] std::optional<StringArrayReader> ReadStringArray(const DataTag& tag) const noexcept;
    [[nodiscard]] std::optional<BinaryArrayReader> ReadBinaryArray(const DataTag& tag) const noexcept;
    [[nodiscard]] std::optional<ObjectArrayReader> ReadObjectArray(const DataTag& tag) const noexcept;

    // ---------------------------------
    // Read vectors - Template-based
    // ---------------------------------

    template <Primitive T, uint32_t size>
        requires(size >= 2) && (size <= 4)
    [[nodiscard]] std::optional<std::array<T, size>> ReadVector(const DataTag& tag) const noexcept;

   private:
    bool ReadStringInternal(const CacheEntry& entry, std::string_view& out_value) const noexcept;

    [[nodiscard]] std::optional<ObjectReader> ReadObjectInternal(const CacheEntry& entry) const noexcept;

    template <Primitive T, uint32_t size>
        requires(size >= 2) && (size <= 4)
    [[nodiscard]] std::optional<std::array<T, size>> ReadVectorInternal(const CacheEntry& entry) const noexcept;
};

template <typename ElementSizeType>
    requires std::is_integral<ElementSizeType>::value
class ArrayReader {
   protected:
    class BaseIterator {
       private:
        friend class ArrayReader;

       public:
        using difference_type = std::ptrdiff_t;
        using iterator_category = std::forward_iterator_tag;

       private:
        const uint8_t* m_current_ptr;
        const uint8_t* m_end_ptr;
        uint32_t m_index;

       protected:
        BaseIterator(const void* array, uint32_t index, bool at_end) noexcept;

       public:
        bool operator==(const BaseIterator& other) const noexcept {
            return m_current_ptr == other.m_current_ptr;
        }

        uint32_t Index() const noexcept { return m_index; }

       protected:
        void Advance() noexcept;
        const void* CurrentElement(ElementSizeType* out_size = nullptr) const noexcept;
    };

   protected:
    const void* m_array;

    uint32_t m_element_count;
    bool m_valid;

   protected:
    ArrayReader(const void* array) noexcept;

   public:
    ArrayReader(const ArrayReader&) = delete;
    ArrayReader& operator=(const ArrayReader&) = delete;

    inline uint32_t Size() const noexcept { return m_element_count; }
    inline bool IsValid() const noexcept { return m_valid; }

   protected:
    bool GetElement(uint32_t index, const void*& out_ptr, ElementSizeType* size = nullptr) const noexcept;

    inline void Invalidate() noexcept {
        m_valid = false;
        m_element_count = 0;
    }

   private:
    void Initialize() noexcept;
};

extern template class ArrayReader<uint16_t>;
extern template class ArrayReader<FieldSize>;

class StringArrayReader : public ArrayReader<uint16_t> {
   private:
    friend class ObjectReader;

   public:
    class Iterator : public ArrayReader<uint16_t>::BaseIterator {
       private:
        friend class StringArrayReader;

       public:
        using value_type = std::string_view;
        using pointer = const std::string_view*;
        using reference = std::string_view;

       private:
        Iterator(const void* array, uint32_t index, bool at_end) noexcept
            : BaseIterator(array, index, at_end) {}

       public:
        value_type operator*() const noexcept;

        Iterator& operator++() noexcept {
            this->Advance();
            return *this;
        }

        Iterator operator++(int) noexcept {
            Iterator tmp = *this;
            this->Advance();
            return tmp;
        }

        bool operator==(const Iterator& other) const noexcept {
            return this->BaseIterator::operator==(other);
        }
    };

   public:
    StringArrayReader(const ObjectReader::CacheEntry& entry) noexcept;

    bool GetElement(uint32_t index, std::string_view& out_value) const noexcept;

    [[nodiscard]]
    inline std::optional<std::string_view> GetElement(uint32_t index) const noexcept {
        std::string_view value;
        return GetElement(index, value) ? std::optional<std::string_view>(value) : std::nullopt;
    }

    Iterator begin() const noexcept {
        return IsValid() ? Iterator(m_array, 0, false) : end();
    }

    Iterator end() const noexcept {
        return Iterator(m_array, m_element_count, true);
    }
};

class BinaryArrayReader : public ArrayReader<FieldSize> {
   private:
    friend class ObjectReader;

   public:
    class Iterator : public ArrayReader<FieldSize>::BaseIterator {
       private:
        friend class BinaryArrayReader;

       public:
        using value_type = std::span<const uint8_t>;
        using pointer = const value_type*;
        using reference = value_type;

       private:
        Iterator(const void* array, uint32_t index, bool at_end) noexcept
            : BaseIterator(array, index, at_end) {}

       public:
        value_type operator*() const noexcept;

        Iterator& operator++() noexcept {
            this->Advance();
            return *this;
        }

        Iterator operator++(int) noexcept {
            Iterator tmp = *this;
            this->Advance();
            return tmp;
        }

        bool operator==(const Iterator& other) const noexcept {
            return this->BaseIterator::operator==(other);
        }
    };

   public:
    BinaryArrayReader(const ObjectReader::CacheEntry& entry) noexcept;

    bool GetElement(uint32_t index, const void*& out_data, FieldSize& out_size) const noexcept;

    Iterator begin() const noexcept {
        return IsValid() ? Iterator(m_array, 0, false) : end();
    }

    Iterator end() const noexcept {
        return Iterator(m_array, m_element_count, true);
    }
};

class ObjectArrayReader : public ArrayReader<FieldSize> {
   private:
    friend class ObjectReader;

   private:
    bool m_name_based;

   public:
    class Iterator : public ArrayReader<FieldSize>::BaseIterator {
       private:
        friend class ObjectArrayReader;

       public:
        using value_type = ObjectReader;
        using pointer = const ObjectReader*;
        using reference = ObjectReader;

       private:
        bool m_name_based;

       private:
        Iterator(const void* array, uint32_t index, bool at_end, bool name_based) noexcept
            : BaseIterator(array, index, at_end), m_name_based(name_based) {}

       public:
        value_type operator*() const noexcept;

        Iterator& operator++() noexcept {
            this->Advance();
            return *this;
        }

        Iterator operator++(int) noexcept {
            Iterator tmp = *this;
            this->Advance();
            return tmp;
        }

        bool operator==(const Iterator& other) const noexcept {
            return this->BaseIterator::operator==(other);
        }
    };

   public:
    ObjectArrayReader(const ObjectReader::CacheEntry& entry, bool name_based) noexcept;

    std::optional<ObjectReader> GetElement(uint32_t index) const noexcept;

    Iterator begin() const noexcept {
        return IsValid() ? Iterator(m_array, 0, false, m_name_based) : end();
    }

    Iterator end() const noexcept {
        return Iterator(m_array, m_element_count, true, m_name_based);
    }
};

class Reader {
   private:
    ObjectReader m_root_object;

   public:
    Reader(const void* buffer, size_t size, bool name_based) noexcept;

    Reader(const Reader&) = delete;
    Reader& operator=(const Reader&) = delete;

    inline const ObjectReader& RootObject() const noexcept { return m_root_object; }
    inline bool IsValid() const noexcept { return m_root_object.IsValid(); }
};

// ==============================================================================
// Template implementations
// ==============================================================================

template <Primitive T>
bool ObjectReader::ReadPrimitive(const DataTag& tag, T& out_value) const noexcept {
    CacheEntry entry;
    if (!FindTag(tag, entry) || entry.type != Type<T>::type) {
        return false;
    }
    std::memcpy(&out_value, &entry.value, sizeof(T));
    return true;
}

template <Primitive T>
std::optional<T> ObjectReader::Read(const DataTag& tag) const noexcept {
    T value;
    return ReadPrimitive<T>(tag, value) ? std::optional<T>(value) : std::nullopt;
}

template <Primitive T>
bool ObjectReader::Read(const DataTag& tag, T& out_value) const noexcept {
    return ReadPrimitive<T>(tag, out_value);
}

template <typename Enum>
    requires std::is_enum_v<Enum>
std::optional<Enum> ObjectReader::ReadEnum(const DataTag& tag) const noexcept {
    using UnderlyingType = std::underlying_type_t<Enum>;
    UnderlyingType value;
    if (ReadPrimitive<UnderlyingType>(tag, value)) {
        return std::optional<Enum>(static_cast<Enum>(value));
    }
    return std::nullopt;
}

template <ArrayElement T>
std::span<const T> ObjectReader::ReadArray(const DataTag& tag) const noexcept {
    FieldSize out_size;
    const void* value_ptr = ReadPointerData(tag, Type<T>::array_type, out_size);

    if (value_ptr != nullptr) {
        constexpr uint32_t element_size = sizeof(T);
        uint32_t array_length = out_size / element_size;

        if (array_length * element_size != out_size) [[unlikely]] {
            return std::span<const T>();
        }

        return std::span<const T>(static_cast<const T*>(value_ptr), array_length);
    }

    return std::span<const T>();
}

template <ArrayElement T>
const T* ObjectReader::ReadArray(const DataTag& tag, uint32_t& out_length) const noexcept {
    FieldSize out_size;
    const void* value_ptr = ReadPointerData(tag, Type<T>::array_type, out_size);

    if (value_ptr != nullptr) {
        constexpr uint32_t element_size = sizeof(T);
        uint32_t array_length = out_size / element_size;

        if (array_length * element_size != out_size) [[unlikely]] {
            out_length = 0;
            return nullptr;
        }

        out_length = array_length;
        return static_cast<const T*>(value_ptr);
    }

    out_length = 0;
    return nullptr;
}

template <Primitive T, uint32_t size>
    requires(size >= 2) && (size <= 4)
std::optional<std::array<T, size>> ObjectReader::ReadVector(const DataTag& tag) const noexcept {
    CacheEntry entry;
    if (!FindTag(tag, entry)) {
        return std::nullopt;
    }
    return ReadVectorInternal<T, size>(entry);
}

template <Primitive T, uint32_t size>
    requires(size >= 2) && (size <= 4)
std::optional<std::array<T, size>> ObjectReader::ReadVectorInternal(const CacheEntry& entry) const noexcept {
    DataType expected_type = VectorType<size, T>();
    if (entry.type != expected_type) {
        return std::nullopt;
    }

    std::array<T, size> result;
    const T* src = static_cast<const T*>(entry.value.ptr);
    for (uint32_t i = 0; i < size; ++i) {
        result[i] = src[i];
    }
    return std::optional<std::array<T, size>>(result);
}

}  // namespace tbf