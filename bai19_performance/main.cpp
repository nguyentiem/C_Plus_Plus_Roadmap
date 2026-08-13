// Bai 19: Performance — padding/alignment, AoS vs SoA benchmark, volatile sink
// Bien dich: g++ -std=c++17 -Wall -Wextra -O2 main.cpp -o bai19_performance.exe
#include <chrono>
#include <cstdint>
#include <iostream>
#include <random>
#include <vector>

// ================================================================
// Demo 1: object layout — padding & alignment
// ================================================================

// Sap xep TE: char/int/char -> padding lon
struct LayoutXau {
    char a;      // 1 byte + 3 padding (int can align 4)
    int b;       // 4 byte
    char c;      // 1 byte + 3 padding (struct align theo member lon nhat)
};

// Sap xep TOT: member lon truoc, nho sau -> it padding
struct LayoutTot {
    int b;       // 4 byte
    char a;      // 1 byte
    char c;      // 1 byte + 2 padding cuoi
};

// alignas: ep can chinh 64 byte (vi du chong false sharing / buffer DMA)
struct CanChinh64 {
    alignas(64) uint32_t du_lieu;
};

// Khoa layout luc compile — vo la biet ngay, khong doi den runtime
static_assert(sizeof(LayoutXau) == 12, "LayoutXau phai 12 byte");
static_assert(sizeof(LayoutTot) == 8, "LayoutTot phai 8 byte");
static_assert(alignof(CanChinh64) == 64, "CanChinh64 phai align 64");

void demo_padding() {
    std::cout << "=== Demo 1: padding & alignment ===\n";
    std::cout << "  LayoutXau {char,int,char}: sizeof = " << sizeof(LayoutXau)
              << ", alignof = " << alignof(LayoutXau) << "\n";
    std::cout << "  LayoutTot {int,char,char}: sizeof = " << sizeof(LayoutTot)
              << ", alignof = " << alignof(LayoutTot) << "\n";
    std::cout << "  -> Cung du lieu, tiet kiem " << (sizeof(LayoutXau) - sizeof(LayoutTot))
              << " byte/phan tu; mang 10000 phan tu tiet kiem "
              << (sizeof(LayoutXau) - sizeof(LayoutTot)) * 10000 / 1024 << " KB RAM\n";
    std::cout << "  CanChinh64: sizeof = " << sizeof(CanChinh64)
              << ", alignof = " << alignof(CanChinh64) << " (alignas(64))\n\n";
}

// ================================================================
// Demo 2: AoS vs SoA — cache locality benchmark
// Bai toan: N "hat", moi vong chi cap nhat x theo vx
// ================================================================

struct HatAoS { // Array of Structs: moi hat 32 byte, chi dung 8 byte moi vong
    float x, y, z;
    float vx, vy, vz;
    float khoi_luong;
    float thoi_gian_song;
};

struct HatSoA { // Struct of Arrays: mang x[] va vx[] lien mach
    std::vector<float> x, y, z;
    std::vector<float> vx, vy, vz;
    std::vector<float> khoi_luong;
    std::vector<float> thoi_gian_song;
};

// volatile sink: "ho den" buoc compiler giu lai phep tinh
static volatile float g_sink = 0.0f;

static double benchmark_aos(std::vector<HatAoS>& hat, int soVong) {
    const auto t0 = std::chrono::steady_clock::now();
    for (int v = 0; v < soVong; ++v)
        for (auto& h : hat)
            h.x += h.vx * 0.016f; // chi cham x va vx — 8/32 byte moi struct
    const auto t1 = std::chrono::steady_clock::now();
    g_sink = hat[0].x; // chong dead-code elimination
    return std::chrono::duration<double, std::milli>(t1 - t0).count();
}

static double benchmark_soa(HatSoA& hat, int soVong) {
    const std::size_t n = hat.x.size();
    const auto t0 = std::chrono::steady_clock::now();
    for (int v = 0; v < soVong; ++v)
        for (std::size_t i = 0; i < n; ++i)
            hat.x[i] += hat.vx[i] * 0.016f; // x[] lien mach -> cache line dung 100%
    const auto t1 = std::chrono::steady_clock::now();
    g_sink = hat.x[0];
    return std::chrono::duration<double, std::milli>(t1 - t0).count();
}

void demo_aos_vs_soa() {
    std::cout << "=== Demo 2: AoS vs SoA (cache locality) ===\n";
    constexpr std::size_t N = 200000;
    constexpr int SO_VONG = 100;

    // Du lieu dau vao sinh runtime (khong phai hang compile-time)
    std::mt19937 rng(12345);
    std::uniform_real_distribution<float> phanBo(-1.0f, 1.0f);

    std::vector<HatAoS> aos(N);
    HatSoA soa;
    soa.x.resize(N); soa.y.resize(N); soa.z.resize(N);
    soa.vx.resize(N); soa.vy.resize(N); soa.vz.resize(N);
    soa.khoi_luong.resize(N); soa.thoi_gian_song.resize(N);
    for (std::size_t i = 0; i < N; ++i) {
        const float x = phanBo(rng), vx = phanBo(rng);
        aos[i] = HatAoS{x, 0, 0, vx, 0, 0, 1.0f, 0};
        soa.x[i] = x;
        soa.vx[i] = vx;
    }

    // Warm-up: nap cache, on dinh truoc khi do
    benchmark_aos(aos, 5);
    benchmark_soa(soa, 5);

    const double msAos = benchmark_aos(aos, SO_VONG);
    const double msSoa = benchmark_soa(soa, SO_VONG);

    std::cout << "  N = " << N << " hat, " << SO_VONG << " vong, chi cap nhat x += vx*dt\n";
    std::cout << "  AoS (struct 32B/hat): " << msAos << " ms\n";
    std::cout << "  SoA (mang lien mach): " << msSoa << " ms\n";
    std::cout << "  -> SoA thuong nhanh hon vi moi cache line 64B chua 16 float huu ich,\n"
                 "     con AoS chi dung 8/32 byte cua moi struct tai ve.\n\n";
}

// ================================================================
// Demo 3: volatile sink — chong optimizer xoa benchmark
// ================================================================
void demo_volatile_sink() {
    std::cout << "=== Demo 3: volatile sink ===\n";
    constexpr int N = 1000000;

    // Phien ban KHONG sink: ket qua khong dung -> -O2 co the xoa ca vong lap
    auto t0 = std::chrono::steady_clock::now();
    float tongBiXoa = 0.0f;
    for (int i = 0; i < N; ++i) tongBiXoa += static_cast<float>(i) * 0.5f;
    auto t1 = std::chrono::steady_clock::now();
    (void)tongBiXoa; // co y khong dung — moi cho optimizer

    // Phien ban CO sink: ghi vao volatile -> vong lap buoc phai chay
    auto t2 = std::chrono::steady_clock::now();
    float tongGiuLai = 0.0f;
    for (int i = 0; i < N; ++i) tongGiuLai += static_cast<float>(i) * 0.5f;
    g_sink = tongGiuLai; // <-- diem khac biet duy nhat
    auto t3 = std::chrono::steady_clock::now();

    const double msXoa = std::chrono::duration<double, std::milli>(t1 - t0).count();
    const double msGiu = std::chrono::duration<double, std::milli>(t3 - t2).count();
    std::cout << "  Khong sink (co the bi xoa) : " << msXoa << " ms\n";
    std::cout << "  Co volatile sink           : " << msGiu << " ms\n";
    std::cout << "  -> Neu ban dau ~0 ms tuc vong lap da bi optimizer loai bo.\n"
                 "     Benchmark luon can sink + du lieu runtime + warm-up.\n\n";
}

int main() {
    std::cout << "Bai 19: Performance & data-oriented design\n\n";
    demo_padding();
    demo_aos_vs_soa();
    demo_volatile_sink();
    std::cout << "Hoan tat. (Ket qua do phu thuoc may — hay tu chay nhieu lan!)\n";
    return 0;
}
