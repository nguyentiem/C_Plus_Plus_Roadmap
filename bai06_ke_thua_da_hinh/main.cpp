// Bài 06: Kế thừa & Đa hình
// Biên dịch: g++ -std=c++17 -Wall -Wextra main.cpp -o bai06_ke_thua_da_hinh.exe
#include <iostream>
#include <memory>
#include <vector>

// ---------- 1. Interface + đa hình cơ bản ----------
class Sensor {                        // abstract class (interface)
public:
    virtual ~Sensor() = default;      // virtual destructor: BẮT BUỘC cho base đa hình
    virtual const char* name() const = 0;   // pure virtual
    virtual int read() = 0;
};

class TempSensor final : public Sensor {    // final: không cho kế thừa tiếp
public:
    const char* name() const override { return "TempSensor"; }
    int read() override { return 25; }
};

class AdcSensor : public Sensor {
    int raw_ = 1023;
public:
    ~AdcSensor() override { std::cout << "  ~AdcSensor() duoc goi (nho virtual dtor)\n"; }
    const char* name() const override { return "AdcSensor"; }
    int read() override { return raw_; }
};

void demo_dynamic_dispatch() {
    std::cout << "== Demo dynamic dispatch (vtable/vptr) ==\n";
    std::vector<std::unique_ptr<Sensor>> sensors;
    sensors.push_back(std::make_unique<TempSensor>());
    sensors.push_back(std::make_unique<AdcSensor>());
    for (auto& s : sensors) {
        // Cung mot loi goi s->read() nhung hanh vi khac nhau: tra cuu qua vtable
        std::cout << "  " << s->name() << " doc duoc: " << s->read() << "\n";
    }
    // unique_ptr<Sensor> huy -> delete qua Sensor*: virtual dtor dam bao dtor con chay
}

// ---------- 2. Object slicing ----------
class Base {
public:
    virtual ~Base() = default;
    virtual const char* who() const { return "Base"; }
};
class Derived : public Base {
public:
    const char* who() const override { return "Derived"; }
};

void nhan_theo_gia_tri(Base b)        { std::cout << "  theo gia tri : " << b.who() << " (bi slicing!)\n"; }
void nhan_theo_tham_chieu(const Base& b) { std::cout << "  theo tham chieu: " << b.who() << " (giu da hinh)\n"; }

void demo_object_slicing() {
    std::cout << "== Demo object slicing ==\n";
    Derived d;
    nhan_theo_gia_tri(d);      // phan Derived bi cat, vptr thanh cua Base
    nhan_theo_tham_chieu(d);   // tham chieu giu nguyen kieu dong
}

// ---------- 3. Chi phí bộ nhớ của vptr ----------
struct KhongVirtual { int x; };
struct CoVirtual    { int x; virtual ~CoVirtual() = default; };

void demo_chi_phi_vptr() {
    std::cout << "== Demo chi phi vptr ==\n";
    std::cout << "  sizeof(KhongVirtual) = " << sizeof(KhongVirtual)
              << ", sizeof(CoVirtual) = " << sizeof(CoVirtual)
              << " (chenh lech = vptr + padding)\n";
}

// ---------- 4. Diamond problem & virtual inheritance ----------
struct A            { int a = 1; };
struct B_thuong : A { };
struct C_thuong : A { };
struct D_thuong : B_thuong, C_thuong { };   // chua HAI ban A

struct Bv : virtual A { };
struct Cv : virtual A { };
struct Dv : Bv, Cv { };                     // chi MOT ban A duy nhat

void demo_diamond() {
    std::cout << "== Demo diamond problem ==\n";
    D_thuong d1;
    // d1.a; // LOI BIEN DICH: mo ho (ambiguous) - phai chi ro:
    d1.B_thuong::a = 10;
    d1.C_thuong::a = 20;
    std::cout << "  Khong virtual: hai ban A rieng biet: "
              << d1.B_thuong::a << " va " << d1.C_thuong::a
              << ", sizeof(D_thuong) = " << sizeof(D_thuong) << "\n";
    Dv d2;
    d2.a = 99;                              // khong mo ho: chi mot ban A
    std::cout << "  Virtual inheritance: mot ban A duy nhat: " << d2.a
              << ", sizeof(Dv) = " << sizeof(Dv) << " (lon hon vi vbase pointer)\n";
}

// ---------- 5. Composition vs Inheritance ----------
class Motor {
public:
    void quay() const { std::cout << "  Motor dang quay\n"; }
};
class Robot {                 // Robot "co" Motor (has-a) -> composition
    Motor motor_;
public:
    void chay() const { motor_.quay(); }
};

void demo_composition() {
    std::cout << "== Demo composition (has-a) ==\n";
    Robot r;
    r.chay();   // tai su dung Motor ma khong ke thua
}

int main() {
    demo_dynamic_dispatch();
    demo_object_slicing();
    demo_chi_phi_vptr();
    demo_diamond();
    demo_composition();
    std::cout << "Bai 06 hoan thanh.\n";
    return 0;
}
