// Bài 12: Move Semantics — Buffer tự quản lý bộ nhớ, Rule of 5, đếm copy/move
// Biên dịch: g++ -std=c++17 -Wall -Wextra -O2 main.cpp -o bai12_move_semantics.exe
#include <cstdio>
#include <cstring>
#include <string>
#include <utility>
#include <vector>

// ---------- Class Buffer: Rule of 5 + bộ đếm copy/move ----------
class Buffer {
public:
    static int copies; // đếm số lần copy
    static int moves;  // đếm số lần move

    explicit Buffer(std::size_t n) : n_(n), d_(new char[n]) {
        std::memset(d_, 0, n_);
    }

    // 1. Destructor
    ~Buffer() { delete[] d_; }

    // 2. Copy constructor: cấp phát mới + copy dữ liệu (O(n))
    Buffer(const Buffer& o) : n_(o.n_), d_(new char[o.n_]) {
        std::memcpy(d_, o.d_, n_);
        ++copies;
    }

    // 3. Copy assignment (copy-and-swap đơn giản hoá)
    Buffer& operator=(const Buffer& o) {
        if (this != &o) {
            char* nd = new char[o.n_];      // cấp phát trước (exception-safe)
            std::memcpy(nd, o.d_, o.n_);
            delete[] d_;
            d_ = nd;
            n_ = o.n_;
            ++copies;
        }
        return *this;
    }

    // 4. Move constructor: chỉ trộm con trỏ (O(1)), noexcept BẮT BUỘC cho vector
    Buffer(Buffer&& o) noexcept : n_(o.n_), d_(o.d_) {
        o.d_ = nullptr; // reset nguồn -> tránh double-free
        o.n_ = 0;
        ++moves;
    }

    // 5. Move assignment
    Buffer& operator=(Buffer&& o) noexcept {
        if (this != &o) {
            delete[] d_;    // giải phóng tài nguyên cũ -> tránh leak
            d_ = o.d_;
            n_ = o.n_;
            o.d_ = nullptr;
            o.n_ = 0;
            ++moves;
        }
        return *this;
    }

    std::size_t size() const { return n_; }
    static void reset_counters() { copies = 0; moves = 0; }
    static void report(const char* label) {
        std::printf("  [%s] copies = %d, moves = %d\n", label, copies, moves);
    }

private:
    std::size_t n_;
    char* d_;
};

int Buffer::copies = 0;
int Buffer::moves = 0;

// ---------- 1. Copy vs Move cơ bản ----------
static void demo_copy_vs_move() {
    std::printf("--- demo_copy_vs_move ---\n");
    Buffer::reset_counters();
    Buffer a(1024);
    Buffer b = a;             // lvalue -> COPY (O(n))
    Buffer c = std::move(a);  // std::move -> MOVE (O(1)); a giờ rỗng
    std::printf("  sau move: a.size() = %zu (rong), c.size() = %zu\n",
                a.size(), c.size());
    std::printf("  b.size() = %zu (ban sao doc lap)\n", b.size());
    Buffer::report("copy_vs_move");
}

// ---------- 2. RVO/NRVO: trả về by value KHÔNG tốn copy/move ----------
static Buffer make_buffer(std::size_t n) {
    Buffer local(n);
    return local; // NRVO: xây thẳng tại chỗ caller (đừng viết std::move(local)!)
}

static void demo_rvo() {
    std::printf("--- demo_rvo ---\n");
    Buffer::reset_counters();
    Buffer x = make_buffer(2048); // C++17: không copy, thường cũng không move
    std::printf("  x.size() = %zu\n", x.size());
    Buffer::report("rvo (0/0 = elision hoat dong)");
}

// ---------- 3. vector reallocation: noexcept move được dùng ----------
static void demo_vector_move() {
    std::printf("--- demo_vector_move ---\n");
    Buffer::reset_counters();
    std::vector<Buffer> v;
    for (int i = 0; i < 4; ++i) {
        v.push_back(Buffer(64)); // rvalue -> move vào vector; reallocate cũng move
    }
    Buffer::report("push_back x4, khong reserve");

    Buffer::reset_counters();
    std::vector<Buffer> w;
    w.reserve(4);                // đặt trước capacity -> không reallocation
    for (int i = 0; i < 4; ++i) {
        w.push_back(Buffer(64));
    }
    Buffer::report("push_back x4, co reserve (it move hon)");
}

// ---------- 4. Perfect forwarding với std::forward ----------
static void take(const std::string& s) { std::printf("  take(lvalue): copy \"%s\"\n", s.c_str()); }
static void take(std::string&& s) { std::printf("  take(rvalue): move \"%s\"\n", s.c_str()); }

template <typename T>
static void relay(T&& v) {           // forwarding reference
    take(std::forward<T>(v));        // giữ nguyên value category
}

static void demo_forwarding() {
    std::printf("--- demo_forwarding ---\n");
    std::string name = "sensor_log";
    relay(name);                     // lvalue -> take(const string&)
    relay(std::string("tmp_data"));  // rvalue -> take(string&&)
    relay(std::move(name));          // xvalue -> take(string&&)
}

int main() {
    std::printf("=== Bai 12: Move Semantics ===\n");
    demo_copy_vs_move();
    demo_rvo();
    demo_vector_move();
    demo_forwarding();
    std::printf("=== Ket thuc ===\n");
    return 0;
}
