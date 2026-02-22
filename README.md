# Tagged Binary Format (TBF)

A high-performance C++ binary serialization library with tagged fields, designed for speed and efficiency.

## Features

- 🏷️ **Tagged Fields** - Flexible schema with named or ID-based tags
- 📦 **Compact Format** - Efficient binary encoding
- 🔧 **Type Safe** - Field type validation
- 🎯 **Zero-Copy** - Direct memory access where possible
- 🔄 **Endian Aware** - Automatic endianness handling

## Supported Types

- **Primitives**: `int8`, `int16`, `int32`, `int64`, `uint8`, `uint16`, `uint32`, `uint64`, `float`, `double`, `bool`
- **Strings**: Variable-length UTF-8 strings
- **Binary**: Raw binary data
- **Objects**: Nested object structures
- **Arrays**: Arrays of any supported type
- **Vectors**: Fixed-size 2D, 3D, and 4D vectors

## Format Specification

For detailed information about the binary format, field types, and encoding specifications, see [FORMAT.md](docs/FORMAT.md).

## Building

### Standalone Build

```bash
# Basic build (library only)
mkdir build && cd build
cmake ..
make

# Build with tests
cmake -DTBF_BUILD_TESTS=ON ..
make

# Run tests
ctest
# or
./tests/tbf_tests
```

### Using as a Subdirectory in Your Project

Add TBF to your CMake project by including it as a subdirectory:

**Project Structure:**
```
your-project/
├── CMakeLists.txt
├── src/
│   └── main.cpp
└── external/
    └── tbf/          # Clone or add TBF here
```

**Your CMakeLists.txt:**
```cmake
cmake_minimum_required(VERSION 3.10)
project(YourProject)

set(CMAKE_CXX_STANDARD 23)

# Add TBF as subdirectory
set(TBF_BUILD_DOCS OFF CACHE BOOL "" FORCE)
add_subdirectory(external/tbf)

# Your executable
add_executable(your_app src/main.cpp)

# Link against TBF
target_link_libraries(your_app PRIVATE tbf)

# TBF headers are automatically available via target_include_directories
```

**Your source code (src/main.cpp):**
```cpp
#include <tbf/Writer.hpp>
#include <tbf/Reader.hpp>
#include <tbf/DataTag.hpp>

int main() {
    tbf::Writer writer(true);
    auto& root = writer.RootObject();
    root.FieldString("message", "Hello, TBF!");
    writer.Finish();
    
    return 0;
}
```

### Build Options

- `TBF_BUILD_TESTS` - Build test suite (default: OFF)

## License

This project is licensed under the MIT License - see the [LICENSE.md](LICENSE.md) file for details.

**Copyright (c) 2026 Electrodiux** - www.electrodiux.com

### Key Points:
- ✅ Free to use, modify, and distribute
- ✅ Private forks allowed
- ✅ Commercial use permitted
- ⚠️ Must credit Electrodiux as the original author
- ⚠️ No warranty or liability

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

When contributing, please:
- Maintain code quality and style consistency
- Add tests for new features
- Update documentation as needed
- Credit Electrodiux as the original author in derivative works
