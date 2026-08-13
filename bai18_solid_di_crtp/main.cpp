// Bai 18: SOLID + Dependency Injection (IUart/MockUart) + CRTP vs virtual
// Bien dich: g++ -std=c++17 -Wall -Wextra -O2 main.cpp -o bai18_solid_di_crtp.exe
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <vector>

// ================================================================
// Phan 1: Dependency Injection cho testability
// Logic dong goi frame duoc test KHONG can phan cung nho MockUart
// ================================================================

// Interface nho (ISP): chi nhung gi client can
class IUart {
public:
    virtual ~IUart() = default;
    virtual void guiByte(uint8_t b) = 0;
};

// "Driver that" — production se goi HAL (nrf_uarte_tx...). O day gia lap.
class UartThat : public IUart {
public:
    void guiByte(uint8_t b) override {
        // Tren board: ghi vao thanh ghi TXD. O demo: in ra man hinh.
        std::cout << std::hex << std::setw(2) << std::setfill('0')
                  << static_cast<int>(b) << " " << std::dec;
    }
};

// MockUart: chi GHI LAI cac byte da gui — khong logic, de kiem tra trong test
class MockUart : public IUart {
public:
    void guiByte(uint8_t b) override { daGui.push_back(b); }
    std::vector<uint8_t> daGui;
};

// Logic nghiep vu: dong goi frame [0xAA][len][payload...][checksum]
// Phu thuoc ABSTRACTION (IUart&) — Dependency Inversion Principle
class BoDongGoiFrame {
public:
    explicit BoDongGoiFrame(IUart& uart) : uart_(uart) {} // constructor injection

    void guiFrame(const std::vector<uint8_t>& payload) {
        uart_.guiByte(0xAA); // start byte
        uart_.guiByte(static_cast<uint8_t>(payload.size()));
        uint8_t checksum = 0;
        for (uint8_t b : payload) {
            uart_.guiByte(b);
            checksum = static_cast<uint8_t>(checksum ^ b);
        }
        uart_.guiByte(checksum);
    }

private:
    IUart& uart_; // khong so huu, khong tu tao — duoc TIEM tu ngoai
};

// "Unit test" thu cong: kiem tra logic frame bang mock (chay tren host!)
static int soTestFail = 0;
static void kiemTra(bool dieuKien, const char* moTa) {
    std::cout << "  [" << (dieuKien ? "PASS" : "FAIL") << "] " << moTa << "\n";
    if (!dieuKien) ++soTestFail;
}

void demo_di_mock() {
    std::cout << "=== Demo 1: DI + MockUart (unit test khong can phan cung) ===\n";

    MockUart mock;
    BoDongGoiFrame boGoi(mock); // tiem mock thay vi driver that
    boGoi.guiFrame({0x01, 0x02, 0x03});

    // Frame ky vong: AA 03 01 02 03 (01^02^03 = 00)
    kiemTra(mock.daGui.size() == 6, "frame co 6 byte");
    kiemTra(mock.daGui[0] == 0xAA, "byte dau la 0xAA");
    kiemTra(mock.daGui[1] == 0x03, "do dai payload = 3");
    kiemTra(mock.daGui[5] == 0x00, "checksum XOR = 0x00");

    // Cung logic do, production chi can tiem driver that:
    UartThat that;
    BoDongGoiFrame boGoiThat(that);
    std::cout << "  Production gui: ";
    boGoiThat.guiFrame({0x10, 0x20});
    std::cout << "\n\n";
}

// ================================================================
// Phan 2: CRTP (da hinh tinh) vs virtual (da hinh dong)
// ================================================================

// --- Cach 1: virtual — linh hoat runtime, ton vtable pointer ---
class IGpioAo {
public:
    virtual ~IGpioAo() = default;
    virtual void ghiMuc(bool muc) = 0;
    void bat() { ghiMuc(true); }   // indirect call qua vtable
    void tat() { ghiMuc(false); }
};

class GpioLedAo : public IGpioAo {
public:
    void ghiMuc(bool muc) override { trangThai_ = muc; ++soLanGhi_; }
    bool trangThai() const { return trangThai_; }
    int soLanGhi() const { return soLanGhi_; }

private:
    bool trangThai_ = false;
    int soLanGhi_ = 0;
};

// --- Cach 2: CRTP — resolve luc compile, inline duoc, khong vtable ---
template <typename D>
class GpioBase {
public:
    // static_cast xuong lop con: hop le vi D ke thua GpioBase<D>
    void bat() { static_cast<D*>(this)->ghiMuc(true); }
    void tat() { static_cast<D*>(this)->ghiMuc(false); }
};

class GpioLedCrtp : public GpioBase<GpioLedCrtp> {
public:
    void ghiMuc(bool muc) { trangThai_ = muc; ++soLanGhi_; } // KHONG virtual
    bool trangThai() const { return trangThai_; }
    int soLanGhi() const { return soLanGhi_; }

private:
    bool trangThai_ = false;
    int soLanGhi_ = 0;
};

void demo_crtp_vs_virtual() {
    std::cout << "=== Demo 2: CRTP vs virtual ===\n";

    GpioLedAo ledVirtual;
    GpioLedCrtp ledCrtp;

    // Cung API, cung hanh vi
    ledVirtual.bat();
    ledVirtual.tat();
    ledCrtp.bat();
    ledCrtp.tat();

    kiemTra(ledVirtual.soLanGhi() == 2 && !ledVirtual.trangThai(), "virtual: 2 lan ghi, dang tat");
    kiemTra(ledCrtp.soLanGhi() == 2 && !ledCrtp.trangThai(), "CRTP  : 2 lan ghi, dang tat");

    // Chi phi bo nho: lop virtual mang them vtable pointer
    std::cout << "  sizeof(GpioLedAo)   = " << sizeof(GpioLedAo)
              << " byte (co vtable pointer)\n";
    std::cout << "  sizeof(GpioLedCrtp) = " << sizeof(GpioLedCrtp)
              << " byte (khong vtable)\n";
    std::cout << "  -> CRTP: goi ham resolve compile-time, inline duoc; "
                 "virtual: indirect call, nhung nhet chung vao mang IGpioAo* duoc.\n\n";
}

// Ham generic dung CRTP: nhan bat ky driver nao ke thua GpioBase<D>
template <typename D>
void nhayLed(GpioBase<D>& led, int lan) {
    for (int i = 0; i < lan; ++i) {
        led.bat();
        led.tat();
    }
}

void demo_crtp_generic() {
    std::cout << "=== Demo 3: ham generic voi CRTP ===\n";
    GpioLedCrtp led;
    nhayLed(led, 5); // template deduce GpioBase<GpioLedCrtp>
    kiemTra(led.soLanGhi() == 10, "nhay 5 lan = 10 lan ghi muc");
    std::cout << "\n";
}

int main() {
    std::cout << "Bai 18: SOLID, DI, CRTP\n\n";
    demo_di_mock();
    demo_crtp_vs_virtual();
    demo_crtp_generic();
    if (soTestFail == 0)
        std::cout << "Hoan tat — tat ca kiem tra PASS.\n";
    else
        std::cout << "Co " << soTestFail << " kiem tra FAIL!\n";
    return soTestFail == 0 ? 0 : 1;
}
