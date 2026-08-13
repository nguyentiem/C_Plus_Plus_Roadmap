# Bài 15: Modern C++ — lambda, auto, optional/variant, if constexpr, concepts

## Định nghĩa & Khái niệm

- **Lambda**: hàm vô danh + trạng thái bắt được (capture). Compiler sinh một class có `operator()` (closure type).
- **Capture**: `[x]` copy, `[&x]` tham chiếu, `[=]`/`[&]` bắt tất cả, `[this]` bắt con trỏ object, `[v = std::move(x)]` init-capture.
- **`auto` / `decltype`**: suy luận kiểu — `auto` từ initializer (bỏ ref/const trừ khi viết `auto&`/`const auto&`), `decltype(expr)` lấy kiểu *chính xác* của biểu thức.
- **Structured bindings** (C++17): `auto [a, b] = pair;` — bung tuple/pair/struct/mảng thành tên riêng.
- **`std::optional<T>`**: "có thể có T hoặc không" — thay magic value (-1, nullptr) và cờ bool đi kèm.
- **`std::variant<A,B,C>`**: union type-safe — đúng MỘT trong các kiểu, truy cập bằng `std::get`/`std::visit`.
- **`std::any`**: chứa kiểu bất kỳ (type-erased); truy cập phải biết đúng kiểu (`any_cast`).
- **`if constexpr`** (C++17): rẽ nhánh **tại compile-time** trong template — nhánh sai bị loại bỏ, không cần biên dịch được.
- **Concepts** (C++20): ràng buộc template có tên (`requires`) — lỗi template rõ ràng, overload theo tính chất kiểu.

## Giải thích chi tiết

### Lambda dưới nắp capo
```cpp
int k = 3;
auto f = [k](int x) { return x * k; };
// tương đương compiler sinh:
struct __Lambda { int k; int operator()(int x) const { return x * k; } };
```
- `mutable` cho phép sửa bản copy đã capture (bỏ `const` trên `operator()`).
- **Generic lambda** `[](auto a, auto b)` → `operator()` là template.
- Bẫy lifetime: `[&]` hoặc `[this]` trong callback chạy **sau này** (timer, thread, queue) → tham chiếu/this có thể đã chết → dangling. Quy tắc: callback thoát khỏi scope hiện tại ⇒ capture **by value** (hoặc shared/weak_ptr).

### optional / variant / any — thang đo "biết kiểu lúc compile"
```
optional<T> : có / không có T           (thay -1, nullptr, bool + out-param)
variant<A,B>: đúng một trong A, B       (thay union + tag tự chế)
any         : kiểu gì cũng được          (mất kiểm tra compile-time — dùng dè dặt)
```
`std::visit(overloaded{...}, v)` là pattern chuẩn xử lý variant: compiler **bắt buộc** xử lý đủ mọi kiểu — thêm kiểu mới vào variant mà quên nhánh là lỗi biên dịch (khác `switch` + enum dễ quên `case`).

### if constexpr vs if thường
```cpp
template <typename T>
auto describe(T v) {
    if constexpr (std::is_floating_point_v<T>) return v * 0.5;  // chỉ tồn tại khi T là float
    else                                        return v * 2;   // nhánh kia bị LOẠI BỎ
}
```
`if` thường yêu cầu **cả hai nhánh** hợp lệ với mọi T; `if constexpr` thì không — nền tảng của template metaprogramming hiện đại.

### Concepts (C++20)
```cpp
template <typename T>
concept Arithmetic = std::is_arithmetic_v<T>;

template <Arithmetic T>       // gọn
T twice(T v) { return v + v; }

template <typename T> requires Arithmetic<T>   // dạng requires-clause
T half(T v) { return v / 2; }
```
Trước concepts: gọi sai kiểu → 200 dòng lỗi template khó hiểu. Sau concepts: *"constraint 'Arithmetic' not satisfied"* — một dòng, đúng chỗ. Concepts còn cho phép **overload theo tính chất**: bản cho integral, bản cho floating_point.

## Cách dùng
```cpp
auto [it, ok] = mymap.insert({k, v});          // structured binding
std::optional<int> parse(std::string_view s);
if (auto r = parse("42")) use(*r);              // optional trong if
std::variant<int, std::string> v = 5;
std::visit([](const auto& x){ print(x); }, v);  // generic lambda + visit
```

## Tips & Tricks

- `auto` bỏ reference: `auto x = get_ref();` là **copy**; muốn tham chiếu viết `auto&`/`const auto&`.
- Init-capture để move vào lambda: `[buf = std::move(big)]{ ... }` — tránh copy nặng.
- Pattern `overloaded` (kế thừa nhiều lambda) + `std::visit` = "pattern matching" của C++17/20.
- `optional::value_or(default)` gọn hơn if/else; nhưng lưu ý nó luôn *tính* default.
- Concepts chuẩn có sẵn trong `<concepts>`: `std::integral`, `std::floating_point`, `std::invocable`...

## Lỗi thường gặp / Bẫy

1. **Capture `[&]` / `[this]` dangling**: lambda sống lâu hơn biến/object bị bắt (callback, thread) → UB. (main.cpp demo cách an toàn.)
2. `mutable` sửa **bản copy** trong closure — biến gốc bên ngoài không đổi; hai lần gọi liên tiếp giữ trạng thái giữa các lần.
3. `auto x = m[key];` với map trả copy — sửa `x` không sửa map.
4. `std::get<T>(variant)` sai kiểu đang giữ → ném `bad_variant_access`; kiểm tra bằng `holds_alternative` hoặc dùng `visit`.
5. `optional<T&>` không tồn tại — dùng `T*` hoặc `reference_wrapper`.
6. Quên `-std=c++20` → `requires`/concepts không biên dịch.

## Ghi chú Embedded

- Lambda không capture chuyển đổi ngầm thành **con trỏ hàm** → gắn thẳng vào C callback/ISR table (Zephyr, nRF SDK) mà zero overhead.
- Lambda có capture = struct trên stack — vẫn không heap; nhưng `std::function` thì **có thể cấp phát heap** → firmware ưu tiên template tham số callable hoặc con trỏ hàm.
- `std::optional`/`variant` không heap (in-place storage) → rất hợp firmware: `optional<Reading>` cho sensor đọc lỗi, `variant<Idle, Measuring, Error>` làm state machine type-safe.
- `if constexpr` + template giúp viết driver cấu hình theo board tại compile-time — code nhánh không dùng bị loại khỏi binary (tiết kiệm flash).
- `std::any` dùng type-erasure + có thể heap → tránh trên MCU.

## Bài tập tự luyện

1. Viết bộ đếm bằng lambda `mutable` và giải thích tại sao biến gốc không đổi; sau đó sửa bằng capture by reference và chỉ ra khi nào cách này nguy hiểm.
2. Cài state machine LED bằng `std::variant<Off, Blinking, On>` + `std::visit` với pattern `overloaded`; thêm state mới và quan sát compiler bắt thiếu nhánh.
3. Viết concept `Sensor` yêu cầu có `read() -> int` và `id() -> const char*`; hàm template `log_sensor` chỉ nhận kiểu thoả concept — thử truyền kiểu sai và đọc thông báo lỗi.

## Tóm tắt

- Lambda = closure object; nắm chắc capture (value/ref/init) và lifetime để tránh dangling — bẫy số 1 với callback.
- `auto` tiện nhưng bỏ ref/const; `decltype` lấy kiểu chính xác; structured bindings bung nhiều giá trị gọn gàng.
- `optional`/`variant` (+`visit`) mã hoá "có thể vắng mặt"/"một trong nhiều" vào type system, không heap; `any` là lối thoát cuối cùng.
- `if constexpr` rẽ nhánh compile-time; concepts (C++20) làm template có ràng buộc rõ ràng, lỗi dễ đọc.
