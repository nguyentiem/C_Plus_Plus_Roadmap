# Bài 13: STL Containers — chọn đúng container, hiểu đúng chi phí

## Định nghĩa & Khái niệm

- **`std::array<T, N>`**: mảng kích thước cố định lúc compile, nằm trên stack (hoặc static), zero-overhead so với mảng C nhưng có `.size()`, iterator, bounds-check qua `.at()`.
- **`std::vector<T>`**: mảng động, phần tử **liên tục trong bộ nhớ**. Có `size` (số phần tử) và `capacity` (chỗ đã cấp phát).
- **`std::deque<T>`**: double-ended queue, các block rời rạc → push/pop hai đầu O(1), nhưng không liên tục bộ nhớ.
- **`std::list<T>`**: doubly-linked list — chèn/xoá O(1) *khi đã có iterator*, nhưng mỗi node một lần cấp phát heap.
- **`std::map` / `std::set`**: cây đỏ-đen (red-black tree), có thứ tự, O(log n).
- **`std::unordered_map` / `unordered_set`**: hash table, trung bình O(1), worst-case O(n), không thứ tự.
- **Iterator invalidation**: thao tác làm iterator/pointer/reference cũ trở thành dangling.

## Giải thích chi tiết

### vector: capacity và reallocation
```
size=3, capacity=4:  [A][B][C][ . ]
push_back(D):        [A][B][C][D]          (không reallocate)
push_back(E):        cấp vùng mới cap=8 → move/copy toàn bộ → giải phóng cũ
                     [A][B][C][D][E][.][.][.]
                     ⚠ MỌI iterator/pointer cũ dangling!
```
Growth factor thường 1.5–2× → `push_back` là **amortized O(1)** nhưng từng lần reallocate là O(n) + cấp phát heap. `reserve(n)` trước loại bỏ hoàn toàn reallocation.

### map vs unordered_map — Big-O
| Thao tác        | vector | list | map (RB-tree) | unordered_map |
|-----------------|--------|------|---------------|----------------|
| Truy cập index  | O(1)   | O(n) | —             | —              |
| Tìm theo key    | O(n)*  | O(n) | O(log n)      | O(1) tb / O(n) xấu |
| Chèn cuối       | O(1)†  | O(1) | —             | —              |
| Chèn giữa/key   | O(n)   | O(1)‡| O(log n)      | O(1) tb        |
| Duyệt có thứ tự | có     | có   | CÓ (sorted)   | KHÔNG          |
| Bộ nhớ/phần tử  | thấp nhất | cao (2 ptr/node) | cao (3 ptr + màu) | trung bình (bucket) |

\* O(log n) nếu đã sort + `lower_bound`. † amortized. ‡ cần iterator sẵn.

Chọn `map` khi cần thứ tự/duyệt theo khoảng; `unordered_map` khi chỉ cần lookup nhanh và key hash tốt.

### Iterator categories
```
Input → Forward → Bidirectional → RandomAccess → Contiguous (C++17)
        (forward_list) (list, map)  (deque)        (vector, array)
```
Category quyết định thuật toán nào dùng được: `std::sort` cần random-access → không sort được `std::list` bằng `std::sort` (list có `.sort()` riêng).

### Cache locality — vì sao vector thắng list
```
vector: [A][B][C][D][E]  → 1 cache line chứa nhiều phần tử, prefetcher đoán được
list:   [A]→ ...heap... →[B]→ ...heap... →[C]   → mỗi node 1 cache miss tiềm năng
```
Trên CPU hiện đại, 1 cache miss ≈ 100+ chu kỳ. Thực nghiệm nổi tiếng (Stroustrup): kể cả bài toán "chèn giữa nhiều" — điểm mạnh lý thuyết của list — vector vẫn thắng tới hàng chục nghìn phần tử, vì chi phí **tìm vị trí chèn** (duyệt tuyến tính, cache miss liên tục) áp đảo chi phí dịch chuyển phần tử liên tục.

## Cách dùng
```cpp
std::array<int, 8> regs{};             // khởi tạo 0, size cố định
std::vector<int> v; v.reserve(100);    // tránh reallocation
std::map<std::string, int> cfg{{"baud", 115200}};
std::unordered_map<int, std::string> id2name;
for (const auto& [key, val] : cfg) { /* structured binding */ }
v.erase(std::remove(v.begin(), v.end(), 0), v.end()); // erase-remove idiom
```

## Tips & Tricks

- `emplace_back(args...)` xây object tại chỗ, tránh 1 lần move/copy so với `push_back(T(args...))`.
- `shrink_to_fit()` chỉ là *gợi ý* giảm capacity — không đảm bảo.
- `map::operator[]` **tạo phần tử mới** (default-construct) nếu key chưa có → dùng `.find()`/`.at()` khi chỉ đọc.
- Xoá trong lúc duyệt map: `it = m.erase(it);` (erase trả iterator kế tiếp).
- `std::vector<bool>` là bit-packed đặc biệt — `operator[]` trả proxy, không phải `bool&`; cần mảng bool thật thì dùng `std::array<bool,N>` hoặc `vector<char>`.

## Lỗi thường gặp / Bẫy

1. Giữ iterator/pointer vào vector rồi `push_back` → reallocation → **dangling** (demo trong main.cpp).
2. `erase(it)` trong vòng lặp mà vẫn `++it` → bỏ sót phần tử hoặc UB.
3. So sánh hiệu năng bằng Big-O lý thuyết mà quên hằng số cache: list "O(1) chèn" thường thua vector thực tế.
4. Dùng `unordered_map` với key tự định nghĩa nhưng hash tệ → thoái hoá O(n).
5. `map::operator[]` trong hàm `const` không biên dịch được — và trong hàm thường thì vô tình chèn key rác.

## Ghi chú Embedded

- **`std::array` thay `vector`**: kích thước biết trước → không heap, không fragmentation, footprint dự đoán được — mặc định đúng cho firmware.
- **ETL (Embedded Template Library)**: `etl::vector<T, MAX>`, `etl::map<K,V,MAX>` — API giống STL nhưng capacity cố định, cấp phát tĩnh, không exception. Lựa chọn chuẩn cho nRF52/STM32.
- Nếu buộc dùng `vector`: **`reserve()` một lần lúc khởi động**, coi reallocation lúc runtime là bug; cân nhắc custom allocator trỏ vào pool tĩnh.
- `unordered_map` trên MCU thường quá đắt (bucket array + node heap); bảng nhỏ thì mảng sorted + `lower_bound` nhanh và gọn hơn.

## Bài tập tự luyện

1. Viết chương trình in `size`/`capacity` của vector sau mỗi `push_back` từ 1→100, suy ra growth factor của g++ (libstdc++).
2. Benchmark: chèn 10.000 số vào giữa `vector` vs `list` (dùng `std::chrono`), giải thích kết quả bằng cache locality.
3. Cài bảng tra `id → tên lỗi` hai cách: `std::map` và mảng sorted `std::array<std::pair<int,const char*>,N>` + `lower_bound`; so sánh footprint và tốc độ.

## Tóm tắt

- Mặc định dùng `vector` (hoặc `array` khi size cố định); chỉ đổi container khi profiler chứng minh cần.
- Hiểu `capacity`/reallocation/iterator invalidation là bắt buộc — nguồn bug memory phổ biến nhất với vector.
- `map` = sorted O(log n), `unordered_map` = hash O(1) trung bình; chọn theo nhu cầu thứ tự.
- Cache locality thắng Big-O trên dữ liệu nhỏ/vừa; embedded: `std::array`, ETL, `reserve()` trước.
