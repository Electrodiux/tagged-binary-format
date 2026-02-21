# ----------- Find or fetch Google Test -----------

include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# ----------- Add test executables -----------

file(GLOB TEST_SOURCES "test_*.cpp")
add_executable(tbf_tests ${TEST_SOURCES})

target_link_libraries(tbf_tests PRIVATE 
    tbf
    GTest::gtest_main
)

# ----------- Discover tests -----------

include(GoogleTest)
gtest_discover_tests(tbf_tests)
