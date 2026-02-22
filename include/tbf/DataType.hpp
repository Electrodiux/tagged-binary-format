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
#include <type_traits>

namespace tbf {

using FieldSize = uint32_t;

constexpr uint8_t CLASSIFICATION_MASK = 0xF0;
constexpr uint8_t BASE_TYPE_MASK = 0x0F;

enum class DataType : uint8_t {
    // Classification bits

    Raw = 0x00,
    Array = 0xA0,

    Vector2 = 0x20,
    Vector3 = 0x30,
    Vector4 = 0x40,

    // Primitive types

    SignedInteger = 0b0000,
    UnsignedInteger = 0b0100,
    FloatingPointAndBool = 0b1000,
    NonPrimitive = 0b1100,

    Int8 = Raw | SignedInteger | 0b00,
    Int16 = Raw | SignedInteger | 0b01,
    Int32 = Raw | SignedInteger | 0b10,
    Int64 = Raw | SignedInteger | 0b11,

    UInt8 = Raw | UnsignedInteger | 0b00,
    UInt16 = Raw | UnsignedInteger | 0b01,
    UInt32 = Raw | UnsignedInteger | 0b10,
    UInt64 = Raw | UnsignedInteger | 0b11,

    Boolean = Raw | FloatingPointAndBool | 0b00,
    Float16 = Raw | FloatingPointAndBool | 0b01,
    Float32 = Raw | FloatingPointAndBool | 0b10,
    Float64 = Raw | FloatingPointAndBool | 0b11,

    UUID = Raw | NonPrimitive | 0b00,
    String = Raw | NonPrimitive | 0b01,
    Binary = Raw | NonPrimitive | 0b10,
    Object = Raw | NonPrimitive | 0b11,

    // Vector2

    Vector2i8 = Vector2 | Int8,
    Vector2i16 = Vector2 | Int16,
    Vector2i32 = Vector2 | Int32,
    Vector2i64 = Vector2 | Int64,

    Vector2b = Vector2 | Boolean,
    Vector2f16 = Vector2 | Float16,
    Vector2f32 = Vector2 | Float32,
    Vector2f64 = Vector2 | Float64,

    // Vector3

    Vector3i8 = Vector3 | Int8,
    Vector3i16 = Vector3 | Int16,
    Vector3i32 = Vector3 | Int32,
    Vector3i64 = Vector3 | Int64,

    Vector3b = Vector3 | Boolean,
    Vector3f16 = Vector3 | Float16,
    Vector3f32 = Vector3 | Float32,
    Vector3f64 = Vector3 | Float64,

    // Vector4

    Vector4i8 = Vector4 | Int8,
    Vector4i16 = Vector4 | Int16,
    Vector4i32 = Vector4 | Int32,
    Vector4i64 = Vector4 | Int64,

    Vector4b = Vector4 | Boolean,
    Vector4f16 = Vector4 | Float16,
    Vector4f32 = Vector4 | Float32,
    Vector4f64 = Vector4 | Float64,

    // Array

    Int8Array = Array | Int8,
    Int16Array = Array | Int16,
    Int32Array = Array | Int32,
    Int64Array = Array | Int64,

    UInt8Array = Array | UInt8,
    UInt16Array = Array | UInt16,
    UInt32Array = Array | UInt32,
    UInt64Array = Array | UInt64,

    BooleanArray = Array | Boolean,
    Float16Array = Array | Float16,
    Float32Array = Array | Float32,
    Float64Array = Array | Float64,

    UUIDArray = Array | UUID,
    StringArray = Array | String,
    BinaryArray = Array | Binary,
    ObjectArray = Array | Object,

    // Error value

    Invalid = 0xFF
};

inline constexpr DataType TypeClassification(DataType type) {
    return static_cast<DataType>(static_cast<uint8_t>(type) & CLASSIFICATION_MASK);
}

inline constexpr DataType BaseDataType(DataType type) {
    return static_cast<DataType>(static_cast<uint8_t>(type) & BASE_TYPE_MASK);
}

inline constexpr bool IsPrimitiveType(DataType type) {
    return TypeClassification(type) == DataType::Raw;
}

inline constexpr bool IsVectorType(DataType type) {
    uint8_t classification = static_cast<uint8_t>(TypeClassification(type));
    return classification >= static_cast<uint8_t>(DataType::Vector2) && classification <= static_cast<uint8_t>(DataType::Vector4);
}

inline constexpr bool IsArrayType(DataType type) {
    return TypeClassification(type) == DataType::Array;
}

inline constexpr bool IsDynamicArrayType(DataType type) {
    return type == DataType::StringArray || type == DataType::BinaryArray || type == DataType::ObjectArray;
}

inline constexpr bool IsFixedSizeArrayType(DataType type) {
    return IsArrayType(type) && !IsDynamicArrayType(type);
}

inline constexpr DataType PrimitiveToArrayType(DataType primitive) {
    return static_cast<DataType>(static_cast<uint8_t>(primitive) | static_cast<uint8_t>(DataType::Array));
}

inline constexpr bool IsPrimitive(DataType type) {
    return (static_cast<uint8_t>(type) & 0b1100) != 0b1100;
}

inline constexpr bool IsValidDataType(DataType type) {
    switch (TypeClassification(type)) {
        case DataType::Raw:
            return true;
        case DataType::Array:
            return true;
        case DataType::Vector2:
        case DataType::Vector3:
        case DataType::Vector4:
            return IsPrimitive(type);
        default:
            return false;
    }
}

template <typename Type>
    requires std::is_integral<Type>::value
consteval DataType IntegerType() {
    if constexpr (std::is_signed<Type>::value) {
        switch (sizeof(Type)) {
            case 1: return DataType::Int8;
            case 2: return DataType::Int16;
            case 4: return DataType::Int32;
            case 8: return DataType::Int64;
        }
    } else {
        switch (sizeof(Type)) {
            case 1: return DataType::UInt8;
            case 2: return DataType::UInt16;
            case 4: return DataType::UInt32;
            case 8: return DataType::UInt64;
        }
    }
}

inline constexpr uint32_t DataTypeSize(DataType type) {
    switch (type) {
        case DataType::Int8:
        case DataType::UInt8:
        case DataType::Boolean:
            return 1;
        case DataType::Int16:
        case DataType::UInt16:
        case DataType::Float16:
            return 2;
        case DataType::Int32:
        case DataType::UInt32:
        case DataType::Float32:
            return 4;
        case DataType::Int64:
        case DataType::UInt64:
        case DataType::Float64:
            return 8;
        case DataType::UUID:
            return 16;
        default:
            return 0;
    }
}

inline constexpr uint32_t VectorTypeDimension(DataType type) {
    switch (TypeClassification(type)) {
        case DataType::Vector2: return 2;
        case DataType::Vector3: return 3;
        case DataType::Vector4: return 4;
        default: return 0;
    }
}

}  // namespace tbf