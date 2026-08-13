// Bài 03: Class cơ bản và RAII
// Biên dịch: g++ -std=c++17 -Wall -Wextra main.cpp -o bai03_class_raii.exe

#include <cstdio>
#include <iostream>

// ---------------------------------------------------------------
// 1. Class với đủ loại constructor
// ---------------------------------------------------------------
class Diem {
    int x_;
    int y_;
    inline static int so_luong_ = 0;   // static member (C++17: inline trong class)

public:
    // Delegating ctor: default ctor gọi lại ctor 2 tham số
    Diem() : Diem(0, 0) {}

    // Parameterized ctor + initialization list (khởi tạo TRỰC TIẾP member)
    Diem(int x, int y) : x_(x), y_(y) {
        ++so_luong_;
        std::cout << "  ctor (" << x_ << "," << y_ << ")\n";
    }

    // explicit: chặn chuyển đổi ngầm int -> Diem (Diem d = 5; sẽ lỗi biên dịch)
    explicit Diem(int c) : Diem(c, c) {}

    // Copy ctor
    Diem(const Diem& o) : x_(o.x_), y_(o.y_) {
        ++so_luong_;
        std::cout << "  copy ctor tu (" << o.x_ << "," << o.y_ << ")\n";
    }

    // Copy assignment: để compiler sinh, khai báo tường minh cho rõ
    Diem& operator=(const Diem&) = default;

    // Destructor
    ~Diem() {
        --so_luong_;
        std::cout << "  dtor (" << x_ << "," << y_ << ")\n";
    }

    // this pointer: phân biệt member với tham số trùng tên, chain method
    Diem& setX(int x) { this->x_ = x; return *this; }
    Diem& setY(int y) { this->y_ = y; return *this; }

    int x() const { return x_; }       // method const: không sửa trạng thái
    int y() const { return y_; }

    static int soLuong() { return so_luong_; }  // static method

    // friend: cho operator<< đọc private member
    friend std::ostream& operator<<(std::ostream& os, const Diem& d);
};

std::ostream& operator<<(std::ostream& os, const Diem& d) {
    return os << "Diem(" << d.x_ << "," << d.y_ << ")";  // truy cập private nhờ friend
}

void demo_class_co_ban() {
    std::cout << "--- 1. Cac loai constructor ---\n";
    Diem a;                 // default (delegating -> (0,0))
    Diem b(3, 4);           // parameterized
    Diem c(b);              // copy ctor
    Diem d(7);              // explicit ctor: phải gọi tường minh
    // Diem e = 7;          // LỖI biên dịch nếu bỏ comment: ctor là explicit

    d.setX(1).setY(2);      // chain nhờ return *this
    std::cout << "  d sau chain: " << d << "\n";
    std::cout << "  so doi tuong dang song (static): " << Diem::soLuong() << "\n";
    std::cout << "  (dtor se chay khi ra khoi scope, theo thu tu nguoc)\n";
}

// ---------------------------------------------------------------
// 2. RAII quản lý FILE* — trọng tâm bài học
// ---------------------------------------------------------------
class FileRAII {
    FILE* f_ = nullptr;

public:
    // Ctor XIN tài nguyên
    explicit FileRAII(const char* path, const char* mode)
        : f_(std::fopen(path, mode)) {
        std::cout << "  [RAII] mo file " << path
                  << (f_ ? " -> OK\n" : " -> THAT BAI\n");
    }

    // Dtor TRẢ tài nguyên — chạy trên MỌI đường thoát khỏi scope
    ~FileRAII() {
        if (f_ != nullptr) {
            std::fclose(f_);
            std::cout << "  [RAII] dtor tu dong fclose\n";
        }
    }

    // Cấm copy: nếu copy được, 2 đối tượng cùng sở hữu 1 FILE* -> double-close!
    FileRAII(const FileRAII&) = delete;
    FileRAII& operator=(const FileRAII&) = delete;

    bool ok() const { return f_ != nullptr; }
    FILE* get() const { return f_; }
};

void demo_raii_file() {
    std::cout << "\n--- 2. RAII quan ly FILE* ---\n";
    {
        FileRAII f("bai03_demo.txt", "w");
        if (!f.ok()) {
            return;  // return sớm cũng KHÔNG rò rỉ: chưa mở được thì không có gì để đóng
        }
        std::fprintf(f.get(), "RAII: tai nguyen song chet cung doi tuong\n");
        std::cout << "  da ghi vao file, sap ra khoi scope...\n";
    }   // <- ra khỏi scope: dtor tự fclose, kể cả khi return sớm/exception
    std::remove("bai03_demo.txt");   // dọn file demo cho gọn
}

// ---------------------------------------------------------------
// 3. RAII kiểu embedded: guard tắt/bật ngắt (mô phỏng)
// ---------------------------------------------------------------
class IrqLock {
public:
    IrqLock()  { std::cout << "  [IrqLock] __disable_irq() (mo phong)\n"; }
    ~IrqLock() { std::cout << "  [IrqLock] __enable_irq()  (mo phong)\n"; }
    IrqLock(const IrqLock&) = delete;
    IrqLock& operator=(const IrqLock&) = delete;
};

void demo_irq_guard(bool loi_giua_chung) {
    std::cout << "\n--- 3. RAII guard ngat (loi_giua_chung="
              << std::boolalpha << loi_giua_chung << ") ---\n";
    IrqLock lock;                       // vào critical section
    std::cout << "  ... thao tac du lieu chia se ...\n";
    if (loi_giua_chung) {
        std::cout << "  gap loi -> return som\n";
        return;                         // dtor VẪN bật lại ngắt
    }
    std::cout << "  hoan tat binh thuong\n";
}                                       // dtor bật lại ngắt ở đây

int main() {
    demo_class_co_ban();
    demo_raii_file();
    demo_irq_guard(false);
    demo_irq_guard(true);   // chứng minh mọi đường thoát đều trả tài nguyên
    std::cout << "\nket thuc main, so Diem con song: " << Diem::soLuong() << "\n";
    return 0;
}
