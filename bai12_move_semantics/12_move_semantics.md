# Bài 12: Move Semantics — lvalue/rvalue, std::move, Rule of 5

## Định nghĩa & Khái niệm

- **lvalue**: biểu thức có *danh tính* (identity), thường có tên, lấy địa chỉ được (`x`, `arr[i]`, `*p`).
- **rvalue**: biểu thức tạm, sắp "chết" (`x + y`, `foo()` trả by value, literal `42`).
- **rvalue reference `T&&`**: tham chiếu chỉ bind vào rvalue → cho phép viết hàm "biết" đối số là đồ tạm và **cướp (steal)** tài nguyên của nó thay vì copy.
- **`std::move`**: KHÔNG move gì cả! Chỉ là một `static_cast<T&&>` — đánh dấu "tôi cho phép cướp tài nguyên của biến này".
- **`std::forward<T>`**: giữ nguyên value category của đối số trong template (perfect forwarding).
- **Rule of 3/5/0**: nếu tự viết một trong {destructor, copy ctor, copy assign} → cần cả 3; thêm move ctor + move assign → 5; tốt nhất là **0** — dùng RAII member (vector, unique_ptr) để compiler tự sinh tất cả.
- **Copy elision / RVO/NRVO**: compiler xây object trả về *trực tiếp tại chỗ caller* — không copy, không move (C++17 bắt buộc với RVO).

## Giải thích chi tiết

### Copy vs Move
```
COPY:  src [ptr]──>[■■■■■■ data ]        MOVE:  src [ptr]──> nullptr
       dst [ptr]──>[■■■■■■ copy ]  O(n)         dst [ptr]──>[■■■■■■ data ]  O(1)
```
Move chỉ "trộm" con trỏ và để nguồn ở trạng thái hợp lệ-nhưng-rỗng. Với class quản lý heap buffer, move biến O(n) thành O(1).

### Overload resolution
```cpp
void f(const T&);  // nhận mọi thứ (lvalue + rvalue)
void f(T&&);       // ưu tiên bắt rvalue → đường "nhanh"
Buffer b = make_buffer();     // rvalue → move ctor (hoặc elision)
Buffer c = b;                 // lvalue → copy ctor
Buffer d = std::move(b);      // ép lvalue thành xvalue → move ctor; b rỗng!
```

### Perfect forwarding
```cpp
template <typename T, typename... Args>
std::unique_ptr<T> my_make(Args&&... args) {
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}
```
`Args&&` ở đây là **forwarding reference** (không phải rvalue reference thuần) nhờ reference collapsing: `& + && = &`, `&& + && = &&`. `std::forward` khôi phục đúng value category: lvalue vào → lvalue ra (copy), rvalue vào → rvalue ra (move).

### RVO/NRVO
```cpp
Buffer make() { Buffer b(1024); return b; }   // NRVO: thường 0 copy 0 move
Buffer x = Buffer(512);                        // C++17: elision BẮT BUỘC
```
Vì vậy: **đừng viết `return std::move(local);`** — nó *tắt* NRVO và ép move không cần thiết.

## Cách dùng
```cpp
class Buffer {
    size_t n_ = 0; int* d_ = nullptr;
public:
    Buffer(Buffer&& o) noexcept : n_(o.n_), d_(o.d_) { o.d_ = nullptr; o.n_ = 0; }
    Buffer& operator=(Buffer&& o) noexcept {
        if (this != &o) { delete[] d_; d_ = o.d_; n_ = o.n_; o.d_ = nullptr; o.n_ = 0; }
        return *this;
    }
    // ... copy ctor/assign, dtor (Rule of 5)
};
```
Luôn đánh dấu move operations **`noexcept`** — nếu không, `std::vector` khi reallocate sẽ COPY thay vì move (để đảm bảo strong exception guarantee).

## Tips & Tricks

- Sau `std::move(x)`, `x` ở trạng thái "valid but unspecified" — chỉ được gán lại hoặc hủy, đừng đọc giá trị.
- `std::move` trên `const` object → âm thầm rơi về **copy** (bẫy kinh điển!).
- Ưu tiên **Rule of 0**: member là `std::vector`, `std::string`, `unique_ptr` → compiler tự sinh move/copy đúng.
- Tham số "sink" (hàm sẽ giữ lại giá trị): nhận **by value** rồi `std::move` vào member — 1 hàm phục vụ cả copy lẫn move.
- Đo đếm copy/move bằng counter trong ctor (như main.cpp) là cách thuyết phục nhất khi review hiệu năng.

## Lỗi thường gặp / Bẫy

1. `return std::move(local);` → tắt NRVO, chậm hơn.
2. Quên `noexcept` trên move ctor → vector reallocate bằng copy.
3. Move ctor quên reset nguồn (`o.d_ = nullptr`) → double-free.
4. Move assign quên kiểm tra self-move hoặc quên giải phóng tài nguyên cũ → leak.
5. Dùng `std::move` trong forwarding template thay vì `std::forward` → lvalue của caller bị "cướp" bất ngờ.
6. Nghĩ `std::move` di chuyển dữ liệu — nó chỉ là cast; move thật xảy ra trong move ctor/assign.

## Ghi chú Embedded

- Move semantics gần như **miễn phí** (vài lệnh gán con trỏ) → rất phù hợp firmware: truyền buffer DMA, message queue payload mà không copy.
- Với buffer tĩnh (không heap), move đôi khi vẫn là copy dữ liệu (ví dụ `std::array`) — hiểu rõ member là gì trước khi kỳ vọng O(1).
- `-fno-exceptions` phổ biến trong firmware: `noexcept` khi đó không đổi hành vi nhưng vẫn nên viết đúng để code portable.
- ETL (Embedded Template Library) container cũng tôn trọng move — thiết kế class Rule of 5 đúng giúp dùng lại được trên host lẫn target.

## Bài tập tự luyện

1. Thêm phương thức `append(const Buffer&)` vào class `Buffer` trong main.cpp và đo lại số copy/move khi `push_back` vào vector có/không `reserve`.
2. Bỏ `noexcept` khỏi move ctor của `Buffer` rồi chạy lại demo vector — giải thích vì sao copy count tăng.
3. Viết template `sink(T&& v)` dùng `std::forward` đẩy vào `std::vector<std::string>`; chứng minh lvalue bị copy còn rvalue bị move.

## Tóm tắt

- rvalue = đồ tạm → cho phép **move**: cướp tài nguyên O(1) thay vì copy O(n).
- `std::move` = cast xin phép; `std::forward` = giữ nguyên value category trong template.
- Rule of 5 khi tự quản lý tài nguyên; Rule of 0 khi có thể; move luôn `noexcept`.
- RVO/NRVO thường xoá cả move — đừng `return std::move(local)`.
