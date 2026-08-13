# Bài 22: Custom Memory Management — placement new, pool, arena, allocator, PMR

## Định nghĩa & Khái niệm

- **Placement `new`**: xây object *tại địa chỉ có sẵn* thay vì cấp phát mới: `new (ptr) T(args)`. Không cấp phát — chỉ gọi constructor. Hủy bằng cách gọi destructor thủ công `p->~T()`.
- **Memory pool (fixed-size pool)**: mảng block cùng kích thước cấp phát trước; alloc/free = tháo/gắn free-list → **O(1), không fragmentation, deterministic** — chuẩn firmware.
- **Arena (bump/linear allocator)**: một buffer lớn, alloc = đẩy con trỏ lên (bump); không free từng cái, chỉ **reset cả arena một lần**. Nhanh nhất có thể, hợp dữ liệu theo "phiên" (frame render, packet parse).
- **Custom STL allocator**: struct có `allocate/deallocate` + `value_type` → cắm vào `std::vector<T, MyAlloc<T>>` để container lấy memory từ pool/arena của bạn.
- **PMR (`std::pmr`, C++17)**: allocator đa hình — container giữ con trỏ `memory_resource*`, đổi nguồn memory **runtime** mà không đổi *kiểu* container. `monotonic_buffer_resource` = arena có sẵn trong chuẩn.
- **Overhead của heap tổng quát**: mỗi `malloc` mang header 8-16 byte, tìm kiếm free block (không O(1)), lock giữa các thread, và **fragmentation** — lý do firmware cấm/hạn chế heap sau khởi động.

## Giải thích chi tiết

### Placement new — nền móng của mọi allocator
```cpp
alignas(Widget) unsigned char storage[sizeof(Widget)];  // memory thô, ĐÚNG alignment
Widget* w = new (storage) Widget(42);   // chỉ chạy constructor
w->~Widget();                            // chỉ chạy destructor — KHÔNG delete!
```
Đây chính là cách `std::vector` tách "cấp phát capacity" khỏi "xây object" (size), và cách `optional`/`variant` hoạt động bên trong.

### Fixed-size pool — free list trong chính block
```text
Pool ban đầu:  [blk0]->[blk1]->[blk2]->[blk3]-> null     (mỗi block chứa con trỏ next)
alloc()     :  trả blk0, head = blk1                      O(1)
free(blk0)  :  blk0->next = head; head = blk0             O(1)
```
Không cần metadata riêng: khi block rảnh, chính vùng nhớ đó chứa con trỏ `next`.

### Arena — bump pointer
```text
[////////used////////|__________free___________]
                     ^offset — alloc(n): offset += align_up(n)
reset(): offset = 0  (mọi object "biến mất" cùng lúc — chỉ dùng cho POD/trivially destructible
                      hoặc phải tự gọi destructor trước)
```

### Custom allocator cho STL
```cpp
template <typename T> struct PoolAlloc {
    using value_type = T;
    T* allocate(size_t n);            // lấy từ pool của bạn
    void deallocate(T*, size_t);
    // rebind tự động qua value_type (C++11 allocator_traits)
};
std::vector<int, PoolAlloc<int>> v;   // kiểu KHÁC std::vector<int> — lây lan qua API!
```
Nhược điểm "lây lan kiểu" là lý do PMR ra đời.

### PMR — đổi nguồn memory không đổi kiểu
```cpp
std::byte buf[4096];
std::pmr::monotonic_buffer_resource arena(buf, sizeof(buf));  // arena trên STACK
std::pmr::vector<int> v(&arena);      // vẫn là "pmr::vector<int>" dù nguồn nào
```
`unsynchronized_pool_resource` = pool nhiều size class; `null_memory_resource()` = ném khi alloc (dùng để *chứng minh* code không đụng heap).

## Cách dùng — chọn allocator nào?

| Nhu cầu | Giải pháp |
|---|---|
| Object cùng size, alloc/free lắt nhắt (message, node) | Fixed pool |
| Dữ liệu sống theo "phiên" rồi bỏ cả cụm | Arena / `monotonic_buffer_resource` |
| Container STL nhưng cấm heap | PMR + buffer tĩnh, hoặc `std::array` + size |
| Firmware không heap tuyệt đối | Static allocation + placement new + pool |
| Cần chứng minh không alloc runtime | `null_memory_resource` làm upstream |

## Tips & Tricks

- **Alignment là bắt buộc**: buffer thô phải `alignas(std::max_align_t)` (hoặc alignment của T). Sai alignment = UB, trên Cortex-M là HardFault.
- Arena: `align_up(offset, alignof(T))` trước mỗi lần bump.
- Pool block size phải `>= sizeof(void*)` để chứa được con trỏ free-list.
- Đo fragmentation heap trên firmware: alloc/free ngẫu nhiên vài nghìn lần rồi thử alloc block lớn — sẽ fail dù tổng free đủ.
- `std::pmr::polymorphic_allocator` truyền xuống container con (scoped allocator) — map của vector cùng dùng 1 arena.
- Override `operator new/delete` toàn cục để *đếm* alloc — cách rẻ nhất phát hiện heap lén lút trong thư viện (xem main.cpp).

## Lỗi thường gặp / Bẫy

1. Placement new xong gọi `delete` → UB (delete sẽ free vùng nhớ không phải của heap).
2. Quên gọi destructor thủ công cho object non-trivial trong arena → leak resource (không leak memory nhưng leak file handle, mutex...).
3. Buffer `char[]` không `alignas` → misaligned access.
4. Trả block về **sai pool** (pool A free vào pool B) → hỏng free-list.
5. Dùng `memcpy` object non-trivially-copyable vào pool → UB (bài 8).
6. PMR: container sống lâu hơn `memory_resource` → dangling (resource trên stack, vector trả về ngoài hàm).
7. Custom allocator thiếu `operator==` đúng nghĩa → swap/move giữa 2 container hành xử sai.

## Ghi chú Embedded

- Quy tắc vàng firmware: **cấp phát hết lúc init, sau đó không malloc** — mọi đường alloc runtime là rủi ro fragmentation + non-deterministic.
- FreeRTOS heap_1..heap_5 chính là các chiến lược trên: heap_1 = arena không free, heap_4 = coalescing heap, heap_5 = nhiều region.
- Zephyr: `k_mem_slab` = fixed pool, `k_heap` = heap có giới hạn; `CONFIG_HEAP_MEM_POOL_SIZE=0` để cấm `k_malloc`.
- nRF52840 có 256KB RAM — budget từng vùng (stack mỗi task, pool size) phải nằm trong linker map, kiểm bằng `arm-none-eabi-size`.

## Bài tập

1. Viết `ObjectPool<T, N>` template: `create(args...)` (placement new) + `destroy(p)` (dtor + trả free-list), `static_assert` block đủ lớn.
2. Thêm "high-water mark" vào arena để đo lượng dùng đỉnh — kỹ thuật sizing buffer thực tế.
3. Dùng `std::pmr::null_memory_resource()` làm upstream và chứng minh code của bạn không rơi xuống heap.
4. Benchmark: 100k alloc/free ngẫu nhiên bằng `new/delete` vs pool — so sánh thời gian.
