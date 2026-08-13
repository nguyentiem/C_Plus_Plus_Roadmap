# Bài 10: constexpr, Type Traits & SFINAE

## Định nghĩa & Khái niệm

- **`constexpr`**: hàm/biến *có thể* được lượng giá lúc biên dịch. Biến `constexpr`
  bắt buộc là hằng compile-time; hàm `constexpr` chạy compile-time **nếu** đầu vào
  là hằng, ngược lại chạy runtime như hàm thường.
- **`consteval`** (C++20): hàm **bắt buộc** chạy lúc biên dịch (immediate function).
- **`constinit`** (C++20): đảm bảo biến toàn cục được **khởi tạo tĩnh** (không qua
  dynamic initialization lúc khởi động) nhưng vẫn được phép thay đổi sau đó.
- **Type traits**: bộ template trong `<type_traits>` trả lời câu hỏi về kiểu lúc
  biên dịch (`std::is_same`, `std::is_integral`, ...) hoặc biến đổi kiểu
  (`std::remove_const`, ...).
- **SFINAE** (Substitution Failure Is Not An Error): khi thay thế tham số template
  tạo ra kiểu không hợp lệ, ứng viên đó chỉ bị **loại khỏi danh sách overload** chứ
  không gây lỗi biên dịch — nền tảng của `std::enable_if`.
- **`static_assert`**: kiểm tra điều kiện lúc biên dịch, sai thì build fail kèm thông báo.
- **`if constexpr`** (C++17): rẽ nhánh lúc biên dịch — nhánh sai bị **loại bỏ**,
  không cần biên dịch được hoàn chỉnh.

## Giải thích chi tiết

### 1. constexpr vs consteval vs constinit

| Từ khóa | Áp dụng | Ý nghĩa |
|---|---|---|
| `constexpr` biến | biến | hằng compile-time, ngầm `const` |
| `constexpr` hàm | hàm | *có thể* chạy compile-time (hai chế độ) |
| `consteval` | hàm | *bắt buộc* compile-time, gọi với runtime arg → lỗi biên dịch |
| `constinit` | biến static/global | khởi tạo tĩnh bắt buộc, **không** ngầm const |

Tại sao cần `constinit`: tránh "static initialization order fiasco" và tránh code chạy
constructor lúc startup (trước `main`) — trên MCU, dynamic init tốn thời gian boot và
có thể chạy trước khi clock/RAM sẵn sàng.

Lưu ý C++17 (bài này build C++17): chỉ có `constexpr`; `consteval`/`constinit` là C++20
— nắm khái niệm, ví dụ code dùng `constexpr` + `static_assert` để *ép* lượng giá compile-time.

### 2. Type traits — "hỏi kiểu" lúc biên dịch

```cpp
static_assert(std::is_same_v<int, std::int32_t>);       // (tùy platform!)
static_assert(std::is_integral_v<char>);
static_assert(!std::is_floating_point_v<int>);
using T = std::remove_const_t<const int>;               // T = int
```

Traits là các struct template có member `::value` (predicate) hoặc `::type` (biến đổi).
Hậu tố `_v` / `_t` (C++14/17) là alias cho gọn. Tự viết trait cũng đơn giản —
specialization cho trường hợp "đúng":

```cpp
template<typename T> struct la_con_tro       : std::false_type {};
template<typename T> struct la_con_tro<T*>   : std::true_type  {};
```

### 3. SFINAE và enable_if

```cpp
template<typename T>
std::enable_if_t<std::is_integral_v<T>, T>   // kiểu trả về chỉ tồn tại nếu T nguyên
chia_doi(T x) { return x / 2; }

template<typename T>
std::enable_if_t<std::is_floating_point_v<T>, T>
chia_doi(T x) { return x / 2.0; }
```

Cơ chế: khi gọi `chia_doi(3.5)`, compiler thay `T = double` vào **cả hai** overload.
Bản integral tạo ra `enable_if_t<false, T>` — không có member `type` → substitution
failure → bản đó **âm thầm bị loại** (không phải lỗi!), còn lại bản floating được chọn.

```
Gọi chia_doi(3.5)
  ├─ overload integral : enable_if<false> → không có ::type → LOẠI (SFINAE)
  └─ overload floating : enable_if<true>  → hợp lệ          → CHỌN
```

### 4. if constexpr — cách hiện đại, dễ đọc hơn

```cpp
template<typename T>
T chia_doi(T x) {
    if constexpr (std::is_integral_v<T>) return x / 2;
    else                                 return x / T(2);
}
```

So sánh với SFINAE:
- **if constexpr**: một hàm duy nhất, nhánh sai bị loại bỏ (kể cả khi nhánh đó không
  biên dịch được với T hiện tại). Dễ đọc, dễ debug. **Ưu tiên dùng.**
- **SFINAE/enable_if**: cần khi phải chọn giữa các **overload/specialization khác nhau**,
  hoặc khi kiểm tra "kiểu T có method X không" (expression SFINAE). Vẫn gặp nhiều
  trong code thư viện. C++20 concepts thay thế đẹp hơn.

### 5. static_assert — hàng rào compile-time

`static_assert(dieu_kien, "thong bao")` — chi phí runtime **bằng 0**. Dùng để:
khóa layout struct (bài 08), ràng buộc tham số template, kiểm chứng bảng tính sẵn,
xác nhận giả định platform (`sizeof(int) == 4`).

## Cách dùng

```cpp
// Bảng CRC8 tính lúc BIÊN DỊCH — nằm trong flash, 0 chu kỳ runtime để tạo
constexpr std::uint8_t crc8_byte(std::uint8_t c) {
    for (int i = 0; i < 8; ++i)
        c = (c & 0x80) ? static_cast<std::uint8_t>((c << 1) ^ 0x07)
                       : static_cast<std::uint8_t>(c << 1);
    return c;
}
struct BangCrc8 { std::uint8_t v[256]; };
constexpr BangCrc8 tao_bang() {
    BangCrc8 b{};
    for (int i = 0; i < 256; ++i) b.v[i] = crc8_byte(static_cast<std::uint8_t>(i));
    return b;
}
constexpr auto BANG_CRC8 = tao_bang();
static_assert(BANG_CRC8.v[0x01] == 0x07, "kiem chung bang luc bien dich");
```

## Tips & Tricks

- Đánh `constexpr` cho mọi hàm thuần túy có thể — không mất gì, được thêm khả năng
  dùng trong `static_assert`/kích thước mảng/NTTP.
- Ép lượng giá compile-time ở C++17: gán vào biến `constexpr` hoặc dùng trong
  `static_assert` (vai trò của `consteval` khi chưa có C++20).
- `std::is_same_v<int32_t, int>` có thể khác nhau giữa các platform — chính là lý do
  firmware luôn dùng kiểu cố định `<cstdint>`.
- Trait tự viết: kế thừa `std::true_type`/`std::false_type` để có sẵn `::value`.
- `if constexpr` phải nằm trong **template** (hoặc phụ thuộc tham số template) thì
  nhánh sai mới được miễn kiểm tra đầy đủ.

## Lỗi thường gặp / Bẫy

1. Nghĩ `constexpr` hàm *luôn* chạy compile-time — không; với đầu vào runtime nó là
   hàm thường. Muốn chắc chắn: gán kết quả vào biến `constexpr`.
2. `if` thường thay vì `if constexpr` trong template → **cả hai** nhánh phải biên dịch
   được với mọi T → lỗi khó hiểu.
3. Hai overload `enable_if` không loại trừ lẫn nhau → gọi bị "ambiguous".
4. Đặt điều kiện `enable_if` ở default argument của template parameter rồi viết hai
   overload cùng chữ ký → lỗi redefinition (default arg không thuộc chữ ký).
5. Biến global `constexpr` trong header trước C++17 gây ODR issue — dùng `inline constexpr`.
6. Bảng tính bằng constexpr nhưng quên `constexpr`/`const` khi khai báo biến → bảng
   bị tạo lúc runtime và nằm trong RAM thay vì flash.
7. Đệ quy constexpr quá sâu → vượt giới hạn bước lượng giá của compiler (có cờ chỉnh:
   `-fconstexpr-depth`, `-fconstexpr-ops-limit`).

## Ghi chú Embedded

- **Bảng lookup compile-time** (CRC, sin, gamma, linearization của cảm biến NTC...):
  `constexpr` + mảng → dữ liệu nằm ở **.rodata/flash**, không tốn RAM, không tốn thời
  gian boot để tính. Ví dụ CRC8 ở trên đúng mẫu dùng cho frame UART/BLE.
- `static_assert` kiểm chứng bảng ngay lúc build — thiết bị không bao giờ chạy với
  bảng sai.
- `constinit` (C++20) loại bỏ dynamic initialization lúc startup — quan trọng khi
  firmware có yêu cầu thời gian boot hoặc thứ tự init nghiêm ngặt.
- Traits + `static_assert` làm hợp đồng API driver: 
  `static_assert(std::is_trivially_copyable_v<T>)` trong hàm gửi DMA — chặn kiểu có
  vtable/con trỏ nội bộ bị copy sai.
- Chuyển tính toán từ runtime sang compile-time = tiết kiệm chu kỳ CPU và năng lượng —
  đáng kể với thiết bị pin như nRF52840 BLE sensor.

## Bài tập tự luyện

1. Viết `constexpr` hàm tính bảng sin 8-bit 64 mẫu (xấp xỉ đa thức hoặc Taylor),
   kiểm chứng vài giá trị bằng `static_assert`, in bảng ở runtime.
2. Viết trait `co_ham_size<T>` (expression SFINAE với `decltype(t.size())`) trả
   true/false; dùng `if constexpr` in "có size()" / "không có size()" cho
   `std::array` và `int`.
3. Viết `to_bytes(T v)` chỉ nhận kiểu số nguyên (chặn bằng `enable_if` HOẶC
   `static_assert`), trả mảng byte little-endian; so sánh hai cách chặn về thông báo lỗi.

## Tóm tắt

- `constexpr` = *có thể* compile-time; `consteval` = *bắt buộc* (C++20);
  `constinit` = khởi tạo tĩnh, không const (C++20).
- Type traits trả lời/biến đổi kiểu lúc biên dịch; hậu tố `_v`/`_t` cho gọn.
- SFINAE: substitution fail chỉ loại ứng viên khỏi overload — cơ chế của `enable_if`.
- `if constexpr` dễ đọc hơn SFINAE cho rẽ nhánh theo kiểu trong một hàm; SFINAE khi
  cần chọn overload hoặc dò khả năng của kiểu.
- `static_assert` là hàng rào compile-time miễn phí.
- Embedded: bảng CRC/lookup `constexpr` nằm trong flash, được kiểm chứng lúc build —
  mẫu thiết kế tiêu biểu của C++ hiện đại cho firmware.
