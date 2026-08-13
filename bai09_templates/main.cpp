// Bài 09: Templates
// Biên dịch: g++ -std=c++17 -Wall -Wextra main.cpp -o bai09_templates.exe
#include <iostream>
#include <cstdint>
#include <cstddef>

// ---------- 1. Function template ----------
template<typename T>
constexpr const T& max2(const T& a, const T& b) {
    return a < b ? b : a;
}

void demo_function_template() {
    std::cout << "== Demo function template ==\n";
    std::cout << "  max2(3, 7)       = " << max2(3, 7) << "\n";
    std::cout << "  max2(2.5, 1.5)   = " << max2(2.5, 1.5) << "\n";
    // max2(1, 2.5);  // LOI: deduction xung dot int/double
    std::cout << "  max2<double>(1, 2.5) = " << max2<double>(1, 2.5)
              << " (chi dinh tuong minh)\n";
}

// ---------- 2. Class template + non-type template parameter (NTTP) ----------
// Vi du embedded: ring buffer kich thuoc compile-time, KHONG dung heap
template<typename T, std::size_t N>
class RingBuffer {
    static_assert(N > 0 && (N & (N - 1)) == 0, "N phai la luy thua cua 2");
    T data_[N] = {};                 // mang TINH: sizeof biet luc bien dich
    std::size_t head_ = 0, tail_ = 0, count_ = 0;
public:
    bool push(const T& v) {
        if (count_ == N) return false;
        data_[head_] = v;
        head_ = (head_ + 1) & (N - 1);   // % N toi uu thanh AND vi N = 2^k
        ++count_;
        return true;
    }
    bool pop(T& ra) {
        if (count_ == 0) return false;
        ra = data_[tail_];
        tail_ = (tail_ + 1) & (N - 1);
        --count_;
        return true;
    }
    static constexpr std::size_t capacity() { return N; }  // N nam trong KIEU
};

void demo_class_template_nttp() {
    std::cout << "== Demo class template + NTTP (buffer compile-time) ==\n";
    RingBuffer<std::uint8_t, 8> rb;   // 8 byte tinh, khong malloc
    std::cout << "  capacity (hang bien dich) = " << rb.capacity()
              << ", sizeof(rb) = " << sizeof(rb) << "\n";
    for (std::uint8_t i = 1; i <= 3; ++i) rb.push(i);
    std::uint8_t v;
    std::cout << "  pop:";
    while (rb.pop(v)) std::cout << " " << static_cast<int>(v);
    std::cout << "\n";
}

// ---------- 3. Specialization: full & partial ----------
template<typename T>
struct MoTa {                                   // ban tong quat
    static const char* ten() { return "kieu tong quat"; }
};
template<>
struct MoTa<bool> {                             // FULL specialization cho bool
    static const char* ten() { return "bool (full specialization)"; }
};
template<typename T>
struct MoTa<T*> {                               // PARTIAL specialization cho moi con tro
    static const char* ten() { return "con tro (partial specialization)"; }
};

void demo_specialization() {
    std::cout << "== Demo specialization ==\n";
    std::cout << "  MoTa<int>    : " << MoTa<int>::ten() << "\n";
    std::cout << "  MoTa<bool>   : " << MoTa<bool>::ten() << "\n";
    std::cout << "  MoTa<float*> : " << MoTa<float*>::ten() << "\n";
}

// ---------- 4. Variadic template + fold expressions (C++17) ----------
template<typename... Ts>
constexpr auto tong(Ts... v) {
    return (v + ... + 0);            // binary right fold: hop le ca khi pack rong
}

template<typename... Ts>
constexpr std::uint8_t checksum_xor(Ts... bytes) {
    static_assert(sizeof...(Ts) > 0, "Can it nhat 1 byte");
    return static_cast<std::uint8_t>((bytes ^ ...));   // unary fold qua XOR
}

template<typename... Ts>
void in_tat_ca(const Ts&... vals) {
    ((std::cout << vals << ' '), ...);   // fold qua toan tu phay
    std::cout << "\n";
}

void demo_variadic_fold() {
    std::cout << "== Demo variadic + fold expressions ==\n";
    std::cout << "  tong(1,2,3,4) = " << tong(1, 2, 3, 4)
              << ", tong() = " << tong() << " (pack rong nho '+ 0')\n";
    // Tinh checksum luc BIEN DICH:
    constexpr auto cs = checksum_xor(0xAAu, 0x55u, 0x0Fu);
    static_assert(cs == 0xF0, "checksum tinh sai luc bien dich");
    std::cout << "  checksum_xor(0xAA,0x55,0x0F) = 0x" << std::hex
              << static_cast<int>(cs) << std::dec << " (tinh luc compile-time)\n";
    std::cout << "  in_tat_ca: ";
    in_tat_ca(42, 3.14, "chuoi", 'X');
}

int main() {
    demo_function_template();
    demo_class_template_nttp();
    demo_specialization();
    demo_variadic_fold();
    std::cout << "Bai 09 hoan thanh.\n";
    return 0;
}
