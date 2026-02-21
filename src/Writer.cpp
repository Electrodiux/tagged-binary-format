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

#include "tbf/Writer.hpp"

#include "tbf/DataTag.hpp"
#include "tbf/DataType.hpp"

#include <cstdint>
#include <string_view>

namespace tbf {

// ---------------------------------
// Constructors & Destructor
// ---------------------------------

Writer::Writer(bool name_based, uint32_t buff_grow_size) noexcept
    : m_name_based(name_based),
      m_root_object(*this) {
    SetBufferGrowSize(buff_grow_size);
    m_buffer.reserve(m_buffer_grow_size);
}

void Writer::SetBufferGrowSize(uint32_t grow_size) noexcept {
    if (grow_size > MIN_BUFFER_GROW_SIZE) {
        m_buffer_grow_size = grow_size;
    } else {
        m_buffer_grow_size = MIN_BUFFER_GROW_SIZE;
    }
}

// ---------------------------------
// Writing methods
// ---------------------------------

[[gnu::always_inline]]
inline void Writer::ReserveBuffer(size_t size) noexcept {
    if (m_buffer.capacity() - m_buffer.size() < size) [[unlikely]] {
        size_t reserve_space = m_buffer_grow_size;

        if (size > reserve_space) [[unlikely]] {
            reserve_space = size + m_buffer_grow_size;
        }

        m_buffer.reserve(m_buffer.capacity() + reserve_space);
    }
}

[[gnu::always_inline]]
inline BufferOffset Writer::WriteData(const void* data, size_t size) noexcept {
    const uint8_t* byte_data = static_cast<const uint8_t*>(data);
    BufferOffset offset = m_buffer.size();
    ReserveBuffer(size);
    m_buffer.insert(m_buffer.end(), byte_data, byte_data + size);
    return offset;
}

template <typename Type, bool swap_endianess>
inline void Writer::WriteData(Type value) noexcept {
    if constexpr (sizeof(Type) > 1) {
        if constexpr (swap_endianess) {
            AdjustEndianess(value);
        }
        const uint8_t* byte_data = reinterpret_cast<const uint8_t*>(&value);
        ReserveBuffer(sizeof(Type));
        m_buffer.insert(m_buffer.end(), byte_data, byte_data + sizeof(Type));
    } else {
        ReserveBuffer(1);
        m_buffer.push_back(static_cast<uint8_t>(value));
    }
}

inline void Writer::WriteFieldHeader(const DataTag& tag, DataType type) noexcept {
    // Write type
    WriteData<DataType>(type);

    if (m_name_based) {
        // Write tag name
        WriteData<DataTag::NameSize>(static_cast<DataTag::NameSize>(tag.GetName().size()));
        ReserveBuffer(tag.GetName().size());
        m_buffer.insert(m_buffer.end(), tag.GetName().begin(), tag.GetName().end());
    } else {
        // Write tag ID
        WriteData<DataTag::Id>(tag.GetId());
    }
}

[[gnu::always_inline]]
inline BufferOffset Writer::ReserveDataSizeField() noexcept {
    BufferOffset offset = m_buffer.size();
    ReserveBuffer(sizeof(FieldSize));
    m_buffer.insert(m_buffer.end(), sizeof(FieldSize), 0);
    return offset;
}

[[gnu::always_inline]]
inline void Writer::WriteDataSizeField(BufferOffset offset) noexcept {
    FieldSize size = static_cast<FieldSize>(m_buffer.size() - offset - sizeof(FieldSize));

    AdjustEndianess(size);

    std::memcpy(m_buffer.data() + offset, &size, sizeof(size));
}

[[gnu::always_inline]]
inline void* Writer::GetBufferPointer(BufferOffset offset) noexcept {
    return m_buffer.data() + offset;
}

[[gnu::always_inline]]
inline void Writer::WriteString(const std::string_view& str) noexcept {
    const uint16_t length = static_cast<uint16_t>(str.size());
    WriteData<uint16_t>(length);
    WriteData(str.data(), length);
}

[[gnu::always_inline]]
inline void Writer::WriteBinary(const void* data, FieldSize size) noexcept {
    WriteData<FieldSize>(size);
    WriteData(data, size);
}

// ---------------------------------
// ObjectWriter
// ---------------------------------

ObjectWriter::ObjectWriter(Writer& writer) noexcept
    : m_writer(writer),
      m_is_finished(false) {
    m_obj_size_pos = writer.ReserveDataSizeField();
}

void ObjectWriter::Finish() noexcept {
    if (!IsFinished()) {
        m_writer.WriteDataSizeField(m_obj_size_pos);
        m_is_finished = true;
    }
}

// ---------------------------------
// Field methods
// ---------------------------------

void ObjectWriter::FieldInt8(const DataTag& tag, int8_t value) noexcept {
    m_writer.WriteFieldHeader(tag, DataType::Int8);
    m_writer.WriteData<int8_t>(value);
}

void ObjectWriter::FieldInt16(const DataTag& tag, int16_t value) noexcept {
    m_writer.WriteFieldHeader(tag, DataType::Int16);
    m_writer.WriteData<int16_t>(value);
}

void ObjectWriter::FieldInt32(const DataTag& tag, int32_t value) noexcept {
    m_writer.WriteFieldHeader(tag, DataType::Int32);
    m_writer.WriteData<int32_t>(value);
}

void ObjectWriter::FieldInt64(const DataTag& tag, int64_t value) noexcept {
    m_writer.WriteFieldHeader(tag, DataType::Int64);
    m_writer.WriteData<int64_t>(value);
}

void ObjectWriter::FieldUInt8(const DataTag& tag, uint8_t value) noexcept {
    m_writer.WriteFieldHeader(tag, DataType::UInt8);
    m_writer.WriteData<uint8_t>(value);
}

void ObjectWriter::FieldUInt16(const DataTag& tag, uint16_t value) noexcept {
    m_writer.WriteFieldHeader(tag, DataType::UInt16);
    m_writer.WriteData<uint16_t>(value);
}

void ObjectWriter::FieldUInt32(const DataTag& tag, uint32_t value) noexcept {
    m_writer.WriteFieldHeader(tag, DataType::UInt32);
    m_writer.WriteData<uint32_t>(value);
}

void ObjectWriter::FieldUInt64(const DataTag& tag, uint64_t value) noexcept {
    m_writer.WriteFieldHeader(tag, DataType::UInt64);
    m_writer.WriteData<uint64_t>(value);
}

void ObjectWriter::FieldBoolean(const DataTag& tag, bool value) noexcept {
    m_writer.WriteFieldHeader(tag, DataType::Boolean);
    m_writer.WriteData<bool>(value);
}

void ObjectWriter::FieldFloat16(const DataTag& tag, uint16_t value) noexcept {
    m_writer.WriteFieldHeader(tag, DataType::Float16);
    m_writer.WriteData<uint16_t, false>(value);
}

void ObjectWriter::FieldFloat32(const DataTag& tag, float value) noexcept {
    m_writer.WriteFieldHeader(tag, DataType::Float32);
    m_writer.WriteData<uint32_t, false>(std::bit_cast<uint32_t>(value));
}

void ObjectWriter::FieldFloat64(const DataTag& tag, double value) noexcept {
    m_writer.WriteFieldHeader(tag, DataType::Float64);
    m_writer.WriteData<uint64_t, false>(std::bit_cast<uint64_t>(value));
}

void ObjectWriter::FieldString(const DataTag& tag, std::string_view value) noexcept {
    m_writer.WriteFieldHeader(tag, DataType::String);
    m_writer.WriteString(value);
}

void ObjectWriter::FieldBinary(const DataTag& tag, const void* data, size_t size) noexcept {
    m_writer.WriteFieldHeader(tag, DataType::Binary);
    m_writer.WriteBinary(data, size);
}

ObjectWriter ObjectWriter::FieldObject(const DataTag& tag) noexcept {
    m_writer.WriteFieldHeader(tag, DataType::Object);
    return ObjectWriter(m_writer);
}

// ---------------------------------
// Array field methods
// ---------------------------------

template <typename Type>
[[gnu::always_inline]]
inline void ObjectWriter::FieldArray(const DataTag& tag, DataType array_type, const Type* data, uint32_t length) noexcept {
    m_writer.WriteFieldHeader(tag, array_type);

    // Write array length and array data
    FieldSize size = length * sizeof(Type);
    m_writer.WriteData<FieldSize>(size);
    BufferOffset offset = m_writer.WriteData(data, size);

    AdjustArrayEndianess(reinterpret_cast<Type*>(m_writer.GetBufferPointer(offset)), length);
}

void ObjectWriter::FieldArrayInt8(const DataTag& tag, const int8_t* data, uint32_t length) noexcept {
    FieldArray<int8_t>(tag, DataType::Int8Array, data, length);
}

void ObjectWriter::FieldArrayInt16(const DataTag& tag, const int16_t* data, uint32_t length) noexcept {
    FieldArray<int16_t>(tag, DataType::Int16Array, data, length);
}

void ObjectWriter::FieldArrayInt32(const DataTag& tag, const int32_t* data, uint32_t length) noexcept {
    FieldArray<int32_t>(tag, DataType::Int32Array, data, length);
}

void ObjectWriter::FieldArrayInt64(const DataTag& tag, const int64_t* data, uint32_t length) noexcept {
    FieldArray<int64_t>(tag, DataType::Int64Array, data, length);
}

void ObjectWriter::FieldArrayUInt8(const DataTag& tag, const uint8_t* data, uint32_t length) noexcept {
    FieldArray<uint8_t>(tag, DataType::UInt8Array, data, length);
}

void ObjectWriter::FieldArrayUInt16(const DataTag& tag, const uint16_t* data, uint32_t length) noexcept {
    FieldArray<uint16_t>(tag, DataType::UInt16Array, data, length);
}

void ObjectWriter::FieldArrayUInt32(const DataTag& tag, const uint32_t* data, uint32_t length) noexcept {
    FieldArray<uint32_t>(tag, DataType::UInt32Array, data, length);
}

void ObjectWriter::FieldArrayUInt64(const DataTag& tag, const uint64_t* data, uint32_t length) noexcept {
    FieldArray<uint64_t>(tag, DataType::UInt64Array, data, length);
}

void ObjectWriter::FieldArrayBoolean(const DataTag& tag, const bool* data, uint32_t length) noexcept {
    FieldArray<bool>(tag, DataType::BooleanArray, data, length);
}

void ObjectWriter::FieldArrayFloat16(const DataTag& tag, const uint16_t* data, uint32_t length) noexcept {
    FieldArray<uint16_t>(tag, DataType::Float16Array, data, length);
}

void ObjectWriter::FieldArrayFloat32(const DataTag& tag, const float* data, uint32_t length) noexcept {
    FieldArray<uint32_t>(tag, DataType::Float32Array, reinterpret_cast<const uint32_t*>(data), length);
}

void ObjectWriter::FieldArrayFloat64(const DataTag& tag, const double* data, uint32_t length) noexcept {
    FieldArray<uint64_t>(tag, DataType::Float64Array, reinterpret_cast<const uint64_t*>(data), length);
}

StringArrayWriter ObjectWriter::FieldStringArray(const DataTag& tag) noexcept {
    m_writer.WriteFieldHeader(tag, DataType::StringArray);
    return StringArrayWriter(*this);
}

void ObjectWriter::FieldStringArray(const DataTag& tag, const std::string_view* data, uint32_t length) noexcept {
    m_writer.WriteFieldHeader(tag, DataType::StringArray);

    // Write array size
    size_t offset = m_writer.ReserveDataSizeField();

    // Write each string in the array
    for (uint32_t i = 0; i < length; ++i) {
        m_writer.WriteString(data[i]);
    }

    m_writer.WriteDataSizeField(offset);
}

BinaryArrayWriter ObjectWriter::FieldBinaryArray(const DataTag& tag) noexcept {
    m_writer.WriteFieldHeader(tag, DataType::BinaryArray);
    return BinaryArrayWriter(*this);
}

void ObjectWriter::FieldBinaryArray(const DataTag& tag, const void* const* data, const uint32_t* sizes, uint32_t length) noexcept {
    m_writer.WriteFieldHeader(tag, DataType::BinaryArray);

    // Write array size
    size_t offset = m_writer.ReserveDataSizeField();

    // Write each binary blob in the array
    for (uint32_t i = 0; i < length; ++i) {
        m_writer.WriteBinary(data[i], sizes[i]);
    }

    m_writer.WriteDataSizeField(offset);
}

ObjectArrayWriter ObjectWriter::FieldObjectArray(const DataTag& tag) noexcept {
    m_writer.WriteFieldHeader(tag, DataType::ObjectArray);
    return ObjectArrayWriter(*this);
}

// ---------------------------------
// ArrayWriter
// ---------------------------------

ArrayWriter::ArrayWriter(ObjectWriter& obj) noexcept
    : m_obj(obj),
      m_is_finished(false) {
    m_array_size_pos = obj.GetWriter().ReserveDataSizeField();
}

void ArrayWriter::Finish() noexcept {
    if (!IsFinished()) [[unlikely]] {
        m_obj.GetWriter().WriteDataSizeField(m_array_size_pos);
        m_is_finished = true;
    }
}

void StringArrayWriter::AddElement(std::string_view element) noexcept {
    m_obj.GetWriter().WriteString(element);
}

void BinaryArrayWriter::AddElement(const void* element, FieldSize size) noexcept {
    m_obj.GetWriter().WriteBinary(element, size);
}

ObjectWriter ObjectArrayWriter::CreateElement() noexcept {
    return ObjectWriter(m_obj.GetWriter());
}

}  // namespace tbf