# Bài 09: Templates (Khuôn mẫu)

## Định nghĩa & Khái niệm

- **Template**: cơ chế lập trình tổng quát (generic) của C++ — viết code một lần,
  compiler **sinh mã** (instantiate) cho từng kiểu/giá trị cụ thể lúc biên dịch.
- **Function template**: hàm tổng quát theo kiểu tham số.
- **Class template**: lớp tổng quát (vd `std::vector<T>`).
- **Specialization (đặc tả hóa)**: cung cấp hiện thực riêng cho một kiểu cụ thể
  (full) hoặc một họ kiểu (partial — chỉ có với class template).
- **Non-type template parameter (NTTP)**: tham số template là **giá trị** (số nguyên,
  con trỏ...) chứ không phải kiểu — vd kích thước buffer biết lúc biên dịch.
- **Variadic template**: template nhận số lượng tham số tùy ý (`typename... Ts`).
- **Fold expression (C++17)**: cú pháp gọn để "gấp" parameter pack qua một toán tử.
- **Two-phase lookup**: template được kiểm tra hai lần — lúc định nghĩa và lúc instantiate.

## Giải thích chi tiết

### 1. Template không phải code — nó là khuôn sinh code

```
template<typename T> T max2(T a, T b);
        │
        ├── max2<int>    ──► một hàm riêng cho int    (khi gọi max2(1, 2))
        ├── max2<double> ──► một hàm riêng cho double (khi gọi max2(1.5, 2.5))
        └── không gọi    ──► KHÔNG sinh code nào cả
```

Hệ quả:
- Template phải nằm trong **header** (compiler cần thấy toàn bộ định nghĩa để instantiate).
- Mỗi instantiation là code riêng → gọi trực tiếp, inline được, **zero overhead** —
  nhưng nhiều instantiation = **code bloat** (phình flash).
- Lỗi trong nhánh code không được instantiate có thể không bao giờ bị phát hiện.

### 2. Type deduction (suy luận kiểu)

`max2(1, 2.5)` **lỗi**: T không thể vừa là `int` vừa là `double`. Sửa: `max2<double>(1, 2.5)`
(chỉ định tường minh) hoặc hai tham số kiểu `T, U`. Deduction không thực hiện chuyển đổi
ngầm — đây là khác biệt lớn so với hàm thường.

### 3. Specialization

```cpp
template<typename T> struct Serializer { /* tong quat */ };
template<> struct Serializer<bool> { /* FULL: rieng cho bool */ };
template<typename T> struct Serializer<T*> { /* PARTIAL: cho moi con tro */ };
```

- **Full specialization** (`template<>`): thay thế hoàn toàn cho một kiểu cụ thể.
  Cả function lẫn class template đều có.
- **Partial specialization**: chỉ ràng buộc một phần (mọi `T*`, mọi `pair<T,int>`...).
  **Chỉ class template có** — với function, dùng overload thay thế (và overload
  thường tốt hơn cả full specialization vì tham gia overload resolution tự nhiên).

### 4. Non-type template parameter — vũ khí embedded

```cpp
template<typename T, std::size_t N>
class RingBuffer {
    T data_[N];             // mảng TĨNH, N biết lúc biên dịch
    ...
};
RingBuffer<uint8_t, 64> uartRx;   // không heap, sizeof biết trước, N là hằng
```

Tại sao quan trọng: kích thước nằm **trong kiểu** → không cần lưu ở runtime, compiler
tối ưu phép chia lấy dư (`% N` với N là lũy thừa 2 thành AND), không cấp phát động —
đúng triết lý firmware. `std::array<T, N>` chính là mẫu này.

### 5. Variadic templates + fold expressions

```cpp
template<typename... Ts>              // Ts... = parameter pack
auto tong(Ts... vals) {
    return (vals + ...);              // unary right fold: v1 + (v2 + (v3 + ...))
}
template<typename... Ts>
void in_tat_ca(const Ts&... vals) {
    ((std::cout << vals << ' '), ...); // fold qua toán tử phẩy
}
```

Trước C++17 phải đệ quy (hàm nhận 1 phần tử + hàm nhận phần còn lại); fold expression
làm phẳng tất cả trong một biểu thức. `sizeof...(Ts)` cho số phần tử của pack.

### 6. Two-phase lookup (cơ bản)

- **Phase 1 (lúc định nghĩa template)**: kiểm tra cú pháp + phân giải mọi tên
  **không phụ thuộc** vào T (non-dependent names).
- **Phase 2 (lúc instantiate)**: phân giải các tên **phụ thuộc** T (dependent names).

Hệ quả thực tế:
1. Lỗi cú pháp báo ngay cả khi chưa dùng template; lỗi dependent chỉ báo khi instantiate.
2. Trong template kế thừa base phụ thuộc T, phải viết `this->member` hoặc
   `Base<T>::member` — vì phase 1 không nhìn vào base phụ thuộc.
3. `typename` bắt buộc trước tên kiểu phụ thuộc: `typename T::value_type x;`
   (không có `typename`, compiler đoán đó là biến/hàm).

## Cách dùng

```cpp
// Function template
template<typename T>
constexpr const T& max2(const T& a, const T& b) { return a < b ? b : a; }

// Class template + NTTP
template<typename T, std::size_t N>
struct StaticBuffer {
    T data[N];
    static constexpr std::size_t size() { return N; }
};

// Variadic + fold
template<typename... Ts>
constexpr auto tong(Ts... v) { return (v + ... + 0); }  // + 0: hợp lệ cả khi pack rỗng
```

## Tips & Tricks

- Binary right fold `(v + ... + 0)` xử lý được pack rỗng; unary fold `(v + ...)` thì không.
- `if constexpr` (C++17) thay thế nhiều specialization: rẽ nhánh theo kiểu ngay trong
  một hàm, nhánh sai bị loại bỏ **không cần biên dịch được hoàn chỉnh**.
- Đặt `static_assert` trong template để cho lỗi dễ đọc: `static_assert(N > 0, "...")`.
- Giảm code bloat: tách phần không phụ thuộc T ra hàm thường/base class không template.
- Function: ưu tiên **overload** hơn full specialization.
- Với NTTP là kích thước, kiểm tra lũy thừa 2 lúc biên dịch: `static_assert((N & (N-1)) == 0)`.

## Lỗi thường gặp / Bẫy

1. Định nghĩa template trong `.cpp` → **linker error** "undefined reference" khi dùng
   từ file khác. Template ở trong header.
2. `max2(1, 2.5)` — deduction conflict, không tự chuyển đổi kiểu.
3. Quên `typename` trước kiểu phụ thuộc (`typename std::vector<T>::iterator`).
4. Trong template kế thừa `Base<T>`, gọi `member` trần → "not declared"; phải `this->member`.
5. Partial specialization cho **function** template → lỗi biên dịch (không tồn tại).
6. Code bloat: instantiate `RingBuffer<uint8_t, 64>`, `<uint8_t, 128>`, ... mỗi cái
   một bản code đầy đủ — cân nhắc trên MCU flash nhỏ.
7. Thông báo lỗi template dài hàng trăm dòng — đọc từ **dòng đầu tiên** và tìm
   "required from here".

## Ghi chú Embedded

- NTTP + mảng tĩnh = thay thế heap: `RingBuffer<uint8_t, N>` cho UART/BLE buffer —
  không `malloc`, không fragmentation, phù hợp quy tắc MISRA/no-heap.
- Template là nền của "đăng ký ngoại vi kiểu an toàn": `Gpio<PORT0, 13>` — pin sai
  bị bắt lúc biên dịch, mã sinh ra tương đương thao tác register trực tiếp.
- Đo code bloat bằng `arm-none-eabi-size`/map file khi dùng nhiều instantiation.
- CRTP (đa hình tĩnh bằng template) thay virtual trong đường nóng: zero indirect call.
- Toàn bộ tính toán template diễn ra lúc biên dịch — **không tốn một chu kỳ CPU nào** ở runtime.

## Bài tập tự luyện

1. Viết `template<typename T, std::size_t N> class RingBuffer` (push/pop/full/empty)
   dùng mảng tĩnh, kèm `static_assert` N là lũy thừa 2, index bằng mask `& (N-1)`.
2. Viết `Serializer<T>` tổng quát dùng `memcpy`, full specialization cho `bool`
   (1 byte 0/1), partial specialization cho `T*` (serialize địa chỉ — chỉ để học).
3. Viết variadic `checksum(Ts... bytes)` trả XOR của tất cả tham số bằng fold
   expression, dùng `static_assert(sizeof...(Ts) > 0)`.

## Tóm tắt

- Template = khuôn sinh code lúc biên dịch: zero overhead runtime, giá là thời gian
  biên dịch + có thể phình code.
- Định nghĩa template phải ở header; lỗi dependent chỉ hiện khi instantiate (two-phase lookup).
- Full specialization cho kiểu cụ thể; partial chỉ có với class; function dùng overload.
- NTTP đưa hằng số (kích thước buffer) vào kiểu — mẫu thiết kế cốt lõi cho embedded.
- Variadic + fold expression (C++17) xử lý số tham số tùy ý gọn gàng.
- `typename` và `this->` cần thiết khi tên phụ thuộc vào tham số template.
