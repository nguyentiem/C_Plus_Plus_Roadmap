# Bài 11: Smart Pointers — Quản lý Ownership hiện đại

## Định nghĩa & Khái niệm

- **Ownership (quyền sở hữu)**: đối tượng/con trỏ nào chịu trách nhiệm **giải phóng** tài nguyên. Trong C thuần, ownership là "quy ước ngầm" → dễ leak/double-free. Smart pointer biến quy ước đó thành **kiểu dữ liệu** mà compiler kiểm tra được.
- **`std::unique_ptr<T>`**: sở hữu **độc quyền**. Không copy được, chỉ move. Zero-overhead so với con trỏ thô (khi dùng default deleter).
- **`std::shared_ptr<T>`**: sở hữu **chia sẻ** qua đếm tham chiếu (reference counting). Object bị hủy khi refcount về 0.
- **`std::weak_ptr<T>`**: tham chiếu **không sở hữu** tới object do shared_ptr quản lý. Dùng để quan sát và **phá vòng tham chiếu (circular reference)**.
- **Dangling pointer**: con trỏ trỏ tới vùng nhớ đã bị giải phóng. Smart pointer đúng cách loại bỏ gần hết lớp bug này.

## Giải thích chi tiết

### unique_ptr — ownership độc quyền
```
  unique_ptr A ── sở hữu ──> [ Object ]
  A ra khỏi scope  →  delete tự động (RAII)
  move(A) → B      →  A = nullptr, B sở hữu
```
Tại sao không copy được? Vì nếu 2 unique_ptr cùng trỏ 1 object → double-free. Compiler chặn ngay từ lúc biên dịch — đây là điểm mạnh: **lỗi runtime biến thành lỗi compile-time**.

### shared_ptr — control block
```
  shared_ptr P1 ─┐
  shared_ptr P2 ─┼──> [ Control Block ] ──> [ Object ]
  weak_ptr   W  ─┘      strong = 2
                        weak   = 1
```
- **Control block** chứa: strong count, weak count, deleter, allocator. Được cấp phát **trên heap, riêng** với object (trừ khi dùng `make_shared` — gộp 1 lần cấp phát).
- **Chi phí atomic**: tăng/giảm refcount là thao tác **atomic** (thread-safe) → mỗi lần copy shared_ptr tốn 1 atomic RMW, đắt hơn nhiều so với copy con trỏ thô, gây contention trên cache line khi nhiều thread copy cùng lúc.
- Object bị hủy khi strong = 0; control block bị hủy khi strong = 0 **và** weak = 0.

### weak_ptr — phá vòng shared_ptr
```
  Node A ──shared──> Node B
    ^                  │
    └────shared────────┘   ← strong count không bao giờ về 0 → LEAK!

  Sửa: chiều "ngược" (child → parent, observer → subject) dùng weak_ptr.
```
`weak_ptr::lock()` trả về `shared_ptr` (rỗng nếu object đã chết) → truy cập an toàn, không dangling.

### Các câu hỏi ownership kiểu senior
Trước khi viết bất kỳ con trỏ nào, tự hỏi:
1. **Ai sở hữu** object này? (một chủ → unique_ptr; nhiều chủ thật sự → shared_ptr; không sở hữu → raw pointer/reference/weak_ptr)
2. **Lifetime** của nó so với người dùng nó? Caller sống lâu hơn callee không?
3. **Ai destroy** và destroy khi nào? Có deterministic không?
4. **nullptr có hợp lệ không?** Nếu không bao giờ null → dùng reference hoặc `T&`.

## Cách dùng
```cpp
auto p = std::make_unique<Motor>(42);          // ưu tiên make_unique
std::unique_ptr<FILE, decltype(&fclose)> f(fopen("a.txt","r"), &fclose); // custom deleter

auto s1 = std::make_shared<Config>();           // 1 lần cấp phát (gộp control block)
std::weak_ptr<Config> w = s1;
if (auto s2 = w.lock()) { /* object còn sống */ }
```
Truyền tham số: hàm chỉ *dùng* object → nhận `T&`/`const T&`/`T*`; hàm *nhận ownership* → nhận `unique_ptr<T>` by value; hàm *chia sẻ ownership* → `shared_ptr<T>` by value.

## Tips & Tricks

- `make_unique`/`make_shared` thay vì `new`: exception-safe, ngắn gọn, `make_shared` tiết kiệm 1 lần cấp phát.
- `unique_ptr` có thể chứa **mảng**: `std::unique_ptr<int[]>` → tự gọi `delete[]`.
- Custom deleter của `unique_ptr` là **một phần của kiểu** → deleter stateless không tốn thêm bộ nhớ (EBO). Deleter của `shared_ptr` được type-erase trong control block.
- Trả về `unique_ptr` từ factory function — chuyển đổi ngầm sang `shared_ptr` được nếu caller cần.
- `enable_shared_from_this` khi object cần tự tạo `shared_ptr` tới chính nó (đừng bao giờ `shared_ptr<T>(this)`).

## Lỗi thường gặp / Bẫy

1. **Hai shared_ptr độc lập từ một raw pointer** → 2 control block → double-free:
   ```cpp
   T* raw = new T; std::shared_ptr<T> a(raw), b(raw); // BUG!
   ```
2. **Vòng shared_ptr** (parent ↔ child) → leak thầm lặng (xem demo trong main.cpp).
3. **`get()` rồi lưu raw pointer lâu dài** → dangling khi smart pointer chết.
4. **Bắt `this` trong lambda async** khi object do shared_ptr quản lý → dùng `weak_from_this()`.
5. Nghĩ `shared_ptr` thread-safe hoàn toàn: chỉ **refcount** là atomic; truy cập object vẫn cần đồng bộ riêng.

## Ghi chú Embedded

- **Firmware thường tránh `shared_ptr`**: (1) control block cấp phát heap động → fragmentation, không deterministic; (2) atomic refcount tốn chu kỳ, trên Cortex-M0 không có LDREX/STREX phải tắt ngắt; (3) lifetime "ai đó sẽ giải phóng lúc nào đó" khó audit trong hệ thống real-time.
- **`unique_ptr` với static allocation** vẫn rất hữu ích: dùng custom deleter trả block về memory pool tĩnh, hoặc `unique_ptr<T, NoopDeleter>` để diễn đạt ownership của object đặt trong buffer tĩnh — được ngữ nghĩa ownership mà không cần heap.
- Zephyr/nRF Connect SDK: heap nhỏ (`CONFIG_HEAP_MEM_POOL_SIZE`), ưu tiên object tĩnh + `unique_ptr` non-owning semantics hoặc ETL pool.

## Bài tập tự luyện

1. Viết `unique_ptr` với custom deleter quản lý "handle" giả lập (hàm `open_handle()/close_handle()` in ra log), chứng minh không leak khi có exception.
2. Tạo cây `Node {parent, children}` bằng shared_ptr thuần → quan sát destructor không chạy; sửa `parent` thành `weak_ptr` → destructor chạy đủ.
3. Viết `PoolAllocator` tĩnh 4 slot + factory trả `unique_ptr<T, PoolDeleter>`; kiểm tra hết slot trả nullptr.

## Tóm tắt

- Smart pointer mã hoá **ownership** vào type system: unique = độc quyền (mặc định nên dùng), shared = đếm tham chiếu (đắt, dùng khi thật sự cần), weak = quan sát/phá vòng.
- `shared_ptr` trả giá bằng control block + atomic refcount; vòng shared_ptr gây leak — phá bằng `weak_ptr`.
- Câu hỏi cốt lõi: *ai sở hữu, lifetime bao lâu, ai destroy, null có hợp lệ?*
- Embedded: tránh shared_ptr, tận dụng unique_ptr + custom deleter với pool/static allocation.
