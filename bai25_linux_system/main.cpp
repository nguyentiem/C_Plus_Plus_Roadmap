// Bài 25: Linux/System Programming — demo portable + phần POSIX (#ifdef)
// Trên Windows/MinGW: chạy phần portable. Trên WSL/Linux: chạy đầy đủ.
// Build: make && ./bai25_linux_system.exe
#include <cstdio>
#include <csignal>
#include <cstring>
#include <thread>
#include <mutex>
#include <vector>
#include <chrono>

// ---------------------------------------------------------------
// 1) Signal handler an toàn: chỉ ghi sig_atomic_t
// ---------------------------------------------------------------
static volatile std::sig_atomic_t g_stop = 0;
extern "C" void on_signal(int) { g_stop = 1; }   // KHONG printf/malloc o day!

// ---------------------------------------------------------------
// 2) Thread chia sẻ memory (so sánh với process cách ly)
// ---------------------------------------------------------------
static int g_shared = 0;
static std::mutex g_mtx;

void worker(int id) {
    for (int i = 0; i < 100'000; ++i) {
        std::lock_guard<std::mutex> lk(g_mtx);   // thread chung memory -> can mutex
        ++g_shared;
    }
    (void)id;
}

// ---------------------------------------------------------------
// 3) Mô phỏng "syscall đắt hơn function call": buffered vs unbuffered I/O
// ---------------------------------------------------------------
void io_benchmark() {
    using clk = std::chrono::steady_clock;
    const int N = 20'000;

    std::FILE* f1 = std::fopen("io_buffered.tmp", "w");     // stdio: co buffer
    auto t0 = clk::now();
    for (int i = 0; i < N; ++i) std::fprintf(f1, "line %d\n", i);
    std::fclose(f1);
    auto t1 = clk::now();

    std::FILE* f2 = std::fopen("io_unbuffered.tmp", "w");
    std::setvbuf(f2, nullptr, _IONBF, 0);                    // tat buffer -> moi write la 1 syscall
    auto t2 = clk::now();
    for (int i = 0; i < N; ++i) std::fprintf(f2, "line %d\n", i);
    std::fclose(f2);
    auto t3 = clk::now();

    auto ms = [](auto d) {
        return std::chrono::duration_cast<std::chrono::milliseconds>(d).count();
    };
    std::printf("  ghi %d dong: buffered=%lld ms | unbuffered=%lld ms\n",
                N, (long long)ms(t1 - t0), (long long)ms(t3 - t2));
    std::printf("  => syscall dat -> buffer I/O (stdio lam ho ban)\n");
    std::remove("io_buffered.tmp");
    std::remove("io_unbuffered.tmp");
}

#if defined(__unix__) || defined(__APPLE__)
// ---------------------------------------------------------------
// 4) POSIX-only: fork + pipe (chạy trên WSL/Linux)
// ---------------------------------------------------------------
#include <unistd.h>
#include <sys/wait.h>

void demo_fork_pipe() {
    int fds[2];
    if (pipe(fds) != 0) { std::perror("pipe"); return; }
    pid_t pid = fork();
    if (pid == 0) {                       // === process CON ===
        close(fds[0]);                    // con chi ghi
        const char* msg = "hello tu process con qua pipe";
        write(fds[1], msg, std::strlen(msg) + 1);
        close(fds[1]);
        _exit(0);
    }
    // === process CHA ===
    close(fds[1]);                        // cha chi doc
    char buf[64] = {};
    read(fds[0], buf, sizeof(buf) - 1);
    close(fds[0]);
    int status = 0;
    waitpid(pid, &status, 0);             // tranh zombie!
    std::printf("  cha (pid=%d) nhan tu con (pid=%d): \"%s\"\n",
                (int)getpid(), (int)pid, buf);
}
#endif

int main() {
    std::printf("== 1) Signal: nhan SIGINT de graceful shutdown ==\n");
    std::signal(SIGINT, on_signal);
    std::printf("  handler chi ghi sig_atomic_t; main loop kiem tra g_stop\n");

    std::printf("\n== 2) Thread chia se memory (mutex) ==\n");
    std::vector<std::thread> pool;
    for (int i = 0; i < 4; ++i) pool.emplace_back(worker, i);
    for (auto& t : pool) t.join();
    std::printf("  4 thread x 100k tang: g_shared=%d (dung nho mutex)\n", g_shared);
    std::printf("  process thi NGUOC LAI: memory rieng, muon chia se phai IPC\n");

    std::printf("\n== 3) Buffered vs unbuffered I/O (chi phi syscall) ==\n");
    io_benchmark();

#if defined(__unix__) || defined(__APPLE__)
    std::printf("\n== 4) fork + pipe + waitpid (POSIX) ==\n");
    demo_fork_pipe();
#else
    std::printf("\n== 4) fork/pipe/mmap/socket: chay tren WSL hoac Linux ==\n");
    std::printf("  (Windows khong co fork; xem ly thuyet trong 25_linux_system.md)\n");
#endif

    if (g_stop) std::printf("\n(nhan duoc SIGINT — thoat sach)\n");
    return 0;
}
