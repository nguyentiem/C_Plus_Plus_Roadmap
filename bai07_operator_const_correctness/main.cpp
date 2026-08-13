// Bài 07: Operator Overloading & Const Correctness
// Biên dịch: g++ -std=c++17 -Wall -Wextra main.cpp -o bai07_operator_const_correctness.exe
#include <iostream>
#include <cstddef>

// ---------- 1. Lớp Vec2: nạp chồng +, +=, ==, << ----------
class Vec2 {
    double x_, y_;
public:
    Vec2(double x, double y) : x_(x), y_(y) {}

    // += la member: sua *this, tra ve tham chieu de chaining
    Vec2& operator+=(const Vec2& r) { x_ += r.x_; y_ += r.y_; return *this; }

    // + viet bang +=: lhs nhan theo GIA TRI (copy), khong sua toan hang goc
    friend Vec2 operator+(Vec2 lhs, const Vec2& rhs) { lhs += rhs; return lhs; }

    friend bool operator==(const Vec2& a, const Vec2& b) {
        return a.x_ == b.x_ && a.y_ == b.y_;
    }
    friend bool operator!=(const Vec2& a, const Vec2& b) { return !(a == b); }

    // << la non-member vi ve trai la ostream
    friend std::ostream& operator<<(std::ostream& os, const Vec2& v) {
        return os << "(" << v.x_ << ", " << v.y_ << ")";
    }
};

void demo_vec2() {
    std::cout << "== Demo Vec2: +, ==, << ==\n";
    Vec2 a(1, 2), b(3, 4);
    Vec2 c = a + b;
    std::cout << "  " << a << " + " << b << " = " << c << "\n";
    std::cout << "  a == b ? " << (a == b ? "dung" : "sai")
              << ", a != b ? " << (a != b ? "dung" : "sai") << "\n";
}

// ---------- 2. Buffer: operator[] const/non-const, static member, copy assignment ----------
class Buffer {
    int data_[8] = {};
    std::size_t size_ = 8;
    inline static int so_instance = 0;   // C++17: inline static, mot ban cho ca lop
public:
    Buffer() { ++so_instance; }
    Buffer(const Buffer& o) : size_(o.size_) {
        for (std::size_t i = 0; i < size_; ++i) data_[i] = o.data_[i];
        ++so_instance;
    }
    ~Buffer() { --so_instance; }

    // Copy assignment: kiem tra tu gan, tra ve *this
    Buffer& operator=(const Buffer& o) {
        if (this != &o) {
            size_ = o.size_;
            for (std::size_t i = 0; i < size_; ++i) data_[i] = o.data_[i];
        }
        return *this;
    }

    // Hai phien ban [] : non-const (doc+ghi) va const (chi doc)
    int&       operator[](std::size_t i)       { return data_[i]; }
    const int& operator[](std::size_t i) const { return data_[i]; }

    std::size_t size() const { return size_; }          // const member function
    static int dem_instance() { return so_instance; }   // static: khong co this
};

void in_buffer(const Buffer& b) {   // tham so const& -> chi goi duoc ham const
    std::cout << "  buffer:";
    for (std::size_t i = 0; i < b.size(); ++i) std::cout << " " << b[i];  // goi ban const cua []
    std::cout << "\n";
}

void demo_buffer() {
    std::cout << "== Demo Buffer: [], static, copy assignment ==\n";
    Buffer b1;
    for (std::size_t i = 0; i < b1.size(); ++i) b1[i] = static_cast<int>(i * 10);  // ban non-const
    in_buffer(b1);
    Buffer b2;
    b2 = b1;               // copy assignment
    b2 = b2;               // tu gan: an toan nho kiem tra this != &o
    in_buffer(b2);
    std::cout << "  So instance dang song (static): " << Buffer::dem_instance() << "\n";
}

// ---------- 3. Functor: operator() mang trạng thái + mutable cache ----------
class DemGoi {
    int he_so_;
    mutable int so_lan_goi_ = 0;   // mutable: sua duoc trong ham const (trang thai vat ly)
public:
    explicit DemGoi(int he_so) : he_so_(he_so) {}
    int operator()(int x) const {  // const: trang thai LOGIC (he_so_) khong doi
        ++so_lan_goi_;             // van dem duoc nho mutable
        return x * he_so_;
    }
    int so_lan_goi() const { return so_lan_goi_; }
};

void demo_functor() {
    std::cout << "== Demo functor operator() + mutable ==\n";
    const DemGoi nhan3(3);         // object const van goi duoc vi operator() la const
    std::cout << "  nhan3(5) = " << nhan3(5) << ", nhan3(7) = " << nhan3(7) << "\n";
    std::cout << "  So lan goi (dem qua mutable): " << nhan3.so_lan_goi() << "\n";
}

int main() {
    demo_vec2();
    demo_buffer();
    demo_functor();
    std::cout << "Bai 07 hoan thanh.\n";
    return 0;
}
