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

   public:
    void Finish() noexcept;
    inline bool IsFinished() const noexcept { return m_is_finished; }

    inline Writer& GetWriter() noexcept { return m_writer; }
    inline const Writer& GetWriter() const noexcept { return m_writer; }

    // ---------------------------------
    // Field methods
    // ---------------------------------

   public:
    void FieldInt8(const DataTag& tag, int8_t value) noexcept;
    void FieldInt16(const DataTag& tag, int16_t value) noexcept;
    void FieldInt32(const DataTag& tag, int32_t value) noexcept;
    void FieldInt64(const DataTag& tag, int64_t value) noexcept;

    void FieldUInt8(const DataTag& tag, uint8_t value) noexcept;
    void FieldUInt16(const DataTag& tag, uint16_t value) noexcept;
    void FieldUInt32(const DataTag& tag, uint32_t value) noexcept;
    void FieldUInt64(const DataTag& tag, uint64_t value) noexcept;

    void FieldBoolean(const DataTag& tag, bool value) noexcept;
    void FieldFloat16(const DataTag& tag, uint16_t value) noexcept;
    void FieldFloat32(const DataTag& tag, float value) noexcept;
    void FieldFloat64(const DataTag& tag, double value) noexcept;

    void FieldUUID(const DataTag& tag, const void* uuid) noexcept;
    void FieldString(const DataTag& tag, std::string_view value) noexcept;
    void FieldBinary(const DataTag& tag, const void* data, size_t size) noexcept;
    [[nodiscard]] ObjectWriter FieldObject(const DataTag& tag) noexcept;

    template <typename Enum>
        requires std::is_enum<Enum>::value
    inline void FieldEnum(const DataTag& tag, Enum value);

    // ---------------------------------
    // Array field methods
    // ---------------------------------

   private:
    template <typename Type>
    void FieldArray(const DataTag& tag, DataType array_type, const Type* data, uint32_t length) noexcept;

   public:
    void FieldArrayInt8(const DataTag& tag, const int8_t* data, uint32_t length) noexcept;
    void FieldArrayInt16(const DataTag& tag, const int16_t* data, uint32_t length) noexcept;
    void FieldArrayInt32(const DataTag& tag, const int32_t* data, uint32_t length) noexcept;
    void FieldArrayInt64(const DataTag& tag, const int64_t* data, uint32_t length) noexcept;

    void FieldArrayUInt8(const DataTag& tag, const uint8_t* data, uint32_t length) noexcept;
    void FieldArrayUInt16(const DataTag& tag, const uint16_t* data, uint32_t length) noexcept;
    void FieldArrayUInt32(const DataTag& tag, const uint32_t* data, uint32_t length) noexcept;
    void FieldArrayUInt64(const DataTag& tag, const uint64_t* data, uint32_t length) noexcept;

    void FieldArrayBoolean(const DataTag& tag, const bool* data, uint32_t length) noexcept;
    void FieldArrayFloat16(const DataTag& tag, const uint16_t* data, uint32_t length) noexcept;
    void FieldArrayFloat32(const DataTag& tag, const float* data, uint32_t length) noexcept;
    void FieldArrayFloat64(const DataTag& tag, const double* data, uint32_t length) noexcept;

    [[nodiscard]] StringArrayWriter FieldStringArray(const DataTag& tag) noexcept;
    void FieldStringArray(const DataTag& tag, const std::string_view* data, uint32_t length) noexcept;

    [[nodiscard]] BinaryArrayWriter FieldBinaryArray(const DataTag& tag) noexcept;
    void FieldBinaryArray(const DataTag& tag, const void* const* data, const uint32_t* sizes, uint32_t length) noexcept;

    [[nodiscard]] ObjectArrayWriter FieldObjectArray(const DataTag& tag) noexcept;

    // ---------------------------------
    // Array field with std::span
    // ---------------------------------

   public:
    inline void FieldArrayInt8(const DataTag& tag, std::span<const int8_t> data) noexcept {
        FieldArrayInt8(tag, data.data(), static_cast<uint32_t>(data.size()));
    }

    inline void FieldArrayInt16(const DataTag& tag, std::span<const int16_t> data) noexcept {
        FieldArrayInt16(tag, data.data(), static_cast<uint32_t>(data.size()));
    }

    inline void FieldArrayInt32(const DataTag& tag, std::span<const int32_t> data) noexcept {
        FieldArrayInt32(tag, data.data(), static_cast<uint32_t>(data.size()));
    }

    inline void FieldArrayInt64(const DataTag& tag, std::span<const int64_t> data) noexcept {
        FieldArrayInt64(tag, data.data(), static_cast<uint32_t>(data.size()));
    }

    inline void FieldArrayUInt8(const DataTag& tag, std::span<const uint8_t> data) noexcept {
        FieldArrayUInt8(tag, data.data(), static_cast<uint32_t>(data.size()));
    }

    inline void FieldArrayUInt16(const DataTag& tag, std::span<const uint16_t> data) noexcept {
        FieldArrayUInt16(tag, data.data(), static_cast<uint32_t>(data.size()));
    }

    inline void FieldArrayUInt32(const DataTag& tag, std::span<const uint32_t> data) noexcept {
        FieldArrayUInt32(tag, data.data(), static_cast<uint32_t>(data.size()));
    }

    inline void FieldArrayUInt64(const DataTag& tag, std::span<const uint64_t> data) noexcept {
        FieldArrayUInt64(tag, data.data(), static_cast<uint32_t>(data.size()));
    }

    inline void FieldArrayBoolean(const DataTag& tag, std::span<const bool> data) noexcept {
        FieldArrayBoolean(tag, data.data(), static_cast<uint32_t>(data.size()));
    }

    inline void FieldArrayFloat16(const DataTag& tag, std::span<const uint16_t> data) noexcept {
        FieldArrayFloat16(tag, data.data(), static_cast<uint32_t>(data.size()));
    }

    inline void FieldArrayFloat32(const DataTag& tag, std::span<const float> data) noexcept {
        FieldArrayFloat32(tag, data.data(), static_cast<uint32_t>(data.size()));
    }

    inline void FieldArrayFloat64(const DataTag& tag, std::span<const double> data) noexcept {
        FieldArrayFloat64(tag, data.data(), static_cast<uint32_t>(data.size()));
    }

    // ---------------------------------
    // Field vectors
    // ---------------------------------

   private:
    template <typename Type, uint32_t dim>
        requires std::is_arithmetic<Type>::value && (dim >= 2) && (dim <= 4)
    void FieldVector(const DataTag& tag, DataType vector_type, const Type* data) noexcept;

   public:
    // Vector 2

    void FieldVector2i8(const DataTag& tag, const int8_t* data) noexcept;
    void FieldVector2i16(const DataTag& tag, const int16_t* data) noexcept;
    void FieldVector2i32(const DataTag& tag, const int32_t* data) noexcept;
    void FieldVector2i64(const DataTag& tag, const int64_t* data) noexcept;

    inline void FieldVector2i8(const DataTag& tag, const uint8_t* data) noexcept {
        FieldVector2i8(tag, reinterpret_cast<const int8_t*>(data));
    }

    inline void FieldVector2i16(const DataTag& tag, const uint16_t* data) noexcept {
        FieldVector2i16(tag, reinterpret_cast<const int16_t*>(data));
    }

    inline void FieldVector2i32(const DataTag& tag, const uint32_t* data) noexcept {
        FieldVector2i32(tag, reinterpret_cast<const int32_t*>(data));
    }

    inline void FieldVector2i64(const DataTag& tag, const uint64_t* data) noexcept {
        FieldVector2i64(tag, reinterpret_cast<const int64_t*>(data));
    }

    void FieldVector2b(const DataTag& tag, const bool* data) noexcept;
    void FieldVector2f16(const DataTag& tag, const uint16_t* data) noexcept;
    void FieldVector2f32(const DataTag& tag, const float* data) noexcept;
    void FieldVector2f64(const DataTag& tag, const double* data) noexcept;

    // Vector 3

    void FieldVector3i8(const DataTag& tag, const int8_t* data) noexcept;
    void FieldVector3i16(const DataTag& tag, const int16_t* data) noexcept;
    void FieldVector3i32(const DataTag& tag, const int32_t* data) noexcept;
    void FieldVector3i64(const DataTag& tag, const int64_t* data) noexcept;

    inline void FieldVector3i8(const DataTag& tag, const uint8_t* data) noexcept {
        FieldVector3i8(tag, reinterpret_cast<const int8_t*>(data));
    }

    inline void FieldVector3i16(const DataTag& tag, const uint16_t* data) noexcept {
        FieldVector3i16(tag, reinterpret_cast<const int16_t*>(data));
    }

    inline void FieldVector3i32(const DataTag& tag, const uint32_t* data) noexcept {
        FieldVector3i32(tag, reinterpret_cast<const int32_t*>(data));
    }

    inline void FieldVector3i64(const DataTag& tag, const uint64_t* data) noexcept {
        FieldVector3i64(tag, reinterpret_cast<const int64_t*>(data));
    }

    void FieldVector3b(const DataTag& tag, const bool* data) noexcept;
    void FieldVector3f16(const DataTag& tag, const uint16_t* data) noexcept;
    void FieldVector3f32(const DataTag& tag, const float* data) noexcept;
    void FieldVector3f64(const DataTag& tag, const double* data) noexcept;

    // Vector 4

    void FieldVector4i8(const DataTag& tag, const int8_t* data) noexcept;
    void FieldVector4i16(const DataTag& tag, const int16_t* data) noexcept;
    void FieldVector4i32(const DataTag& tag, const int32_t* data) noexcept;
    void FieldVector4i64(const DataTag& tag, const int64_t* data) noexcept;

    inline void FieldVector4i8(const DataTag& tag, const uint8_t* data) noexcept {
        FieldVector4i8(tag, reinterpret_cast<const int8_t*>(data));
    }

    inline void FieldVector4i16(const DataTag& tag, const uint16_t* data) noexcept {
        FieldVector4i16(tag, reinterpret_cast<const int16_t*>(data));
    }

    inline void FieldVector4i32(const DataTag& tag, const uint32_t* data) noexcept {
        FieldVector4i32(tag, reinterpret_cast<const int32_t*>(data));
    }

    inline void FieldVector4i64(const DataTag& tag, const uint64_t* data) noexcept {
        FieldVector4i64(tag, reinterpret_cast<const int64_t*>(data));
    }

    void FieldVector4b(const DataTag& tag, const bool* data) noexcept;
    void FieldVector4f16(const DataTag& tag, const uint16_t* data) noexcept;
    void FieldVector4f32(const DataTag& tag, const float* data) noexcept;
    void FieldVector4f64(const DataTag& tag, const double* data) noexcept;
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

    virtual ~ArrayWriter() { Finish(); }

    void Finish() noexcept;
    inline bool IsFinished() const noexcept { return m_is_finished; }
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
    void AddElement(const void* element, FieldSize size) noexcept;
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

    inline const void* Data() const noexcept { return m_buffer.data(); }
    inline size_t Size() const noexcept { return m_buffer.size(); }

    inline ObjectWriter& RootObject() noexcept { return m_root_object; }
    inline void Finish() noexcept { m_root_object.Finish(); }

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

    void WriteString(const std::string_view& str) noexcept;
    void WriteBinary(const void* data, FieldSize size) noexcept;
};

template <typename Enum>
    requires std::is_enum<Enum>::value
void ObjectWriter::FieldEnum(const DataTag& tag, Enum value) {
    using UnderlyingType = typename std::underlying_type<Enum>::type;
    m_writer.WriteFieldHeader(tag, IntegerType<UnderlyingType>());
    m_writer.WriteData<UnderlyingType, true>(static_cast<UnderlyingType>(value));
}

}  // namespace tbf