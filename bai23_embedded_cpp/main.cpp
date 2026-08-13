// Bài 23: Embedded C++ — volatile, MMIO (mô phỏng), ISR-safe ring buffer,
//         static init order, template driver vs virtual driver
// Build: make && ./bai23_embedded_cpp.exe   (chạy trên PC, hardware được mô phỏng)
#include <cstdio>
#include <cstdint>
#include <cstddef>
#include <atomic>
#include <thread>

// ---------------------------------------------------------------
// 1) Struct thanh ghi kiểu CMSIS + static_assert layout
//    (trên PC ta mô phỏng bằng một vùng RAM thay vì 0x50000000)
// ---------------------------------------------------------------
struct GpioRegs {                       // theo datasheet nRF52840 GPIO
    volatile uint32_t OUT;              // 0x504 (rút gọn demo: offset từ 0)
    volatile uint32_t OUTSET;           // ghi 1 = set bit, không cần RMW
    volatile uint32_t OUTCLR;           // ghi 1 = clear bit
    volatile uint32_t IN;
};
static_assert(sizeof(GpioRegs) == 16, "layout thanh ghi sai!");
#if defined(__GNUC__) || defined(__clang__)
static_assert(__builtin_offsetof(GpioRegs, OUTCLR) == 8, "offset OUTCLR sai!");
#endif

// Mô phỏng: vùng "thanh ghi" là RAM tĩnh; trên target sẽ là:
//   #define P0 reinterpret_cast<GpioRegs*>(0x50000504)
static GpioRegs sim_gpio{};
static GpioRegs* const P0 = &sim_gpio;

// "Hardware" mô phỏng: OUTSET/OUTCLR có side effect lên OUT
void hw_tick() {                        // giả lập hành vi phần cứng
    if (P0->OUTSET) { P0->OUT |= P0->OUTSET; P0->OUTSET = 0; }
    if (P0->OUTCLR) { P0->OUT &= ~P0->OUTCLR; P0->OUTCLR = 0; }
}

// ---------------------------------------------------------------
// 2) ISR-safe SPSC ring buffer: ISR đẩy vào, main lấy ra
// ---------------------------------------------------------------
template <typename T, uint32_t N>       // N phải là 2^n
class SpscRing {
    static_assert((N & (N - 1)) == 0, "N phai la luy thua cua 2");
    T buf_[N];
    std::atomic<uint32_t> head_{0};     // chỉ ISR (producer) ghi
    std::atomic<uint32_t> tail_{0};     // chỉ main (consumer) ghi
public:
    bool push(const T& v) {             // gọi từ "ISR": không lock, không alloc
        uint32_t h = head_.load(std::memory_order_relaxed);
        uint32_t t = tail_.load(std::memory_order_acquire);
        if (h - t == N) return false;   // đầy — đếm overrun thay vì block!
        buf_[h & (N - 1)] = v;
        head_.store(h + 1, std::memory_order_release);
        return true;
    }
    bool pop(T& out) {                  // gọi từ main loop
        uint32_t t = tail_.load(std::memory_order_relaxed);
        uint32_t h = head_.load(std::memory_order_acquire);
        if (t == h) return false;       // rỗng
        out = buf_[t & (N - 1)];
        tail_.store(t + 1, std::memory_order_release);
        return true;
    }
};

static SpscRing<uint8_t, 64> g_rx_ring;
static std::atomic<bool> g_isr_done{false};

void fake_uart_isr() {                  // thread đóng vai ISR
    for (int i = 0; i < 200; ++i) {
        while (!g_rx_ring.push(static_cast<uint8_t>(i))) {}  // demo: chờ chỗ trống
    }
    g_isr_done.store(true, std::memory_order_release);
}

// ---------------------------------------------------------------
// 3) Static init order fiasco — trị bằng construct-on-first-use
// ---------------------------------------------------------------
struct Logger {
    Logger() { std::printf("  Logger ctor (lan goi dau tien)\n"); }
    void log(const char* m) { std::printf("  [log] %s\n", m); }
};
Logger& logger() {                      // an toàn thay cho global Logger g_logger;
    static Logger l;                    // C++11: thread-safe, ctor đúng 1 lần
    return l;
}

// ---------------------------------------------------------------
// 4) Virtual driver vs Template driver
// ---------------------------------------------------------------
struct ILed {                           // runtime polymorphism: vtable
    virtual void on() = 0;
    virtual ~ILed() = default;
};
struct HwLed : ILed {
    void on() override { P0->OUTSET = (1u << 13); }
};

template <typename Port, uint32_t PIN>  // static polymorphism: 0 chi phí
struct Led {
    Port* port;
    void on() { port->OUTSET = (1u << PIN); }   // inline thẳng thành 1 lệnh store
};

int main() {
    std::printf("== 1) MMIO: OUTSET/OUTCLR khong can read-modify-write ==\n");
    P0->OUTSET = (1u << 13); hw_tick();
    std::printf("  sau OUTSET bit13: OUT=0x%08X\n", P0->OUT);
    P0->OUTCLR = (1u << 13); hw_tick();
    std::printf("  sau OUTCLR bit13: OUT=0x%08X (ISR-safe, khong RMW)\n", P0->OUT);

    std::printf("\n== 2) ISR-safe SPSC ring buffer (ISR = thread mo phong) ==\n");
    std::thread isr(fake_uart_isr);
    uint32_t received = 0, checksum = 0;
    uint8_t byte;
    while (!g_isr_done.load(std::memory_order_acquire) || g_rx_ring.pop(byte)) {
        if (g_rx_ring.pop(byte)) { ++received; checksum += byte; }
    }
    while (g_rx_ring.pop(byte)) { ++received; checksum += byte; }   // vét nốt
    isr.join();
    std::printf("  nhan %u byte, checksum=%u — khong lock, khong malloc\n",
                received, checksum);

    std::printf("\n== 3) Construct-on-first-use ==\n");
    logger().log("uart init ok");       // ctor chạy đúng lúc này, thứ tự xác định
    logger().log("goi lan 2 — khong ctor lai");

    std::printf("\n== 4) Virtual vs template driver ==\n");
    HwLed hw;
    ILed* iled = &hw;
    iled->on(); hw_tick();              // qua vtable: load vptr -> load slot -> call
    Led<GpioRegs, 14> tled{P0};
    tled.on(); hw_tick();               // template: inline 1 lenh store
    std::printf("  OUT=0x%08X (bit13 tu virtual, bit14 tu template)\n", P0->OUT);
    std::printf("  sizeof(HwLed)=%zu (co vptr) vs sizeof(Led)=%zu (chi con tro port)\n",
                sizeof(HwLed), sizeof(tled));
    return 0;
}
