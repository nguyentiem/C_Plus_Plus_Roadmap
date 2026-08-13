// Bài 14: STL Algorithms, std::string (SSO), string_view, span tự viết, chrono
// Biên dịch: g++ -std=c++17 -Wall -Wextra -O2 main.cpp -o bai14_algorithms_string.exe
#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdio>
#include <numeric>
#include <string>
#include <string_view>
#include <vector>

// ---------- 1. Các algorithm chủ lực ----------
static void demo_algorithms() {
    std::printf("--- demo_algorithms ---\n");
    std::vector<int> v{7, 2, 9, 4, 2, 8, 1};

    std::sort(v.begin(), v.end());
    std::printf("  sort:        ");
    for (int x : v) std::printf("%d ", x);
    std::printf("\n");

    auto it = std::find_if(v.begin(), v.end(), [](int x) { return x > 5; });
    std::printf("  find_if >5:  %d\n", (it != v.end()) ? *it : -1);

    long chan = std::count_if(v.begin(), v.end(), [](int x) { return x % 2 == 0; });
    int tong = std::accumulate(v.begin(), v.end(), 0);
    std::printf("  count_if chan = %ld, accumulate = %d\n", chan, tong);

    std::transform(v.begin(), v.end(), v.begin(), [](int x) { return x * 10; });
    std::printf("  transform x10: ");
    for (int x : v) std::printf("%d ", x);
    std::printf("\n");

    // lower_bound: tìm nhị phân O(log n) — YÊU CẦU dữ liệu đã sort
    auto lb = std::lower_bound(v.begin(), v.end(), 40);
    std::printf("  lower_bound(40) -> %d (index %td)\n", *lb, lb - v.begin());
}

// ---------- 2. remove/erase idiom ----------
static void demo_remove_erase() {
    std::printf("--- demo_remove_erase ---\n");
    std::vector<int> v{1, 0, 2, 0, 3, 0};
    auto new_end = std::remove(v.begin(), v.end(), 0); // chỉ DỒN, không xoá
    std::printf("  sau remove: size van = %zu (remove khong xoa!)\n", v.size());
    v.erase(new_end, v.end());                          // erase mới thật sự co lại
    std::printf("  sau erase:  size = %zu, noi dung: ", v.size());
    for (int x : v) std::printf("%d ", x);
    std::printf("\n");
}

// ---------- 3. std::string và SSO ----------
static void demo_string_sso() {
    std::printf("--- demo_string_sso ---\n");
    std::string ngan = "abc";                       // ngắn: SSO nếu ABI hỗ trợ
    auto trong_object = [](const std::string& s) {
        const char* d = s.data();
        const char* lo = reinterpret_cast<const char*>(&s);
        return d >= lo && d < lo + sizeof(s);       // data nằm TRONG object?
    };
    // libstdc++ ABI mới (CXX11): sizeof(string)=32, có SSO (buffer 15 ký tự).
    // ABI cũ (COW):              sizeof(string)=8 (chỉ 1 con trỏ), KHÔNG có SSO.
    std::printf("  sizeof(std::string) = %zu -> ABI %s\n", sizeof(std::string),
                sizeof(std::string) > sizeof(void*) ? "CXX11 (co SSO)"
                                                    : "COW cu (KHONG co SSO)");
    std::printf("  \"%s\": data nam trong object = %s\n", ngan.c_str(),
                trong_object(ngan) ? "CO (SSO hoat dong)"
                                   : "khong (chuoi nam tren heap)");
}

// ---------- 4. string_view: không copy + bẫy dangling ----------
static std::string_view first_word(std::string_view s) { // nhận sv: không cấp phát
    return s.substr(0, s.find(' '));                     // sv.substr trả sv — an toàn
}

static void demo_string_view() {
    std::printf("--- demo_string_view ---\n");
    std::string line = "TEMP 25.5 C";
    std::string_view w = first_word(line); // trỏ vào buffer của line (còn sống -> OK)
    std::printf("  first_word = \"%.*s\" (khong copy byte nao)\n",
                static_cast<int>(w.size()), w.data());
    // BẪY (mô tả, không chạy): std::string_view sv = line.substr(0, 4);
    //   string::substr trả std::string TẠM -> sv dangling ngay sau dấu ';'
    std::printf("  BAY: string_view tro vao string::substr() tam -> dangling!\n");
}

// ---------- 5. SimpleSpan: bản C++17 tự viết của std::span ----------
template <typename T>
class SimpleSpan { // chỉ {con trỏ, độ dài} — thay cho cặp tham số (ptr, len)
public:
    template <std::size_t N>
    SimpleSpan(T (&arr)[N]) : p_(arr), n_(N) {}                    // mảng C
    template <std::size_t N>
    SimpleSpan(std::array<T, N>& arr) : p_(arr.data()), n_(N) {}   // std::array
    SimpleSpan(T* p, std::size_t n) : p_(p), n_(n) {}
    T* begin() const { return p_; }
    T* end() const { return p_ + n_; }
    std::size_t size() const { return n_; }
private:
    T* p_;
    std::size_t n_;
};

static int sum_span(SimpleSpan<const int> data) { // MỘT chữ ký cho mọi nguồn
    return std::accumulate(data.begin(), data.end(), 0);
}

static void demo_span() {
    std::printf("--- demo_span ---\n");
    const int c_arr[] = {1, 2, 3};
    std::array<const int, 4> std_arr{10, 20, 30, 40};
    std::vector<int> vec{100, 200};
    std::printf("  sum(mang C)      = %d\n", sum_span(c_arr));
    std::printf("  sum(std::array)  = %d\n", sum_span(std_arr));
    std::printf("  sum(vector)      = %d\n",
                sum_span({vec.data(), vec.size()}));
    // C++20 thật: std::span<const int> — chữ ký y hệt, có sẵn trong <span>
}

// ---------- 6. chrono cơ bản ----------
static void demo_chrono() {
    std::printf("--- demo_chrono ---\n");
    using namespace std::chrono;
    auto t0 = steady_clock::now();          // steady: đơn điệu, dùng để ĐO
    volatile long long s = 0;
    for (int i = 0; i < 1000000; ++i) s += i;
    auto dt = steady_clock::now() - t0;
    std::printf("  vong lap ton %lld us\n",
                static_cast<long long>(duration_cast<microseconds>(dt).count()));
    milliseconds timeout{1500};             // duration type-safe: hết nhầm ms/us
    std::printf("  timeout 1500 ms = %lld us\n",
                static_cast<long long>(duration_cast<microseconds>(timeout).count()));
}

int main() {
    std::printf("=== Bai 14: Algorithms & String ===\n");
    demo_algorithms();
    demo_remove_erase();
    demo_string_sso();
    demo_string_view();
    demo_span();
    demo_chrono();
    std::printf("=== Ket thuc ===\n");
    return 0;
}
