# Bài 02: Con trỏ, Tham chiếu, Mảng và Con trỏ hàm

## Định nghĩa & Khái niệm

- **Con trỏ (pointer)**: biến chứa **địa chỉ** của một vùng nhớ khác. Kiểu `T*` trỏ tới đối tượng kiểu `T`.
- **Tham chiếu (reference)**: **bí danh (alias)** của một biến đã tồn tại. Kiểu `T&`. Phải khởi tạo ngay khi khai báo và không thể "trỏ lại" sang biến khác.
- **Mảng (array)**: dãy phần tử cùng kiểu, nằm liên tiếp trong bộ nhớ. Tên mảng "phân rã" (decay) thành con trỏ tới phần tử đầu khi truyền vào hàm.
- **Con trỏ hàm (function pointer)**: biến chứa địa chỉ của một hàm, cho phép gọi hàm gián tiếp (callback).
- **`nullptr`**: hằng con trỏ rỗng kiểu `std::nullptr_t` (C++11), thay cho `NULL`/`0` của C.
- **Dangling pointer**: con trỏ trỏ tới vùng nhớ **đã bị hủy** (đối tượng hết vòng đời hoặc đã `delete`).

## Giải thích chi tiết

### Mô hình bộ nhớ của con trỏ

```
   int x = 42;          int* p = &x;

   Địa chỉ    Nội dung
   0x1000  ┌─────────┐
           │   42    │  ◄── x
           └─────────┘
   0x2000  ┌─────────┐
           │ 0x1000  │  ◄── p (chứa ĐỊA CHỈ của x)
           └─────────┘

   *p  → "đi theo địa chỉ trong p" → đọc/ghi giá trị 42
   &x  → lấy địa chỉ của x         → 0x1000
```

**Tại sao cần con trỏ?** Vì đôi khi ta cần: (1) thay đổi biến của caller từ trong hàm, (2) duyệt vùng nhớ liên tiếp hiệu quả, (3) quản lý bộ nhớ động, (4) truy cập thanh ghi phần cứng qua địa chỉ cố định (embedded).

### Tham chiếu — con trỏ "an toàn hoá"

Tham chiếu về bản chất thường được compiler hiện thực bằng con trỏ, nhưng ngữ nghĩa khác hẳn:

| Tiêu chí              | Con trỏ `T*`        | Tham chiếu `T&`         |
|-----------------------|---------------------|--------------------------|
| Có thể null           | Có (`nullptr`)      | Không (phải gắn với biến)|
| Gán lại đích khác     | Được (`p = &y`)     | Không                    |
| Số học địa chỉ        | Có (`p+1`)          | Không                    |
| Cú pháp truy cập      | `*p`, `p->m`        | Dùng như biến thường     |

Quy tắc chọn (hiện đại): **mặc định dùng tham chiếu**; chỉ dùng con trỏ khi cần biểu diễn "có thể không tồn tại" (nullable) hoặc cần trỏ lại/đi qua mảng.

### Pointer arithmetic (số học con trỏ)

`p + 1` không cộng 1 byte mà cộng `sizeof(T)` byte — con trỏ "nhảy" đúng một phần tử. Đây là lý do `arr[i]` tương đương `*(arr + i)`. Chỉ hợp lệ **bên trong cùng một mảng** (kể cả vị trí one-past-the-end); vượt ra ngoài là undefined behavior (UB).

### const pointer vs pointer to const

Đọc từ **phải sang trái**:

```cpp
const int* p1;        // con trỏ tới int hằng: KHÔNG sửa được *p1, sửa được p1
int* const p2 = &x;   // con trỏ hằng tới int: sửa được *p2, KHÔNG sửa được p2
const int* const p3 = &x; // cả hai đều hằng
```

### Con trỏ hàm

```cpp
int add(int a, int b);
int (*fp)(int, int) = &add;   // hoặc = add (tự decay)
int r = fp(2, 3);             // gọi gián tiếp
```

Dùng cho callback, bảng lệnh (dispatch table), driver API. Trong C++ hiện đại có thể thay bằng `std::function` / lambda, nhưng con trỏ hàm thuần vẫn phổ biến trong firmware vì **không cấp phát heap**.

### Khác biệt C vs C++ (nhắc lại ngắn — chi tiết ở bài 1)

- C++ có **namespace**, **function overloading**, **default arguments** — C không có.
- C++ có tham chiếu; C chỉ có con trỏ.
- C++ dùng `nullptr` thay cho `NULL` (macro `0` hoặc `(void*)0` trong C) — `nullptr` có kiểu riêng nên không nhầm với số nguyên khi overload.
- Ép kiểu `void*` sang `T*` trong C++ phải tường minh; trong C thì ngầm định.

## Cách dùng

```cpp
void tang(int& x) { ++x; }          // tham chiếu: sửa trực tiếp biến caller
void tang_ptr(int* x) { if (x) ++*x; } // con trỏ: phải kiểm tra null

int arr[4] = {1, 2, 3, 4};
int* p = arr;                        // decay: arr -> &arr[0]
for (int* it = p; it != p + 4; ++it) // duyệt bằng pointer arithmetic
    std::cout << *it << ' ';
```

## Tips & Tricks

- Luôn khởi tạo con trỏ (`= nullptr` nếu chưa có đích) — con trỏ rác nguy hiểm hơn con trỏ null vì không kiểm tra được.
- Truyền tham số: object to đọc-only → `const T&`; sửa được → `T&`; nullable → `T*`.
- `sizeof(arr)/sizeof(arr[0])` chỉ đúng với mảng thật, **sai với con trỏ** (mảng đã decay). Dùng `std::size(arr)` (C++17).
- Đọc khai báo phức tạp từ trong ra ngoài, phải sang trái: `int (*fp)(int)` = "fp là con trỏ tới hàm nhận int trả int".
- Gán `p = nullptr` ngay sau `delete p` để biến dangling pointer thành null pointer (crash rõ ràng thay vì UB âm thầm).

## Lỗi thường gặp / Bẫy

1. **Dangling pointer**: trả về địa chỉ biến cục bộ, hoặc dùng con trỏ sau khi vùng nhớ bị hủy.
   ```cpp
   int* bad() { int x = 1; return &x; } // x chết khi hàm return -> dangling!
   ```
2. **Dereference null/garbage pointer**: `*p` khi `p == nullptr` hoặc chưa khởi tạo → crash (may mắn) hoặc UB.
3. **Off-by-one khi duyệt mảng**: `<= n` thay vì `< n` → đọc/ghi ngoài mảng.
4. **So sánh con trỏ giữa hai mảng khác nhau**: UB (chỉ dùng `std::less` hoặc tránh hẳn).
5. **Nhầm `const int*` với `int* const`** — đọc từ phải sang trái để phân biệt.
6. **Dùng `NULL` trong C++**: `f(NULL)` có thể chọn overload `f(int)` thay vì `f(int*)`. Dùng `nullptr`.

## Ghi chú Embedded

- Truy cập thanh ghi ngoại vi chính là con trỏ tới địa chỉ cố định:
  ```cpp
  volatile uint32_t* const GPIO_OUT = reinterpret_cast<uint32_t*>(0x50000504); // nRF52840 P0.OUT
  *GPIO_OUT |= (1u << 13);  // bật LED
  ```
  `volatile` bắt buộc: giá trị thanh ghi thay đổi ngoài tầm nhìn compiler, không được tối ưu bỏ lệnh đọc/ghi.
- Con trỏ hàm là xương sống của **vector table** (bảng vector ngắt của Cortex-M là mảng con trỏ hàm) và callback trong driver (`nrfx`, HAL).
- Trên MCU không có MMU: dereference con trỏ sai không segfault ngay mà gây **HardFault** hoặc âm thầm ghi đè RAM — khó debug hơn PC rất nhiều.
- Tránh `std::function` trong ISR/firmware nhỏ (có thể cấp phát heap); ưu tiên con trỏ hàm thuần.

## Bài tập tự luyện

1. Viết hàm `void swap2(int& a, int& b)` và `void swap_ptr(int* a, int* b)`; so sánh cú pháp phía caller. Trường hợp nào `swap_ptr` cần kiểm tra gì thêm?
2. Viết hàm `int sum(const int* arr, size_t n)` chỉ dùng pointer arithmetic (không dùng `arr[i]`). Giải thích vì sao tham số là `const int*`.
3. Tạo bảng lệnh `struct Cmd { const char* name; void (*handler)(); };` với 3 lệnh, viết vòng lặp tra tên và gọi handler tương ứng (mô phỏng shell UART trên MCU).

## Tóm tắt

- Con trỏ chứa địa chỉ; tham chiếu là bí danh — mặc định dùng tham chiếu, dùng con trỏ khi cần nullable hoặc số học địa chỉ.
- `p + 1` nhảy `sizeof(T)` byte; `arr[i]` ≡ `*(arr + i)`; chỉ hợp lệ trong phạm vi mảng.
- Đọc `const` từ phải sang trái: `const int*` (không sửa giá trị) ≠ `int* const` (không đổi đích).
- Dùng `nullptr`, khởi tạo mọi con trỏ, cẩn thận dangling pointer.
- Embedded: thanh ghi = con trỏ `volatile` tới địa chỉ cố định; vector table = mảng con trỏ hàm.
