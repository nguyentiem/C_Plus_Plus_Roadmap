// Bài 16: Concurrency — thread, mutex, condition_variable, atomic, false sharing
// Biên dịch: g++ -std=c++17 -Wall -Wextra -O2 -pthread main.cpp -o bai16_concurrency.exe
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

using namespace std::chrono_literals;

// ---------------------------------------------------------------
// Demo 1: race condition vs mutex vs atomic
// ---------------------------------------------------------------
void demo_race_va_khac_phuc() {
    std::cout << "=== Demo 1: race condition & cach khac phuc ===\n";
    constexpr int N_THREAD = 4;
    constexpr int N_LAP = 100000;

    // 1a) Counter KHONG dong bo — data race (UB!), ket qua thuong sai
    // Luu y: day la vi du minh hoa loi, khong bao gio viet the nay trong san pham.
    int counter_race = 0;
    {
        std::vector<std::thread> ths;
        for (int t = 0; t < N_THREAD; ++t)
            ths.emplace_back([&counter_race] {
                for (int i = 0; i < N_LAP; ++i) ++counter_race; // RACE!
            });
        for (auto& th : ths) th.join();
    }

    // 1b) Dung mutex — dung nhung co chi phi khoa
    int counter_mutex = 0;
    std::mutex m;
    {
        std::vector<std::thread> ths;
        for (int t = 0; t < N_THREAD; ++t)
            ths.emplace_back([&] {
                for (int i = 0; i < N_LAP; ++i) {
                    std::lock_guard<std::mutex> lk(m); // RAII: tu unlock
                    ++counter_mutex;
                }
            });
        for (auto& th : ths) th.join();
    }

    // 1c) Dung atomic — nguyen tu, khong can khoa
    std::atomic<int> counter_atomic{0};
    {
        std::vector<std::thread> ths;
        for (int t = 0; t < N_THREAD; ++t)
            ths.emplace_back([&counter_atomic] {
                for (int i = 0; i < N_LAP; ++i)
                    counter_atomic.fetch_add(1, std::memory_order_relaxed);
            });
        for (auto& th : ths) th.join();
    }

    const int expect = N_THREAD * N_LAP;
    std::cout << "  Ky vong        : " << expect << "\n";
    std::cout << "  Khong dong bo  : " << counter_race
              << (counter_race == expect ? "  (may man, van la UB!)" : "  <-- SAI vi race") << "\n";
    std::cout << "  Mutex          : " << counter_mutex << "\n";
    std::cout << "  Atomic relaxed : " << counter_atomic.load() << "\n\n";
}

// ---------------------------------------------------------------
// Demo 2: producer/consumer voi queue + condition_variable
// (tuong duong xQueueSend/xQueueReceive cua FreeRTOS)
// ---------------------------------------------------------------
class HangDoiAnToan {
public:
    void push(int v) {
        {
            std::lock_guard<std::mutex> lk(m_);
            q_.push(v);
        }
        cv_.notify_one(); // notify ngoai khoa: giam contention
    }
    // Tra ve false khi da dong va het hang
    bool pop(int& out) {
        std::unique_lock<std::mutex> lk(m_);
        // LUON dung predicate: chong spurious wakeup
        cv_.wait(lk, [this] { return !q_.empty() || dong_; });
        if (q_.empty()) return false; // dong_ == true va het du lieu
        out = q_.front();
        q_.pop();
        return true;
    }
    void dong() {
        {
            std::lock_guard<std::mutex> lk(m_);
            dong_ = true;
        }
        cv_.notify_all(); // danh thuc moi consumer dang cho
    }

private:
    std::mutex m_;
    std::condition_variable cv_;
    std::queue<int> q_;
    bool dong_ = false;
};

void demo_producer_consumer() {
    std::cout << "=== Demo 2: producer/consumer (queue + condition_variable) ===\n";
    HangDoiAnToan hq;
    std::atomic<int> tong_tieu_thu{0};
    std::atomic<int> so_phan_tu{0};

    std::thread producer([&hq] {
        for (int i = 1; i <= 20; ++i) {
            hq.push(i);
            std::this_thread::sleep_for(1ms); // gia lap sensor sinh du lieu
        }
        hq.dong();
    });

    std::vector<std::thread> consumers;
    for (int c = 0; c < 2; ++c)
        consumers.emplace_back([&] {
            int v = 0;
            while (hq.pop(v)) { // ngu cho, khong ton CPU (khac voi busy-wait)
                tong_tieu_thu.fetch_add(v, std::memory_order_relaxed);
                so_phan_tu.fetch_add(1, std::memory_order_relaxed);
            }
        });

    producer.join();
    for (auto& t : consumers) t.join();
    std::cout << "  Da tieu thu " << so_phan_tu.load()
              << " phan tu, tong = " << tong_tieu_thu.load()
              << " (ky vong 20 phan tu, tong 210)\n\n";
}

// ---------------------------------------------------------------
// Demo 3: release/acquire — cong bo du lieu qua co (flag)
// ---------------------------------------------------------------
void demo_release_acquire() {
    std::cout << "=== Demo 3: memory_order release/acquire ===\n";
    int du_lieu = 0;                 // du lieu thuong (khong atomic)
    std::atomic<bool> san_sang{false};

    std::thread ghi([&] {
        du_lieu = 42;                              // (1) ghi truoc...
        san_sang.store(true, std::memory_order_release); // (2) ...roi cong bo
    });
    std::thread doc([&] {
        while (!san_sang.load(std::memory_order_acquire)) {
            std::this_thread::yield();
        }
        // acquire dong bo voi release => (1) happens-before day
        std::cout << "  Thread doc thay du_lieu = " << du_lieu
                  << " (dam bao 42 nho release->acquire)\n";
    });
    ghi.join();
    doc.join();
    std::cout << "\n";
}

// ---------------------------------------------------------------
// Demo 4: false sharing — hai counter cung cache line vs tach line
// ---------------------------------------------------------------
struct KeNhau {           // hai counter nam cung cache line 64B
    std::atomic<uint64_t> a{0};
    std::atomic<uint64_t> b{0};
};
struct TachLine {         // alignas(64) day moi counter ra rieng mot line
    alignas(64) std::atomic<uint64_t> a{0};
    alignas(64) std::atomic<uint64_t> b{0};
};

template <typename T>
static double do_thoi_gian_ms(T& cap) {
    constexpr uint64_t N = 2000000;
    const auto t0 = std::chrono::steady_clock::now();
    std::thread t1([&] { for (uint64_t i = 0; i < N; ++i) cap.a.fetch_add(1, std::memory_order_relaxed); });
    std::thread t2([&] { for (uint64_t i = 0; i < N; ++i) cap.b.fetch_add(1, std::memory_order_relaxed); });
    t1.join();
    t2.join();
    const auto t1e = std::chrono::steady_clock::now();
    return std::chrono::duration<double, std::milli>(t1e - t0).count();
}

void demo_false_sharing() {
    std::cout << "=== Demo 4: false sharing ===\n";
    KeNhau ke;
    TachLine tach;
    std::cout << "  sizeof(KeNhau)   = " << sizeof(KeNhau)
              << " byte (2 counter chung cache line)\n";
    std::cout << "  sizeof(TachLine) = " << sizeof(TachLine)
              << " byte (moi counter mot line 64B)\n";
    const double ms_ke = do_thoi_gian_ms(ke);
    const double ms_tach = do_thoi_gian_ms(tach);
    std::cout << "  Cung cache line : " << ms_ke << " ms\n";
    std::cout << "  alignas(64)     : " << ms_tach << " ms"
              << "  (thuong nhanh hon ro ret tren CPU da nhan)\n\n";
}

int main() {
    std::cout << "Bai 16: Concurrency trong C++ (senior)\n";
    std::cout << "hardware_concurrency = " << std::thread::hardware_concurrency() << "\n\n";
    demo_race_va_khac_phuc();
    demo_producer_consumer();
    demo_release_acquire();
    demo_false_sharing();
    std::cout << "Hoan tat — moi thread da join, thoat sach.\n";
    return 0;
}
