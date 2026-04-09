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

#include "tbf/DataTag.hpp"
#include "tbf/DataType.hpp"
#include "tbf/Endianness.hpp"

#include <cstdint>
#include <cstring>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

namespace tbf {

// ---------------------------------
// Reader
// ---------------------------------

Reader::Reader(const void* buffer, size_t size, bool name_based) noexcept
    : m_root_object(buffer, size, name_based) {}

// ---------------------------------
// Constructors & Destructor
// ---------------------------------

ObjectReader::ObjectReader(const void* buffer, size_t size, bool name_based) noexcept
    : ObjectReader(buffer, name_based) {
    if (m_size + sizeof(FieldSize) > size) {
        Invalidate();
    }
}

ObjectReader::ObjectReader(const void* buffer, bool name_based) noexcept
    : m_size(0),
      m_name_based(name_based),
      m_cache_built(false),
      m_is_valid(false) {
    if (buffer == nullptr) {
        return;
    }

    std::memcpy(&m_size, buffer, sizeof(FieldSize));
    AdjustEndianess(m_size);
    m_buffer = static_cast<const uint8_t*>(buffer) + sizeof(FieldSize);

    // Initialize variant with correct type
    if (name_based) {
        m_cache = NameCache {};
    } else {
        m_cache = IdCache {};
    }

    m_is_valid = true;
}

// ---------------------------------
// Memory checking helpers
// ---------------------------------

static bool CanAccessBuffer(const void* beg, const void* end, size_t size) noexcept {
    return static_cast<size_t>(static_cast<const uint8_t*>(end) - static_cast<const uint8_t*>(beg)) >= size;
}

template <typename Type, bool swap_endianess = true>
static bool ReadData(const uint8_t*& read_ptr, const uint8_t* end_ptr, Type& out_value) noexcept {
    if (CanAccessBuffer(read_ptr, end_ptr, sizeof(Type))) [[likely]] {
        std::memcpy(&out_value, read_ptr, sizeof(Type));

        if constexpr (swap_endianess) {
            AdjustEndianess(out_value);
        }

        read_ptr += sizeof(Type);

        return true;
    }
    return false;
}

// ---------------------------------
// Cache management
// ---------------------------------

void ObjectReader::CreateCache(uint32_t initial_size) const noexcept {
    if (m_cache_built) [[likely]] {
        return;
    }

    if (m_buffer == nullptr) [[unlikely]] {
        m_is_valid = false;
        m_cache_built = true;
        return;
    }

    // Empty object (m_size == 0) is valid - just has no fields
    if (m_size == 0) {
        m_cache_built = true;
        m_is_valid = true;
        return;
    }

    if (m_name_based) {
        std::get<NameCache>(m_cache).clear();
        std::get<NameCache>(m_cache).reserve(initial_size);
    } else {
        std::get<IdCache>(m_cache).clear();
        std::get<IdCache>(m_cache).reserve(initial_size);
    }

    const uint8_t* read_ptr = static_cast<const uint8_t*>(m_buffer);
    const uint8_t* buff_end = static_cast<const uint8_t*>(m_buffer) + m_size;

    bool errors = false;

    while (read_ptr < buff_end) {
        // Read register

        DataType type;
        if (!ReadData<DataType>(read_ptr, buff_end, type) || !IsValidDataType(type)) [[unlikely]] {
            errors = true;
            break;
        }

        const uint8_t* tag_ptr = nullptr;
        uint8_t tag_size;

        // Read tag based on the mode (name-based or id-based)

        if (m_name_based) {
            if (
                !ReadData<uint8_t>(read_ptr, buff_end, tag_size) ||
                !CanAccessBuffer(read_ptr, buff_end, tag_size)) [[unlikely]] {
                errors = true;
                break;
            }

            tag_ptr = read_ptr;
            read_ptr += tag_size;
        } else {
            if (!CanAccessBuffer(read_ptr, buff_end, sizeof(DataTag::Id))) [[unlikely]] {
                errors = true;
                break;
            }

            tag_ptr = read_ptr;
            read_ptr += sizeof(DataTag::Id);
        }

        // Read the corresponding entry

        CacheEntry entry = {.type = type, .value = {.ptr = nullptr}};

        if (IsArrayType(type)) {
            entry.value.ptr = read_ptr;

            FieldSize array_size;
            if (!ReadData<FieldSize>(read_ptr, buff_end, array_size)) [[unlikely]] {
                errors = true;
                break;
            } else {
                // Adjust endianness for array elements during cache creation
                uint32_t element_size = DataTypeSize(BaseDataType(type));

                if (element_size > 1) {
                    uint32_t array_length = array_size / element_size;

                    // Verify array size is consistent
                    if (array_length * element_size == array_size) [[likely]] {
                        void* mutable_ptr = const_cast<void*>(static_cast<const void*>(read_ptr));
                        switch (element_size) {
                            case 2:
                                AdjustArrayEndianess<2>(mutable_ptr, array_length);
                                break;
                            case 4:
                                AdjustArrayEndianess<4>(mutable_ptr, array_length);
                                break;
                            case 8:
                                AdjustArrayEndianess<8>(mutable_ptr, array_length);
                                break;
                        }
                    }
                }

                read_ptr += array_size;
            }
        } else if (IsVectorType(type)) {
            entry.value.ptr = read_ptr;

            uint32_t vector_length = VectorTypeDimension(type);
            uint32_t element_size = DataTypeSize(BaseDataType(type));

            uint32_t vector_size = vector_length * element_size;

            if (!CanAccessBuffer(read_ptr, buff_end, vector_size)) [[unlikely]] {
                errors = true;
            } else {
                // Adjust endianness for vector elements during cache creation
                if (element_size > 1) {
                    void* mutable_ptr = const_cast<void*>(static_cast<const void*>(read_ptr));
                    switch (element_size) {
                        case 2:
                            AdjustArrayEndianess<2>(mutable_ptr, vector_length);
                            break;
                        case 4:
                            AdjustArrayEndianess<4>(mutable_ptr, vector_length);
                            break;
                        case 8:
                            AdjustArrayEndianess<8>(mutable_ptr, vector_length);
                            break;
                    }
                }
                read_ptr += vector_size;
            }
        } else if (IsPrimitiveType(type)) {
            switch (type) {
                // Primitives
                case DataType::Boolean:
                case DataType::Int8:
                    if (!ReadData<int8_t>(read_ptr, buff_end, entry.value.v_int8)) [[unlikely]] {
                        errors = true;
                    }
                    break;
                case DataType::Float16:
                case DataType::Int16:
                    if (!ReadData<int16_t>(read_ptr, buff_end, entry.value.v_int16)) [[unlikely]] {
                        errors = true;
                    }
                    break;
                case DataType::Float32:
                case DataType::Int32:
                    if (!ReadData<int32_t>(read_ptr, buff_end, entry.value.v_int32)) [[unlikely]] {
                        errors = true;
                    }
                    break;
                case DataType::Float64:
                case DataType::Int64:
                    if (!ReadData<int64_t>(read_ptr, buff_end, entry.value.v_int64)) [[unlikely]] {
                        errors = true;
                    }
                    break;
                case DataType::UUID:
                    entry.value.ptr = read_ptr;

                    if (!CanAccessBuffer(read_ptr, buff_end, 16)) [[unlikely]] {
                        errors = true;
                    } else {
                        read_ptr += 16;
                    }
                    break;
                case DataType::String: {
                    entry.value.ptr = read_ptr;

                    uint16_t length;
                    if (!ReadData<uint16_t>(read_ptr, buff_end, length)) [[unlikely]] {
                        errors = true;
                    } else {
                        read_ptr += length;
                    }

                    break;
                }
                case DataType::Object:
                case DataType::Binary: {
                    entry.value.ptr = read_ptr;

                    FieldSize size;
                    if (!ReadData<FieldSize>(read_ptr, buff_end, size)) [[unlikely]] {
                        errors = true;
                    } else {
                        read_ptr += size;
                    }

                    break;
                }
                default:
                    errors = true;
                    break;
            }
        } else {
            errors = true;
        }

        // If there is an error stop parsing

        if (errors || read_ptr > buff_end) [[unlikely]] {
            errors = true;
            break;
        }

        // Add tag to cache

        if (m_name_based) {
            std::string_view tag_name(reinterpret_cast<const char*>(tag_ptr), tag_size);
            std::get<NameCache>(m_cache).emplace(tag_name, entry);
        } else {
            DataTag::Id tag_id;
            std::memcpy(&tag_id, tag_ptr, sizeof(tag_id));
            AdjustEndianess(tag_id);
            std::get<IdCache>(m_cache).emplace(tag_id, entry);
        }
    }

    m_cache_built = true;
    m_is_valid = !errors && read_ptr == buff_end;
}

bool ObjectReader::FindTag(const DataTag& tag, CacheEntry& out_entry) const noexcept {
    if (!IsValid()) [[unlikely]] {
        return false;
    }

    if (m_name_based) {
        auto& cache = std::get<NameCache>(m_cache);
        auto it = cache.find(tag.GetName());
        if (it != cache.end()) [[likely]] {
            out_entry = it->second;
            return true;
        }
    } else {
        auto& cache = std::get<IdCache>(m_cache);
        auto it = cache.find(tag.GetId());
        if (it != cache.end()) [[likely]] {
            out_entry = it->second;
            return true;
        }
    }

    return false;
}

// ---------------------------------
// Methods
// ---------------------------------

std::vector<DataTag> ObjectReader::GetAllTags() const noexcept {
    std::vector<DataTag> tags;

    if (!IsValid()) [[unlikely]] {
        return tags;
    }

    if (m_name_based) {
        for (const auto& [key, entry] : std::get<NameCache>(m_cache)) {
            tags.emplace_back(key);
        }
    } else {
        for (const auto& [key, entry] : std::get<IdCache>(m_cache)) {
            tags.emplace_back(key);
        }
    }

    return tags;
}

// ---------------------------------
// Read methods
// ---------------------------------

const void* ObjectReader::ReadPointerData(const DataTag& tag, DataType expected_type, FieldSize& out_size) const noexcept {
    CacheEntry entry;
    if (!FindTag(tag, entry) || entry.type != expected_type) {
        return nullptr;
    }

    const uint8_t* value_ptr = static_cast<const uint8_t*>(entry.value.ptr);

    std::memcpy(&out_size, value_ptr, sizeof(out_size));
    AdjustEndianess(out_size);
    value_ptr += sizeof(out_size);

    return value_ptr;
}

std::optional<std::string_view> ObjectReader::ReadString(const DataTag& tag) const noexcept {
    CacheEntry entry;
    if (!FindTag(tag, entry)) {
        return std::nullopt;
    }
    std::string_view value;
    return ReadStringInternal(entry, value) ? std::optional<std::string_view>(value) : std::nullopt;
}

bool ObjectReader::ReadStringInternal(const CacheEntry& entry, std::string_view& out_value) const noexcept {
    if (entry.type != DataType::String) [[unlikely]] {
        return false;
    }
    const uint8_t* value_ptr = static_cast<const uint8_t*>(entry.value.ptr);
    uint16_t length;
    std::memcpy(&length, value_ptr, sizeof(length));
    AdjustEndianess(length);
    const char* str_ptr = reinterpret_cast<const char*>(value_ptr + sizeof(length));
    out_value = std::string_view(str_ptr, length);
    return true;
}

std::span<const uint8_t> ObjectReader::ReadBinary(const DataTag& tag) const noexcept {
    FieldSize size;
    const void* data = ReadPointerData(tag, DataType::Binary, size);
    return data ? std::span<const uint8_t>(static_cast<const uint8_t*>(data), size) : std::span<const uint8_t>();
}

const void* ObjectReader::ReadUUID(const DataTag& tag) const noexcept {
    CacheEntry entry;
    if (!FindTag(tag, entry) || entry.type != DataType::UUID) {
        return nullptr;
    }
    return entry.value.ptr;
}

std::optional<ObjectReader> ObjectReader::ReadObject(const DataTag& tag) const noexcept {
    CacheEntry entry;
    if (!FindTag(tag, entry)) {
        return std::nullopt;
    }
    return std::make_optional<ObjectReader>(entry.value.ptr, m_name_based);
}

// ---------------------------------
// Read variable arrays
// ---------------------------------

std::optional<StringArrayReader> ObjectReader::ReadStringArray(const DataTag& tag) const noexcept {
    CacheEntry entry;
    if (!FindTag(tag, entry) || entry.type != DataType::StringArray) {
        return std::nullopt;
    }
    return std::make_optional<StringArrayReader>(entry);
}

std::optional<BinaryArrayReader> ObjectReader::ReadBinaryArray(const DataTag& tag) const noexcept {
    CacheEntry entry;
    if (!FindTag(tag, entry) || entry.type != DataType::BinaryArray) {
        return std::nullopt;
    }
    return std::make_optional<BinaryArrayReader>(entry);
}

std::optional<ObjectArrayReader> ObjectReader::ReadObjectArray(const DataTag& tag) const noexcept {
    CacheEntry entry;
    if (!FindTag(tag, entry) || entry.type != DataType::ObjectArray) {
        return std::nullopt;
    }
    return std::make_optional<ObjectArrayReader>(entry, m_name_based);
}

// ---------------------------------
// Array readers
// ---------------------------------

template <typename ElementSizeType>
    requires std::is_integral<ElementSizeType>::value
ArrayReader<ElementSizeType>::ArrayReader(const void* array) noexcept
    : m_array(array) {
    Initialize();
}

template <typename ElementSizeType>
    requires std::is_integral<ElementSizeType>::value
bool ArrayReader<ElementSizeType>::GetElement(uint32_t index, const void*& out_ptr, ElementSizeType* size) const noexcept {
    if (!IsValid() || index >= m_element_count) {
        return false;
    }

    BaseIterator it(m_array, index, false);
    out_ptr = it.CurrentElement(size);
    return true;
}

static FieldSize GetArraySize(const void* array) noexcept {
    const uint8_t* read_ptr = reinterpret_cast<const uint8_t*>(array);

    FieldSize array_size;
    std::memcpy(&array_size, read_ptr, sizeof(array_size));
    AdjustEndianess(array_size);

    return array_size;
}

template <typename ElementSizeType>
    requires std::is_integral<ElementSizeType>::value
void ArrayReader<ElementSizeType>::Initialize() noexcept {
    m_element_count = 0;
    m_valid = false;

    const uint8_t* read_ptr = static_cast<const uint8_t*>(m_array);
    FieldSize array_size = GetArraySize(m_array);
    read_ptr += sizeof(FieldSize);
    const uint8_t* buff_end = read_ptr + array_size;

    while (read_ptr < buff_end) {
        if (!CanAccessBuffer(read_ptr, buff_end, sizeof(FieldSize))) {
            Invalidate();
            return;
        }

        ElementSizeType object_size;
        std::memcpy(&object_size, read_ptr, sizeof(object_size));
        AdjustEndianess(object_size);
        read_ptr += sizeof(object_size);

        if (!CanAccessBuffer(read_ptr, buff_end, object_size)) {
            Invalidate();
            break;
        }

        read_ptr += object_size;

        m_element_count++;
    }

    m_valid = read_ptr == buff_end;

    if (!m_valid) {
        Invalidate();
    }
}

template class ArrayReader<uint16_t>;
template class ArrayReader<FieldSize>;

ObjectArrayReader::ObjectArrayReader(const ObjectReader::CacheEntry& entry, bool name_based) noexcept
    : ArrayReader<FieldSize>(entry.value.ptr),
      m_name_based(name_based) {
    if (entry.type != DataType::ObjectArray) {
        Invalidate();
    }
}

std::optional<ObjectReader> ObjectArrayReader::GetElement(uint32_t index) const noexcept {
    const void* element_ptr;
    if (!ArrayReader<FieldSize>::GetElement(index, element_ptr)) {
        return std::nullopt;
    }
    return std::make_optional<ObjectReader>(element_ptr, m_name_based);
}

StringArrayReader::StringArrayReader(const ObjectReader::CacheEntry& entry) noexcept
    : ArrayReader<uint16_t>(entry.value.ptr) {
    if (entry.type != DataType::StringArray) {
        Invalidate();
    }
}

bool StringArrayReader::GetElement(uint32_t index, std::string_view& out_value) const noexcept {
    const void* element_ptr;
    uint16_t element_size;

    if (!ArrayReader<uint16_t>::GetElement(index, element_ptr, &element_size)) {
        return false;
    }

    out_value = std::string_view(reinterpret_cast<const char*>(element_ptr) + sizeof(element_size), element_size);

    return true;
}

BinaryArrayReader::BinaryArrayReader(const ObjectReader::CacheEntry& entry) noexcept
    : ArrayReader<FieldSize>(entry.value.ptr) {
    if (entry.type != DataType::BinaryArray) {
        Invalidate();
    }
}

bool BinaryArrayReader::GetElement(uint32_t index, const void*& out_data, FieldSize& out_size) const noexcept {
    return ArrayReader<FieldSize>::GetElement(index, out_data, &out_size);
}

// ---------------------------------
// Array reader iterators
// ---------------------------------

template <typename ElementSizeType>
    requires std::is_integral<ElementSizeType>::value
ArrayReader<ElementSizeType>::BaseIterator::BaseIterator(const void* array, uint32_t index, bool at_end) noexcept
    : m_index(index) {
    const uint8_t* read_ptr = static_cast<const uint8_t*>(array);

    FieldSize array_size = GetArraySize(array);
    read_ptr += sizeof(FieldSize);
    m_end_ptr = read_ptr + array_size;

    if (at_end) {
        m_current_ptr = m_end_ptr;
        return;
    }

    m_current_ptr = read_ptr;

    // Advance to the correct index
    for (uint32_t i = 0; i < index; ++i) {
        Advance();
    }
}

template <typename ElementSizeType>
    requires std::is_integral<ElementSizeType>::value
void ArrayReader<ElementSizeType>::BaseIterator::Advance() noexcept {
    // Simple advancement - array was already validated
    ElementSizeType element_size;
    std::memcpy(&element_size, m_current_ptr, sizeof(element_size));
    AdjustEndianess(element_size);
    m_current_ptr += sizeof(element_size) + element_size;
    m_index++;
}

template <typename ElementSizeType>
    requires std::is_integral<ElementSizeType>::value
const void* ArrayReader<ElementSizeType>::BaseIterator::CurrentElement(ElementSizeType* out_size) const noexcept {
    if (out_size) {
        std::memcpy(out_size, m_current_ptr, sizeof(ElementSizeType));
        AdjustEndianess(*out_size);
    }
    return m_current_ptr;
}

std::string_view StringArrayReader::Iterator::operator*() const noexcept {
    uint16_t size;
    const void* ptr = this->CurrentElement(&size);
    return std::string_view(reinterpret_cast<const char*>(ptr) + sizeof(size), size);
}

std::span<const uint8_t> BinaryArrayReader::Iterator::operator*() const noexcept {
    FieldSize size;
    const void* ptr = this->CurrentElement(&size);
    const uint8_t* data_ptr = static_cast<const uint8_t*>(ptr) + sizeof(size);
    return std::span<const uint8_t>(data_ptr, size);
}

ObjectReader ObjectArrayReader::Iterator::operator*() const noexcept {
    const void* ptr = this->CurrentElement();
    return ObjectReader(ptr, m_name_based);
}

}  // namespace tbf