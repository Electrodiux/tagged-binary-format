# Tagged Binary Format (TBF) Specification

## Overview

Tagged Binary Format (TBF) is a compact, efficient binary serialization format designed for high-performance data interchange. It supports a rich type system including primitives, vectors, arrays, and nested objects.

**Version:** 1.0
**Endianness:** Little-endian

## Table of Contents

1. [Format Structure](#format-structure)
2. [Data Types](#data-types)
3. [Field Encoding](#field-encoding)
4. [Primitive Types](#primitive-types)
5. [Vector Types](#vector-types)
6. [Array Types](#array-types)
7. [Complex Types](#complex-types)
8. [Object Structure](#object-structure)
9. [Tag System](#tag-system)

---

## Format Structure

### Binary Layout

All multi-byte values are stored in **little-endian** byte order.

### Top-Level Structure

```
+------------------+
| Object Data      |
| (Field Sequence) |
+------------------+
```

An object consists of a size field followed by a sequence of fields.

---

## Data Types

### Type Byte Encoding (1 byte)

The type byte is structured as follows:

```
Bit Layout: CCCC BBBB
            │    │
            │    └─── Base Type (4 bits)
            └──────── Classification (4 bits)
```

#### Classification Bits (Upper 4 bits)

| Value | Classification | Description |
|-------|---------------|-------------|
| `0x0` | Raw           | Single primitive value |
| `0x2` | Vector2       | 2-component vector |
| `0x3` | Vector3       | 3-component vector |
| `0x4` | Vector4       | 4-component vector |
| `0xA` | Array         | Dynamic array |

#### Base Type Bits (Lower 4 bits)

| Value | Category | Type Group |
|-------|----------|------------|
| `0b00xx` | Signed Integer | Int8, Int16, Int32, Int64 |
| `0b01xx` | Unsigned Integer | UInt8, UInt16, UInt32, UInt64 |
| `0b10xx` | Floating Point / Boolean | Boolean, Float16, Float32, Float64 |
| `0b11xx` | Non-Primitive | UUID, String, Binary, Object |

### Complete Type List

#### Primitive Types (Raw)

> ⚠️ **DEPRECATION NOTICE**: Unsigned integer types (UInt8, UInt16, UInt32, UInt64) may be discontinued in future versions of TBF. It is recommended to use signed integer types (Int8, Int16, Int32, Int64) for both signed and unsigned values to ensure forward compatibility.

| Type ID | Name | Size | Description |
|---------|------|------|-------------|
| `0x00` | Int8 | 1 byte | Signed 8-bit integer |
| `0x01` | Int16 | 2 bytes | Signed 16-bit integer |
| `0x02` | Int32 | 4 bytes | Signed 32-bit integer |
| `0x03` | Int64 | 8 bytes | Signed 64-bit integer |
| `0x04` | UInt8 | 1 byte | Unsigned 8-bit integer ⚠️ |
| `0x05` | UInt16 | 2 bytes | Unsigned 16-bit integer ⚠️ |
| `0x06` | UInt32 | 4 bytes | Unsigned 32-bit integer ⚠️ |
| `0x07` | UInt64 | 8 bytes | Unsigned 64-bit integer ⚠️ |
| `0x08` | Boolean | 1 byte | Boolean value (0 or 1) |
| `0x09` | Float16 | 2 bytes | 16-bit floating point |
| `0x0A` | Float32 | 4 bytes | 32-bit floating point |
| `0x0B` | Float64 | 8 bytes | 64-bit floating point |
| `0x0C` | UUID | 16 bytes | Universally Unique Identifier |
| `0x0D` | String | Variable | UTF-8 string |
| `0x0E` | Binary | Variable | Raw binary data |
| `0x0F` | Object | Variable | Nested object |

#### Vector Types

Vectors are fixed-size collections of 2, 3, or 4 elements of the same primitive type.

**Vector2 Types** (`0x2X`): Vector2i8, Vector2i16, Vector2i32, Vector2i64, Vector2b, Vector2f16, Vector2f32, Vector2f64

**Vector3 Types** (`0x3X`): Vector3i8, Vector3i16, Vector3i32, Vector3i64, Vector3b, Vector3f16, Vector3f32, Vector3f64

**Vector4 Types** (`0x4X`): Vector4i8, Vector4i16, Vector4i32, Vector4i64, Vector4b, Vector4f16, Vector4f32, Vector4f64

#### Array Types

Arrays are dynamic-length collections with a size prefix.

**Fixed-Size Element Arrays** (`0xA0-0xAC`): Int8Array, Int16Array, Int32Array, Int64Array, UInt8Array, UInt16Array, UInt32Array, UInt64Array, BooleanArray, Float16Array, Float32Array, Float64Array, UUIDArray

**Variable-Size Element Arrays** (`0xAD-0xAF`): StringArray, BinaryArray, ObjectArray

---

## Field Encoding

Each field in an object consists of:

### Name-Based Mode

```
+------------+------------------+------------------+
| Type Byte  | Name Length (u8) | Name (UTF-8)     |
| (1 byte)   | (1 byte)         | (variable)       |
+------------+------------------+------------------+
| Field Data                                       |
| (variable)                                       |
+--------------------------------------------------+
```

### ID-Based Mode

```
+------------+------------------+
| Type Byte  | Tag ID (u16)     |
| (1 byte)   | (2 bytes, LE)    |
+------------+------------------+
| Field Data                    |
| (variable)                    |
+-------------------------------+
```

- **Name Length**: 1 byte, maximum 255 characters
- **Name**: UTF-8 encoded string, allowed characters: `a-z`, `A-Z`, `0-9`, `_`
- **Tag ID**: 16-bit unsigned integer, little-endian (cannot be 0)

---

## Primitive Types

### Fixed-Size Primitives

Fixed-size primitives store their value directly after the field header.

**Structure:**
```
[Type Byte] [Tag] [Value]
```

**Examples:**

- **Int32** (`0x02`):
  ```
  Type: 0x02
  Tag: "count"
  Value: 0x2A000000 (42 in little-endian)
  ```

- **Float64** (`0x0B`):
  ```
  Type: 0x0B
  Tag: "pi"
  Value: 0x182D4454FB210940 (3.14159265358979 in IEEE 754)
  ```

- **Boolean** (`0x08`):
  ```
  Type: 0x08
  Tag: "enabled"
  Value: 0x01 (true)
  ```

### UUID (16 bytes)

Stored as 16 raw bytes in standard UUID format.

**Structure:**
```
[Type: 0x0C] [Tag] [16 bytes]
```

---

## Vector Types

Vectors store a fixed number of elements inline (2, 3, or 4 components).

**Structure:**
```
[Type Byte] [Tag] [Element1] [Element2] ... [ElementN]
```

**Example: Vector3f32** (`0x3A`):
```
Type: 0x3A
Tag: "position"
Data: [x: float32] [y: float32] [z: float32]
      (12 bytes total)
```

---

## Array Types

### Fixed-Size Element Arrays

Arrays with fixed-size elements store a size followed by tightly packed elements.

**Structure:**
```
[Type Byte] [Tag] [Size: u32] [Element1] [Element2] ... [ElementN]
```

**Size Field:**
- The size field represents the **total byte size** of all elements, not the element count
- For fixed-size arrays, the size **must be divisible** by the element type's size
- Using size instead of length provides:
   - Compatibility with variable-size element arrays (String, Binary, Object)
   - Additional validation (invalid sizes can be detected immediately)
   - Consistency across all array types

**Example: Int32Array** (`0xA2`):
```
Type: 0xA2
Tag: "values"
Size: 0x0C000000 (12 bytes = 3 elements × 4 bytes)
Data: [42, -10, 1000] -> 0x2A000000 F6FFFFFF E8030000
```

### Variable-Size Element Arrays

Variable-size element arrays store the total byte size of all elements, not the element count.

**Structure:**
```
[Type Byte] [Tag] [Size: u32] [Element1] [Element2] ... [ElementN]
```

**Size Field:**
- The size field represents the **total byte size** of all elements (including length/size prefixes)
- The element count is not directly stored and must be calculated by parsing the elements
- This provides parsing efficiency by allowing readers to skip unwanted elements without parsing their contents

#### String Array

Each string is prefixed with its length (16-bit).

**Structure:**
```
[Type: 0xAD] [Tag] [Size: u32] 
[Len1: u16] [String1] [Len2: u16] [String2] ...
```

**Example:**
```
Type: 0xAD
Tag: "names"
Size: 0x0C000000 (12 bytes total: (2+5)+(2+3)=12)
String 1: Len=0x0500, Data="Alice"
String 2: Len=0x0300, Data="Bob"
```

#### Binary Array

Each binary blob is prefixed with its size (32-bit).

**Structure:**
```
[Type: 0xAE] [Tag] [Size: u32]
[Size1: u32] [Data1] [Size2: u32] [Data2] ...
```

#### Object Array

Each object stores its own size field followed by field data.

**Structure:**
```
[Type: 0xAF] [Tag] [Size: u32]
[Object1] [Object2] ...
```

---

## Complex Types

### String

Strings are UTF-8 encoded with a 16-bit length prefix.

**Structure:**
```
[Type: 0x0D] [Tag] [Length: u16] [UTF-8 Data]
```

**Constraints:**
- Maximum length: 65,535 bytes
- Encoding: UTF-8

> **Note:** For strings exceeding 65,535 bytes, use the Binary type with UTF-8 encoded data instead.


**Example:**
```
Type: 0x0D
Tag: "message"
Length: 0x0B00 (11 bytes)
Data: "Hello World"
```

### Binary

Raw binary data with a 32-bit size prefix.

**Structure:**
```
[Type: 0x0E] [Tag] [Size: u32] [Raw Data]
```

**Constraints:**
- Maximum size: 4,294,967,295 bytes

### Object

**Structure:**
```
[Type: 0x0F] [Tag] [Object]
```

See [Object Structure](#object-structure) for details on object encoding.

---

## Object Structure

All objects (both root and nested) follow the same encoding format:

```
[Size: u32] [Field1] [Field2] ... [FieldN]
```

**Size Field:**
- Represents the total byte size of all fields contained in the object
- Does not include the size field itself (4 bytes)
- Allows readers to skip entire objects efficiently

**Root Object:**
The root-level object begins directly with the size field, as there is no type byte or tag.

**Nested Objects:**
Nested objects (type `0x0F`) include the standard field header (type byte + tag) followed by the object structure above.


### Example Complete Object

```
[Size: 0x1E000000]  (30 bytes of field data)
  [Type: 0x02] [Tag: "id"] [Value: 0x01000000]
  [Type: 0x0D] [Tag: "name"] [Len: 0x0400] [Data: "John"]
  [Type: 0x08] [Tag: "active"] [Value: 0x01]
```

---

## Tag System

### Name-Based Tags

- **Format**: UTF-8 string (1-255 characters)
- **Allowed characters**: `a-z`, `A-Z`, `0-9`, `_`
- **Case sensitive**: `userName` ≠ `username`

### ID-Based Tags

- **Format**: 16-bit unsigned integer
- **Range**: 1-65,535 (0 is invalid)

#### FNV-1a Hash for Tag Names

For scenarios requiring maximum data compression (e.g., network transmission), TBF supports ID-based tags using a 32-bit FNV-1a hash function to convert tag names to 16-bit IDs:

```
Initial: hash = 2166136261
For each character c:
   - Map to value (a-z: 1-26, A-Z: 1-26, 0-9: 27-36, _: 37)
   - hash ^= mapped_value
   - hash *= 16777619
Final: tag_id = hash & 0xFFFF (lower 16 bits)
```

> **Note:** This hash algorithm treats lowercase and uppercase letters identically (both 'a' and 'A' map to value 1). While this reduces hash collisions, it means field names that differ only in capitalization (e.g., "userName" vs "username") will produce the same tag ID. Therefore, when using automatic hash generation for ID-based tags, avoid distinguishing fields solely by capitalization.


**Implementation Notes:**
- The library provides compile-time hash calculation for automatic tag ID generation
- Manual tag IDs can be specified explicitly when needed
- ID-based encoding reduces overhead compared to name-based tags, making it ideal for bandwidth-constrained environments

### Tag Uniqueness

Within a single object, tag names or IDs must be unique. **The behavior when duplicate tags exist is undefined** but typically results in the first occurrence being used.

---

## Endianness

**All multi-byte values use little-endian byte order**, including:
- Object sizes (u32)
- Array element sizes (u32)
- Binary sizes (u32)
- String lengths (u16)
- Tag IDs (u16)
- Numeric values (int16, int32, int64, uint16, uint32, uint64, float32, float64)

Single-byte values (int8, uint8, bool, type bytes) do not require byte swapping.

---

## Comparison to Other Formats

| Feature | TBF | JSON | MessagePack | Protocol Buffers |
|---------|-----|------|-------------|------------------|
| Binary | ✓ | ✗ | ✓ | ✓ |
| Self-describing | ✓ | ✓ | ✓ | ✗ |
| Schema-free | ✓ | ✓ | ✓ | ✗ |
| Vector types | ✓ | ✗ | ✗ | ✗ |
| Fixed arrays | ✓ | ✗ | ✗ | ✓ |
| Name-based fields | ✓ | ✓ | ✓ | ✗ |
| ID-based fields | ✓ | ✗ | ✗ | ✓ |

---

## License

Copyright (c) 2026 Electrodiux. All rights reserved.

Licensed under the MIT License. See LICENSE.md for details.
