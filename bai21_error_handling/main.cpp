// Bài 21: Error Handling — exceptions, noexcept, error_code, Result/expected
// Build: make && ./bai21_error_handling.exe
#include <cstdio>
#include <new>
#include <stdexcept>
#include <string>
#include <system_error>
#include <utility>
#include <vector>
#include <chrono>

// ---------------------------------------------------------------
// 1) RAII + stack unwinding: không leak dù ném
// ---------------------------------------------------------------
struct Resource {
    const char* name;
    explicit Resource(const char* n) : name(n) { std::printf("  + acquire %s\n", n); }
    ~Resource() { std::printf("  - release %s\n", name); }
};

void may_throw(bool doThrow) {
    Resource r1("buffer");
    Resource r2("mutex");
    if (doThrow) throw std::runtime_error("uart timeout");
    std::printf("  (no throw)\n");
}

// ---------------------------------------------------------------
// 2) Exception safety: copy-and-swap = strong guarantee
// ---------------------------------------------------------------
class Widget {
    std::vector<int> data_;
public:
    explicit Widget(size_t n) : data_(n, 7) {}
    friend void swap(Widget& a, Widget& b) noexcept { a.data_.swap(b.data_); }
    Widget& operator=(const Widget& o) {
        Widget tmp(o);      // có thể ném — nhưng *this chưa bị đụng
        swap(*this, tmp);   // nothrow
        return *this;
    }
    Widget(const Widget&) = default;
    size_t size() const noexcept { return data_.size(); }
};

// ---------------------------------------------------------------
// 3) std::error_code với category riêng (kiểu không-exception)
// ---------------------------------------------------------------
enum class UartError { ok = 0, timeout = 1, framing = 2, overrun = 3 };

class UartCategory : public std::error_category {
public:
    const char* name() const noexcept override { return "uart"; }
    std::string message(int ev) const override {
        switch (static_cast<UartError>(ev)) {
            case UartError::ok:      return "ok";
            case UartError::timeout: return "rx timeout";
            case UartError::framing: return "framing error";
            case UartError::overrun: return "fifo overrun";
        }
        return "unknown";
    }
};
const UartCategory& uart_category() { static UartCategory c; return c; }
std::error_code make_error_code(UartError e) {
    return {static_cast<int>(e), uart_category()};
}
namespace std { template <> struct is_error_code_enum<UartError> : true_type {}; }

size_t uart_read_ec(int simulate, std::error_code& ec) {
    if (simulate == 0) { ec.clear(); return 16; }
    ec = static_cast<UartError>(simulate);
    return 0;
}

// ---------------------------------------------------------------
// 4) Result<T,E> — expected tự chế cho C++17 (firmware-friendly)
// ---------------------------------------------------------------
template <typename T, typename E>
class [[nodiscard]] Result {
    union { T val_; E err_; };
    bool ok_;
public:
    Result(T v) : val_(std::move(v)), ok_(true) {}
    struct Err { E e; };                       // tag để phân biệt ctor
    Result(Err e) : err_(std::move(e.e)), ok_(false) {}
    ~Result() { if (ok_) val_.~T(); else err_.~E(); }
    Result(Result&& o) noexcept : ok_(o.ok_) {  // move: nothrow!
        if (ok_) new (&val_) T(std::move(o.val_));
        else     new (&err_) E(std::move(o.err_));
    }
    Result(const Result&) = delete;             // demo tối giản: cấm copy
    Result& operator=(const Result&) = delete;
    Result& operator=(Result&&) = delete;

    explicit operator bool() const noexcept { return ok_; }
    T& operator*() { return val_; }
    const E& error() const { return err_; }
};

struct Frame { int id; int len; };

Result<Frame, UartError> read_frame(int simulate) {
    if (simulate != 0)
        return Result<Frame, UartError>::Err{static_cast<UartError>(simulate)};
    return Frame{0x42, 8};
}

#if defined(__cpp_lib_expected)                 // C++23, GCC 12+
#include <expected>
std::expected<Frame, UartError> read_frame23(int simulate) {
    if (simulate != 0) return std::unexpected(static_cast<UartError>(simulate));
    return Frame{0x99, 4};
}
#endif

// ---------------------------------------------------------------
// 5) Chi phí: return code vs throw
// ---------------------------------------------------------------
int by_return(int x) noexcept { return x < 0 ? -1 : x; }
int by_throw(int x) { if (x < 0) throw std::invalid_argument("neg"); return x; }

int main() {
    std::printf("== 1) RAII + stack unwinding ==\n");
    try {
        may_throw(true);
    } catch (const std::exception& e) {          // bắt by const ref!
        std::printf("  caught: %s\n", e.what());
    }

    std::printf("\n== 2) Strong guarantee (copy-and-swap) ==\n");
    Widget a(4), b(9);
    a = b;
    std::printf("  a.size()=%zu (rollback-safe assignment)\n", a.size());

    std::printf("\n== 3) std::error_code ==\n");
    std::error_code ec;
    uart_read_ec(1, ec);
    if (ec) std::printf("  read failed: [%s] %s\n", ec.category().name(),
                        ec.message().c_str());
    std::printf("  ec == UartError::timeout? %s\n",
                (ec == UartError::timeout) ? "yes" : "no");

    std::printf("\n== 4) Result<T,E> (expected tu che) ==\n");
    auto r = read_frame(0);
    if (r) std::printf("  frame id=0x%X len=%d\n", (*r).id, (*r).len);
    auto r2 = read_frame(2);
    if (!r2) std::printf("  error: %s\n",
                         uart_category().message(static_cast<int>(r2.error())).c_str());
#if defined(__cpp_lib_expected)
    auto r3 = read_frame23(0);
    if (r3) std::printf("  std::expected: frame id=0x%X\n", r3->id);
#else
    std::printf("  (std::expected can C++23/GCC12+ — dung Result tu che)\n");
#endif

    std::printf("\n== 5) Chi phi: return code vs throw (1M lan loi) ==\n");
    using clk = std::chrono::steady_clock;
    volatile int sink = 0;
    auto t0 = clk::now();
    for (int i = 0; i < 1'000'000; ++i) sink += by_return(-1);
    auto t1 = clk::now();
    for (int i = 0; i < 1'000'000; ++i) {
        try { sink += by_throw(-1); } catch (const std::exception&) { sink -= 1; }
    }
    auto t2 = clk::now();
    auto us = [](auto d) {
        return std::chrono::duration_cast<std::chrono::microseconds>(d).count();
    };
    std::printf("  return code: %lld us | throw/catch: %lld us\n",
                (long long)us(t1 - t0), (long long)us(t2 - t1));
    std::printf("  => exception cho loi HIEM, khong phai control flow\n");
    return 0;
}
