# TBF Library API Modernization

Replace the ~100+ named method API with a unified template-based interface, remove unsigned integer types from the wire format, add RAII safety, modernize internals, and improve portability.

---

## Scope

**In scope:** `DataType.hpp`, `DataTag.hpp`, `Endianness.hpp`, `Reader.hpp`, `Writer.hpp`, `Reader.cpp`, `Writer.cpp`, all test files, `FORMAT.md`.  
**Out of scope:** `CMakeLists.txt` (unless needed), build system changes.

## Acceptance Criteria

- All named per-type methods removed; replaced by template equivalents
- Existing test scenarios pass (rewritten to use new API)
- ObjectWriter/ArrayWriter auto-finish via RAII
- ObjectReader cache uses `std::variant` with type aliases
- All `[[gnu::always_inline]]` annotations and `__builtin_prefetch` calls removed (let the compiler decide)
- Unsigned integer types (`UInt8/16/32/64`) removed from wire format, enum, and API entirely
- Vectors use `std::array<T, N>` for read/write
- No regression in wire format compatibility for remaining types

## Constraints

- C++23 (already set in CMakeLists.txt)
- Keep `noexcept` discipline on all public methods
- Keep `consteval` DataTag validation
- Keep the zero-copy reader design (read from buffer without allocations beyond cache)
- Keep both name-based and ID-based tag modes
- Preserve existing license headers

---

## Step 1 — Strip `[[gnu::always_inline]]` and `__builtin_prefetch`

Remove all `[[gnu::always_inline]]` annotations from `Endianness.hpp`, `Reader.cpp`, and `Writer.cpp`. Remove the `__builtin_prefetch` call in `Reader.cpp`. Functions remain `inline` where appropriate — the compiler handles inlining via its own heuristics and LTO (`-flto` is already enabled in Release).

## Step 2 — Remove unsigned integer types from `DataType.hpp`

Remove from the `DataType` enum:
- `UnsignedInteger` classification (`0b0100`)
- `UInt8`, `UInt16`, `UInt32`, `UInt64`
- `UInt8Array`, `UInt16Array`, `UInt32Array`, `UInt64Array`

Update helper functions:
- `DataTypeSize()`: remove `UInt*` cases
- `IntegerType<T>()`: remove entirely (replaced by `Type<T>` traits)
- The `0b0100` classification bits become reserved/unused

The user has already removed `Type<uint*>` specializations and renamed `TbfType` → `Type`, `TbfPrimitive` → `Primitive`, `TbfArrayElement` → `ArrayElement`.

## Step 3 — Update `FORMAT.md`

Remove unsigned integer type documentation from the format spec. Mark the `0b0100` classification as reserved.

## Step 4 — Finalize type traits in `DataType.hpp`

The user has already done most of this work:
- `Type<T>` traits struct with specializations for `int8_t..int64_t`, `bool`, `float`, `double` (no unsigned)
- `Primitive` and `ArrayElement` concepts
- `VectorType<size, T>()` using bitfield arithmetic

Remaining:
- Remove the old `IntegerType<T>()` function (superseded by `Type<T>`)

## Step 5 — Rewrite `Writer.hpp` / `Writer.cpp`

### ObjectWriter new API

Replace all named `FieldXxx` methods with:

```cpp
// Primitives (int8..int64, bool, float, double)
template <Primitive T>
void Field(const DataTag& tag, T value) noexcept;

// String
void Field(const DataTag& tag, std::string_view value) noexcept;

// Binary
void Field(const DataTag& tag, std::span<const uint8_t> data) noexcept;

// UUID (16-byte pointer)
void FieldUUID(const DataTag& tag, const void* uuid) noexcept;

// Object (returns sub-writer)
[[nodiscard]] ObjectWriter FieldObject(const DataTag& tag) noexcept;

// Enum
template <typename Enum> requires std::is_enum_v<Enum>
void Field(const DataTag& tag, Enum value) noexcept;

// Fixed-size arrays
template <ArrayElement T>
void FieldArray(const DataTag& tag, std::span<const T> data) noexcept;

// Also accept pointer + length:
template <ArrayElement T>
void FieldArray(const DataTag& tag, const T* data, uint32_t length) noexcept;

// Variable arrays (string, binary, object) — keep builder pattern
[[nodiscard]] StringArrayWriter FieldStringArray(const DataTag& tag) noexcept;
[[nodiscard]] BinaryArrayWriter FieldBinaryArray(const DataTag& tag) noexcept;
[[nodiscard]] ObjectArrayWriter FieldObjectArray(const DataTag& tag) noexcept;

// Convenience: write string array from span
void FieldStringArray(const DataTag& tag, std::span<const std::string_view> data) noexcept;

// Vectors
template <Primitive T, uint32_t N>
    requires (N >= 2) && (N <= 4)
void FieldVector(const DataTag& tag, const std::array<T, N>& data) noexcept;

// Also accept raw pointer for vectors:
template <Primitive T, uint32_t N>
    requires (N >= 2) && (N <= 4)
void FieldVector(const DataTag& tag, const T* data) noexcept;
```

### RAII

- Add `~ObjectWriter() noexcept { Finish(); }` 
- Add move constructor/assignment (transfer ownership, mark source as finished)
- Same for `ArrayWriter` (already has virtual dtor calling Finish)

### Implementation

Most of the implementation moves to the header as template definitions (after the class). The `.cpp` file shrinks dramatically — only non-template methods remain (Writer constructor, Finish, buffer management, WriteFieldHeader, etc.).

## Step 6 — Rewrite `Reader.hpp` / `Reader.cpp`

### ObjectReader cache modernization

Replace:
```cpp
union {
    mutable std::unordered_map<DataTag::Id, CacheEntry> m_id_cache;
    mutable std::unordered_map<std::string_view, CacheEntry> m_name_cache;
};
```

With:
```cpp
using IdCache = std::unordered_map<DataTag::Id, CacheEntry>;
using NameCache = std::unordered_map<std::string_view, CacheEntry>;
mutable std::variant<IdCache, NameCache> m_cache;
```

Remove manual placement new/destroy from constructor/destructor.

### ObjectReader new API

Replace all named `ReadXxx` methods with:

```cpp
// Primitives — returns optional
template <Primitive T>
[[nodiscard]] std::optional<T> Read(const DataTag& tag) const noexcept;

// Primitives — out-param style
template <Primitive T>
bool Read(const DataTag& tag, T& out_value) const noexcept;

// Enum
template <typename Enum> requires std::is_enum_v<Enum>
[[nodiscard]] std::optional<Enum> ReadEnum(const DataTag& tag) const noexcept;

// String
[[nodiscard]] std::optional<std::string_view> ReadString(const DataTag& tag) const noexcept;

// Binary
[[nodiscard]] std::span<const uint8_t> ReadBinary(const DataTag& tag) const noexcept;

// UUID
[[nodiscard]] const void* ReadUUID(const DataTag& tag) const noexcept;

// Object
[[nodiscard]] std::optional<ObjectReader> ReadObject(const DataTag& tag) const noexcept;

// Fixed arrays — span return
template <ArrayElement T>
[[nodiscard]] std::span<const T> ReadArray(const DataTag& tag) const noexcept;

// Fixed arrays — pointer + length return
template <ArrayElement T>
[[nodiscard]] const T* ReadArray(const DataTag& tag, uint32_t& out_length) const noexcept;

// Variable arrays
[[nodiscard]] std::optional<StringArrayReader> ReadStringArray(const DataTag& tag) const noexcept;
[[nodiscard]] std::optional<BinaryArrayReader> ReadBinaryArray(const DataTag& tag) const noexcept;
[[nodiscard]] std::optional<ObjectArrayReader> ReadObjectArray(const DataTag& tag) const noexcept;

// Vectors
template <Primitive T, uint32_t N>
    requires (N >= 2) && (N <= 4)
[[nodiscard]] std::optional<std::array<T, N>> ReadVector(const DataTag& tag) const noexcept;
```

### Implementation

- Template methods defined in header (after the class, or in a separate `Reader.inl` included at the bottom of the header)
- `Reader.cpp` retains: `CreateCache`, `FindTag`, `ReadPointerData`, `ReadStringInternal`, `ReadObjectInternal`, array reader implementations, iterator implementations

## Step 7 — Rewrite all test files

Rewrite tests to use the new template API:

**Before:**
```cpp
root.FieldInt32(TAG_INT32, -123456789);
// ...
auto value = read_root.ReadInt32(TAG_INT32);
```

**After:**
```cpp
root.Field(TAG_INT32, int32_t{-123456789});
// ...
auto value = read_root.Read<int32_t>(TAG_INT32);
```

**Vectors before:**
```cpp
root.FieldVector2f32(TAG_VEC, data);
float* read = read_root.ReadVector2f32(TAG_VEC);
```

**Vectors after:**
```cpp
root.FieldVector<float, 2>(TAG_VEC, data);
auto read = read_root.ReadVector<float, 2>(TAG_VEC);
```

**Arrays before:**
```cpp
root.FieldArrayInt32(TAG, data, 5);
auto arr = read_root.ReadInt32Array(TAG);
```

**Arrays after:**
```cpp
root.FieldArray<int32_t>(TAG, {data, 5});
auto arr = read_root.ReadArray<int32_t>(TAG);
```

## Step 8 — Build & verify

- Build library + tests
- Run all tests, ensure pass
- Verify no new compiler warnings

---

## Execution Order

| # | Task | Files |
|---|------|-------|
| 1 | Strip forced-inline annotations | `Endianness.hpp`, `Reader.cpp`, `Writer.cpp` |
| 2 | Remove unsigned int types from enum | `include/tbf/DataType.hpp` |
| 3 | Update format spec | `docs/FORMAT.md` |
| 4 | Expand type traits (partially done) | `include/tbf/DataType.hpp` |
| 5 | Rewrite Writer | `include/tbf/Writer.hpp`, `src/Writer.cpp` |
| 6 | Rewrite Reader | `include/tbf/Reader.hpp`, `src/Reader.cpp` |
| 7 | Rewrite tests | `tests/test_*.cpp` |
| 8 | Build & verify | — |

## API Quick Reference (new vs old)

| Operation | Old API | New API |
|-----------|---------|---------|
| Write int32 | `obj.FieldInt32(tag, 42)` | `obj.Field<int32_t>(tag, 42)` |
| Read int32 | `obj.ReadInt32(tag)` | `obj.Read<int32_t>(tag)` |
| Write float array | `obj.FieldArrayFloat32(tag, p, n)` | `obj.FieldArray<float>(tag, {p, n})` |
| Read float array | `obj.ReadFloat32Array(tag)` | `obj.ReadArray<float>(tag)` |
| Write vec3 float | `obj.FieldVector3f32(tag, p)` | `obj.FieldVector<float, 3>(tag, arr)` |
| Read vec3 float | `obj.ReadVector3f32(tag)` | `obj.ReadVector<float, 3>(tag)` → `optional<array<float,3>>` |
| Write enum | `obj.FieldEnum(tag, val)` | `obj.Field(tag, val)` |
| Read enum | `obj.FieldEnum<E>(tag)` | `obj.ReadEnum<E>(tag)` |
