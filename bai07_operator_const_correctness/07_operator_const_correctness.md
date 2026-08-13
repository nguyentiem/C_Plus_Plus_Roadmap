# Bài 07: Operator Overloading & Const Correctness

## Định nghĩa & Khái niệm

- **Operator overloading (nạp chồng toán tử)**: định nghĩa lại ý nghĩa của các toán tử
  (`+`, `==`, `<<`, `[]`, `()`, `=` ...) cho kiểu do người dùng định nghĩa, giúp code
  đọc như toán học: `a + b` thay vì `a.add(b)`.
- **Const correctness**: kỷ luật đánh dấu `const` ở mọi nơi có thể — tham số, hàm thành
  viên, biến — để compiler **ép buộc** những gì không được phép thay đổi.
- **Const member function**: hàm thành viên có `const` sau danh sách tham số — cam kết
  không sửa đổi trạng thái object (`this` có kiểu `const T*`).
- **mutable**: cho phép một data member được sửa ngay cả trong const member function
  (dùng cho cache, mutex — thứ không thuộc "trạng thái logic").
- **static member**: thuộc về **lớp**, không thuộc object — một bản duy nhất, không có `this`.

## Giải thích chi tiết

### 1. Nạp chồng toán tử: member hay non-member?

| Toán tử | Nên viết ở đâu | Tại sao |
|---|---|---|
| `=`, `[]`, `()`, `->` | **bắt buộc** member | chuẩn C++ quy định |
| `+`, `-`, `==`, `!=` | non-member (thường `friend`) | đối xứng: `2 + a` hoạt động nhờ chuyển đổi ngầm ở cả hai vế |
| `<<`, `>>` (stream) | non-member | vế trái là `std::ostream`, không sửa được lớp đó |
| `+=`, `-=` | member | sửa `*this`, trả về `T&` |

Quy tắc vàng: hiện thực `+` **bằng** `+=`:

```cpp
T& operator+=(const T& rhs);                       // member, làm việc thật
friend T operator+(T lhs, const T& rhs) {          // lhs copy theo giá trị
    lhs += rhs; return lhs;
}
```

Tương tự `!=` viết bằng `==`. Điều này tránh trùng lặp logic và bug lệch nhau.

### 2. Copy assignment (`operator=`)

```cpp
T& operator=(const T& other) {
    if (this != &other) {   // chống tự gán a = a
        // copy dữ liệu
    }
    return *this;           // cho phép chuỗi a = b = c
}
```

Với lớp quản lý tài nguyên, ưu tiên **copy-and-swap**: nhận tham số theo giá trị rồi
`swap` — an toàn exception, tự xử lý tự gán. Nếu không quản lý tài nguyên thô,
tuân theo **Rule of Zero**: đừng viết gì cả, để compiler sinh.

### 3. `operator[]` — hai phiên bản const và non-const

```cpp
int&       operator[](std::size_t i)       { return data_[i]; }  // đọc + ghi
const int& operator[](std::size_t i) const { return data_[i]; }  // cho object const
```

Không có bản `const` thì `const Buffer b; b[0];` không biên dịch được — mất tính dùng được
của lớp trong ngữ cảnh const (tham số `const&`...).

### 4. `operator()` — functor (đối tượng gọi được như hàm)

Object có `operator()` gọi được như hàm nhưng **mang trạng thái**. Đây là nền tảng của
lambda (lambda chính là functor compiler sinh ra). Rất hữu ích làm callback có ngữ cảnh
mà không cần con trỏ hàm + `void* user_data` kiểu C.

### 5. Const correctness — lan truyền như thế nào?

```
const T obj  ──chỉ gọi được──►  hàm thành viên const
T& tham số   ──nhận được──►     T, không nhận const T
const T& ts  ──nhận được──►     cả T lẫn const T lẫn giá trị tạm (temporary)
```

- Tham số vào chỉ-đọc: **luôn** `const T&` (với kiểu to) hoặc theo giá trị (kiểu nhỏ: int, double).
- Hàm thành viên không sửa trạng thái: **luôn** đánh `const`. Nếu quên, người dùng
  giữ `const T&` sẽ không gọi được — lỗi lan ngược rất khó sửa về sau ("const poisoning").
- `mutable` dành cho trạng thái *vật lý* không phải trạng thái *logic*: bộ đếm gọi,
  giá trị cache, mutex. Người ngoài nhìn vào object vẫn "không đổi".

### 6. Static members / functions

- `static` data member: một bản cho cả lớp, khai báo trong class, định nghĩa ngoài class
  (hoặc `inline static` từ C++17 — định nghĩa ngay trong class, tiện nhất).
- `static` member function: không có `this`, gọi qua `T::ham()`. Dùng cho factory,
  đếm instance, tiện ích gắn với lớp.

## Cách dùng

```cpp
class Vec2 {
    double x_, y_;
public:
    Vec2(double x, double y) : x_(x), y_(y) {}
    Vec2& operator+=(const Vec2& r) { x_ += r.x_; y_ += r.y_; return *this; }
    friend Vec2 operator+(Vec2 l, const Vec2& r) { l += r; return l; }
    friend bool operator==(const Vec2& a, const Vec2& b) {
        return a.x_ == b.x_ && a.y_ == b.y_;
    }
    friend std::ostream& operator<<(std::ostream& os, const Vec2& v) {
        return os << "(" << v.x_ << ", " << v.y_ << ")";
    }
    double x() const { return x_; }   // const member function
};
```

## Tips & Tricks

- Chỉ nạp chồng toán tử khi ý nghĩa **hiển nhiên** (Vec2 + Vec2 rõ; Employee + Employee thì không).
- `operator+` nhận vế trái **theo giá trị** (`T lhs`) — copy này thường được tối ưu (copy elision / move).
- Viết `==` rồi suy ra `!=`; viết `<` rồi suy ra `>`, `<=`, `>=` (C++20 có `<=>` làm hộ).
- Mặc định mọi hàm thành viên là `const` cho đến khi bắt buộc phải khác.
- `inline static` (C++17) tránh phải định nghĩa static member ở file .cpp riêng.
- Trả về `T&` từ `operator=`/`operator+=` để hỗ trợ chaining.

## Lỗi thường gặp / Bẫy

1. **`operator=` không kiểm tra tự gán** khi quản lý tài nguyên thô → giải phóng rồi copy từ vùng đã giải phóng.
2. **Quên bản const của `operator[]`** → object const không dùng được.
3. **`operator+` trả về tham chiếu tới biến cục bộ** → dangling reference, UB. Phải trả theo giá trị.
4. **Nạp chồng `&&`, `||`, `,`** → mất short-circuit, thứ tự lượng giá thay đổi — đừng làm.
5. **Const member function trả về `T&` tới member** → lách luật const. Bản const phải trả `const T&`.
6. Quên định nghĩa static data member (trước C++17) → lỗi linker "undefined reference".
7. Dùng `mutable` để né lỗi biên dịch thay vì suy nghĩ lại thiết kế — dấu hiệu code smell.

## Ghi chú Embedded

- Nạp chồng toán tử là **zero-cost abstraction**: `Vec2::operator+` biên dịch ra đúng
  mã máy như hàm `add()` thường — không có chi phí ẩn.
- `const` giúp compiler đặt dữ liệu vào **flash (.rodata)** thay vì RAM — quan trọng
  trên MCU (nRF52840: 1MB flash vs 256KB RAM). Lưu ý: phải là `constexpr`/`const` với
  initializer hằng.
- Cẩn thận toán tử che giấu chi phí: `operator+` cấp phát heap (như `std::string`)
  không phù hợp trong ISR hoặc hệ không dùng heap.
- Kiểu "strong typedef" bằng operator overloading (vd `Milliseconds`, `Volts`) bắt lỗi
  đơn vị lúc biên dịch — cực kỳ giá trị cho firmware.

## Bài tập tự luyện

1. Viết lớp `Fraction` (phân số) với `+`, `*`, `==`, `<<`, rút gọn tự động bằng GCD.
   Đảm bảo `2 + Fraction(1,2)` hoạt động (gợi ý: constructor không `explicit` + non-member operator).
2. Viết lớp `RingBuffer` có `operator[]` hai phiên bản const/non-const và một
   `static` counter đếm tổng số instance đã tạo.
3. Viết functor `Debounce` có `operator()(bool raw)` giữ trạng thái đếm, trả `true`
   khi tín hiệu ổn định N lần liên tiếp — mô phỏng chống dội phím.

## Tóm tắt

- Toán tử đối xứng (`+`, `==`) → non-member; `=`, `[]`, `()` → member.
- Hiện thực `+` qua `+=`, `!=` qua `==` — một nguồn sự thật duy nhất.
- `operator=`: chống tự gán, trả `*this`; ưu tiên Rule of Zero.
- Const correctness: đánh `const` mặc định — compiler thành công cụ kiểm chứng thiết kế.
- `mutable` cho trạng thái vật lý (cache/mutex), không phải để né lỗi.
- `static` member thuộc lớp; dùng `inline static` (C++17) cho gọn.
