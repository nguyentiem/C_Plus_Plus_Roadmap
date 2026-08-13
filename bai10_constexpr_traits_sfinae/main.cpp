// Bài 10: constexpr, Type Traits & SFINAE
// Biên dịch: g++ -std=c++17 -Wall -Wextra main.cpp -o bai10_constexpr_traits_sfinae.exe
#include <iostream>
#include <cstdint>
#include <type_traits>

// ---------- 1. constexpr: hai chế độ compile-time / runtime ----------
constexpr int giai_thua(int n) {
    return n <= 1 ? 1 : n * giai_thua(n - 1);
}

void demo_constexpr() {
    std::cout << "== Demo constexpr ==\n";
    constexpr int ct = giai_thua(5);          // BAT BUOC luong gia luc bien dich
    static_assert(ct == 120, "giai_thua(5) phai bang 120");
    int n = 6;                                 // gia tri runtime
    int rt = giai_thua(n);                     // cung ham do, chay luc runtime
    std::cout << "  compile-time: 5! = " << ct << " (kiem chung bang static_assert)\n";
    std::cout << "  runtime     : 6! = " << rt << "\n";
    // C++20 co them: consteval (bat buoc compile-time), constinit (khoi tao tinh)
}

// ---------- 2. Type traits ----------
template<typename T> struct la_con_tro     : std::false_type {};  // trait tu viet
template<typename T> struct la_con_tro<T*> : std::true_type  {};

void demo_type_traits() {
    std::cout << "== Demo type traits ==\n";
    std::cout << std::boolalpha;
    std::cout << "  is_same<int, std::int32_t>   : " << std::is_same<int, std::int32_t>::value << "\n";
    std::cout << "  is_integral<char>            : " << std::is_integral<char>::value << "\n";
    std::cout << "  is_floating_point<int>       : " << std::is_floating_point<int>::value << "\n";
    std::cout << "  la_con_tro<float*> (tu viet) : " << la_con_tro<float*>::value << "\n";
    std::cout << "  la_con_tro<float>            : " << la_con_tro<float>::value << "\n";
    // Bien doi kieu: remove_const_t<const int> == int
    static_assert(std::is_same<std::remove_const_t<const int>, int>::value, "remove_const");
    std::cout << std::noboolalpha;
}

// ---------- 3. SFINAE với enable_if: chọn overload theo loại kiểu ----------
template<typename T>
std::enable_if_t<std::is_integral<T>::value, T>
chia_doi_sfinae(T x) {
    std::cout << "  [SFINAE] ban INTEGRAL duoc chon: ";
    return static_cast<T>(x / 2);
}

template<typename T>
std::enable_if_t<std::is_floating_point<T>::value, T>
chia_doi_sfinae(T x) {
    std::cout << "  [SFINAE] ban FLOATING duoc chon: ";
    return x / T(2);
}

// ---------- 4. if constexpr: cùng logic, một hàm duy nhất, dễ đọc hơn ----------
template<typename T>
T chia_doi_ifconstexpr(T x) {
    if constexpr (std::is_integral<T>::value) {
        std::cout << "  [if constexpr] nhanh integral: ";
        return static_cast<T>(x / 2);
    } else {
        std::cout << "  [if constexpr] nhanh floating: ";
        return x / T(2);          // nhanh sai bi LOAI BO, khong can bien dich cho int
    }
}

void demo_sfinae_vs_ifconstexpr() {
    std::cout << "== Demo SFINAE vs if constexpr ==\n";
    std::cout << chia_doi_sfinae(9) << "\n";
    std::cout << chia_doi_sfinae(9.0) << "\n";
    std::cout << chia_doi_ifconstexpr(9) << "\n";
    std::cout << chia_doi_ifconstexpr(9.0) << "\n";
}

// ---------- 5. Embedded: bảng CRC8 tính lúc BIÊN DỊCH (poly 0x07) ----------
constexpr std::uint8_t crc8_byte(std::uint8_t c) {
    for (int i = 0; i < 8; ++i) {
        c = (c & 0x80u) ? static_cast<std::uint8_t>((c << 1) ^ 0x07u)
                        : static_cast<std::uint8_t>(c << 1);
    }
    return c;
}

struct BangCrc8 { std::uint8_t v[256]; };

constexpr BangCrc8 tao_bang_crc8() {
    BangCrc8 b{};
    for (int i = 0; i < 256; ++i) b.v[i] = crc8_byte(static_cast<std::uint8_t>(i));
    return b;
}

// Bang nam trong .rodata (flash tren MCU), tao luc bien dich, 0 chu ky runtime:
constexpr BangCrc8 BANG_CRC8 = tao_bang_crc8();
static_assert(BANG_CRC8.v[0x00] == 0x00, "CRC8(0x00) phai la 0x00");
static_assert(BANG_CRC8.v[0x01] == 0x07, "CRC8(0x01) phai la 0x07 (chinh la poly)");

std::uint8_t crc8(const std::uint8_t* data, std::size_t len) {
    std::uint8_t crc = 0;
    for (std::size_t i = 0; i < len; ++i)
        crc = BANG_CRC8.v[crc ^ data[i]];     // tra bang: 1 phep XOR + 1 lan doc
    return crc;
}

void demo_crc8_compile_time() {
    std::cout << "== Demo bang CRC8 compile-time ==\n";
    const std::uint8_t frame[] = {0xAA, 0x01, 0xD2, 0x04};
    std::cout << "  CRC8 cua frame {AA 01 D2 04} = 0x" << std::hex
              << static_cast<int>(crc8(frame, sizeof frame)) << std::dec << "\n";
    std::cout << "  Vai gia tri bang (tinh luc bien dich): "
              << "v[1]=0x" << std::hex << static_cast<int>(BANG_CRC8.v[1])
              << ", v[255]=0x" << static_cast<int>(BANG_CRC8.v[255]) << std::dec << "\n";
}

int main() {
    demo_constexpr();
    demo_type_traits();
    demo_sfinae_vs_ifconstexpr();
    demo_crc8_compile_time();
    std::cout << "Bai 10 hoan thanh.\n";
    return 0;
}
