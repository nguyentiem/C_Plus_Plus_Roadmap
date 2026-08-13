# Bài 14: STL Algorithms, std::string, string_view, span, chrono

## Định nghĩa & Khái niệm

- **STL algorithms** (`<algorithm>`, `<numeric>`): hàm generic hoạt động trên **cặp iterator** — tách "thuật toán" khỏi "container". `sort`, `find_if`, `transform`, `accumulate`, `count_if`, `remove`, `lower_bound`...
- **`std::string`**: chuỗi sở hữu bộ nhớ riêng, có **SSO (Small String Optimization)** — chuỗi ngắn nằm ngay trong object, không heap.
- **`std::string_view`** (C++17): "cửa sổ" chỉ-đọc `{con trỏ, độ dài}` nhìn vào chuỗi của người khác — **không sở hữu**, không copy, không cấp phát.
- **`std::span<T>`** (C++20): tương tự string_view nhưng cho mảng bất kỳ kiểu `T`, có thể ghi — thay thế cặp tham số `(T* ptr, size_t len)`.
- **`std::chrono`**: thư viện thời gian type-safe — `duration` (khoảng), `time_point` (mốc), `clock` (nguồn).

## Giải thích chi tiết

### Vì sao dùng algorithm thay vòng lặp tay?
Ý đồ rõ ràng (`count_if` nói ngay "đếm theo điều kiện"), ít bug off-by-one, được tối ưu sẵn, và là ngôn ngữ chung khi review code.

### remove/erase idiom
`std::remove` **không xoá** — nó chỉ dồn các phần tử được giữ lên đầu và trả iterator "đuôi logic" (vì algorithm chỉ thấy iterator, không thấy container):
```
truoc:  [1][0][2][0][3]
remove(0):  [1][2][3][?][?]
                      ^ new_end
erase(new_end, end())  → vector thực sự co lại
```

### SSO — Small String Optimization
```
string ngắn (libstdc++: ≤15 ký tự):        string dài:
[ptr|size|  buffer nội bộ 16B  ]           [ptr ──────> heap "very long..." ]
     └─ ptr trỏ vào chính object!               size, capacity
```
→ chuỗi ngắn: tạo/copy rẻ, không heap. Hệ quả: move một chuỗi ngắn thực chất là copy buffer nội bộ.

Lưu ý toolchain: SSO là chi tiết cài đặt. libstdc++ ABI cũ (COW, `_GLIBCXX_USE_CXX11_ABI=0`, `sizeof(string)==8`) **không có SSO** — một số bản MinGW/Cygwin g++ vẫn dùng ABI này; demo trong main.cpp tự phát hiện và in ra ABI đang dùng.

### string_view — nhanh nhưng đầy bẫy dangling
```
string s = "hello world";
string_view sv = s;      // OK: s còn sống
sv = s.substr(0, 5);     // BẪY! substr trả string TẠM → sv dangling ngay lập tức
```
string_view không giữ object sống — nó chỉ là con trỏ + độ dài. Quy tắc: **không bao giờ** trả string_view trỏ vào biến cục bộ/tạm; không lưu string_view làm member trừ khi chắc chắn lifetime nguồn dài hơn.

### span — thay cho (ptr, len)
```cpp
void process(std::span<const uint8_t> data);   // nhận array, vector, C-array...
```
Một chữ ký hàm phục vụ mọi nguồn dữ liệu liên tục, kèm `.size()` — hết lỗi truyền sai độ dài. (main.cpp tự viết `SimpleSpan` minh hoạ vì bài này dùng C++17.)

### chrono
```cpp
auto t0 = std::chrono::steady_clock::now();   // steady: không bị chỉnh giờ hệ thống
// ... việc cần đo ...
auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(clk::now() - t0);
```
`system_clock` = giờ thực (có thể nhảy); `steady_clock` = đơn điệu, dùng để **đo khoảng thời gian**.

## Cách dùng
```cpp
std::sort(v.begin(), v.end(), [](auto a, auto b){ return a > b; });
auto it  = std::find_if(v.begin(), v.end(), [](int x){ return x > 10; });
int  n   = std::count_if(v.begin(), v.end(), [](int x){ return x % 2 == 0; });
int  sum = std::accumulate(v.begin(), v.end(), 0);
std::transform(v.begin(), v.end(), v.begin(), [](int x){ return x * 2; });
auto pos = std::lower_bound(v.begin(), v.end(), 42); // v phải đã sort, O(log n)
```

## Tips & Tricks

- `lower_bound` trên vector đã sort = "map của người nghèo": O(log n) lookup, footprint nhỏ nhất.
- `accumulate(v.begin(), v.end(), 0)` với vector<double> → cộng bằng **int** (kiểu của giá trị khởi đầu)! Dùng `0.0`.
- Nhận tham số chuỗi chỉ-đọc: `std::string_view` thay `const std::string&` — tránh cấp phát khi caller truyền literal.
- `std::transform` có thể ghi ra container khác (kết hợp `std::back_inserter`).
- C++20: `std::erase(v, value)` / `std::erase_if` gói sẵn remove/erase idiom một dòng.

## Lỗi thường gặp / Bẫy

1. Gọi `std::remove` mà quên `erase` → container còn nguyên size, đuôi chứa giá trị không xác định.
2. **string_view dangling**: trỏ vào `substr()`/chuỗi tạm/biến cục bộ đã chết (demo trong main.cpp).
3. `string_view` **không null-terminated** — cấm truyền `.data()` cho hàm C mong chuỗi kết thúc `\0`.
4. `lower_bound` trên dữ liệu chưa sort → kết quả vô nghĩa (yêu cầu tiền điều kiện!).
5. Đo thời gian bằng `system_clock` → kết quả âm khi NTP chỉnh giờ; luôn dùng `steady_clock`.

## Ghi chú Embedded

- Algorithms header-only, không heap (trừ `stable_sort`...) → dùng thoải mái trên MCU; `std::sort` trên `std::array` hoàn toàn static.
- `std::string` trên firmware: SSO giúp chuỗi ngắn không heap, nhưng vượt SSO là cấp phát động — nhiều codebase cấm string, dùng `etl::string<N>` hoặc buffer + `string_view`.
- `string_view`/`span` là công cụ **vàng** cho firmware: parse frame UART/BLE ngay trên buffer DMA mà không copy byte nào.
- `std::chrono::duration` dùng được để type-safe hoá tick RTOS: `k_msleep(std::chrono::milliseconds{10}.count())` — hết nhầm ms/us.

## Bài tập tự luyện

1. Cho `std::vector<int> adc` giả lập: dùng algorithm (không vòng lặp tay) tính min/max/trung bình, đếm mẫu vượt ngưỡng, và xoá mọi mẫu bão hoà (=4095).
2. Viết `parse_key_value(std::string_view line)` tách `"key=value"` thành cặp string_view — không cấp phát heap; chỉ ra trường hợp nào kết quả dangling.
3. Viết hàm `crc8(SimpleSpan<const unsigned char>)` và gọi với cả `std::array` lẫn mảng C — chứng minh một chữ ký phục vụ hai nguồn.

## Tóm tắt

- Algorithm + lambda thay vòng lặp tay: rõ ý đồ, ít bug; nhớ remove/erase idiom và tiền điều kiện sorted của `lower_bound`.
- `std::string` có SSO; `string_view` = con trỏ + độ dài, cực nhanh nhưng phải quản lý lifetime nguồn — bẫy dangling số 1.
- `span` (C++20) thay cặp `(ptr, len)`; C++17 tự viết vài dòng là có bản đơn giản.
- `chrono`: đo thời gian bằng `steady_clock`, biểu diễn khoảng bằng `duration` type-safe.
