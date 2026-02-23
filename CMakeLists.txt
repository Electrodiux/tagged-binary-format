# ----------- CMake Configuration -----------

cmake_minimum_required(VERSION 3.10)

# ----------- Project Configuration -----------

project(TaggedBinaryFormat VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON) # Generate compile_commands.json for IDE support

option(TBF_BUILD_TESTS "Build the TBF tests" OFF)

# ----------- Include Directories & Source Files -----------

file(GLOB LIBRARY_SOURCES "src/*.cpp")

# ----------- Library Creation -----------

add_library(tbf STATIC ${LIBRARY_SOURCES})
target_include_directories(tbf PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Apply flags based on build type
target_compile_options(tbf PRIVATE
   $<$<CONFIG:Debug>:-O0 -g -Wall -Wextra -Wpedantic>
   $<$<CONFIG:Release>:-O3 -DNDEBUG -march=native -mtune=native -flto -funroll-loops>
)

# ----------- Test Configuration -----------

if(TBF_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()