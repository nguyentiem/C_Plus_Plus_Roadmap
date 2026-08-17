// Bài 03: OOP trong C++ — class, lifetime, RAII và đa hình
// Biên dịch: g++ -std=c++17 -Wall -Wextra main.cpp -o bai03_class_raii.exe

#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <exception>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

class DongCoDC {
    int toc_do_ = 0;
    inline static int so_luong_ = 0;

public:
    DongCoDC() { ++so_luong_; }
    explicit DongCoDC(int toc_do) : toc_do_(toc_do) { ++so_luong_; }
    ~DongCoDC() { --so_luong_; }

    DongCoDC& datTocDo(int toc_do) {
        this->toc_do_ = toc_do;
        return *this;
    }

    int tocDo() const { return toc_do_; }
    static int soLuong() { return so_luong_; }

    friend std::ostream& operator<<(std::ostream& os, const DongCoDC& dong_co) {
        return os << "DongCoDC(" << dong_co.toc_do_ << ")";
    }
};

class ThanhPhanLog {
    const char* ten_;

public:
    explicit ThanhPhanLog(const char* ten) : ten_(ten) {
        std::cout << "  khoi tao member " << ten_ << "\n";
    }
    ~ThanhPhanLog() { std::cout << "  huy member " << ten_ << "\n"; }
};

class CauHinh {
    ThanhPhanLog dau_tien_{"dau_tien"};
    const int ma_;
    int& tham_chieu_;
    ThanhPhanLog thu_hai_{"thu_hai"};

public:
    CauHinh(int ma, int& tham_chieu)
        : ma_(ma), tham_chieu_(tham_chieu) {
        std::cout << "  than ctor CauHinh, ma=" << ma_ << "\n";
    }
};

class TheoDoiDoiSong {
    std::string ten_;

public:
    explicit TheoDoiDoiSong(std::string ten) : ten_(std::move(ten)) {
        std::cout << "  + " << ten_ << "\n";
    }
    ~TheoDoiDoiSong() { std::cout << "  - " << ten_ << "\n"; }
};

class ShallowView {
    int* du_lieu_;

public:
    explicit ShallowView(int* du_lieu) : du_lieu_(du_lieu) {}
    // Copy compiler sinh chỉ copy địa chỉ. Class này KHÔNG sở hữu du_lieu_.
    const int* data() const { return du_lieu_; }
};

class DeepBuffer {
    std::size_t kich_thuoc_ = 0;
    std::unique_ptr<int[]> du_lieu_;

public:
    explicit DeepBuffer(std::size_t kich_thuoc)
        : kich_thuoc_(kich_thuoc), du_lieu_(std::make_unique<int[]>(kich_thuoc)) {}

    DeepBuffer(const DeepBuffer& khac)
        : kich_thuoc_(khac.kich_thuoc_), du_lieu_(std::make_unique<int[]>(khac.kich_thuoc_)) {
        for (std::size_t i = 0; i < kich_thuoc_; ++i) {
            du_lieu_[i] = khac.du_lieu_[i];
        }
    }

    int* data() { return du_lieu_.get(); }
    const int* data() const { return du_lieu_.get(); }
};

class Buffer {
    std::size_t kich_thuoc_ = 0;
    int* du_lieu_ = nullptr;
    inline static int so_copy_ = 0;
    inline static int so_move_ = 0;

public:
    explicit Buffer(std::size_t kich_thuoc = 0)
        : kich_thuoc_(kich_thuoc), du_lieu_(kich_thuoc == 0 ? nullptr : new int[kich_thuoc]{}) {}

    ~Buffer() { delete[] du_lieu_; }

    Buffer(const Buffer& khac)
        : kich_thuoc_(khac.kich_thuoc_),
          du_lieu_(khac.kich_thuoc_ == 0 ? nullptr : new int[khac.kich_thuoc_]) {
        for (std::size_t i = 0; i < kich_thuoc_; ++i) {
            du_lieu_[i] = khac.du_lieu_[i];
        }
        ++so_copy_;
    }

    Buffer& operator=(const Buffer& khac) {
        if (this != &khac) {
            Buffer tam(khac);
            swap(tam);
        }
        return *this;
    }

    Buffer(Buffer&& khac) noexcept
        : kich_thuoc_(khac.kich_thuoc_), du_lieu_(khac.du_lieu_) {
        khac.kich_thuoc_ = 0;
        khac.du_lieu_ = nullptr;
        ++so_move_;
    }

    Buffer& operator=(Buffer&& khac) noexcept {
        if (this != &khac) {
            delete[] du_lieu_;
            kich_thuoc_ = khac.kich_thuoc_;
            du_lieu_ = khac.du_lieu_;
            khac.kich_thuoc_ = 0;
            khac.du_lieu_ = nullptr;
            ++so_move_;
        }
        return *this;
    }

    void swap(Buffer& khac) noexcept {
        using std::swap;
        swap(kich_thuoc_, khac.kich_thuoc_);
        swap(du_lieu_, khac.du_lieu_);
    }

    static void resetDem() { so_copy_ = 0; so_move_ = 0; }
    static int soCopy() { return so_copy_; }
    static int soMove() { return so_move_; }
};

class BufferRuleZero {
    std::vector<int> du_lieu_;

public:
    explicit BufferRuleZero(std::size_t kich_thuoc) : du_lieu_(kich_thuoc) {}
    std::size_t size() const { return du_lieu_.size(); }
};

class FileRAII {
    FILE* file_ = nullptr;

public:
    FileRAII(const char* path, const char* mode) : file_(std::fopen(path, mode)) {
        std::cout << "  mo file: " << (file_ != nullptr ? "OK" : "THAT BAI") << "\n";
    }
    ~FileRAII() {
        if (file_ != nullptr) {
            std::fclose(file_);
            std::cout << "  dtor: fclose\n";
        }
    }
    FileRAII(const FileRAII&) = delete;
    FileRAII& operator=(const FileRAII&) = delete;
    bool ok() const { return file_ != nullptr; }
    FILE* get() const { return file_; }
};

class IrqLock {
public:
    IrqLock() { std::cout << "  [IrqLock] disable IRQ (mo phong)\n"; }
    ~IrqLock() { std::cout << "  [IrqLock] enable IRQ (mo phong)\n"; }
    IrqLock(const IrqLock&) = delete;
    IrqLock& operator=(const IrqLock&) = delete;
};

class IThietBi {
public:
    virtual ~IThietBi() = default;
    virtual void khoiDong() const = 0;
};

class CamBien final : public IThietBi {
public:
    void khoiDong() const override { std::cout << "  CamBien doc du lieu\n"; }
};

class DongCo final : public IThietBi {
public:
    void khoiDong() const override { std::cout << "  DongCo quay\n"; }
};

class Hinh {
public:
    virtual ~Hinh() = default;
    virtual const char* ten() const { return "Hinh"; }
};

class HinhTron final : public Hinh {
public:
    const char* ten() const override { return "HinhTron"; }
};

void inByValue(Hinh hinh) { std::cout << "  by value: " << hinh.ten() << " (bi slicing)\n"; }
void inByReference(const Hinh& hinh) { std::cout << "  by reference: " << hinh.ten() << "\n"; }

struct AThuong { int a = 1; };
struct BThuong : AThuong {};
struct CThuong : AThuong {};
struct DThuong : BThuong, CThuong {};

struct AAo { int a = 1; };
struct BAo : virtual AAo {};
struct CAo : virtual AAo {};
struct DAo : BAo, CAo {};

template <typename T, std::size_t N>
class Stack {
    std::array<T, N> du_lieu_{};
    std::size_t so_phan_tu_ = 0;
    inline static int so_stack_ = 0;

public:
    Stack() { ++so_stack_; }
    ~Stack() { --so_stack_; }

    bool push(const T& gia_tri) {
        if (so_phan_tu_ == N) {
            return false;
        }
        du_lieu_[so_phan_tu_++] = gia_tri;
        return true;
    }
    const T& top() const { return du_lieu_[so_phan_tu_ - 1]; }
    static int soStack() { return so_stack_; }
};

class GuardException {
public:
    GuardException() { std::cout << "  Guard nhan tai nguyen\n"; }
    ~GuardException() { std::cout << "  Guard tra tai nguyen khi unwind\n"; }
};

class HaiTaiNguyen {
    std::unique_ptr<int> mot_;
    std::unique_ptr<int> hai_;

public:
    HaiTaiNguyen() : mot_(std::make_unique<int>(1)), hai_(std::make_unique<int>(2)) {}
    int tong() const { return *mot_ + *hai_; }
};

struct PlainLayout {
    char c;
    int i;
    double d;
};

struct EmptyLayout {};

class CoVirtual {
    int gia_tri_ = 0;

public:
    virtual ~CoVirtual() = default;
    virtual int giaTri() const { return gia_tri_; }
};

class CoVirtualKhac final : public CoVirtual {
public:
    int giaTri() const override { return 1; }
};

void demo_class_va_access() {
    std::cout << "\n--- 1. Class, access, this, const, static ---\n";
    DongCoDC dong_co(10);
    dong_co.datTocDo(42);
    std::cout << "  " << dong_co << ", toc do const=" << dong_co.tocDo() << "\n";
    std::cout << "  so doi tuong: " << DongCoDC::soLuong() << "\n";
    // dong_co.toc_do_ = 100; // Loi: member private khong the truy cap tu ben ngoai.
}

void demo_constructor_va_init_list() {
    std::cout << "\n--- 2. Constructor, init-list va thu tu member ---\n";
    int bien_ngoai = 7;
    CauHinh cau_hinh(99, bien_ngoai);
    (void)cau_hinh;
    std::cout << "  Member khoi tao theo thu tu khai bao, khong theo init-list.\n";
}

void demo_lifetime() {
    std::cout << "\n--- 3. Object lifetime ---\n";
    TheoDoiDoiSong ngoai("ngoai scope");
    {
        TheoDoiDoiSong trong("trong scope");
        (void)trong;
    }
    auto dong = std::make_unique<TheoDoiDoiSong>("dong unique_ptr");
    dong.reset();
    static TheoDoiDoiSong static_local("static local, huy sau main");
    (void)ngoai;
    (void)static_local;
}

void demo_shallow_va_deep_copy() {
    std::cout << "\n--- 4. Shallow copy va deep copy ---\n";
    int mang[2] = {1, 2};
    ShallowView a(mang);
    ShallowView b = a;
    std::cout << "  shallow: " << static_cast<const void*>(a.data()) << " va "
              << static_cast<const void*>(b.data()) << " (cung dia chi)\n";

    DeepBuffer x(2);
    x.data()[0] = 42;
    DeepBuffer y = x;
    std::cout << "  deep:    " << static_cast<const void*>(x.data()) << " va "
              << static_cast<const void*>(y.data()) << " (dia chi khac), y[0]=" << y.data()[0] << "\n";
}

void demo_rule_of_five() {
    std::cout << "\n--- 5. Rule of Five va move noexcept ---\n";
    Buffer::resetDem();
    Buffer a(8);
    Buffer b = a;
    Buffer c = std::move(b);
    a = c;
    c = Buffer(4);
    std::cout << "  copy=" << Buffer::soCopy() << ", move=" << Buffer::soMove() << "\n";
}

void demo_rule_of_zero() {
    std::cout << "\n--- 6. Rule of Zero ---\n";
    BufferRuleZero a(4);
    BufferRuleZero b = a;
    std::cout << "  vector quan ly tai nguyen: a=" << a.size() << ", b=" << b.size()
              << "; class khong can tu viet special members.\n";
}

void demo_raii() {
    std::cout << "\n--- 7. RAII: FILE va critical section ---\n";
    {
        FileRAII file("bai03_demo.txt", "w");
        if (file.ok()) {
            std::fputs("RAII cleanup\n", file.get());
        }
    }
    std::remove("bai03_demo.txt");
    {
        IrqLock lock;
        std::cout << "  return som/exception cung se goi dtor cua lock.\n";
    }
}

void demo_da_hinh() {
    std::cout << "\n--- 8. Abstract class, virtual va composition ---\n";
    std::vector<std::unique_ptr<IThietBi>> thiet_bi;
    thiet_bi.push_back(std::make_unique<CamBien>());
    thiet_bi.push_back(std::make_unique<DongCo>());
    for (const auto& item : thiet_bi) {
        item->khoiDong();
    }
}

void demo_slicing_va_da_ke_thua() {
    std::cout << "\n--- 9. Slicing va multiple inheritance ---\n";
    HinhTron tron;
    inByValue(tron);
    inByReference(tron);
    std::cout << "  sizeof DThuong=" << sizeof(DThuong) << ", sizeof DAo=" << sizeof(DAo) << "\n";
    std::cout << "  DThuong co 2 AThuong; DAo co 1 AAo chia se qua virtual inheritance.\n";
}

void demo_class_template() {
    std::cout << "\n--- 10. Class template ---\n";
    Stack<int, 4> so_nguyen;
    Stack<std::string, 2> chuoi;
    so_nguyen.push(7);
    chuoi.push("C++");
    std::cout << "  top int=" << so_nguyen.top() << ", top string=" << chuoi.top() << "\n";
    std::cout << "  static rieng: Stack<int,4>=" << Stack<int, 4>::soStack()
              << ", Stack<string,2>=" << Stack<std::string, 2>::soStack() << "\n";
}

void hamNemException() {
    GuardException guard;
    throw std::runtime_error("loi mo phong");
}

void demo_exception_safety() {
    std::cout << "\n--- 11. Exception safety va smart pointer ---\n";
    try {
        hamNemException();
    } catch (const std::exception& loi) {
        std::cout << "  catch: " << loi.what() << "\n";
    }
    HaiTaiNguyen tai_nguyen;
    std::cout << "  unique_ptr member quan ly an toan, tong=" << tai_nguyen.tong() << "\n";
}

void demo_memory_layout() {
    std::cout << "\n--- 12. Memory layout, padding, vptr/vtable ---\n";
    PlainLayout plain{};
    const auto* base = reinterpret_cast<const unsigned char*>(&plain);
    const auto* member_i = reinterpret_cast<const unsigned char*>(&plain.i);
    const auto* member_d = reinterpret_cast<const unsigned char*>(&plain.d);
    std::cout << "  sizeof PlainLayout=" << sizeof(PlainLayout)
              << ", offset i=" << (member_i - base)
              << ", offset d=" << (member_d - base) << "\n";
    std::cout << "  sizeof empty struct=" << sizeof(EmptyLayout)
              << ", sizeof CoVirtual=" << sizeof(CoVirtual) << "\n";

    CoVirtual mot;
    CoVirtual hai;
    CoVirtualKhac khac;
    std::uintptr_t word_mot = 0;
    std::uintptr_t word_hai = 0;
    std::uintptr_t word_khac = 0;
    std::memcpy(&word_mot, &mot, sizeof(word_mot));
    std::memcpy(&word_hai, &hai, sizeof(word_hai));
    std::memcpy(&word_khac, &khac, sizeof(word_khac));
    std::cout << "  word dau 2 object cung class: " << std::hex << word_mot << " / " << word_hai
              << ", class khac: " << word_khac << std::dec << "\n";
    std::cout << "  Day la quan sat ABI pho bien; chuan C++ khong bat buoc vi tri vptr/vtable.\n";
}

int main() {
    demo_class_va_access();
    demo_constructor_va_init_list();
    demo_lifetime();
    demo_shallow_va_deep_copy();
    demo_rule_of_five();
    demo_rule_of_zero();
    demo_raii();
    demo_da_hinh();
    demo_slicing_va_da_ke_thua();
    demo_class_template();
    demo_exception_safety();
    demo_memory_layout();
    std::cout << "\nKet thuc main.\n";
    return 0;
}
