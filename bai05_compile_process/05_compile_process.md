# Bài 05: Quá trình biên dịch — từ .cpp đến Executable/ELF

## Định nghĩa & Khái niệm

- **Preprocessor (tiền xử lý)**: xử lý văn bản thuần: `#include` (dán file), `#define` (thay macro), `#ifdef` (biên dịch có điều kiện). Kết quả là **translation unit (TU)** — một file .cpp sau khi mở rộng hết.
- **Compiler (biên dịch)**: dịch TU thành **assembly** rồi **object file (.o)** — mã máy nhưng địa chỉ các symbol ngoài chưa xác định.
- **Linker (liên kết)**: ghép các .o và thư viện, **resolve symbol** (nối lời gọi hàm với định nghĩa thật), gán địa chỉ cuối cùng → executable (PE trên Windows, **ELF** trên Linux/embedded).
- **ODR (One Definition Rule)**: mỗi symbol (hàm, biến, class) chỉ được có **một định nghĩa** trong toàn chương trình (khai báo thì bao nhiêu cũng được).
- **Name mangling**: compiler C++ mã hoá tên hàm kèm kiểu tham số (`add(int,int)` → `_Z3addii`) để hỗ trợ overloading.

## Giải thích chi tiết

### Pipeline đầy đủ

```
main.cpp ──(preprocessor: #include, #define)──► main.ii   (TU: văn bản C++ thuần)
main.ii  ──(compiler: parse, optimize)────────► main.s    (assembly)
main.s   ──(assembler)────────────────────────► main.o    (object: mã máy + symbol table)
main.o + utils.o + libc... ──(linker)─────────► a.exe/ELF (địa chỉ đã chốt)

Xem từng bước với g++:
  g++ -E main.cpp   # dừng sau preprocessor
  g++ -S main.cpp   # sinh main.s (assembly)
  g++ -c main.cpp   # sinh main.o (object)
  g++ main.o -o app # chỉ link
```

**Tại sao chia bước?** Mỗi .cpp biên dịch **độc lập** thành .o → sửa 1 file chỉ dịch lại 1 file (incremental build); linker mới là người nhìn thấy toàn cục.

### Header vs Source

- Header (.h/.hpp): **khai báo** (function prototype, class definition, extern, template) — là "hợp đồng" để các TU khác biết symbol tồn tại và có kiểu gì.
- Source (.cpp): **định nghĩa** (thân hàm, biến toàn cục) — mỗi định nghĩa đúng một nơi (ODR).
- Compiler không hề biết đến file khác; nó chỉ tin prototype trong header. Nếu prototype khai láo (sai kiểu) → compiler cho qua, **linker hoặc runtime mới vỡ**.

### Include guard vs `#pragma once`

```cpp
// Cách chuẩn:                     // Cách phổ biến:
#ifndef UTILS_H                    #pragma once
#define UTILS_H
...                                ...
#endif
```
Cả hai chống việc một header bị dán 2 lần vào cùng TU (gây định nghĩa lặp). `#pragma once` gọn, mọi compiler lớn đều hỗ trợ nhưng không thuộc chuẩn; include guard chuẩn 100%, nhưng phải đặt tên macro duy nhất.

### ODR — các cách vi phạm hay gặp

- Định nghĩa hàm/biến trong header rồi include vào 2 .cpp → linker báo "multiple definition". Sửa: chuyển định nghĩa vào .cpp, hoặc đánh dấu `inline` (C++17: cả biến `inline`).
- Hai .cpp định nghĩa hàm trùng chữ ký nhưng thân khác nhau → có thể link được nhưng UB (linker chọn 1 tùy ý).

### Name mangling và `extern "C"`

C++ mangling cho phép overload: `_Z3addii` (add(int,int)) khác `_Z3adddd` (add(double,double)). C **không** mangle: hàm `add` là symbol `add`. Muốn code C (hoặc code C++ biên dịch riêng) gọi được lẫn nhau:

```cpp
extern "C" void uart_init(void);   // "hàm này dùng tên/ABI kiểu C, đừng mangle"
```
Hệ quả: hàm `extern "C"` **không overload được**. Đây là lý do các header SDK C (nRF SDK, CMSIS) bọc `#ifdef __cplusplus extern "C" { #endif`.

### Static vs Dynamic linking

- **Static**: mã thư viện (`.a`/`.lib`) copy hẳn vào executable — file to hơn, nhưng tự chứa, không phụ thuộc runtime.
- **Dynamic**: executable chỉ ghi "tôi cần libfoo.so/.dll", nạp lúc chạy — file nhỏ, thư viện dùng chung, nhưng lệ thuộc môi trường ("DLL hell").
- Firmware MCU hầu như luôn **static** — không có OS/loader để nạp .so.

### Các section của ELF

| Section  | Chứa gì                              | Nằm ở đâu (MCU)       |
|----------|--------------------------------------|------------------------|
| `.text`  | mã lệnh                              | Flash                  |
| `.rodata`| hằng (`const`, chuỗi literal)        | Flash                  |
| `.data`  | biến global/static **có** khởi tạo ≠0| RAM (copy từ Flash lúc boot) |
| `.bss`   | biến global/static = 0 / chưa init   | RAM (zero-fill lúc boot, **không tốn flash**) |

Xem bằng `size app.exe` hoặc `objdump -h app.elf`.

## Cách dùng

```bash
# Build 2 file tách biệt rồi link
g++ -std=c++17 -Wall -Wextra -c utils.cpp   # -> utils.o
g++ -std=c++17 -Wall -Wextra -c main.cpp    # -> main.o
g++ main.o utils.o -o app

# Soi symbol và mangling
nm main.o                 # liệt kê symbol (U = undefined, T = định nghĩa trong .text)
echo _Z3addii | c++filt   # -> add(int, int)
size app                  # kích thước .text/.data/.bss
```

## Tips & Tricks

- Lỗi "undefined reference" là lỗi **linker** (thiếu định nghĩa/quên link .o hay -l); "was not declared" là lỗi **compiler** (thiếu header). Phân biệt được là debug nhanh gấp đôi.
- `nm`/`objdump`/`readelf` là bạn thân khi lỗi link khó hiểu; `c++filt` để giải mangled name.
- Hằng lớn nên khai `const`/`constexpr` → vào `.rodata` (flash) thay vì chiếm RAM.
- `static` ở phạm vi file (hoặc anonymous namespace trong C++) giới hạn symbol trong TU — tránh đụng ODR giữa các file.
- Thứ tự thư viện khi link có ý nghĩa với `ld`: `g++ main.o -lfoo` — thư viện đặt **sau** file cần nó.

## Lỗi thường gặp / Bẫy

1. **Định nghĩa hàm trong header không `inline`** → multiple definition khi include từ 2 TU.
2. **Quên `extern "C"`** khi link code C với C++ → "undefined reference to `uart_init`" dù thư viện có hàm đó (nhưng dưới tên không mangle).
3. **Sửa header mà không rebuild đủ** các .cpp phụ thuộc → object cũ lệch layout, hành vi kỳ quái. (Build system phải track dependency.)
4. **Khai báo `extern int x;` một kiểu, định nghĩa kiểu khác** → linker không kiểm tra kiểu của biến, chạy sai âm thầm.
5. **Include guard trùng tên macro** giữa 2 header khác nhau → header thứ hai bị "nuốt" mất một cách bí ẩn.
6. **Nhầm khai báo và định nghĩa**: `int x;` trong header là **định nghĩa** (vi phạm ODR nếu nhiều TU); phải là `extern int x;` + định nghĩa trong 1 .cpp.

## Ghi chú Embedded

- **Linker script** (`.ld`) là nơi khai memory map của MCU: nRF52840 có Flash 1MB tại `0x00000000`, RAM 256KB tại `0x20000000`. Script chỉ định `.text/.rodata` → FLASH, `.data/.bss` → RAM, và ký hiệu `_sdata/_edata/_sbss` cho startup code.
- **Startup code** (trước `main()`): copy `.data` từ flash sang RAM, zero-fill `.bss`, gọi ctor global (`.init_array`), rồi mới nhảy vào `main` — đây là lý do biến global "tự nhiên" có giá trị đúng.
- Output map file (`-Wl,-Map=app.map`) cho biết từng symbol nằm đâu, section nào chiếm bao nhiêu — công cụ số một khi "flash/RAM đầy".
- Khi dùng SoftDevice (S140) trên nRF52840, linker script phải **dời địa chỉ bắt đầu** của ứng dụng (ví dụ flash từ `0x27000`) để chừa chỗ cho SoftDevice — sai địa chỉ là không boot.

## Bài tập tự luyện

1. Tách `main.cpp` của bài này thành `utils.h/utils.cpp/main.cpp`; build từng bước bằng `-c` rồi link tay. Cố tình xoá `utils.o` khỏi lệnh link và đọc thông báo "undefined reference".
2. Dùng `g++ -S main.cpp` và tìm trong file .s tên mangled của một hàm overload; giải mã bằng `c++filt`. Thêm `extern "C"` vào hàm đó và quan sát tên thay đổi.
3. Khai báo `int g_bang[1000];` (bss) và `int g_bang2[1000] = {1};` (data); dùng `size` so sánh kích thước section trước/sau. Giải thích vì sao chỉ bản thứ hai làm file to lên.

## Tóm tắt

- Pipeline: preprocessor (văn bản) → compiler (TU → .o, từng file độc lập) → linker (ghép, resolve symbol, gán địa chỉ).
- Header = khai báo (hợp đồng), source = định nghĩa; ODR: mỗi định nghĩa đúng một nơi; chống include lặp bằng guard/`#pragma once`.
- C++ mangle tên để overload; `extern "C"` tắt mangle để tương tác code C.
- Static linking copy thư viện vào file; dynamic nạp lúc chạy; firmware dùng static.
- `.text/.rodata` ở flash, `.data` copy vào RAM lúc boot, `.bss` zero-fill; linker script + map file là công cụ sống còn trên MCU.
