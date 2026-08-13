# Bài 20: Tooling & Testing — CMake, static analysis, sanitizers, unit test, mocking, MISRA

## Định nghĩa & Khái niệm

- **CMake**: hệ sinh build meta — mô tả *targets* và quan hệ giữa chúng, sinh ra Makefile/Ninja/VS project. Chuẩn de-facto của C++ hiện đại (Zephyr, ESP-IDF, Pico SDK đều dùng).
- **Static analysis**: phân tích code KHÔNG chạy — bắt bug (null deref, buffer overflow, UB) và vi phạm style trước khi lên board. Công cụ: `clang-tidy`, `cppcheck`.
- **Sanitizers**: instrument lúc compile để bắt lỗi LÚC CHẠY: ASan (bộ nhớ), UBSan (undefined behavior), TSan (data race).
- **Unit test**: test tự động từng đơn vị logic, chạy trên host, nhanh (mili-giây), chạy mỗi lần build. Framework: GoogleTest, Catch2.
- **Mocking**: thay phụ thuộc phần cứng bằng đối tượng giả có kiểm soát (bài 18 — DI là điều kiện tiên quyết).
- **MISRA C++ / coding standard**: bộ quy tắc viết code cho hệ thống an toàn (automotive, y tế) — cấm/hạn chế các tính năng dễ gây lỗi.

## Giải thích chi tiết

### CMake — tư duy target-based (hiện đại)

CMake cũ: biến toàn cục (`CMAKE_CXX_FLAGS`, `include_directories`) — rò rỉ sang mọi target. CMake hiện đại (>=3.x): **mọi thứ gắn vào target** với phạm vi rõ ràng:

```cmake
cmake_minimum_required(VERSION 3.16)
project(bai20 CXX)

add_library(ringbuffer INTERFACE)              # header-only
target_include_directories(ringbuffer INTERFACE include)

add_executable(app main.cpp)
target_link_libraries(app PRIVATE ringbuffer)  # app dung ringbuffer, khong lan ra ngoai
target_compile_features(app PRIVATE cxx_std_17)
```

Phạm vi: `PRIVATE` (chỉ target này dùng), `INTERFACE` (chỉ người link dùng), `PUBLIC` (cả hai). Chọn sai phạm vi = phụ thuộc rò rỉ, build chậm, lỗi khó hiểu.

**Generator expressions** `$<...>`: giá trị quyết định lúc *generate*, cho phép cấu hình theo config/compiler mà không if-else rối:

```cmake
target_compile_options(app PRIVATE
    $<$<CONFIG:Debug>:-O0 -g>
    $<$<CONFIG:Release>:-O2>
    $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra>)
```

File `CMakeLists.txt` mẫu chạy được nằm ngay trong folder này (build thử: `cmake -S . -B build && cmake --build build`).

### Static analysis

```bash
cppcheck --enable=warning,style,performance --std=c++17 main.cpp
clang-tidy main.cpp -checks='bugprone-*,performance-*,readability-*' -- -std=c++17
```

`clang-tidy` cần compile flags (tốt nhất qua `compile_commands.json` — CMake sinh bằng `-DCMAKE_EXPORT_COMPILE_COMMANDS=ON`). Đưa vào CI: mọi commit đều bị soi — bug rẻ nhất là bug chưa vào repo.

### Sanitizers — chạy trên host TRƯỚC khi lên target

```bash
g++ -std=c++17 -g -fsanitize=address,undefined main.cpp -o test   # ASan + UBSan
g++ -std=c++17 -g -fsanitize=thread main.cpp -o test              # TSan (rieng)
./test   # loi in ra kem stack trace
```

- **ASan**: use-after-free, buffer overflow, leak. Chậm ~2x.
- **UBSan**: signed overflow, shift quá bit, null deref, misaligned. Gần như miễn phí — nên bật thường trực trong build test.
- **TSan**: data race (bài 16). Chậm ~5-15x, không dùng chung với ASan.

Ghi chú quan trọng: sanitizer cần OS + nhiều RAM → **không chạy trên MCU**. Chiến lược đúng: kiến trúc code để logic chạy được trên host (DI — bài 18), sanitize + test trên host, chỉ HAL mỏng test trên target. MinGW hỗ trợ sanitizer hạn chế — dùng WSL/Linux CI cho việc này.

### Unit test — GoogleTest/Catch2 và tư duy cốt lõi

```cpp
// GoogleTest                          // Catch2
TEST(RingBuffer, DayThiTraFalse) {     TEST_CASE("day thi tra false") {
    RingBuffer<int, 4> rb;                 RingBuffer<int, 4> rb;
    ...                                    ...
    EXPECT_FALSE(rb.push(9));              REQUIRE_FALSE(rb.push(9));
}                                      }
```

Bản chất mọi framework test chỉ là: (1) macro so sánh + báo dòng lỗi, (2) đăng ký test tự động, (3) runner đếm pass/fail. Để chứng minh, `main.cpp` bài này tự viết một **micro-framework ~30 dòng** (`TEST_CHECK`) và dùng nó test một ring buffer thật — hiểu ruột rồi thì dùng GoogleTest chỉ là đổi vỏ. Nguyên tắc viết test tốt: mỗi test một hành vi; test hành vi qua interface công khai, không test chi tiết hiện thực; đặt tên đọc thành câu; test biên (rỗng, đầy, wrap-around).

### MISRA C++ / coding standard

MISRA C++:2023 — quy tắc cho code an toàn: cấm cấp phát động sau khởi tạo, mọi switch có default, không dùng union kiểu cũ, khởi tạo mọi biến, hạn chế con trỏ số học... Không cần "đạt chuẩn" mới có ích: chọn tập quy tắc hợp lý + enforce bằng công cụ (cppcheck có addon MISRA; clang-tidy có bộ `cert-*`). Coding standard của team (format bằng `clang-format`, quy tắc đặt tên, quy tắc review) quan trọng hơn tên chuẩn.

## Cách dùng

```bash
# Build bang Makefile (nhu cac bai truoc)
make && ./bai20_tooling_testing.exe

# Build bang CMake (may co cmake + ninja/make)
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

## Tips & Tricks

- `-DCMAKE_EXPORT_COMPILE_COMMANDS=ON` → `compile_commands.json` cho clang-tidy/clangd/IDE — bật mặc định.
- Treat warnings as errors trong CI (`-Werror`) nhưng KHÔNG hardcode trong CMakeLists (người dùng compiler khác sẽ khổ) — truyền qua preset/toolchain.
- Test phải NHANH (<1s cả bộ) — chậm là không ai chạy nữa.
- Fixture dùng chung setup; nhưng test dễ đọc hơn test DRY — lặp một chút trong test là chấp nhận được.
- cppcheck bắt lỗi khác clang-tidy bắt — chạy CẢ HAI, chúng bổ sung nhau.
- Chạy test dưới ASan+UBSan trong CI: test xanh mà sanitizer đỏ vẫn là fail.

## Lỗi thường gặp / Bẫy

1. **Test logic dính chặt phần cứng** → "chỉ test được trên board" → không ai test. Gốc rễ: thiếu DI (bài 18).
2. **Chỉ test happy path** — bug sống ở biên: buffer đầy, size 0, wrap-around, giá trị âm.
3. **Test phụ thuộc thứ tự chạy / trạng thái toàn cục** → flaky. Mỗi test tự dựng thế giới của nó.
4. CMake: dùng `file(GLOB)` cho source — thêm file mới không trigger reconfigure. Liệt kê tường minh.
5. CMake: `include_directories()` toàn cục thay vì `target_include_directories` — phụ thuộc rò rỉ.
6. **Coverage 100% làm mục tiêu** — dễ đạt bằng test vô nghĩa. Coverage là công cụ tìm lỗ hổng, không phải KPI.

## Ghi chú Embedded

- Kim tự tháp test firmware: nhiều unit test trên host (logic thuần + mock HAL) → ít integration test trên emulator/target → rất ít test thủ công trên board.
- Zephyr có sẵn hệ test **ztest** + **twister**; chạy trên `native_sim` (host) trước khi lên nRF52840.
- CMake là ngôn ngữ build của Zephyr/NCS — hiểu target/scope ở đây áp dụng thẳng vào `CMakeLists.txt` của app Zephyr (`target_sources(app PRIVATE src/main.c)`).
- Cross-compile bằng CMake toolchain file (`CMAKE_TOOLCHAIN_FILE`) — tách mô tả build khỏi mô tả toolchain.
- MISRA phổ biến trong automotive/y tế; nếu làm sản phẩm cần chứng nhận (IEC 62304, ISO 26262), tooling + evidence (báo cáo phân tích, coverage) là yêu cầu bắt buộc, không phải tuỳ chọn.

## Bài tập tự luyện

1. Viết thêm 3 test cho ring buffer trong `main.cpp`: (a) push/pop xen kẽ qua điểm wrap-around 2 vòng, (b) pop từ buffer rỗng sau khi đã đầy rồi xả hết, (c) dung lượng 1. Tìm cách làm một test FAIL để xem báo lỗi.
2. Sinh `compile_commands.json` bằng CMake và chạy `clang-tidy` với bộ check `bugprone-*` lên `main.cpp`; sửa mọi cảnh báo.
3. (WSL/Linux) Cố ý viết use-after-free và signed overflow, biên dịch với `-fsanitize=address,undefined`, đọc hiểu stack trace mà sanitizer in ra.

## Tóm tắt

- CMake hiện đại = target + scope (PRIVATE/INTERFACE/PUBLIC) + generator expressions; tránh biến toàn cục và GLOB.
- Static analysis (clang-tidy + cppcheck) chạy trong CI — bắt bug trước khi vào repo.
- Sanitizers bắt lỗi runtime trên host: ASan (bộ nhớ), UBSan (UB, bật thường trực), TSan (race). Không chạy trên MCU — nên mới cần kiến trúc test trên host.
- Framework test chỉ là macro + registry + runner; giá trị nằm ở kỷ luật viết test biên, test hành vi.
- Mock hardware qua DI; MISRA/coding standard enforce bằng công cụ, không bằng lời hứa.
