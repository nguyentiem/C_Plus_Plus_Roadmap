# Bài 16: Concurrency trong C++ (std::thread, mutex, atomic, memory model)

## Định nghĩa & Khái niệm

- **Thread (luồng)**: đơn vị thực thi độc lập trong một process, chia sẻ chung không gian bộ nhớ. C++11 chuẩn hoá `std::thread` — trước đó phải dùng pthread/WinAPI.
- **Race condition (đua dữ liệu)**: hai thread cùng truy cập một vùng nhớ, ít nhất một bên ghi, không có đồng bộ hoá → **undefined behavior** theo chuẩn C++.
- **Mutex (mutual exclusion)**: khoá loại trừ lẫn nhau, chỉ một thread giữ được tại một thời điểm.
- **Condition variable**: cơ chế "ngủ chờ điều kiện" — thread chờ không tốn CPU cho tới khi được đánh thức.
- **std::atomic**: thao tác nguyên tử trên biến đơn, không cần mutex, kèm ngữ nghĩa memory ordering.
- **Memory model**: quy tắc chuẩn C++ định nghĩa khi nào thao tác của thread A "được nhìn thấy" bởi thread B (quan hệ *happens-before*).

## Giải thích chi tiết

### Tại sao cần đồng bộ hoá?

CPU và compiler được phép **sắp xếp lại lệnh** (reorder) và **cache giá trị trong thanh ghi** miễn là kết quả đơn luồng không đổi. Với đa luồng, giả định đó sụp đổ:

```
Thread A:  data = 42;        Thread B:  if (ready)
           ready = true;                    use(data);   // có thể thấy ready=true nhưng data cũ!
```

Không có đồng bộ, B có thể thấy `ready == true` nhưng `data` chưa cập nhật (do reorder hoặc cache line chưa lan truyền). Đây là lý do tồn tại memory model.

### Happens-before và memory_order

Quan hệ **happens-before**: nếu A happens-before B thì mọi ghi của A được B nhìn thấy. Được thiết lập bởi:
1. Thứ tự tuần tự trong cùng thread (sequenced-before).
2. Cặp **release-acquire**: ghi `store(release)` đồng bộ với đọc `load(acquire)` cùng biến atomic.
3. `join()`, tạo thread, mutex lock/unlock...

Các mức `std::memory_order`:

| Mức | Ý nghĩa | Chi phí |
|---|---|---|
| `relaxed` | Chỉ nguyên tử, KHÔNG có thứ tự với biến khác. Dùng cho counter thống kê. | Rẻ nhất |
| `acquire` (load) | Mọi đọc/ghi SAU load không được kéo lên trước. "Mở cổng vào". | Trung bình |
| `release` (store) | Mọi đọc/ghi TRƯỚC store không được đẩy xuống sau. "Đóng gói rồi công bố". | Trung bình |
| `seq_cst` | Mặc định. Tất cả thao tác seq_cst có MỘT thứ tự toàn cục duy nhất mọi thread đồng ý. | Đắt nhất (fence đầy đủ) |

```
   Thread ghi (producer)                Thread đọc (consumer)
   ─────────────────────                ─────────────────────
   data = 42;              ─┐
   flag.store(true,         │ release: mọi ghi trước
              release);  ───┼──── synchronizes-with ────┐
                            │                            ▼
                            │           while(!flag.load(acquire));
                            └──────►    use(data);  // CHẮC CHẮN thấy 42
```

Quy tắc thực dụng mức senior: **dùng seq_cst mặc định**, chỉ hạ xuống acquire/release khi profiling chứng minh cần, và `relaxed` chỉ cho counter độc lập.

### Deadlock / Livelock / Priority inversion

- **Deadlock**: A giữ khoá 1 chờ khoá 2, B giữ khoá 2 chờ khoá 1 → kẹt vĩnh viễn. Phòng: khoá theo THỨ TỰ cố định, hoặc `std::scoped_lock(m1, m2)` (khoá nguyên tử nhiều mutex).
- **Livelock**: các thread liên tục nhường nhau, không ai tiến triển (như hai người né nhau trong hành lang mãi).
- **Priority inversion**: thread ưu tiên thấp giữ khoá mà thread ưu tiên cao cần; thread ưu tiên trung bình chiếm CPU → thread cao bị chặn gián tiếp. Sự cố Mars Pathfinder 1997 là ví dụ kinh điển. Giải pháp RTOS: priority inheritance (mutex FreeRTOS có sẵn).

### False sharing

Hai biến độc lập nằm cùng **cache line** (thường 64 byte): hai core ghi hai biến khác nhau vẫn phải giành nhau cache line → hiệu năng sụp đổ. Sửa: `alignas(64)` tách biến ra hai cache line (xem demo).

## Cách dùng

```cpp
// lock_guard: RAII đơn giản, khoá suốt scope
{ std::lock_guard<std::mutex> lk(m); shared++; }

// unique_lock: linh hoạt hơn — cần cho condition_variable, có thể unlock sớm
std::unique_lock<std::mutex> lk(m);
cv.wait(lk, []{ return !q.empty(); });  // LUÔN dùng predicate (chống spurious wakeup)

// atomic
std::atomic<int> counter{0};
counter.fetch_add(1, std::memory_order_relaxed);
```

## Tips & Tricks

- Luôn `join()` hoặc `detach()` trước khi `std::thread` bị huỷ — nếu không, `std::terminate()`. C++20 có `std::jthread` tự join.
- `cv.wait(lk, pred)` thay vì `wait(lk)` trần — xử lý spurious wakeup và lost wakeup.
- Notify SAU khi đã cập nhật trạng thái; có thể notify ngoài khoá để giảm contention.
- Thu hẹp vùng khoá (critical section) tối đa; không gọi I/O hoặc callback lạ khi đang giữ khoá.
- `std::scoped_lock` (C++17) cho nhiều mutex — tránh deadlock tự động.
- Kiểm tra `atomic<T>::is_lock_free()` — atomic 64-bit trên MCU 32-bit có thể dùng khoá ngầm!

## Lỗi thường gặp / Bẫy

1. **Quên join** → terminate khi thread destructor chạy.
2. **Dùng `wait()` không predicate** → treo vì lost wakeup hoặc chạy nhầm vì spurious wakeup.
3. **Khoá hai mutex khác thứ tự ở hai nơi** → deadlock ngẫu nhiên, khó tái hiện.
4. **Tưởng `volatile` là đồng bộ** — sai hoàn toàn (xem phần Embedded).
5. **Truy cập biến chung "chỉ đọc một chút" không khoá** — vẫn là data race, vẫn UB dù "chạy có vẻ đúng".
6. **Giữ reference/pointer vào biến cục bộ rồi detach thread** → dangling.

## Ghi chú Embedded

- **So với FreeRTOS**: `std::mutex` ≈ `xSemaphoreCreateMutex` (mutex FreeRTOS có priority inheritance — std::mutex thì KHÔNG đảm bảo). Producer/consumer bằng `condition_variable + std::queue` ≈ `xQueueSend/xQueueReceive` — queue FreeRTOS đã gộp sẵn khoá + chờ + copy dữ liệu, và có bản `...FromISR`.
- **ISR-safety**: KHÔNG BAO GIỜ khoá mutex trong ISR (có thể block → chết hệ thống). Trong ISR chỉ dùng primitive dạng `FromISR` (FreeRTOS) hoặc atomic/lock-free (SPSC ring buffer là mẫu kinh điển ISR→task).
- **`volatile` KHÔNG phải atomic**: `volatile` chỉ cấm compiler cache/loại bỏ truy cập bộ nhớ (đúng cho thanh ghi ngoại vi memory-mapped), KHÔNG tạo nguyên tử, KHÔNG tạo happens-before, KHÔNG chặn CPU reorder. Chia sẻ dữ liệu ISR↔main trên Cortex-M: dùng `std::atomic` (hoặc tắt ngắt trong đoạn găng). `volatile` dành cho register ngoại vi; `atomic` dành cho dữ liệu chia sẻ.
- Trên nRF52840 (Cortex-M4, đơn nhân): reorder giữa "core" ít lộ hơn, nhưng compiler reorder vẫn tồn tại, và ISR chen ngang bất kỳ lúc nào — kỷ luật atomic vẫn bắt buộc.
- Zephyr có `k_mutex`, `k_msgq`, `k_condvar` — ánh xạ khái niệm 1-1 với bài này.

## Bài tập tự luyện

1. Viết SPSC (single-producer single-consumer) ring buffer lock-free chỉ dùng hai `std::atomic<size_t>` head/tail với acquire/release. Giải thích tại sao SPSC không cần CAS.
2. Tạo cố ý một deadlock với 2 mutex + 2 thread, sau đó sửa bằng `std::scoped_lock`. Chạy với ThreadSanitizer (`-fsanitize=thread`, trên Linux/WSL) để xác nhận.
3. Benchmark: hai thread tăng hai counter riêng — một phiên bản hai counter kề nhau trong struct, một phiên bản `alignas(64)`. Đo chênh lệch thời gian và giải thích bằng false sharing.

## Tóm tắt

- Data race = UB. Mọi dữ liệu chia sẻ phải qua mutex hoặc atomic.
- `lock_guard` cho khoá đơn giản, `unique_lock` + `condition_variable` (LUÔN có predicate) cho chờ sự kiện.
- Memory model: release-acquire tạo happens-before; seq_cst là mặc định an toàn; relaxed chỉ cho counter.
- Deadlock phòng bằng thứ tự khoá / `scoped_lock`; priority inversion cần priority inheritance (RTOS).
- Embedded: `volatile` ≠ atomic; ISR không được khoá mutex; FreeRTOS queue là bản "đóng gói" của producer/consumer.
