// Bài 08: Casting, POD & Memory Layout
// Biên dịch: g++ -std=c++17 -Wall -Wextra main.cpp -o bai08_casting_pod.exe
#include <iostream>
#include <cstdint>
#include <cstring>
#include <cstddef>
#include <type_traits>

// ---------- 1. Bốn loại cast ----------
class Base {
public:
    virtual ~Base() = default;
    virtual const char* who() const { return "Base"; }
};
class Derived : public Base {
public:
    const char* who() const override { return "Derived"; }
    void ham_rieng() const { std::cout << "  ham_rieng() cua Derived\n"; }
};
class Khac : public Base {
public:
    const char* who() const override { return "Khac"; }
};

void demo_bon_loai_cast() {
    std::cout << "== Demo 4 loai cast ==\n";

    // static_cast: chuyen doi so hoc, compile-time, khong kiem tra runtime
    double d = 3.99;
    int i = static_cast<int>(d);
    std::cout << "  static_cast<int>(3.99) = " << i << "\n";

    // dynamic_cast: downcast AN TOAN, kiem tra runtime bang RTTI
    Derived dv;
    Khac kh;
    Base* mang[] = { &dv, &kh };
    for (Base* b : mang) {
        if (auto* p = dynamic_cast<Derived*>(b)) {       // tra nullptr neu sai kieu
            std::cout << "  dynamic_cast OK: " << p->who() << " -> ";
            p->ham_rieng();
        } else {
            std::cout << "  dynamic_cast that bai voi " << b->who() << " (tra nullptr)\n";
        }
    }

    // const_cast: go const de goi API cu thieu const (object goc KHONG const -> hop le)
    int x = 42;
    const int* cx = &x;
    *const_cast<int*>(cx) = 43;   // hop le vi x von khong phai const
    std::cout << "  const_cast ghi qua con tro const: x = " << x << "\n";

    // reinterpret_cast: dien giai lai bit - vi du con tro <-> so nguyen (dia chi)
    std::uintptr_t dia_chi = reinterpret_cast<std::uintptr_t>(&x);
    std::cout << "  reinterpret_cast dia chi cua x = 0x" << std::hex << dia_chi << std::dec << "\n";
}

// ---------- 2. Frame giao thức: POD + static_assert khóa layout ----------
struct SensorFrame {
    std::uint8_t  sync;      // offset 0
    std::uint8_t  id;        // offset 1
    std::uint16_t value;     // offset 2 (align 2 -> khong padding)
    std::uint32_t timestamp; // offset 4
};
// "Hop dong layout" - build fail ngay neu ai do lam lech frame:
static_assert(sizeof(SensorFrame) == 8, "Frame phai dung 8 byte");
static_assert(std::is_trivially_copyable<SensorFrame>::value, "Phai memcpy duoc");
static_assert(std::is_standard_layout<SensorFrame>::value, "Phai tuong thich layout C");
static_assert(offsetof(SensorFrame, value) == 2, "value phai o offset 2");
static_assert(offsetof(SensorFrame, timestamp) == 4, "timestamp phai o offset 4");

void demo_frame_pod() {
    std::cout << "== Demo dong goi frame (POD + memcpy) ==\n";
    SensorFrame f{0xAA, 0x01, 1234, 567890};

    std::uint8_t tx[sizeof(SensorFrame)];
    std::memcpy(tx, &f, sizeof f);            // serialize: hop le vi trivially-copyable
    std::cout << "  Byte gui di:";
    for (std::uint8_t b : tx) std::cout << " " << std::hex << static_cast<int>(b);
    std::cout << std::dec << "\n";

    SensorFrame nhan;
    std::memcpy(&nhan, tx, sizeof nhan);      // deserialize
    std::cout << "  Giai ma: id=" << static_cast<int>(nhan.id)
              << " value=" << nhan.value << " ts=" << nhan.timestamp << "\n";
}

// ---------- 3. Alignment & padding ----------
struct SapXepTe  { std::uint8_t a; std::uint32_t b; std::uint8_t c; };  // padding nhieu
struct SapXepTot { std::uint32_t b; std::uint8_t a; std::uint8_t c; };  // to -> nho

void demo_alignment_padding() {
    std::cout << "== Demo alignment & padding ==\n";
    std::cout << "  SapXepTe : sizeof=" << sizeof(SapXepTe)
              << " (a@" << offsetof(SapXepTe, a) << ", b@" << offsetof(SapXepTe, b)
              << ", c@" << offsetof(SapXepTe, c) << ") -> nhieu padding\n";
    std::cout << "  SapXepTot: sizeof=" << sizeof(SapXepTot)
              << " (b@" << offsetof(SapXepTot, b) << ", a@" << offsetof(SapXepTot, a)
              << ", c@" << offsetof(SapXepTot, c) << ") -> gon hon\n";
    std::cout << "  alignof(uint32_t)=" << alignof(std::uint32_t)
              << ", alignof(SapXepTe)=" << alignof(SapXepTe) << "\n";
}

// ---------- 4. Type punning ĐÚNG cách: memcpy (tránh strict aliasing UB) ----------
void demo_type_punning() {
    std::cout << "== Demo type punning dung cach ==\n";
    float f = 1.0f;
    std::uint32_t bits;
    // SAI (UB strict aliasing): bits = *reinterpret_cast<uint32_t*>(&f);
    std::memcpy(&bits, &f, sizeof bits);      // DUNG: compiler toi uu thanh 1 lenh move
    std::cout << "  Bieu dien bit cua 1.0f = 0x" << std::hex << bits << std::dec
              << " (dung memcpy, khong reinterpret_cast)\n";
}

int main() {
    demo_bon_loai_cast();
    demo_frame_pod();
    demo_alignment_padding();
    demo_type_punning();
    std::cout << "Bai 08 hoan thanh.\n";
    return 0;
}
