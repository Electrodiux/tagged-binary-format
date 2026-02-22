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

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>
#include <unordered_map>
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

        uint8_t v_uint8;
        uint16_t v_uint16;
        uint32_t v_uint32;
        uint64_t v_uint64;

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

    mutable bool m_cache_built = false;
    mutable bool m_is_valid = false;

    union {
        mutable std::unordered_map<DataTag::Id, CacheEntry> m_id_cache;
        mutable std::unordered_map<std::string_view, CacheEntry> m_name_cache;
    };

    // ---------------------------------
    // Constructors & Destructor
    // ---------------------------------

   public:
    ObjectReader(const void* buffer, size_t size, bool name_based) noexcept;
    ObjectReader(const void* buffer, bool name_based) noexcept;

   public:
    ObjectReader(const ObjectReader&) noexcept = delete;
    ObjectReader& operator=(const ObjectReader&) noexcept = delete;

    ~ObjectReader() noexcept;

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
        if (m_name_based) {
            m_name_cache.clear();
        } else {
            m_id_cache.clear();
        }
        m_cache_built = false;
        m_is_valid = false;
    }

    // ---------------------------------
    // Read methods
    // ---------------------------------

   private:
    template <typename Type, DataType expected_type>
    bool ReadPrimitive(const DataTag& tag, Type& out_value) const noexcept;
    const void* ReadPointerData(const DataTag& tag, DataType expected_type, FieldSize& out_size) const noexcept;

   public:
    bool ReadInt8(const DataTag& tag, int8_t& out_value) const noexcept;
    bool ReadInt16(const DataTag& tag, int16_t& out_value) const noexcept;
    bool ReadInt32(const DataTag& tag, int32_t& out_value) const noexcept;
    bool ReadInt64(const DataTag& tag, int64_t& out_value) const noexcept;

    bool ReadUInt8(const DataTag& tag, uint8_t& out_value) const noexcept;
    bool ReadUInt16(const DataTag& tag, uint16_t& out_value) const noexcept;
    bool ReadUInt32(const DataTag& tag, uint32_t& out_value) const noexcept;
    bool ReadUInt64(const DataTag& tag, uint64_t& out_value) const noexcept;

    bool ReadBoolean(const DataTag& tag, bool& out_value) const noexcept;
    bool ReadFloat16(const DataTag& tag, uint16_t& out_value) const noexcept;
    bool ReadFloat32(const DataTag& tag, float& out_value) const noexcept;
    bool ReadFloat64(const DataTag& tag, double& out_value) const noexcept;

    bool ReadString(const DataTag& tag, std::string_view& out_value) const noexcept;

    [[nodiscard]] const void* ReadUUID(const DataTag& tag) const noexcept;
    [[nodiscard]] const void* ReadBinary(const DataTag& tag, FieldSize& out_size) const noexcept;
    [[nodiscard]] std::optional<ObjectReader> ReadObject(const DataTag& tag) const noexcept;

    template <typename Enum>
        requires std::is_enum<Enum>::value
    inline void FieldEnum(const DataTag& tag, Enum value) {
        using UnderlyingType = typename std::underlying_type<Enum>::type;
        return ReadPrimitive<UnderlyingType, IntegerType<UnderlyingType>()>(tag, reinterpret_cast<UnderlyingType&>(value));
    }

    // ---------------------------------
    // Helper read methods
    // ---------------------------------

   public:
    [[nodiscard]]
    inline std::optional<int8_t> ReadInt8(const DataTag& tag) const noexcept {
        int8_t value;
        return ReadInt8(tag, value) ? std::optional<int8_t>(value) : std::nullopt;
    }

    [[nodiscard]]
    inline std::optional<int16_t> ReadInt16(const DataTag& tag) const noexcept {
        int16_t value;
        return ReadInt16(tag, value) ? std::optional<int16_t>(value) : std::nullopt;
    }

    [[nodiscard]]
    inline std::optional<int32_t> ReadInt32(const DataTag& tag) const noexcept {
        int32_t value;
        return ReadInt32(tag, value) ? std::optional<int32_t>(value) : std::nullopt;
    }

    [[nodiscard]]
    inline std::optional<int64_t> ReadInt64(const DataTag& tag) const noexcept {
        int64_t value;
        return ReadInt64(tag, value) ? std::optional<int64_t>(value) : std::nullopt;
    }

    [[nodiscard]]
    inline std::optional<uint8_t> ReadUInt8(const DataTag& tag) const noexcept {
        uint8_t value;
        return ReadUInt8(tag, value) ? std::optional<uint8_t>(value) : std::nullopt;
    }

    [[nodiscard]]
    inline std::optional<uint16_t> ReadUInt16(const DataTag& tag) const noexcept {
        uint16_t value;
        return ReadUInt16(tag, value) ? std::optional<uint16_t>(value) : std::nullopt;
    }

    [[nodiscard]]
    inline std::optional<uint32_t> ReadUInt32(const DataTag& tag) const noexcept {
        uint32_t value;
        return ReadUInt32(tag, value) ? std::optional<uint32_t>(value) : std::nullopt;
    }

    [[nodiscard]]
    inline std::optional<uint64_t> ReadUInt64(const DataTag& tag) const noexcept {
        uint64_t value;
        return ReadUInt64(tag, value) ? std::optional<uint64_t>(value) : std::nullopt;
    }

    [[nodiscard]]
    inline std::optional<bool> ReadBoolean(const DataTag& tag) const noexcept {
        bool value;
        return ReadBoolean(tag, value) ? std::optional<bool>(value) : std::nullopt;
    }

    [[nodiscard]]
    inline std::optional<uint16_t> ReadFloat16(const DataTag& tag) const noexcept {
        uint16_t value;
        return ReadFloat16(tag, value) ? std::optional<uint16_t>(value) : std::nullopt;
    }

    [[nodiscard]]
    inline std::optional<float> ReadFloat32(const DataTag& tag) const noexcept {
        float value;
        return ReadFloat32(tag, value) ? std::optional<float>(value) : std::nullopt;
    }

    [[nodiscard]]
    inline std::optional<double> ReadFloat64(const DataTag& tag) const noexcept {
        double value;
        return ReadFloat64(tag, value) ? std::optional<double>(value) : std::nullopt;
    }

    [[nodiscard]]
    inline std::optional<std::string_view> ReadString(const DataTag& tag) const noexcept {
        std::string_view value;
        return ReadString(tag, value) ? std::optional<std::string_view>(value) : std::nullopt;
    }

    [[nodiscard]]
    inline std::span<const uint8_t> ReadBinary(const DataTag& tag) const noexcept {
        uint32_t size;
        const void* data = ReadBinary(tag, size);
        return data ? std::span<const uint8_t>(static_cast<const uint8_t*>(data), size) : std::span<const uint8_t>();
    }

    template <typename Enum>
        requires std::is_enum<Enum>::value
    [[nodiscard]]
    inline std::optional<Enum> FieldEnum(const DataTag& tag) const noexcept {
        using UnderlyingType = typename std::underlying_type<Enum>::type;
        UnderlyingType value;
        if (ReadPrimitive<UnderlyingType, IntegerType<UnderlyingType>()>(tag, value)) {
            return std::optional<Enum>(static_cast<Enum>(value));
        }
        return std::nullopt;
    }

    // ---------------------------------
    // Read arrays
    // ---------------------------------

   private:
    template <typename Type, DataType expected_type>
    const Type* ReadArray(const DataTag& tag, uint32_t& out_length) const noexcept;

   public:
    [[nodiscard]] const int8_t* ReadInt8Array(const DataTag& tag, uint32_t& out_length) const noexcept;
    [[nodiscard]] const int16_t* ReadInt16Array(const DataTag& tag, uint32_t& out_length) const noexcept;
    [[nodiscard]] const int32_t* ReadInt32Array(const DataTag& tag, uint32_t& out_length) const noexcept;
    [[nodiscard]] const int64_t* ReadInt64Array(const DataTag& tag, uint32_t& out_length) const noexcept;

    [[nodiscard]] const uint8_t* ReadUInt8Array(const DataTag& tag, uint32_t& out_length) const noexcept;
    [[nodiscard]] const uint16_t* ReadUInt16Array(const DataTag& tag, uint32_t& out_length) const noexcept;
    [[nodiscard]] const uint32_t* ReadUInt32Array(const DataTag& tag, uint32_t& out_length) const noexcept;
    [[nodiscard]] const uint64_t* ReadUInt64Array(const DataTag& tag, uint32_t& out_length) const noexcept;

    [[nodiscard]] const bool* ReadBooleanArray(const DataTag& tag, uint32_t& out_length) const noexcept;
    [[nodiscard]] const uint16_t* ReadFloat16Array(const DataTag& tag, uint32_t& out_length) const noexcept;
    [[nodiscard]] const float* ReadFloat32Array(const DataTag& tag, uint32_t& out_length) const noexcept;
    [[nodiscard]] const double* ReadFloat64Array(const DataTag& tag, uint32_t& out_length) const noexcept;

    [[nodiscard]] std::optional<StringArrayReader> ReadStringArray(const DataTag& tag) const noexcept;
    [[nodiscard]] std::optional<BinaryArrayReader> ReadBinaryArray(const DataTag& tag) const noexcept;
    [[nodiscard]] std::optional<ObjectArrayReader> ReadObjectArray(const DataTag& tag) const noexcept;

    // ---------------------------------
    // Read array as std::span methods
    // ---------------------------------

   private:
    template <typename Type, DataType expected_type>
    std::span<const Type> ReadArray(const DataTag& tag) const noexcept;

   public:
    [[nodiscard]] std::span<const int8_t> ReadInt8Array(const DataTag& tag) const noexcept;
    [[nodiscard]] std::span<const int16_t> ReadInt16Array(const DataTag& tag) const noexcept;
    [[nodiscard]] std::span<const int32_t> ReadInt32Array(const DataTag& tag) const noexcept;
    [[nodiscard]] std::span<const int64_t> ReadInt64Array(const DataTag& tag) const noexcept;

    [[nodiscard]] std::span<const uint8_t> ReadUInt8Array(const DataTag& tag) const noexcept;
    [[nodiscard]] std::span<const uint16_t> ReadUInt16Array(const DataTag& tag) const noexcept;
    [[nodiscard]] std::span<const uint32_t> ReadUInt32Array(const DataTag& tag) const noexcept;
    [[nodiscard]] std::span<const uint64_t> ReadUInt64Array(const DataTag& tag) const noexcept;

    [[nodiscard]] std::span<const bool> ReadBooleanArray(const DataTag& tag) const noexcept;
    [[nodiscard]] std::span<const uint16_t> ReadFloat16Array(const DataTag& tag) const noexcept;
    [[nodiscard]] std::span<const float> ReadFloat32Array(const DataTag& tag) const noexcept;
    [[nodiscard]] std::span<const double> ReadFloat64Array(const DataTag& tag) const noexcept;

   private:
    bool ReadStringInternal(const CacheEntry& entry, std::string_view& out_value) const noexcept;
    [[nodiscard]] std::optional<ObjectReader> ReadObjectInternal(const CacheEntry& entry) const noexcept;
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

}  // namespace tbf