// Bài 22: Custom Memory — placement new, fixed pool, arena, STL allocator, PMR
// Build: make && ./bai22_custom_memory.exe
#include <cstdio>
#include <cstddef>
#include <cstdlib>
#include <new>
#include <vector>
#include <chrono>

// ---------------------------------------------------------------
// 0) Đếm heap alloc toàn cục — phát hiện malloc "lén lút"
// ---------------------------------------------------------------
static std::size_t g_news = 0, g_deletes = 0;
void* operator new(std::size_t n) {
    ++g_news;
    if (void* p = std::malloc(n)) return p;
    std::abort();
}
void operator delete(void* p) noexcept { ++g_deletes; std::free(p); }
void operator delete(void* p, std::size_t) noexcept { ++g_deletes; std::free(p); }

// ---------------------------------------------------------------
// 1) Placement new
// ---------------------------------------------------------------
struct Sensor {
    int id;
    explicit Sensor(int i) : id(i) { std::printf("  Sensor(%d) ctor\n", i); }
    ~Sensor() { std::printf("  ~Sensor(%d) dtor\n", id); }
};

// ---------------------------------------------------------------
// 2) Fixed-size pool: free-list nằm trong chính block
// ---------------------------------------------------------------
template <typename T, std::size_t N>
class ObjectPool {
    // union đảm bảo block đủ lớn cho cả T lẫn con trỏ next của free-list
    union Block { alignas(T) unsigned char raw[sizeof(T)]; Block* next; };
    Block blocks_[N];
    Block* head_;
public:
    ObjectPool() : head_(blocks_) {
        for (std::size_t i = 0; i + 1 < N; ++i) blocks_[i].next = &blocks_[i + 1];
        blocks_[N - 1].next = nullptr;
    }
    template <typename... Args>
    T* create(Args&&... args) {                   // O(1), khong heap
        if (!head_) return nullptr;
        Block* b = head_;
        head_ = b->next;
        return new (b->raw) T(static_cast<Args&&>(args)...);   // placement new
    }
    void destroy(T* p) {                          // O(1)
        p->~T();                                  // dtor thu cong
        Block* b = reinterpret_cast<Block*>(p);
        b->next = head_;
        head_ = b;
    }
};

// ---------------------------------------------------------------
// 3) Arena (bump allocator) + high-water mark
// ---------------------------------------------------------------
class Arena {
    unsigned char* buf_;
    std::size_t cap_, off_ = 0, peak_ = 0;
    static std::size_t align_up(std::size_t n, std::size_t a) {
        return (n + a - 1) & ~(a - 1);
    }
public:
    Arena(void* buf, std::size_t cap) : buf_(static_cast<unsigned char*>(buf)), cap_(cap) {}
    void* alloc(std::size_t n, std::size_t align = alignof(std::max_align_t)) {
        std::size_t start = align_up(off_, align);
        if (start + n > cap_) return nullptr;     // het arena — KHONG fragmentation
        off_ = start + n;
        if (off_ > peak_) peak_ = off_;
        return buf_ + start;
    }
    void reset() { off_ = 0; }                    // "free" tat ca cung luc
    std::size_t peak() const { return peak_; }
};

// ---------------------------------------------------------------
// 4) Custom STL allocator dùng arena
// ---------------------------------------------------------------
template <typename T>
struct ArenaAlloc {
    using value_type = T;
    Arena* arena;
    explicit ArenaAlloc(Arena* a) : arena(a) {}
    template <typename U> ArenaAlloc(const ArenaAlloc<U>& o) : arena(o.arena) {}
    T* allocate(std::size_t n) {
        void* p = arena->alloc(n * sizeof(T), alignof(T));
        if (!p) throw std::bad_alloc();
        return static_cast<T*>(p);
    }
    void deallocate(T*, std::size_t) noexcept {}  // arena: khong free tung cai
    template <typename U> bool operator==(const ArenaAlloc<U>& o) const { return arena == o.arena; }
    template <typename U> bool operator!=(const ArenaAlloc<U>& o) const { return arena != o.arena; }
};

#if defined(__cpp_lib_memory_resource)
#include <memory_resource>
#endif

int main() {
    std::printf("== 1) Placement new ==\n");
    alignas(Sensor) unsigned char storage[sizeof(Sensor)];
    Sensor* s = new (storage) Sensor(7);          // KHONG cap phat
    s->~Sensor();                                  // KHONG delete

    std::printf("\n== 2) Fixed-size pool ==\n");
    static ObjectPool<Sensor, 4> pool;             // static = RAM tinh, khong heap
    Sensor* a = pool.create(1);
    Sensor* b = pool.create(2);
    pool.destroy(a);
    Sensor* c = pool.create(3);                    // tai su dung block cua a
    std::printf("  a=%p duoc tai su dung boi c=%p: %s\n", (void*)a, (void*)c,
                (a == c) ? "YES (free-list O(1))" : "no");
    pool.destroy(b);
    pool.destroy(c);

    std::printf("\n== 3) Arena ==\n");
    alignas(std::max_align_t) static unsigned char arena_buf[1024];
    Arena arena(arena_buf, sizeof(arena_buf));
    int* x = static_cast<int*>(arena.alloc(sizeof(int), alignof(int)));
    *x = 123;
    double* d = static_cast<double*>(arena.alloc(sizeof(double), alignof(double)));
    *d = 4.5;
    std::printf("  x=%d d=%.1f, peak=%zu bytes\n", *x, *d, arena.peak());
    arena.reset();
    std::printf("  reset(): toan bo arena trong lai, khong fragmentation\n");

    std::printf("\n== 4) std::vector tren arena (custom allocator) ==\n");
    std::size_t news_before = g_news;
    {
        std::vector<int, ArenaAlloc<int>> v{ArenaAlloc<int>(&arena)};
        for (int i = 0; i < 100; ++i) v.push_back(i);
        std::printf("  v.size()=%zu, heap alloc trong khoi nay: %zu (ky vong 0)\n",
                    v.size(), g_news - news_before);
    }

#if defined(__cpp_lib_memory_resource)
    std::printf("\n== 5) PMR: monotonic_buffer_resource ==\n");
    static std::byte pmr_buf[2048];
    std::pmr::monotonic_buffer_resource res(pmr_buf, sizeof(pmr_buf),
                                            std::pmr::null_memory_resource());
    // null_memory_resource: neu tran buffer -> NEM, chung minh khong dung heap
    std::pmr::vector<int> pv(&res);
    for (int i = 0; i < 50; ++i) pv.push_back(i * i);
    std::printf("  pmr::vector size=%zu — cung kieu du nguon memory nao\n", pv.size());
#else
    std::printf("\n== 5) (<memory_resource> khong co — can GCC 9+) ==\n");
#endif

    std::printf("\n== 6) Benchmark: new/delete vs pool (100k vong) ==\n");
    using clk = std::chrono::steady_clock;
    auto t0 = clk::now();
    for (int i = 0; i < 100'000; ++i) { Sensor* p = new Sensor(0); delete p; }
    auto t1 = clk::now();
    static ObjectPool<Sensor, 1> bench_pool;
    for (int i = 0; i < 100'000; ++i) { Sensor* p = bench_pool.create(0); bench_pool.destroy(p); }
    auto t2 = clk::now();
    auto us = [](auto dur) {
        return std::chrono::duration_cast<std::chrono::microseconds>(dur).count();
    };
    std::printf("  heap: %lld us | pool: %lld us\n",
                (long long)us(t1 - t0), (long long)us(t2 - t1));
    std::printf("\nTong heap alloc ca chuong trinh: new=%zu delete=%zu\n", g_news, g_deletes);
    return 0;
}
