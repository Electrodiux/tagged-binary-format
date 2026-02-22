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
        Invalidate();
        return;
    }

    std::memcpy(&m_size, buffer, sizeof(FieldSize));
    m_buffer = static_cast<const uint8_t*>(buffer) + sizeof(FieldSize);

    if (name_based) {
        new (&m_name_cache) std::unordered_map<std::string_view, CacheEntry>();
    } else {
        new (&m_id_cache) std::unordered_map<DataTag::Id, CacheEntry>();
    }
}

ObjectReader::~ObjectReader() noexcept {
    if (m_name_based) {
        m_name_cache.~unordered_map();
    } else {
        m_id_cache.~unordered_map();
    }
}

// ---------------------------------
// Memory checking helpers
// ---------------------------------

[[gnu::always_inline]]
static inline bool CanAccessBuffer(const void* beg, const void* end, size_t size) noexcept {
    return static_cast<size_t>(static_cast<const uint8_t*>(end) - static_cast<const uint8_t*>(beg)) >= size;
}

template <typename Type, bool swap_endianess = true>
[[gnu::always_inline]]
static inline bool ReadData(const uint8_t*& read_ptr, const uint8_t* end_ptr, Type& out_value) noexcept {
    if (CanAccessBuffer(read_ptr, end_ptr, sizeof(Type))) [[likely]] {
        std::memcpy(&out_value, read_ptr, sizeof(Type));
        read_ptr += sizeof(Type);

        if constexpr (swap_endianess) {
            AdjustEndianess(out_value);
        }

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

    if (m_buffer == nullptr || m_size == 0) [[unlikely]] {
        m_is_valid = false;
        m_cache_built = true;
        return;
    }

    if (m_name_based) {
        m_name_cache.clear();
        m_name_cache.reserve(initial_size);
    } else {
        m_id_cache.clear();
        m_id_cache.reserve(initial_size);
    }

    const uint8_t* read_ptr = static_cast<const uint8_t*>(m_buffer);
    const uint8_t* buff_end = static_cast<const uint8_t*>(m_buffer) + m_size;

    // bool terminated = false;
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
                case DataType::UInt8:
                case DataType::Int8:
                    if (!ReadData<int8_t>(read_ptr, buff_end, entry.value.v_int8)) [[unlikely]] {
                        errors = true;
                    }
                    break;
                case DataType::Float16:
                case DataType::UInt16:
                case DataType::Int16:
                    if (!ReadData<int16_t>(read_ptr, buff_end, entry.value.v_int16)) [[unlikely]] {
                        errors = true;
                    }
                    break;
                case DataType::Float32:
                case DataType::UInt32:
                case DataType::Int32:
                    if (!ReadData<int32_t>(read_ptr, buff_end, entry.value.v_int32)) [[unlikely]] {
                        errors = true;
                    }
                    break;
                case DataType::Float64:
                case DataType::UInt64:
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
            m_name_cache.emplace(tag_name, entry);
        } else {
            DataTag::Id tag_id;
            std::memcpy(&tag_id, tag_ptr, sizeof(tag_id));
            m_id_cache.emplace(tag_id, entry);
        }
    }

    m_cache_built = true;
    m_is_valid = !errors && read_ptr == buff_end;
}

[[gnu::always_inline]]
inline bool ObjectReader::FindTag(const DataTag& tag, CacheEntry& out_entry) const noexcept {
    if (!IsValid()) [[unlikely]] {
        return false;
    }

    if (m_name_based) {
        auto it = m_name_cache.find(tag.GetName());
        if (it != m_name_cache.end()) [[likely]] {
            out_entry = it->second;
            return true;
        }
    } else {
        auto it = m_id_cache.find(tag.GetId());
        if (it != m_id_cache.end()) [[likely]] {
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
        for (const auto& [key, entry] : m_name_cache) {
            tags.emplace_back(key);
        }
    } else {
        for (const auto& [key, entry] : m_id_cache) {
            tags.emplace_back(key);
        }
    }

    return tags;
}

// ---------------------------------
// Read methods
// ---------------------------------

template <typename Type, DataType expected_type>
inline bool ObjectReader::ReadPrimitive(const DataTag& tag, Type& out_value) const noexcept {
    CacheEntry entry;
    if (!FindTag(tag, entry) || entry.type != expected_type) {
        return false;
    }

    std::memcpy(&out_value, &entry.value, sizeof(Type));

    return true;
}

[[gnu::always_inline]]
inline const void* ObjectReader::ReadPointerData(const DataTag& tag, DataType expected_type, FieldSize& out_size) const noexcept {
    CacheEntry entry;
    if (!FindTag(tag, entry) || entry.type != expected_type) {
        return nullptr;
    }

    const uint8_t* value_ptr = static_cast<const uint8_t*>(entry.value.ptr);

    std::memcpy(&out_size, value_ptr, sizeof(out_size));
    value_ptr += sizeof(out_size);

    return value_ptr;
}

bool ObjectReader::ReadInt8(const DataTag& tag, int8_t& out_value) const noexcept {
    return ReadPrimitive<int8_t, DataType::Int8>(tag, out_value);
}

bool ObjectReader::ReadInt16(const DataTag& tag, int16_t& out_value) const noexcept {
    return ReadPrimitive<int16_t, DataType::Int16>(tag, out_value);
}

bool ObjectReader::ReadInt32(const DataTag& tag, int32_t& out_value) const noexcept {
    return ReadPrimitive<int32_t, DataType::Int32>(tag, out_value);
}

bool ObjectReader::ReadInt64(const DataTag& tag, int64_t& out_value) const noexcept {
    return ReadPrimitive<int64_t, DataType::Int64>(tag, out_value);
}

bool ObjectReader::ReadUInt8(const DataTag& tag, uint8_t& out_value) const noexcept {
    return ReadPrimitive<uint8_t, DataType::UInt8>(tag, out_value);
}

bool ObjectReader::ReadUInt16(const DataTag& tag, uint16_t& out_value) const noexcept {
    return ReadPrimitive<uint16_t, DataType::UInt16>(tag, out_value);
}

bool ObjectReader::ReadUInt32(const DataTag& tag, uint32_t& out_value) const noexcept {
    return ReadPrimitive<uint32_t, DataType::UInt32>(tag, out_value);
}

bool ObjectReader::ReadUInt64(const DataTag& tag, uint64_t& out_value) const noexcept {
    return ReadPrimitive<uint64_t, DataType::UInt64>(tag, out_value);
}

bool ObjectReader::ReadBoolean(const DataTag& tag, bool& out_value) const noexcept {
    return ReadPrimitive<bool, DataType::Boolean>(tag, out_value);
}

bool ObjectReader::ReadFloat16(const DataTag& tag, uint16_t& out_value) const noexcept {
    return ReadPrimitive<uint16_t, DataType::Float16>(tag, out_value);
}

bool ObjectReader::ReadFloat32(const DataTag& tag, float& out_value) const noexcept {
    return ReadPrimitive<float, DataType::Float32>(tag, out_value);
}

bool ObjectReader::ReadFloat64(const DataTag& tag, double& out_value) const noexcept {
    return ReadPrimitive<double, DataType::Float64>(tag, out_value);
}

bool ObjectReader::ReadString(const DataTag& tag, std::string_view& out_value) const noexcept {
    CacheEntry entry;
    if (!FindTag(tag, entry)) {
        return false;
    }
    return ReadStringInternal(entry, out_value);
}

const void* ObjectReader::ReadBinary(const DataTag& tag, FieldSize& out_size) const noexcept {
    return ReadPointerData(tag, DataType::Binary, out_size);
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
    return ReadObjectInternal(entry);
}

bool ObjectReader::ReadStringInternal(const CacheEntry& entry, std::string_view& out_value) const noexcept {
    if (entry.type != DataType::String) [[unlikely]] {
        return false;
    }

    const uint8_t* value_ptr = static_cast<const uint8_t*>(entry.value.ptr);

    uint16_t length;
    std::memcpy(&length, value_ptr, sizeof(length));

    const char* str_ptr = reinterpret_cast<const char*>(value_ptr + sizeof(length));
    out_value = std::string_view(str_ptr, length);

    return true;
}

std::optional<ObjectReader> ObjectReader::ReadObjectInternal(const CacheEntry& entry) const noexcept {
    if (entry.type != DataType::Object) [[unlikely]] {
        return std::nullopt;
    }
    return std::make_optional<ObjectReader>(entry.value.ptr, m_name_based);
}

// ---------------------------------
// Read arrays
// ---------------------------------

template <typename Type, DataType expected_type>
[[gnu::always_inline]]
inline const Type* ObjectReader::ReadArray(const DataTag& tag, uint32_t& out_length) const noexcept {
    FieldSize out_size;
    const void* value_ptr = ReadPointerData(tag, expected_type, out_size);

    if (value_ptr != nullptr) {
        constexpr uint32_t element_size = DataTypeSize(BaseDataType(expected_type));
        uint32_t array_length = out_size / element_size;

        if (array_length * element_size != out_size) [[unlikely]] {
            out_length = 0;
            return nullptr;
        }

        out_length = array_length;
        return reinterpret_cast<const Type*>(value_ptr);
    }

    out_length = 0;
    return nullptr;
}

const int8_t* ObjectReader::ReadInt8Array(const DataTag& tag, uint32_t& out_length) const noexcept {
    return ReadArray<int8_t, DataType::Int8Array>(tag, out_length);
}

const int16_t* ObjectReader::ReadInt16Array(const DataTag& tag, uint32_t& out_length) const noexcept {
    return ReadArray<int16_t, DataType::Int16Array>(tag, out_length);
}

const int32_t* ObjectReader::ReadInt32Array(const DataTag& tag, uint32_t& out_length) const noexcept {
    return ReadArray<int32_t, DataType::Int32Array>(tag, out_length);
}

const int64_t* ObjectReader::ReadInt64Array(const DataTag& tag, uint32_t& out_length) const noexcept {
    return ReadArray<int64_t, DataType::Int64Array>(tag, out_length);
}

const uint8_t* ObjectReader::ReadUInt8Array(const DataTag& tag, uint32_t& out_length) const noexcept {
    return ReadArray<uint8_t, DataType::UInt8Array>(tag, out_length);
}

const uint16_t* ObjectReader::ReadUInt16Array(const DataTag& tag, uint32_t& out_length) const noexcept {
    return ReadArray<uint16_t, DataType::UInt16Array>(tag, out_length);
}

const uint32_t* ObjectReader::ReadUInt32Array(const DataTag& tag, uint32_t& out_length) const noexcept {
    return ReadArray<uint32_t, DataType::UInt32Array>(tag, out_length);
}

const uint64_t* ObjectReader::ReadUInt64Array(const DataTag& tag, uint32_t& out_length) const noexcept {
    return ReadArray<uint64_t, DataType::UInt64Array>(tag, out_length);
}

const bool* ObjectReader::ReadBooleanArray(const DataTag& tag, uint32_t& out_length) const noexcept {
    return ReadArray<bool, DataType::BooleanArray>(tag, out_length);
}

const uint16_t* ObjectReader::ReadFloat16Array(const DataTag& tag, uint32_t& out_length) const noexcept {
    return ReadArray<uint16_t, DataType::Float16Array>(tag, out_length);
}

const float* ObjectReader::ReadFloat32Array(const DataTag& tag, uint32_t& out_length) const noexcept {
    return ReadArray<float, DataType::Float32Array>(tag, out_length);
}

const double* ObjectReader::ReadFloat64Array(const DataTag& tag, uint32_t& out_length) const noexcept {
    return ReadArray<double, DataType::Float64Array>(tag, out_length);
}

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
// Read array as std::span methods
// ---------------------------------

template <typename Type, DataType expected_type>
[[gnu::always_inline]]
inline std::span<const Type> ObjectReader::ReadArray(const DataTag& tag) const noexcept {
    uint32_t length;
    const Type* data = ReadArray<Type, expected_type>(tag, length);
    return data ? std::span<const Type>(data, length) : std::span<const Type>();
}

std::span<const int8_t> ObjectReader::ReadInt8Array(const DataTag& tag) const noexcept {
    return ReadArray<int8_t, DataType::Int8Array>(tag);
}

std::span<const int16_t> ObjectReader::ReadInt16Array(const DataTag& tag) const noexcept {
    return ReadArray<int16_t, DataType::Int16Array>(tag);
}

std::span<const int32_t> ObjectReader::ReadInt32Array(const DataTag& tag) const noexcept {
    return ReadArray<int32_t, DataType::Int32Array>(tag);
}

std::span<const int64_t> ObjectReader::ReadInt64Array(const DataTag& tag) const noexcept {
    return ReadArray<int64_t, DataType::Int64Array>(tag);
}

std::span<const uint8_t> ObjectReader::ReadUInt8Array(const DataTag& tag) const noexcept {
    return ReadArray<uint8_t, DataType::UInt8Array>(tag);
}

std::span<const uint16_t> ObjectReader::ReadUInt16Array(const DataTag& tag) const noexcept {
    return ReadArray<uint16_t, DataType::UInt16Array>(tag);
}

std::span<const uint32_t> ObjectReader::ReadUInt32Array(const DataTag& tag) const noexcept {
    return ReadArray<uint32_t, DataType::UInt32Array>(tag);
}

std::span<const uint64_t> ObjectReader::ReadUInt64Array(const DataTag& tag) const noexcept {
    return ReadArray<uint64_t, DataType::UInt64Array>(tag);
}

std::span<const bool> ObjectReader::ReadBooleanArray(const DataTag& tag) const noexcept {
    return ReadArray<bool, DataType::BooleanArray>(tag);
}

std::span<const uint16_t> ObjectReader::ReadFloat16Array(const DataTag& tag) const noexcept {
    return ReadArray<uint16_t, DataType::Float16Array>(tag);
}

std::span<const float> ObjectReader::ReadFloat32Array(const DataTag& tag) const noexcept {
    return ReadArray<float, DataType::Float32Array>(tag);
}

std::span<const double> ObjectReader::ReadFloat64Array(const DataTag& tag) const noexcept {
    return ReadArray<double, DataType::Float64Array>(tag);
}

// ---------------------------------
// Read vectors
// ---------------------------------

template <typename Type, uint32_t dim>
    requires std::is_arithmetic<Type>::value && (dim >= 2) && (dim <= 4)
[[gnu::always_inline]]
inline Type* ObjectReader::ReadVector(const DataTag& tag, DataType type) const noexcept {
    CacheEntry entry;
    if (!FindTag(tag, entry) || entry.type != type) {
        return nullptr;
    }
    return reinterpret_cast<Type*>(const_cast<void*>(entry.value.ptr));
}

// Vector 2

int8_t* ObjectReader::ReadVector2i8(const DataTag& tag) const noexcept {
    return ReadVector<int8_t, 2>(tag, DataType::Vector2i8);
}

int16_t* ObjectReader::ReadVector2i16(const DataTag& tag) const noexcept {
    return ReadVector<int16_t, 2>(tag, DataType::Vector2i16);
}

int32_t* ObjectReader::ReadVector2i32(const DataTag& tag) const noexcept {
    return ReadVector<int32_t, 2>(tag, DataType::Vector2i32);
}

int64_t* ObjectReader::ReadVector2i64(const DataTag& tag) const noexcept {
    return ReadVector<int64_t, 2>(tag, DataType::Vector2i64);
}

bool* ObjectReader::ReadVector2b(const DataTag& tag) const noexcept {
    return ReadVector<bool, 2>(tag, DataType::Vector2b);
}

uint16_t* ObjectReader::ReadVector2f16(const DataTag& tag) const noexcept {
    return ReadVector<uint16_t, 2>(tag, DataType::Vector2f16);
}

float* ObjectReader::ReadVector2f32(const DataTag& tag) const noexcept {
    return ReadVector<float, 2>(tag, DataType::Vector2f32);
}

double* ObjectReader::ReadVector2f64(const DataTag& tag) const noexcept {
    return ReadVector<double, 2>(tag, DataType::Vector2f64);
}

// Vector 3

int8_t* ObjectReader::ReadVector3i8(const DataTag& tag) const noexcept {
    return ReadVector<int8_t, 3>(tag, DataType::Vector3i8);
}

int16_t* ObjectReader::ReadVector3i16(const DataTag& tag) const noexcept {
    return ReadVector<int16_t, 3>(tag, DataType::Vector3i16);
}

int32_t* ObjectReader::ReadVector3i32(const DataTag& tag) const noexcept {
    return ReadVector<int32_t, 3>(tag, DataType::Vector3i32);
}

int64_t* ObjectReader::ReadVector3i64(const DataTag& tag) const noexcept {
    return ReadVector<int64_t, 3>(tag, DataType::Vector3i64);
}

bool* ObjectReader::ReadVector3b(const DataTag& tag) const noexcept {
    return ReadVector<bool, 3>(tag, DataType::Vector3b);
}

uint16_t* ObjectReader::ReadVector3f16(const DataTag& tag) const noexcept {
    return ReadVector<uint16_t, 3>(tag, DataType::Vector3f16);
}

float* ObjectReader::ReadVector3f32(const DataTag& tag) const noexcept {
    return ReadVector<float, 3>(tag, DataType::Vector3f32);
}

double* ObjectReader::ReadVector3f64(const DataTag& tag) const noexcept {
    return ReadVector<double, 3>(tag, DataType::Vector3f64);
}

// Vector 4

int8_t* ObjectReader::ReadVector4i8(const DataTag& tag) const noexcept {
    return ReadVector<int8_t, 4>(tag, DataType::Vector4i8);
}

int16_t* ObjectReader::ReadVector4i16(const DataTag& tag) const noexcept {
    return ReadVector<int16_t, 4>(tag, DataType::Vector4i16);
}

int32_t* ObjectReader::ReadVector4i32(const DataTag& tag) const noexcept {
    return ReadVector<int32_t, 4>(tag, DataType::Vector4i32);
}

int64_t* ObjectReader::ReadVector4i64(const DataTag& tag) const noexcept {
    return ReadVector<int64_t, 4>(tag, DataType::Vector4i64);
}

bool* ObjectReader::ReadVector4b(const DataTag& tag) const noexcept {
    return ReadVector<bool, 4>(tag, DataType::Vector4b);
}

uint16_t* ObjectReader::ReadVector4f16(const DataTag& tag) const noexcept {
    return ReadVector<uint16_t, 4>(tag, DataType::Vector4f16);
}

float* ObjectReader::ReadVector4f32(const DataTag& tag) const noexcept {
    return ReadVector<float, 4>(tag, DataType::Vector4f32);
}

double* ObjectReader::ReadVector4f64(const DataTag& tag) const noexcept {
    return ReadVector<double, 4>(tag, DataType::Vector4f64);
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

static inline FieldSize GetArraySize(const void* array) noexcept {
    const uint8_t* read_ptr = reinterpret_cast<const uint8_t*>(array);

    FieldSize array_size;
    std::memcpy(&array_size, read_ptr, sizeof(array_size));

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

bool BinaryArrayReader::GetElement(uint32_t index, const void*& out_ptr, FieldSize& out_size) const noexcept {
    return ArrayReader<FieldSize>::GetElement(index, out_ptr, &out_size);
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
    m_current_ptr += sizeof(element_size) + element_size;
    m_index++;

    if (m_current_ptr < m_end_ptr) {
        __builtin_prefetch(m_current_ptr, 0, 3);
    }
}

template <typename ElementSizeType>
    requires std::is_integral<ElementSizeType>::value
const void* ArrayReader<ElementSizeType>::BaseIterator::CurrentElement(ElementSizeType* out_size) const noexcept {
    if (out_size) {
        std::memcpy(out_size, m_current_ptr, sizeof(ElementSizeType));
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