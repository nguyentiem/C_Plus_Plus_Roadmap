// Bài 02: Con trỏ, Tham chiếu, Mảng, Con trỏ hàm
// Biên dịch: g++ -std=c++17 -Wall -Wextra main.cpp -o bai02_con_tro_tham_chieu.exe

#include <iostream>
#include <iterator> // std::size

// ---------------------------------------------------------------
// 1. Con trỏ cơ bản: địa chỉ (&) và dereference (*)
// ---------------------------------------------------------------
void demo_con_tro_co_ban() {
    std::cout << "--- 1. Con tro co ban ---\n";
    int x = 42;
    int* p = &x;              // p chứa địa chỉ của x

    std::cout << "x = " << x << ", dia chi &x = " << p << "\n";
    std::cout << "*p = " << *p << " (doc gia tri qua con tro)\n";

    *p = 100;                 // ghi qua con trỏ -> sửa chính x
    std::cout << "sau *p = 100 thi x = " << x << "\n";

    p = nullptr;              // con trỏ rỗng: KHÔNG được dereference
    if (p == nullptr) {
        std::cout << "p hien la nullptr, khong duoc dung *p\n";
    }
}

// ---------------------------------------------------------------
// 2. Tham chiếu: bí danh của biến, không thể null, không gán lại đích
// ---------------------------------------------------------------
void tang_bang_ref(int& v) { ++v; }       // sửa trực tiếp biến caller
void tang_bang_ptr(int* v) {              // phiên bản con trỏ: phải check null
    if (v != nullptr) { ++*v; }
}

void demo_tham_chieu() {
    std::cout << "\n--- 2. Tham chieu ---\n";
    int a = 5;
    int& r = a;               // r là bí danh của a
    r = 7;                    // ghi vào r chính là ghi vào a
    std::cout << "a = " << a << " (sua qua tham chieu r)\n";

    tang_bang_ref(a);
    tang_bang_ptr(&a);
    std::cout << "sau 2 lan tang: a = " << a << "\n";
}

// ---------------------------------------------------------------
// 3. Mảng, decay và pointer arithmetic
// ---------------------------------------------------------------
// Khi truyền vào hàm, mảng decay thành con trỏ -> phải truyền kèm kích thước
int tong(const int* arr, std::size_t n) {
    int s = 0;
    // Duyệt thuần bằng số học con trỏ: p chạy từ arr tới arr+n (one-past-end)
    for (const int* p = arr; p != arr + n; ++p) {
        s += *p;
    }
    return s;
}

void demo_mang_va_arithmetic() {
    std::cout << "\n--- 3. Mang va pointer arithmetic ---\n";
    int arr[5] = {10, 20, 30, 40, 50};

    // std::size chỉ dùng được với mảng thật (chưa decay)
    std::cout << "so phan tu: " << std::size(arr) << "\n";
    std::cout << "arr[2] = " << arr[2] << " == *(arr+2) = " << *(arr + 2) << "\n";

    // p + 1 nhảy đúng sizeof(int) byte, không phải 1 byte
    const int* p = arr;
    std::cout << "p = " << static_cast<const void*>(p)
              << ", p+1 = " << static_cast<const void*>(p + 1)
              << " (chenh " << sizeof(int) << " byte)\n";

    std::cout << "tong mang = " << tong(arr, std::size(arr)) << "\n";
}

// ---------------------------------------------------------------
// 4. const pointer vs pointer to const (đọc từ phải sang trái)
// ---------------------------------------------------------------
void demo_const_pointer() {
    std::cout << "\n--- 4. const int* vs int* const ---\n";
    int x = 1, y = 2;

    const int* p1 = &x;   // pointer to const: KHÔNG sửa *p1, ĐƯỢC đổi đích
    // *p1 = 9;           // LỖI biên dịch nếu bỏ comment
    p1 = &y;              // OK
    std::cout << "p1 tro toi y, *p1 = " << *p1 << "\n";

    int* const p2 = &x;   // const pointer: ĐƯỢC sửa *p2, KHÔNG đổi đích
    *p2 = 99;             // OK, sửa x
    // p2 = &y;           // LỖI biên dịch nếu bỏ comment
    std::cout << "*p2 = " << *p2 << " (x da bi sua)\n";
}

// ---------------------------------------------------------------
// 5. Con trỏ hàm và bảng lệnh (dispatch table)
// ---------------------------------------------------------------
int cong(int a, int b) { return a + b; }
int nhan(int a, int b) { return a * b; }

void demo_con_tro_ham() {
    std::cout << "\n--- 5. Con tro ham ---\n";
    int (*fp)(int, int) = cong;       // hàm tự decay thành con trỏ hàm
    std::cout << "fp = cong: fp(3,4) = " << fp(3, 4) << "\n";
    fp = nhan;                        // trỏ lại sang hàm khác
    std::cout << "fp = nhan: fp(3,4) = " << fp(3, 4) << "\n";

    // Bảng lệnh: kiểu hay dùng trong firmware (shell UART, state machine)
    struct Cmd { const char* ten; int (*handler)(int, int); };
    const Cmd bang[] = { {"cong", cong}, {"nhan", nhan} };
    for (const Cmd& c : bang) {
        std::cout << "lenh '" << c.ten << "' (5,6) -> " << c.handler(5, 6) << "\n";
    }
}

// ---------------------------------------------------------------
// 6. Dangling pointer — CHỈ minh hoạ, không thực thi UB
// ---------------------------------------------------------------
void demo_dangling() {
    std::cout << "\n--- 6. Dangling pointer (minh hoa an toan) ---\n";
    // VÍ DỤ SAI (không thực thi):
    //   int* bad() { int cuc_bo = 1; return &cuc_bo; }
    //   -> cuc_bo bị hủy khi hàm return, con trỏ trả về là dangling.
    //   Dereference nó là undefined behavior.
    //
    // Cách phòng thủ: gán nullptr sau khi vùng nhớ không còn hợp lệ.
    int* p = new int(7);
    std::cout << "*p truoc delete = " << *p << "\n";
    delete p;
    p = nullptr;          // biến dangling thành null -> kiểm tra được
    if (p == nullptr) {
        std::cout << "p da duoc gan nullptr sau delete -> an toan kiem tra\n";
    }
}

int main() {
    demo_con_tro_co_ban();
    demo_tham_chieu();
    demo_mang_va_arithmetic();
    demo_const_pointer();
    demo_con_tro_ham();
    demo_dangling();
    return 0;
}
