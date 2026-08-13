# Bài 08: Casting, struct vs class, POD & Memory Layout

## Định nghĩa & Khái niệm

- **Cast (ép kiểu)**: chuyển đổi tường minh giữa các kiểu. C++ có 4 toán tử cast riêng
  biệt thay cho cast kiểu C `(T)x` — mỗi loại nói rõ **ý định** và được compiler kiểm tra.
- **RTTI (Run-Time Type Information)**: thông tin kiểu lúc chạy, nền tảng của
  `dynamic_cast` và `typeid`.
- **POD / standard-layout / trivially-copyable**: các "mức độ đơn giản" của kiểu,
  quyết định có được phép `memcpy`, map thẳng vào bộ nhớ/register hay không.
- **Alignment/padding**: yêu cầu địa chỉ chia hết và các byte đệm compiler chèn vào struct.
- **Strict aliasing**: quy tắc compiler được giả định hai con trỏ khác kiểu không trỏ
  cùng vùng nhớ — vi phạm là UB.

## Giải thích chi tiết

### 1. Bốn loại cast — khi nào dùng cái nào

| Cast | Dùng khi | Kiểm tra | Chi phí runtime |
|---|---|---|---|
| `static_cast` | chuyển đổi "hợp lý": số↔số, `void*`→`T*`, Base*↔Derived* (khi **chắc chắn** kiểu) | compile-time | 0 (hoặc 1 phép đổi biểu diễn) |
| `dynamic_cast` | downcast **an toàn** trong cây đa hình, khi *không chắc* kiểu | runtime (RTTI) | tra cứu RTTI, chậm; trả `nullptr`/ném `bad_cast` nếu sai |
| `const_cast` | gỡ/thêm `const`/`volatile` — chủ yếu để gọi API C cũ thiếu const | compile-time | 0 |
| `reinterpret_cast` | diễn giải lại bit: con trỏ↔số nguyên, `T*`↔địa chỉ register | không kiểm tra gì | 0, nhưng dễ UB nhất |

Nguyên tắc chọn: **thử `static_cast` trước**; cần kiểm tra runtime → `dynamic_cast`;
chỉ đụng const → `const_cast`; còn lại (đụng đến biểu diễn bit/địa chỉ) → `reinterpret_cast`.
Cast kiểu C `(T)x` thử lần lượt các cast trên (kể cả bỏ const!) — che giấu ý định, cấm dùng.

```cpp
Base* b = laySensor();
if (auto* d = dynamic_cast<Derived*>(b)) { /* chac chan la Derived */ }
```

Lưu ý: `dynamic_cast` yêu cầu Base **có virtual function** (kiểu đa hình) và RTTI bật
(embedded thường build `-fno-rtti` → không dùng được).

- **`const_cast`**: chỉ hợp pháp khi object gốc **không** phải const. Sửa object vốn
  được khai báo `const` qua `const_cast` là UB (nó có thể nằm trong flash!).

### 2. struct vs class

Khác biệt **duy nhất**: `struct` mặc định `public` (member + kế thừa), `class` mặc định
`private`. Quy ước phổ biến: `struct` cho túi dữ liệu thuần (không invariant),
`class` khi có đóng gói/invariant cần bảo vệ.

### 3. POD / standard-layout / trivially-copyable

- **Trivially-copyable**: copy bằng `memcpy` là hợp lệ (không có copy ctor/dtor tự viết,
  không virtual). Kiểm tra: `std::is_trivially_copyable_v<T>`.
- **Standard-layout**: layout bộ nhớ tương thích C — mọi member cùng access control,
  không virtual, không base phức tạp. Kiểm tra: `std::is_standard_layout_v<T>`.
  Đây là điều kiện để **map struct chồng lên register/protocol frame**.
- **POD** (khái niệm cũ, C++20 bỏ) = trivially-copyable + standard-layout.

```
Frame giao thức (little-endian):        struct tương ứng:
+--------+--------+----------------+    struct Frame {
| header |  cmd   |    payload16   |        uint8_t  header;
| 1 byte | 1 byte |    2 byte      |        uint8_t  cmd;
+--------+--------+----------------+        uint16_t payload;   // offset 2: OK
                                        };  // sizeof == 4, không padding
```

### 4. Alignment & padding

Mỗi kiểu có `alignof(T)`: `uint32_t` phải nằm ở địa chỉ chia hết 4. Compiler chèn
**padding** để thỏa mãn:

```cpp
struct Xau { uint8_t a; uint32_t b; uint8_t c; };  // 1+3pad+4+1+3pad = 12 byte
struct Tot { uint32_t b; uint8_t a; uint8_t c; };  // 4+1+1+2pad     = 8 byte
```

Quy tắc thực dụng: **sắp member từ to đến nhỏ**. Với frame giao thức, dùng
`static_assert(sizeof(Frame) == N)` và `offsetof` để "khóa" layout — build fail ngay
nếu ai đó thêm field làm lệch.

### 5. Strict aliasing & cast sai → UB

Compiler giả định `float*` và `uint32_t*` **không** trỏ cùng vùng nhớ, nên nó tự do
sắp xếp lại lệnh đọc/ghi. Vi phạm:

```cpp
float f = 1.0f;
uint32_t bits = *reinterpret_cast<uint32_t*>(&f);   // UB! (type punning sai)
```

Cách đúng: `std::memcpy(&bits, &f, 4);` — compiler hiểu idiom này và tối ưu thành
một lệnh move, **zero cost**. (C++20 có `std::bit_cast`.) Ngoại lệ: đọc qua
`char*`/`unsigned char*`/`std::byte*` luôn hợp lệ.

Các UB do cast khác:
- `static_cast<Derived*>(basePtr)` khi object thật không phải Derived → UB.
- `reinterpret_cast` con trỏ tới kiểu có alignment lớn hơn địa chỉ thực → UB
  (Cortex-M0 hardfault ngay; Cortex-M4 chịu unaligned một phần nhưng vẫn là UB theo chuẩn).
- Ghi vào object `const` qua `const_cast` → UB.

## Cách dùng

```cpp
// Map struct vào register (embedded) - reinterpret_cast từ địa chỉ
struct UartRegs { volatile uint32_t DR; volatile uint32_t SR; };
static_assert(std::is_standard_layout_v<UartRegs>);
auto* uart = reinterpret_cast<UartRegs*>(0x40002000u);

// Đóng gói frame rồi gửi qua UART bằng memcpy
Frame f{0xAA, 0x01, 1234};
uint8_t tx[sizeof(Frame)];
std::memcpy(tx, &f, sizeof f);      // hợp lệ vì trivially-copyable
```

## Tips & Tricks

- Grep được: 4 cast C++ dễ tìm kiếm/review hơn cast kiểu C.
- `static_assert(sizeof(T) == N && std::is_trivially_copyable_v<T>)` ngay dưới định nghĩa
  struct giao thức — hợp đồng layout tự kiểm chứng.
- `offsetof(T, field)` kiểm tra vị trí từng field (chỉ hợp lệ với standard-layout).
- Cần packed? `#pragma pack(1)` / `__attribute__((packed))` — nhưng truy cập field có
  thể thành unaligned access; cân nhắc serialize thủ công từng byte.
- Endianness: struct-map chỉ đúng khi hai đầu cùng endian; giao thức chuẩn nên
  serialize tường minh.

## Lỗi thường gặp / Bẫy

1. Dùng cast kiểu C `(T)x` — vô tình gỡ const hoặc reinterpret mà không hay.
2. `dynamic_cast` với lớp không có virtual function → lỗi biên dịch; hoặc quên
   kiểm tra `nullptr` với con trỏ.
3. Type punning bằng `reinterpret_cast`/union → UB strict aliasing; dùng `memcpy`.
4. Quên `volatile` khi map register → compiler tối ưu mất lệnh đọc/ghi.
5. Struct có padding rồi `memcmp` để so sánh → so cả byte rác không xác định.
6. Gửi struct qua mạng/UART mà hai đầu khác padding/endian → data hỏng âm thầm.
7. `const_cast` rồi ghi vào biến vốn là `const` (nằm ở .rodata/flash) → crash hoặc UB.

## Ghi chú Embedded

- Map register: struct `volatile` + standard-layout là mẫu chuẩn CMSIS
  (xem `NRF_UART_Type` trong nrf52840.h — chính là kỹ thuật này).
- `-fno-rtti` (mặc định nhiều dự án firmware) → không có `dynamic_cast`/`typeid`;
  thiết kế để chỉ cần `static_cast` (vd tagged union, CRTP).
- Unaligned access: Cortex-M0/M0+ hardfault; M4/M7 hỗ trợ một phần (trừ LDM/STM,
  và vẫn tốn chu kỳ). Đừng `reinterpret_cast` buffer `uint8_t*` lệch địa chỉ thành `uint32_t*`.
- `static_assert` về layout chạy lúc **biên dịch** — chi phí runtime bằng 0, là hàng
  rào rẻ nhất chống regression khi refactor frame.

## Bài tập tự luyện

1. Định nghĩa frame `SensorPacket {sync, id, value(uint16), crc}` không padding,
   thêm `static_assert` sizeof + offsetof, viết hàm serialize bằng `memcpy` và
   deserialize lại, so sánh giá trị.
2. Viết cây `Shape` → `Circle`, `Rect`; dùng `dynamic_cast` phân loại trong mảng
   `Shape*`. Sau đó thay bằng enum tag + `static_cast` (mô phỏng môi trường `-fno-rtti`).
3. Tạo struct sắp xếp field tệ và tốt, in `sizeof`/`alignof`/`offsetof` từng field
   để thấy padding.

## Tóm tắt

- 4 cast: `static_cast` (mặc định), `dynamic_cast` (downcast an toàn, cần RTTI),
  `const_cast` (chỉ đụng const), `reinterpret_cast` (bit/địa chỉ — nguy hiểm nhất).
- struct vs class chỉ khác access mặc định; quy ước struct = túi dữ liệu.
- Trivially-copyable → được `memcpy`; standard-layout → map được vào register/frame;
  khóa layout bằng `static_assert` + `offsetof`.
- Sắp member to→nhỏ để giảm padding; chú ý endian khi trao đổi dữ liệu.
- Type punning đúng cách: `memcpy` (hoặc `std::bit_cast` C++20), không `reinterpret_cast`.
