# Bài 23: Embedded C++ chuyên sâu — volatile, MMIO, ISR safety, template driver

## Định nghĩa & Khái niệm

- **Memory-mapped I/O (MMIO)**: thanh ghi peripheral xuất hiện như địa chỉ bộ nhớ. Trên nRF52840: `GPIO P0` bắt đầu tại `0x50000000`, `UARTE0` tại `0x40002000`.
- **`volatile`**: bảo compiler "mỗi lần đọc/ghi biến này là một side effect quan sát được — không được cache vào thanh ghi, không được xóa, không được gộp, không đổi thứ tự *với volatile khác*". Bắt buộc cho: thanh ghi hardware, biến chia sẻ với ISR (kèm atomic), biến sửa bởi debugger.
- **`volatile` KHÔNG phải là**: atomic (không đảm bảo nguyên tử), memory barrier với non-volatile, công cụ đồng bộ giữa 2 thread (dùng `std::atomic`).
- **ISR (Interrupt Service Routine)**: hàm chạy ngắt — chen ngang bất kỳ lúc nào. **ISR-safe** nghĩa là: không malloc, không lock mutex (dùng primitives `FromISR`/atomic), không block, ngắn nhất có thể.
- **Static initialization order fiasco**: thứ tự khởi tạo global object *giữa các translation unit* không xác định — global A dùng global B lúc B chưa chạy constructor.
- **`-fno-rtti`**: tắt RTTI (`dynamic_cast`, `typeid`) — tiết kiệm flash, chuẩn firmware; virtual function vẫn dùng bình thường.
- **Template-based driver / static polymorphism**: chọn hardware ở compile-time (template param / CRTP — bài 18) thay vì vtable → inline được, 0 chi phí runtime.

## Giải thích chi tiết

### Vì sao thiếu volatile là chết
```cpp
uint32_t* status = (uint32_t*)0x40002104;   // thanh ghi EVENTS_RXDRDY
while (*status == 0) {}    // -O2: compiler đọc 1 lần rồi loop vô hạn!
                            // (nó không biết hardware thay đổi giá trị)
volatile uint32_t* status = (volatile uint32_t*)0x40002104;
while (*status == 0) {}    // OK: mỗi vòng là một lần load thật
```

### Mô hình thanh ghi bằng struct (chuẩn CMSIS)
```cpp
struct UarteRegs {                       // offset phải khớp datasheet!
    volatile uint32_t TASKS_STARTRX;     // 0x000
    volatile uint32_t TASKS_STOPRX;      // 0x004
    // ...
};
static_assert(offsetof(UarteRegs, TASKS_STOPRX) == 0x004, "sai layout");
#define UARTE0 (reinterpret_cast<UarteRegs*>(0x40002000))
UARTE0->TASKS_STARTRX = 1;
```
`static_assert` + `offsetof` là dây an toàn bắt buộc (kết hợp kiến thức bài 8 về POD).

### ISR ↔ main: single-producer/single-consumer ring buffer
```text
ISR (producer)  ──push──> [ring buffer] ──pop──> main loop (consumer)
   chỉ ghi head                             chỉ ghi tail
   head/tail là std::atomic (hoặc volatile + kích thước word trên Cortex-M)
```
- Chỉ số dùng `memory_order_acquire/release` (bài 16) — trên MCU 1 core vẫn cần để chặn compiler reorder.
- Không bao giờ gọi `printf`/`malloc`/mutex trong ISR.

### Static init order fiasco + cách trị
```cpp
// log.cpp:    Logger g_logger;
// driver.cpp: Uart g_uart{g_logger};   // g_logger đã ctor chưa?? KHÔNG BIẾT!
// Trị: Construct-On-First-Use
Logger& logger() { static Logger l; return l; }   // ctor lần gọi đầu (C++11 thread-safe)
```
Firmware còn khuyến nghị mạnh hơn: **tránh global có constructor phức tạp** — init tường minh trong `main()` theo thứ tự bạn kiểm soát.

### Driver theo tầng, mock được
```text
Application ──> Service ──> IDriver (interface HOẶC template param)
                                 ├── UartDriver  (target)
                                 └── MockDriver  (unit test trên PC)
```
- Runtime polymorphism (virtual): linh hoạt, chọn lúc chạy, chi phí vtable.
- Static polymorphism (template/CRTP): 0 chi phí, chọn lúc build — hợp driver cố định theo board.

## Cách dùng

```cpp
// RAII cho peripheral: bật clock/cấu hình trong ctor, tắt trong dtor
class SpiGuard {
    SpiRegs* spi_;
public:
    explicit SpiGuard(SpiRegs* s) : spi_(s) { spi_->ENABLE = 7; }
    ~SpiGuard() { spi_->ENABLE = 0; }
    SpiGuard(const SpiGuard&) = delete;          // peripheral không copy được!
    SpiGuard& operator=(const SpiGuard&) = delete;
};

// Critical section RAII (tắt ngắt ngắn nhất có thể)
class IrqLock {
public:
    IrqLock()  { /* __disable_irq(); primask = ... */ }
    ~IrqLock() { /* __enable_irq() / khôi phục primask */ }
};
```

## Tips & Tricks

- Đọc-sửa-ghi thanh ghi: `reg = (reg & ~MASK) | (value << POS);` — cẩn thận thanh ghi "write-1-to-clear" (ghi lại nguyên giá trị đọc được sẽ xóa nhầm flag!).
- Biến chia sẻ ISR↔main mà chỉ là cờ: `std::atomic<bool>` với `relaxed` là đủ và rẻ.
- `constexpr` bảng cấu hình pin/clock → nằm ở flash (`.rodata`), không tốn RAM.
- Kiểm size từng phần: `arm-none-eabi-nm --size-sort -C app.elf | tail -20`.
- Debug thanh ghi bằng debugger phải cẩn thận: *đọc* một số thanh ghi (như UART RXD) cũng làm thay đổi trạng thái FIFO.
- Zephyr/nRF Connect SDK: không truy cập thanh ghi thô — dùng devicetree + driver API (`gpio_pin_set_dt`), nhưng vẫn phải hiểu tầng dưới để debug.

## Lỗi thường gặp / Bẫy

1. Thiếu `volatile` trên thanh ghi/flag ISR → chạy đúng ở `-O0`, treo ở `-O2` ("release-only bug" kinh điển).
2. Dùng `volatile` thay cho `std::atomic` giữa 2 thread → race condition thật trên chip đa nhân.
3. `printf`/`malloc`/mutex trong ISR → deadlock hoặc corruption ngẫu nhiên.
4. Struct thanh ghi sai offset (quên padding/reserved) → ghi nhầm thanh ghi bên cạnh.
5. Global object gọi HAL trong constructor — chạy TRƯỚC `main()` khi clock/peripheral chưa init.
6. Ring buffer dùng `%` với size không phải 2^n → phép chia trên MCU không FPU/divider chậm; dùng `& (N-1)`.
7. Copy class quản lý peripheral → 2 object cùng tắt/bật 1 phần cứng (phải `= delete` copy).

## Ghi chú Embedded (nRF52840)

- GPIO P0: `0x50000000`, `OUTSET/OUTCLR` tách riêng — ghi 1 bit không cần read-modify-write (an toàn ISR).
- Chuỗi EasyDMA (UARTE/SPIM/TWIM): buffer phải nằm ở **RAM**, không được ở flash — lỗi "DMA đọc const array" rất phổ biến.
- Errata nRF52840 (doc có sẵn trong project) là ví dụ thật: hardware cũng có bug, driver phải workaround.
- `nrfx`/Zephyr driver chính là các pattern trong bài này được sản phẩm hóa — đọc source của chúng là cách học nhanh nhất.

## Bài tập

1. Viết `template <uintptr_t BASE> class Gpio` với `set(pin)/clear(pin)` ghi vào `OUTSET/OUTCLR` (mô phỏng bằng RAM buffer trên PC) — chứng minh bằng `nm` rằng không có vtable.
2. Viết ring buffer SPSC 64 phần tử dùng `std::atomic<uint32_t>` head/tail, `& 63` thay `%`, test bằng 2 thread.
3. Tạo 2 file .cpp có global phụ thuộc nhau, quan sát static init order fiasco, sửa bằng construct-on-first-use.
4. So sánh size/asm giữa driver virtual và driver template (`g++ -S -Os`).
