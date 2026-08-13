# Bài 21: Error Handling — Exceptions, noexcept, error_code, expected

## Định nghĩa & Khái niệm

- **Exception**: cơ chế báo lỗi tách khỏi luồng return — `throw` ném object, `try/catch` bắt theo kiểu. Stack unwinding tự động gọi destructor của mọi local object (kết hợp RAII → không leak).
- **Exception safety guarantees** (chuẩn để review code):
  - **Nothrow**: hàm không bao giờ ném (`noexcept`) — destructor, move, swap PHẢI đạt mức này.
  - **Strong**: nếu ném, trạng thái chương trình *y như trước khi gọi* (commit-or-rollback).
  - **Basic**: nếu ném, không leak, invariant còn nguyên, nhưng giá trị có thể đã thay đổi.
  - **No guarantee**: code hỏng — không chấp nhận được.
- **`noexcept`**: cam kết không ném. Nếu vẫn ném → `std::terminate` (không unwinding). Cũng là *thông tin cho optimizer và cho STL* (vector chỉ move khi move ctor `noexcept`).
- **`std::error_code` / `std::error_category`**: báo lỗi không dùng exception — giá trị + category (domain), phổ biến trong filesystem, ASIO, hệ thống lớn.
- **`std::expected<T, E>` (C++23)**: kiểu trả về "hoặc giá trị T hoặc lỗi E" — thay thế cặp (bool + out param), tự viết được cho C++17 (Result<T,E>).
- **`-fno-exceptions`**: firmware thường tắt exception vì: code size (unwind tables ~10-15%), thời gian ném không xác định (không real-time), heap allocation khi throw trên một số ABI.

## Giải thích chi tiết

### Stack unwinding + RAII = không leak
```cpp
void f() {
    std::lock_guard<std::mutex> lk(m);   // RAII
    auto p = std::make_unique<int>(42);  // RAII
    may_throw();                          // ném → lk unlock, p delete TỰ ĐỘNG
}
```
Nếu dùng `new` thô + `delete` cuối hàm → ném là leak. **Exception chỉ an toàn khi toàn bộ resource là RAII.**

### Strong guarantee bằng copy-and-swap
```cpp
Widget& operator=(const Widget& o) {
    Widget tmp(o);        // mọi thứ có thể ném xảy ra Ở ĐÂY, this chưa bị đụng
    swap(*this, tmp);     // swap là nothrow
    return *this;         // tmp hủy đồ cũ
}
```

### Ném gì, bắt gì
```cpp
throw std::runtime_error("uart timeout");   // ném BY VALUE, kiểu kế thừa std::exception
catch (const std::exception& e) { ... }     // bắt BY CONST REFERENCE (tránh slicing)
```
- Không `throw new X` (leak), không `catch (std::exception e)` (slicing — mất thông tin lớp con).
- `catch (...)` chỉ dùng ở "biên" (main, thread entry) để log rồi rethrow/terminate sạch.
- Rethrow bằng `throw;` (giữ nguyên object), không `throw e;` (copy + slicing).

### Destructor tuyệt đối không ném
Destructor mặc định là `noexcept`. Nếu destructor ném *trong lúc unwinding* một exception khác → 2 exception cùng bay → `std::terminate`. Lỗi khi cleanup: log rồi nuốt.

### error_code — báo lỗi không-exception
```cpp
enum class UartError { ok = 0, timeout, framing, overrun };
std::error_code make_error_code(UartError e);   // + error_category riêng
std::error_code ec;
size_t n = uart_read(buf, len, ec);
if (ec) log("read failed: %s", ec.message().c_str());
```
Ưu điểm: chi phí xác định, hợp real-time; nhược: dễ *quên kiểm tra* (exception thì không quên được).

### expected — hướng hiện đại
```cpp
std::expected<Frame, UartError> read_frame();      // C++23
auto r = read_frame();
if (r) use(*r); else handle(r.error());
// hoặc chain: read_frame().and_then(parse).or_else(log_err);
```
Với C++17: tự viết `Result<T,E>` bằng union/variant (xem `main.cpp`).

## Cách dùng — chọn chiến lược nào?

| Ngữ cảnh | Chiến lược |
|---|---|
| App desktop/server, lỗi hiếm & nghiêm trọng | Exception |
| Lỗi là "chuyện thường" (parse, I/O timeout) | `expected`/`error_code` |
| Firmware, real-time, `-fno-exceptions` | `expected`/error enum + status LED/log |
| Constructor thất bại | Exception, hoặc factory trả `expected<T,E>` |
| Destructor, move, swap | Không bao giờ ném (`noexcept`) |

## Tips & Tricks

- `noexcept` cho move ctor/assign là **bắt buộc thực dụng** — quên là `vector` âm thầm copy (đã học bài 12).
- `noexcept(expr)` là operator kiểm tra compile-time: `static_assert(noexcept(swap(a,b)));`
- Đánh dấu hàm không-thể-fail (`getter`, hàm toán thuần) là `noexcept` → codegen gọn hơn.
- Exception là cho lỗi **exceptional** — đừng dùng làm control flow (chậm hơn return ~1000 lần khi ném).
- `std::system_error` = exception mang `error_code` — cầu nối 2 thế giới.
- Với `-fno-exceptions`: `new` fail → gọi `std::terminate`; STL container vẫn dùng được nhưng bad_alloc = chết — firmware nghiêm túc dùng static allocation (bài 22, 23).

## Lỗi thường gặp / Bẫy

1. `catch (std::exception e)` by value → slicing, mất message của lớp con.
2. Destructor ném → terminate khi đang unwinding.
3. `throw` trong constructor nhưng member đã cấp phát bằng con trỏ thô → leak (member RAII thì không).
4. Nuốt lỗi: `catch (...) {}` không log — bug "biến mất" rồi nổ chỗ khác.
5. Quên kiểm tra `error_code`/`expected` → dùng giá trị rác. (C++17: `[[nodiscard]]` để compiler cảnh báo.)
6. `noexcept` trên hàm có thể ném → terminate thay vì catch được.
7. Ném exception xuyên qua ranh giới C (callback đăng ký vào thư viện C) → UB.

## Ghi chú Embedded

- Đo thử: bật/tắt `-fno-exceptions -fno-rtti` và so sánh size `.elf` (thường tiết kiệm 10-20KB flash với binary nhỏ).
- ISR **tuyệt đối không ném** exception. Mã lỗi từ ISR: ghi vào ring buffer/flag atomic, task xử lý sau.
- Chuỗi xử lý lỗi firmware điển hình: error enum → error handler tập trung → log + đếm → watchdog/safe-state nếu nghiêm trọng.
- Nordic/Zephyr style: hàm trả `int` âm là mã lỗi (`-EINVAL`, `-ETIMEDOUT`) — chính là dạng error_code tối giản.

## Bài tập

1. Viết class `File` (RAII cho `FILE*`) với strong guarantee cho `reopen(path)` — nếu mở file mới thất bại, file cũ vẫn mở.
2. Thêm `[[nodiscard]]` vào `Result<T,E>` và chứng minh compiler cảnh báo khi quên kiểm tra.
3. Viết `error_category` riêng cho UART errors, in `ec.message()`.
4. Đo chi phí: vòng lặp 1 triệu lần return error code vs throw/catch — so sánh thời gian.
