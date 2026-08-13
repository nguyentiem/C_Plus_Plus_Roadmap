// Bài 24: Debugging — chương trình chứa BUG CỐ Ý để thực hành GDB/sanitizers
// Build: make          (build thường)
//        make asan     (build với ASan + UBSan)
// Chạy:  ./bai24_debugging.exe [stack|heap|ub|race|leak]
#include <cstdio>
#include <cstring>
#include <cstdint>
#include <thread>

// Biến "nạn nhân" cho watchpoint: ai ghi đè tôi?
static uint32_t g_config = 0xCAFEBABE;

// ---------------------------------------------------------------
// BUG 1: stack buffer overflow — strcpy quá kích thước mảng
// ---------------------------------------------------------------
void bug_stack() {
    char name[8];
    const char* input = "day_la_chuoi_qua_dai_cho_buffer_8_byte";
    std::printf("truoc: g_config=0x%08X\n", g_config);
    std::strcpy(name, input);            // BUG: ghi tran ra ngoai name[8]
    std::printf("sau  : name=%s\n", name);
    std::printf("sau  : g_config=0x%08X (co the da bi de!)\n", g_config);
    // GDB: watch g_config  -> dung ngay tai dong strcpy
    // ASan: "stack-buffer-overflow WRITE of size .. at .."
}

// ---------------------------------------------------------------
// BUG 2: use-after-free + double-free
// ---------------------------------------------------------------
void bug_heap() {
    int* data = new int[4]{1, 2, 3, 4};
    delete[] data;
    std::printf("use-after-free doc: %d\n", data[0]);   // BUG: doc vung da free
    data[1] = 99;                                        // BUG: ghi vung da free
    delete[] data;                                       // BUG: double-free
    // ASan chi ra: stack luc GHI, stack luc CAP PHAT, stack luc FREE
}

// ---------------------------------------------------------------
// BUG 3: undefined behavior — signed overflow, shift qua bit
// ---------------------------------------------------------------
int scale(int x) { return x * 3; }       // x lon -> signed overflow = UB

void bug_ub() {
    int big = 2'000'000'000;
    std::printf("scale(%d) = %d (signed overflow — UB!)\n", big, scale(big));
    int sh = 40;
    std::printf("1 << %d = %d (shift >= 32 bit — UB!)\n", sh, 1 << sh);
    // UBSan: "signed integer overflow", "shift exponent 40 is too large"
    // Day la loai bug "chay dung -O0, sai -O2"
}

// ---------------------------------------------------------------
// BUG 4: data race — 2 thread ++ khong dong bo
// ---------------------------------------------------------------
void bug_race() {
    static int counter = 0;              // BUG: khong atomic, khong mutex
    auto work = [] { for (int i = 0; i < 500'000; ++i) ++counter; };
    std::thread t1(work), t2(work);
    t1.join(); t2.join();
    std::printf("counter = %d (ky vong 1000000 — thuong THIEU do race)\n", counter);
    // TSan (-fsanitize=thread, build rieng): "WARNING: ThreadSanitizer: data race"
}

// ---------------------------------------------------------------
// BUG 5: memory leak
// ---------------------------------------------------------------
void bug_leak() {
    for (int i = 0; i < 10; ++i) {
        char* buf = new char[1024];
        std::snprintf(buf, 1024, "message %d", i);
        std::printf("%s\n", buf);
        // BUG: quen delete[] buf — moi vong leak 1KB
    }
    // ASan (khi thoat): "LeakSanitizer: detected memory leaks"
    // Cach sua dung: std::vector<char> / unique_ptr — RAII (bai 3, 11)
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::printf("Cach dung: %s [stack|heap|ub|race|leak]\n", argv[0]);
        std::printf("Build voi 'make asan' roi chay lai de sanitizer chi ra bug.\n");
        return 0;
    }
    const char* mode = argv[1];
    std::printf("=== chay bug case: %s ===\n", mode);
    if      (!std::strcmp(mode, "stack")) bug_stack();
    else if (!std::strcmp(mode, "heap"))  bug_heap();
    else if (!std::strcmp(mode, "ub"))    bug_ub();
    else if (!std::strcmp(mode, "race"))  bug_race();
    else if (!std::strcmp(mode, "leak"))  bug_leak();
    else std::printf("khong biet case '%s'\n", mode);
    return 0;
}
