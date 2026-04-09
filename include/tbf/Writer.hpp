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
#include "tbf/Endianness.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <string_view>
#include <vector>

namespace tbf {

class Writer;
class ObjectWriter;

class StringArrayWriter;
class BinaryArrayWriter;
class ObjectArrayWriter;

using BufferOffset = size_t;

class ObjectWriter {
   private:
    friend class Writer;
    friend class ArrayWriter;
    friend class ObjectArrayWriter;

   private:
    Writer& m_writer;
    BufferOffset m_obj_size_pos;

    bool m_is_finished;

   private:
    ObjectWriter(Writer& writer) noexcept;

   public:
    ObjectWriter(const ObjectWriter&) = delete;
    ObjectWriter& operator=(const ObjectWriter&) = delete;

    // RAII: move semantics for ownership transfer
    ObjectWriter(ObjectWriter&& other) noexcept;
    ObjectWriter& operator=(ObjectWriter&& other) noexcept;

    // RAII: auto-finish on destruction
    ~ObjectWriter() noexcept { Finish(); }

   public:
    void Finish() noexcept;
    bool IsFinished() const noexcept { return m_is_finished; }

    Writer& GetWriter() noexcept { return m_writer; }
    const Writer& GetWriter() const noexcept { return m_writer; }

    // ---------------------------------
    // Field methods - Template-based
    // ---------------------------------

   public:
    // Primitives (int8..int64, bool, float, double)
    template <Primitive T>
    void Field(const DataTag& tag, T value) noexcept;

    // Enum
    template <typename Enum>
        requires std::is_enum_v<Enum>
    void Field(const DataTag& tag, Enum value) noexcept;

    // String
    void Field(const DataTag& tag, std::string_view value) noexcept;

    // Binary
    void Field(const DataTag& tag, std::span<const uint8_t> data) noexcept;
    void Field(const DataTag& tag, const void* data, size_t size) noexcept;

    // UUID (16-byte pointer)
    void FieldUUID(const DataTag& tag, const void* uuid) noexcept;

    // Object (returns sub-writer)
    [[nodiscard]] ObjectWriter FieldObject(const DataTag& tag) noexcept;

    // ---------------------------------
    // Fixed-size arrays
    // ---------------------------------

    template <ArrayElement T>
    void FieldArray(const DataTag& tag, std::span<const T> data) noexcept;

    template <ArrayElement T>
    void FieldArray(const DataTag& tag, const T* data, uint32_t length) noexcept;

    // ---------------------------------
    // Variable arrays - builder pattern
    // ---------------------------------

    [[nodiscard]] StringArrayWriter FieldStringArray(const DataTag& tag) noexcept;
    void FieldStringArray(const DataTag& tag, std::span<const std::string_view> data) noexcept;

    [[nodiscard]] BinaryArrayWriter FieldBinaryArray(const DataTag& tag) noexcept;

    [[nodiscard]] ObjectArrayWriter FieldObjectArray(const DataTag& tag) noexcept;

    // ---------------------------------
    // Vectors (std::array version)
    // ---------------------------------

    template <Primitive T, uint32_t size>
        requires VectorSize<size>
    void FieldVector(const DataTag& tag, const std::array<T, size>& data) noexcept;

    // Vectors (pointer version)
    template <Primitive T, uint32_t size>
        requires VectorSize<size>
    void FieldVector(const DataTag& tag, const T* data) noexcept;

   private:
    template <Primitive T>
    void WritePrimitiveField(const DataTag& tag, DataType type, T value) noexcept;

    template <ArrayElement T>
    void WriteArrayField(const DataTag& tag, const T* data, uint32_t length) noexcept;

    template <Primitive T, uint32_t size>
        requires VectorSize<size>
    void WriteVectorField(const DataTag& tag, const T* data) noexcept;
};

class ArrayWriter {
   private:
    friend class ObjectWriter;

   protected:
    ObjectWriter& m_obj;

   private:
    BufferOffset m_array_size_pos;

    bool m_is_finished;

   protected:
    ArrayWriter(ObjectWriter& obj) noexcept;

   public:
    ArrayWriter(const ArrayWriter&) = delete;
    ArrayWriter& operator=(const ArrayWriter&) = delete;

    // RAII: move semantics
    ArrayWriter(ArrayWriter&& other) noexcept;
    ArrayWriter& operator=(ArrayWriter&& other) noexcept;

    // RAII: auto-finish on destruction
    virtual ~ArrayWriter() { Finish(); }

    void Finish() noexcept;
    bool IsFinished() const noexcept { return m_is_finished; }
};

class StringArrayWriter : public ArrayWriter {
   private:
    friend class ObjectWriter;

   private:
    StringArrayWriter(ObjectWriter& obj) noexcept : ArrayWriter(obj) {}

   public:
    void AddElement(std::string_view element) noexcept;
};

class BinaryArrayWriter : public ArrayWriter {
   private:
    friend class ObjectWriter;

   private:
    BinaryArrayWriter(ObjectWriter& obj) noexcept : ArrayWriter(obj) {}

   public:
    void AddElement(std::span<const uint8_t> data) noexcept;
    void AddElement(const void* data, FieldSize size) noexcept;
};

class ObjectArrayWriter : public ArrayWriter {
   private:
    friend class ObjectWriter;

   protected:
    ObjectArrayWriter(ObjectWriter& obj) noexcept : ArrayWriter(obj) {}

   public:
    ObjectWriter CreateElement() noexcept;
};

class Writer {
   private:
    friend class ObjectWriter;
    friend class ArrayWriter;

    friend class StringArrayWriter;
    friend class BinaryArrayWriter;
    friend class ObjectArrayWriter;

   private:
    static constexpr uint32_t MIN_BUFFER_GROW_SIZE = 1024;             // 1 KiB
    static constexpr uint32_t DEFAULT_BUFFER_GROW_SIZE = 1024 * 1024;  // 1 MiB

   private:
    uint32_t m_buffer_grow_size;
    std::vector<uint8_t> m_buffer;

    bool m_name_based = true;

    ObjectWriter m_root_object;

   public:
    // ---------------------------------
    // Constructors & Destructor
    // ---------------------------------

    Writer(bool name_based = true, uint32_t buff_grow_size = DEFAULT_BUFFER_GROW_SIZE) noexcept;

    // ---------------------------------
    // Methods
    // ---------------------------------

    const void* Data() const noexcept { return m_buffer.data(); }
    size_t Size() const noexcept { return m_buffer.size(); }

    ObjectWriter& RootObject() noexcept { return m_root_object; }
    void Finish() noexcept { m_root_object.Finish(); }

    void SetBufferGrowSize(uint32_t grow_size) noexcept;

    // ---------------------------------
    // Writing methods
    // ---------------------------------

   private:
    void ReserveBuffer(size_t size) noexcept;

    BufferOffset WriteData(const void* data, size_t size) noexcept;

    template <typename Type, bool swap_endianess = true>
    void WriteData(Type value) noexcept;

    void WriteFieldHeader(const DataTag& tag, DataType type) noexcept;

    BufferOffset ReserveDataSizeField() noexcept;
    void WriteDataSizeField(BufferOffset offset) noexcept;

    void* GetBufferPointer(BufferOffset offset) noexcept;

    void WriteString(std::string_view str) noexcept;
    void WriteBinary(std::span<const uint8_t> data) noexcept;
};

// ==============================================================================
// Template implementations
// ==============================================================================

template <Primitive T>
void ObjectWriter::Field(const DataTag& tag, T value) noexcept {
    WritePrimitiveField(tag, Type<T>::type, value);
}

template <typename Enum>
    requires std::is_enum_v<Enum>
void ObjectWriter::Field(const DataTag& tag, Enum value) noexcept {
    using UnderlyingType = std::underlying_type_t<Enum>;
    WritePrimitiveField(tag, Type<UnderlyingType>::type, static_cast<UnderlyingType>(value));
}

template <ArrayElement T>
void ObjectWriter::FieldArray(const DataTag& tag, std::span<const T> data) noexcept {
    WriteArrayField(tag, data.data(), static_cast<uint32_t>(data.size()));
}

template <ArrayElement T>
void ObjectWriter::FieldArray(const DataTag& tag, const T* data, uint32_t length) noexcept {
    WriteArrayField(tag, data, length);
}

template <Primitive T, uint32_t size>
    requires VectorSize<size>
void ObjectWriter::FieldVector(const DataTag& tag, const std::array<T, size>& data) noexcept {
    WriteVectorField<T, size>(tag, data.data());
}

template <Primitive T, uint32_t size>
    requires VectorSize<size>
void ObjectWriter::FieldVector(const DataTag& tag, const T* data) noexcept {
    WriteVectorField<T, size>(tag, data);
}

template <ArrayElement T>
void ObjectWriter::WriteArrayField(const DataTag& tag, const T* data, uint32_t length) noexcept {
    m_writer.WriteFieldHeader(tag, Type<T>::array_type);

    // Write array length and array data
    FieldSize size = length * sizeof(T);
    m_writer.WriteData<FieldSize>(size);
    BufferOffset offset = m_writer.WriteData(data, size);

    AdjustArrayEndianess<sizeof(T)>(m_writer.GetBufferPointer(offset), length);
}

template <Primitive T>
void ObjectWriter::WritePrimitiveField(const DataTag& tag, DataType type, T value) noexcept {
    m_writer.WriteFieldHeader(tag, type);
    if constexpr (std::is_same_v<T, float>) {
        m_writer.WriteData<uint32_t>(std::bit_cast<uint32_t>(value));
    } else if constexpr (std::is_same_v<T, double>) {
        m_writer.WriteData<uint64_t>(std::bit_cast<uint64_t>(value));
    } else {
        m_writer.WriteData<T>(value);
    }
}

template <typename Type, bool swap_endianess>
void Writer::WriteData(Type value) noexcept {
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

template <Primitive T, uint32_t size>
    requires VectorSize<size>
void ObjectWriter::WriteVectorField(const DataTag& tag, const T* data) noexcept {
    DataType vector_type = VectorType<size, T>();
    m_writer.WriteFieldHeader(tag, vector_type);

    // For floats, we need to bit-cast to integer types for proper endian handling
    if constexpr (std::is_same_v<T, float>) {
        uint32_t temp[size];
        for (uint32_t i = 0; i < size; ++i) {
            temp[i] = std::bit_cast<uint32_t>(data[i]);
        }
        BufferOffset offset = m_writer.WriteData(temp, sizeof(temp));
        AdjustArrayEndianess<sizeof(float)>(m_writer.GetBufferPointer(offset), size);
    } else if constexpr (std::is_same_v<T, double>) {
        uint64_t temp[size];
        for (uint32_t i = 0; i < size; ++i) {
            temp[i] = std::bit_cast<uint64_t>(data[i]);
        }
        BufferOffset offset = m_writer.WriteData(temp, sizeof(temp));
        AdjustArrayEndianess<sizeof(double)>(m_writer.GetBufferPointer(offset), size);
    } else {
        BufferOffset offset = m_writer.WriteData(data, sizeof(T) * size);
        AdjustArrayEndianess<sizeof(T)>(m_writer.GetBufferPointer(offset), size);
    }
}

}  // namespace tbf