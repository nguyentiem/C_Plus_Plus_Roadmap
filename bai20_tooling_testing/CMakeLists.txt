# CMakeLists.txt mau — CMake hien dai (target-based) cho bai 20
# Build:  cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
#         cmake --build build
cmake_minimum_required(VERSION 3.16)
project(bai20_tooling_testing CXX)

# Sinh compile_commands.json cho clang-tidy / clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(bai20_tooling_testing main.cpp)

# Yeu cau C++17 gan vao TARGET (khong dung bien toan cuc CMAKE_CXX_STANDARD)
target_compile_features(bai20_tooling_testing PRIVATE cxx_std_17)

# Generator expressions: co canh bao theo compiler, co toi uu theo config
target_compile_options(bai20_tooling_testing PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra>
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
    $<$<AND:$<CONFIG:Release>,$<CXX_COMPILER_ID:GNU,Clang>>:-O2>
    $<$<AND:$<CONFIG:Debug>,$<CXX_COMPILER_ID:GNU,Clang>>:-O0 -g>)

# Vi du them thu vien header-only (INTERFACE) — mau cho du an lon hon:
# add_library(ringbuffer INTERFACE)
# target_include_directories(ringbuffer INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
# target_link_libraries(bai20_tooling_testing PRIVATE ringbuffer)

# Bat test voi CTest: ctest --test-dir build
enable_testing()
add_test(NAME microtest COMMAND bai20_tooling_testing)
