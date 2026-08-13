// Bai 17: Design Patterns — State Machine (2 bien the) + Observer + Strategy/Factory
// Bien dich: g++ -std=c++17 -Wall -Wextra -O2 main.cpp -o bai17_design_patterns.exe
#include <array>
#include <iostream>
#include <memory>

// ================================================================
// Demo 1: State machine kieu bang chuyen trang thai (table-driven)
// Idle -> Connecting -> Connected -> Error, dung enum class + bang
// ================================================================
enum class TrangThai { Idle, Connecting, Connected, Error };
enum class SuKien { KetNoi, ThanhCong, ThatBai, MatKetNoi, Reset };

static const char* ten(TrangThai t) {
    switch (t) {
        case TrangThai::Idle:       return "Idle";
        case TrangThai::Connecting: return "Connecting";
        case TrangThai::Connected:  return "Connected";
        case TrangThai::Error:      return "Error";
    }
    return "?";
}
static const char* ten(SuKien s) {
    switch (s) {
        case SuKien::KetNoi:     return "KetNoi";
        case SuKien::ThanhCong:  return "ThanhCong";
        case SuKien::ThatBai:    return "ThatBai";
        case SuKien::MatKetNoi:  return "MatKetNoi";
        case SuKien::Reset:      return "Reset";
    }
    return "?";
}

struct ChuyenTT {
    TrangThai tu;
    SuKien sk;
    TrangThai den;
};

// Bang chuyen trang thai: constexpr -> nam trong flash tren MCU
constexpr std::array<ChuyenTT, 5> BANG_CHUYEN = {{
    {TrangThai::Idle,       SuKien::KetNoi,     TrangThai::Connecting},
    {TrangThai::Connecting, SuKien::ThanhCong,  TrangThai::Connected},
    {TrangThai::Connecting, SuKien::ThatBai,    TrangThai::Error},
    {TrangThai::Connected,  SuKien::MatKetNoi,  TrangThai::Error},
    {TrangThai::Error,      SuKien::Reset,      TrangThai::Idle},
}};

class FsmBang {
public:
    void xuLy(SuKien sk) {
        for (const auto& c : BANG_CHUYEN) {
            if (c.tu == tt_ && c.sk == sk) {
                std::cout << "  [bang] " << ten(tt_) << " --" << ten(sk)
                          << "--> " << ten(c.den) << "\n";
                tt_ = c.den;
                return;
            }
        }
        // Su kien khong hop le trong trang thai hien tai: quyet dinh ro rang
        std::cout << "  [bang] " << ten(tt_) << ": bo qua su kien " << ten(sk) << "\n";
    }
    TrangThai hienTai() const { return tt_; }

private:
    TrangThai tt_ = TrangThai::Idle;
};

void demo_fsm_bang() {
    std::cout << "=== Demo 1: FSM bang chuyen trang thai ===\n";
    FsmBang fsm;
    fsm.xuLy(SuKien::ThanhCong);  // khong hop le tu Idle -> bo qua
    fsm.xuLy(SuKien::KetNoi);
    fsm.xuLy(SuKien::ThanhCong);
    fsm.xuLy(SuKien::MatKetNoi);
    fsm.xuLy(SuKien::Reset);
    std::cout << "  Trang thai cuoi: " << ten(fsm.hienTai()) << "\n\n";
}

// ================================================================
// Demo 2: State pattern OOP — moi trang thai la mot lop singleton tinh
// (khong heap: phu hop MCU), co entry action
// ================================================================
class MayTrangThai; // forward

class ITrangThai {
public:
    virtual ~ITrangThai() = default;
    virtual const char* ten() const = 0;
    virtual void vao() const {} // entry action mac dinh: khong lam gi
    virtual const ITrangThai* xuLy(SuKien sk) const = 0; // tra ve state moi (hoac chinh no)
};

// Khai bao truoc cac instance tinh
class StIdle;
class StConnecting;
class StConnected;
class StError;
const ITrangThai& idle();
const ITrangThai& connecting();
const ITrangThai& connected();
const ITrangThai& errorSt();

class StIdle : public ITrangThai {
public:
    const char* ten() const override { return "Idle"; }
    const ITrangThai* xuLy(SuKien sk) const override {
        return (sk == SuKien::KetNoi) ? &connecting() : this;
    }
};
class StConnecting : public ITrangThai {
public:
    const char* ten() const override { return "Connecting"; }
    void vao() const override { std::cout << "    (entry) bat radio, gui yeu cau ket noi\n"; }
    const ITrangThai* xuLy(SuKien sk) const override {
        if (sk == SuKien::ThanhCong) return &connected();
        if (sk == SuKien::ThatBai) return &errorSt();
        return this;
    }
};
class StConnected : public ITrangThai {
public:
    const char* ten() const override { return "Connected"; }
    void vao() const override { std::cout << "    (entry) bat LED, bat dau truyen du lieu\n"; }
    const ITrangThai* xuLy(SuKien sk) const override {
        return (sk == SuKien::MatKetNoi) ? &errorSt() : this;
    }
};
class StError : public ITrangThai {
public:
    const char* ten() const override { return "Error"; }
    void vao() const override { std::cout << "    (entry) tat radio, ghi log loi\n"; }
    const ITrangThai* xuLy(SuKien sk) const override {
        return (sk == SuKien::Reset) ? &idle() : this;
    }
};

// Singleton tinh: khong cap phat dong, ton tai suot chuong trinh
const ITrangThai& idle()       { static const StIdle s;       return s; }
const ITrangThai& connecting() { static const StConnecting s; return s; }
const ITrangThai& connected()  { static const StConnected s;  return s; }
const ITrangThai& errorSt()    { static const StError s;      return s; }

class MayTrangThai {
public:
    void xuLy(SuKien sk) {
        const ITrangThai* moi = tt_->xuLy(sk);
        if (moi != tt_) {
            std::cout << "  [oop] " << tt_->ten() << " --" << ::ten(sk)
                      << "--> " << moi->ten() << "\n";
            tt_ = moi;
            tt_->vao(); // entry action tu nhien voi State pattern
        }
    }

private:
    const ITrangThai* tt_ = &idle();
};

void demo_fsm_oop() {
    std::cout << "=== Demo 2: FSM kieu State pattern (OOP, state tinh) ===\n";
    MayTrangThai may;
    may.xuLy(SuKien::KetNoi);
    may.xuLy(SuKien::ThanhCong);
    may.xuLy(SuKien::MatKetNoi);
    may.xuLy(SuKien::Reset);
    std::cout << "\n";
}

// ================================================================
// Demo 3: Observer — sensor notify cac listener qua interface
// Dung mang con tro co dinh (khong vector) — phong cach embedded
// ================================================================
class IListener {
public:
    virtual ~IListener() = default;
    virtual void nhanNhietDo(float doC) = 0;
};

class CamBienNhiet {
public:
    bool dangKy(IListener* l) {
        for (auto& slot : listeners_) {
            if (slot == nullptr) { slot = l; return true; }
        }
        return false; // het cho — embedded: gioi han tinh, khong heap
    }
    void doVaThongBao(float doC) {
        std::cout << "  Sensor doc duoc " << doC << " do C -> notify\n";
        for (auto* l : listeners_)
            if (l != nullptr) l->nhanNhietDo(doC);
    }

private:
    std::array<IListener*, 4> listeners_{}; // toi da 4 listener, khoi tao nullptr
};

class Logger : public IListener {
public:
    void nhanNhietDo(float doC) override {
        std::cout << "    [Logger] ghi log: " << doC << "\n";
    }
};
class BaoDong : public IListener {
public:
    void nhanNhietDo(float doC) override {
        if (doC > 50.0f) std::cout << "    [BaoDong] QUA NHIET!\n";
        else             std::cout << "    [BaoDong] binh thuong\n";
    }
};

void demo_observer() {
    std::cout << "=== Demo 3: Observer (sensor -> listeners) ===\n";
    CamBienNhiet sensor;
    Logger logger;
    BaoDong baoDong;
    sensor.dangKy(&logger);
    sensor.dangKy(&baoDong);
    sensor.doVaThongBao(25.5f);
    sensor.doVaThongBao(85.0f);
    std::cout << "\n";
}

// ================================================================
// Demo 4: Strategy + Factory — chon thuat toan loc luc runtime
// ================================================================
class ILoc {
public:
    virtual ~ILoc() = default;
    virtual float loc(float x) = 0;
    virtual const char* ten() const = 0;
};

class LocTrungBinh : public ILoc { // trung binh truot don gian (2 mau)
public:
    float loc(float x) override {
        const float kq = (x + truoc_) / 2.0f;
        truoc_ = x;
        return kq;
    }
    const char* ten() const override { return "TrungBinhTruot"; }

private:
    float truoc_ = 0.0f;
};

class LocEma : public ILoc { // exponential moving average
public:
    float loc(float x) override {
        gia_tri_ = 0.7f * gia_tri_ + 0.3f * x;
        return gia_tri_;
    }
    const char* ten() const override { return "EMA"; }

private:
    float gia_tri_ = 0.0f;
};

// Factory: caller chi biet interface ILoc
std::unique_ptr<ILoc> taoBoLoc(int loai) {
    if (loai == 0) return std::make_unique<LocTrungBinh>();
    return std::make_unique<LocEma>();
}

void demo_strategy_factory() {
    std::cout << "=== Demo 4: Strategy + Factory ===\n";
    const float mau[] = {10.0f, 12.0f, 11.0f, 30.0f}; // mau 30 la nhieu (spike)
    for (int loai = 0; loai < 2; ++loai) {
        auto boLoc = taoBoLoc(loai);
        std::cout << "  " << boLoc->ten() << ": ";
        for (float m : mau) std::cout << boLoc->loc(m) << " ";
        std::cout << "\n";
    }
    std::cout << "\n";
}

int main() {
    std::cout << "Bai 17: Design Patterns cho firmware\n\n";
    demo_fsm_bang();
    demo_fsm_oop();
    demo_observer();
    demo_strategy_factory();
    std::cout << "Hoan tat.\n";
    return 0;
}
