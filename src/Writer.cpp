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
#include "tbf/Endianness.hpp"

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

void Writer::ReserveBuffer(size_t size) noexcept {
    if (m_buffer.capacity() - m_buffer.size() < size) [[unlikely]] {
        size_t reserve_space = m_buffer_grow_size;

        if (size > reserve_space) [[unlikely]] {
            reserve_space = size + m_buffer_grow_size;
        }

        m_buffer.reserve(m_buffer.capacity() + reserve_space);
    }
}

BufferOffset Writer::WriteData(const void* data, size_t size) noexcept {
    const uint8_t* byte_data = static_cast<const uint8_t*>(data);
    BufferOffset offset = m_buffer.size();
    ReserveBuffer(size);
    m_buffer.insert(m_buffer.end(), byte_data, byte_data + size);
    return offset;
}

void Writer::WriteFieldHeader(const DataTag& tag, DataType type) noexcept {
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

BufferOffset Writer::ReserveDataSizeField() noexcept {
    BufferOffset offset = m_buffer.size();
    ReserveBuffer(sizeof(FieldSize));
    m_buffer.insert(m_buffer.end(), sizeof(FieldSize), 0);
    return offset;
}

void Writer::WriteDataSizeField(BufferOffset offset) noexcept {
    FieldSize size = static_cast<FieldSize>(m_buffer.size() - offset - sizeof(FieldSize));

    AdjustEndianess(size);

    std::memcpy(m_buffer.data() + offset, &size, sizeof(size));
}

void* Writer::GetBufferPointer(BufferOffset offset) noexcept {
    return m_buffer.data() + offset;
}

void Writer::WriteString(std::string_view str) noexcept {
    const uint16_t length = static_cast<uint16_t>(str.size());
    WriteData<uint16_t>(length);
    WriteData(str.data(), length);
}

void Writer::WriteBinary(std::span<const uint8_t> data) noexcept {
    WriteData<FieldSize>(static_cast<FieldSize>(data.size()));
    WriteData(data.data(), data.size());
}

// ---------------------------------
// ObjectWriter
// ---------------------------------

ObjectWriter::ObjectWriter(Writer& writer) noexcept
    : m_writer(writer),
      m_is_finished(false) {
    m_obj_size_pos = writer.ReserveDataSizeField();
}

// RAII: move constructor
ObjectWriter::ObjectWriter(ObjectWriter&& other) noexcept
    : m_writer(other.m_writer),
      m_obj_size_pos(other.m_obj_size_pos),
      m_is_finished(other.m_is_finished) {
    // Mark the source as finished so it doesn't write the size again
    other.m_is_finished = true;
}

// RAII: move assignment
ObjectWriter& ObjectWriter::operator=(ObjectWriter&& other) noexcept {
    if (this != &other) {
        // Finish our current object first if needed
        if (!m_is_finished) {
            Finish();
        }
        // This shouldn't normally happen as we can't reassign references
        // But we handle the move semantics properly
        m_obj_size_pos = other.m_obj_size_pos;
        m_is_finished = other.m_is_finished;
        other.m_is_finished = true;
    }
    return *this;
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

void ObjectWriter::Field(const DataTag& tag, std::string_view value) noexcept {
    m_writer.WriteFieldHeader(tag, DataType::String);
    m_writer.WriteString(value);
}

void ObjectWriter::Field(const DataTag& tag, std::span<const uint8_t> data) noexcept {
    m_writer.WriteFieldHeader(tag, DataType::Binary);
    m_writer.WriteBinary(data);
}

void ObjectWriter::Field(const DataTag& tag, const void* data, size_t size) noexcept {
    m_writer.WriteFieldHeader(tag, DataType::Binary);
    m_writer.WriteData(static_cast<const uint8_t*>(data), size);
}

void ObjectWriter::FieldUUID(const DataTag& tag, const void* uuid) noexcept {
    m_writer.WriteFieldHeader(tag, DataType::UUID);
    m_writer.WriteData(uuid, 16);
}

ObjectWriter ObjectWriter::FieldObject(const DataTag& tag) noexcept {
    m_writer.WriteFieldHeader(tag, DataType::Object);
    return ObjectWriter(m_writer);
}

// ---------------------------------
// Array field methods
// ---------------------------------

StringArrayWriter ObjectWriter::FieldStringArray(const DataTag& tag) noexcept {
    m_writer.WriteFieldHeader(tag, DataType::StringArray);
    return StringArrayWriter(*this);
}

void ObjectWriter::FieldStringArray(const DataTag& tag, std::span<const std::string_view> data) noexcept {
    m_writer.WriteFieldHeader(tag, DataType::StringArray);

    // Write array size
    size_t offset = m_writer.ReserveDataSizeField();

    // Write each string in the array
    for (const auto& str : data) {
        m_writer.WriteString(str);
    }

    m_writer.WriteDataSizeField(offset);
}

BinaryArrayWriter ObjectWriter::FieldBinaryArray(const DataTag& tag) noexcept {
    m_writer.WriteFieldHeader(tag, DataType::BinaryArray);
    return BinaryArrayWriter(*this);
}

ObjectArrayWriter ObjectWriter::FieldObjectArray(const DataTag& tag) noexcept {
    m_writer.WriteFieldHeader(tag, DataType::ObjectArray);
    return ObjectArrayWriter(*this);
}

// ---------------------------------
// Vector field methods
// ---------------------------------

// ---------------------------------
// ArrayWriter
// ---------------------------------

ArrayWriter::ArrayWriter(ObjectWriter& obj) noexcept
    : m_obj(obj),
      m_is_finished(false) {
    m_array_size_pos = obj.GetWriter().ReserveDataSizeField();
}

// RAII: move constructor
ArrayWriter::ArrayWriter(ArrayWriter&& other) noexcept
    : m_obj(other.m_obj),
      m_array_size_pos(other.m_array_size_pos),
      m_is_finished(other.m_is_finished) {
    other.m_is_finished = true;
}

// RAII: move assignment
ArrayWriter& ArrayWriter::operator=(ArrayWriter&& other) noexcept {
    if (this != &other) {
        if (!m_is_finished) {
            Finish();
        }
        m_array_size_pos = other.m_array_size_pos;
        m_is_finished = other.m_is_finished;
        other.m_is_finished = true;
    }
    return *this;
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

void BinaryArrayWriter::AddElement(std::span<const uint8_t> data) noexcept {
    m_obj.GetWriter().WriteBinary(data);
}

void BinaryArrayWriter::AddElement(const void* data, FieldSize size) noexcept {
    m_obj.GetWriter().WriteData(static_cast<const uint8_t*>(data), size);
}

ObjectWriter ObjectArrayWriter::CreateElement() noexcept {
    return ObjectWriter(m_obj.GetWriter());
}

}  // namespace tbf