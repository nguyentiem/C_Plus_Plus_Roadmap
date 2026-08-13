# Bài 04: Stack, Heap, Static storage và Vòng đời đối tượng

## Định nghĩa & Khái niệm

- **Storage duration** (thời gian lưu trữ) — C++ định nghĩa 4 loại:
  - **automatic**: biến cục bộ; sống trong scope, nằm trên **stack**.
  - **dynamic**: cấp phát bằng `new`/`malloc`; sống tới khi `delete`/`free`, nằm trên **heap**.
  - **static**: biến toàn cục, `static` cục bộ; sống suốt chương trình, nằm ở `.data`/`.bss`.
  - **thread**: `thread_local`; sống theo vòng đời thread.
- **Object lifetime** (vòng đời đối tượng): từ khi khởi tạo xong (ctor kết thúc) đến khi hủy (dtor bắt đầu). Dùng đối tượng ngoài vòng đời là **undefined behavior**.
- **Stack**: vùng nhớ LIFO, cấp phát/thu hồi bằng dịch con trỏ stack — cực nhanh, kích thước cố định, tự thu hồi.
- **Heap**: vùng nhớ động do allocator quản lý — linh hoạt kích thước, nhưng chậm hơn, phải tự trả, có thể phân mảnh.

## Giải thích chi tiết

### Bản đồ bộ nhớ tiến trình

```
Địa chỉ cao ┌──────────────────┐
            │      Stack       │ ← biến cục bộ, frame hàm; lớn xuống dưới
            │        ↓         │
            │   (vùng trống)   │
            │        ↑         │
            │       Heap       │ ← new/malloc; lớn lên trên
            ├──────────────────┤
            │       .bss       │ ← biến static/global CHƯA khởi tạo (zero-fill)
            ├──────────────────┤
            │      .data       │ ← biến static/global CÓ giá trị khởi tạo
            ├──────────────────┤
            │  .rodata/.text   │ ← hằng chuỗi + mã lệnh (chỉ đọc)
Địa chỉ thấp└──────────────────┘
```

### Stack — vì sao nhanh?

Mỗi lần gọi hàm, một **stack frame** được cấp bằng đúng **một phép trừ con trỏ stack**; return là một phép cộng. Không có bookkeeping, không phân mảnh, cache locality tốt. Đổi lại: kích thước phải biết lúc biên dịch, và tổng stack có hạn (Windows mặc định ~1MB; MCU thường chỉ vài KB).

### Heap — vì sao cần và vì sao đắt?

Cần khi: kích thước chỉ biết lúc runtime, hoặc đối tượng phải sống lâu hơn scope tạo ra nó. Đắt vì: allocator phải tìm block trống đủ lớn, quản lý metadata, và trả bộ nhớ **thủ công** — quên là leak, trả 2 lần là UB.

### new/delete vs malloc/free

| | `new`/`delete` | `malloc`/`free` |
|---|---|---|
| Gọi ctor/dtor | **Có** | Không (chỉ cấp byte thô) |
| Kiểu trả về | `T*` đúng kiểu | `void*` phải ép kiểu |
| Thất bại | ném `std::bad_alloc` | trả `NULL` |
| Mảng | `new T[n]` / `delete[]` | `malloc(n*sizeof(T))` |

**Không bao giờ trộn**: bộ nhớ từ `new` phải `delete`, từ `malloc` phải `free`. `new[]` phải đi với `delete[]` (không phải `delete`) — sai cặp là UB.

### Các lỗi bộ nhớ kinh điển

- **Memory leak**: cấp phát rồi mất con trỏ mà chưa trả → RAM cạn dần. Trên server: restart cứu được; trên MCU chạy vài tháng: chết chắc.
- **Use-after-free**: dùng con trỏ sau khi `delete` — vùng nhớ có thể đã được cấp cho thứ khác → dữ liệu "ma", crash ngẫu nhiên.
- **Double-free**: `delete` cùng con trỏ 2 lần → phá cấu trúc allocator, UB.
- **Stack overflow**: đệ quy sâu / mảng cục bộ quá lớn → tràn stack; trên MCU thường ghi đè vùng .data/.bss kế bên một cách âm thầm.

### Heap fragmentation (phân mảnh heap)

```
Sau nhiều lần alloc/free kích thước khác nhau:
Heap: [dùng][trống 12B][dùng][trống 8B][dùng][trống 20B]
Xin 32B: tổng trống = 40B nhưng KHÔNG có block liên tiếp 32B -> thất bại!
```

Tổng bộ nhớ trống đủ, nhưng bị "băm nhỏ" thành lỗ không liền kề. Chạy càng lâu càng tệ — lý do chính khiến hệ thống nhúng lâu năm tránh heap động.

### static storage và `static` cục bộ

Biến `static` trong hàm khởi tạo **một lần duy nhất**, lần đầu đi qua khai báo (C++11 đảm bảo thread-safe), sống tới khi chương trình kết thúc — giữ trạng thái giữa các lần gọi mà không cần global.

## Cách dùng

```cpp
void vi_du() {
    int a = 1;                    // automatic: trên stack, tự hủy cuối scope
    static int dem = 0;           // static: khởi tạo 1 lần, sống mãi
    ++dem;

    int* p = new int(42);         // dynamic: trên heap
    delete p;                     // PHẢI tự trả
    p = nullptr;

    auto* arr = new int[100]();   // mảng heap, () = zero-init
    delete[] arr;                 // new[] đi với delete[]
}
```

## Tips & Tricks

- **Ưu tiên stack**: nhanh, tự dọn, không leak. Chỉ ra heap khi buộc phải.
- Cần heap? Dùng **`std::unique_ptr` / `std::vector`** (RAII, bài 03) thay vì `new`/`delete` thô — leak gần như biến mất.
- Gán `nullptr` sau `delete` để double-free thành no-op (`delete nullptr` an toàn) và use-after-free thành crash rõ ràng.
- Mảng cục bộ lớn (> vài KB) → chuyển sang heap hoặc `static`, đừng để trên stack.
- Công cụ: `-fsanitize=address` (ASan) bắt use-after-free/leak lúc chạy; Valgrind trên Linux.

## Lỗi thường gặp / Bẫy

1. **Leak trên đường thoát sớm**: `new` rồi `return`/throw trước khi `delete` → dùng RAII.
2. **`delete` thay vì `delete[]`** cho mảng → UB (thường dtor chỉ chạy cho phần tử đầu).
3. **Trả về con trỏ/tham chiếu tới biến stack** → dangling (bài 02).
4. **Dùng `static` cục bộ trong code đa luồng** như biến "riêng" — nó là biến chia sẻ, cần đồng bộ khi ghi.
5. **Giả định `malloc` zero-init**: không! (`calloc` mới zero). `new int` cũng không zero-init, `new int()` mới zero.
6. **Đệ quy không có điều kiện dừng chặt** → stack overflow, trên MCU biểu hiện là HardFault hoặc dữ liệu hỏng bí ẩn.

## Ghi chú Embedded

- **Vì sao MCU hạn chế heap?** RAM chỉ 64–256KB (nRF52840: 256KB), không có MMU/swap; phân mảnh không có cách cứu; `malloc` thất bại giữa chừng khó xử lý; thời gian alloc không deterministic (tệ cho realtime). Nhiều chuẩn (MISRA C) **cấm** cấp phát động sau khởi động.
- Chiến lược thay thế: cấp phát **tĩnh** toàn bộ lúc compile-time, **memory pool** (block cố định — hết O(1), không phân mảnh), stack allocator. Zephyr có `k_heap`/`k_mem_slab` cho mục đích này.
- Kích thước stack mỗi thread RTOS phải khai báo trước (`CONFIG_MAIN_STACK_SIZE`); tràn stack thread là lỗi kinh điển — Zephyr có `CONFIG_STACK_SENTINEL`/canary để phát hiện.
- `.data` được copy từ flash vào RAM lúc khởi động, `.bss` được zero-fill bởi startup code — biến global khởi tạo khác 0 **tốn cả flash lẫn RAM**.

## Bài tập tự luyện

1. Viết hàm đệ quy in độ sâu hiện tại và địa chỉ một biến cục bộ (`&x`) mỗi lần gọi — quan sát địa chỉ giảm dần (stack lớn xuống). Ước lượng kích thước một frame.
2. Viết `class IntArray` quản lý `new int[n]` theo RAII (ctor cấp, dtor `delete[]`, cấm copy). So sánh với việc dùng `std::vector<int>`.
3. Mô phỏng phân mảnh: dùng mảng `char pool[64]` và tự viết allocator first-fit đơn giản (đánh dấu block dùng/trống); chứng minh trường hợp "tổng trống đủ nhưng xin thất bại".

## Tóm tắt

- 4 loại storage duration: automatic (stack), dynamic (heap), static (.data/.bss), thread.
- Stack: nhanh, tự dọn, kích thước hạn chế. Heap: linh hoạt, chậm, phải tự trả, có thể phân mảnh.
- `new`/`delete` gọi ctor/dtor; `malloc`/`free` chỉ cấp byte thô — không trộn cặp; `new[]` đi với `delete[]`.
- 4 lỗi kinh điển: leak, use-after-free, double-free, stack overflow — phòng bằng RAII + sanitizer.
- Embedded: RAM nhỏ, không MMU, cần deterministic → hạn chế heap, dùng cấp phát tĩnh/memory pool.
