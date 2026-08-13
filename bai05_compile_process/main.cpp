// Bài 05: Quá trình biên dịch — preprocessor, compiler, linker, sections
// Biên dịch: g++ -std=c++17 -Wall -Wextra main.cpp -o bai05_compile_process.exe
// Thử thêm:  g++ -E main.cpp | tail   (xem sau preprocessor)
//            g++ -S main.cpp          (xem assembly, tìm tên mangled)

#include <iostream>

// ---------------------------------------------------------------
// 1. Preprocessor: macro chỉ là THAY THẾ VĂN BẢN trước khi compile
// ---------------------------------------------------------------
#define PHIEN_BAN "1.0"           // thay chuỗi
#define BINH_PHUONG(x) ((x) * (x)) // macro hàm: LUÔN bọc ngoặc từng tham số!

// Biên dịch có điều kiện — cách firmware chọn cấu hình theo board
#if defined(BOARD_NRF52840)
constexpr const char* kBoard = "nRF52840";
#else
constexpr const char* kBoard = "PC (host build)";
#endif

void demo_preprocessor() {
    std::cout << "--- 1. Preprocessor ---\n";
    std::cout << "PHIEN_BAN = " << PHIEN_BAN << " (dan van ban)\n";
    std::cout << "BINH_PHUONG(3+1) = " << BINH_PHUONG(3 + 1)
              << " (dung nho boc ngoac; khong boc se ra 3+1*3+1 = 7!)\n";
    std::cout << "board: " << kBoard << " (#if defined(...))\n";
    // __FILE__/__LINE__ do preprocessor điền — nền tảng của mọi macro LOG()
    std::cout << "vi tri nay: " << __FILE__ << ":" << __LINE__ << "\n";
}

// ---------------------------------------------------------------
// 2. Name mangling: overload được vì tên đã mã hoá kiểu tham số
//    (chạy `g++ -S main.cpp` sẽ thấy _Z3addii và _Z3adddd)
// ---------------------------------------------------------------
int add(int a, int b) { return a + b; }          // mangled: _Z3addii
double add(double a, double b) { return a + b; } // mangled: _Z3adddd

// extern "C": TẮT mangling -> tên symbol là "ham_kieu_c" y nguyên.
// Đây là cách gọi hàm từ SDK C (nRF SDK, CMSIS) trong code C++.
// Hệ quả: hàm extern "C" KHÔNG overload được.
extern "C" int ham_kieu_c(int x) { return x * 2; }

void demo_mangling() {
    std::cout << "\n--- 2. Name mangling & extern \"C\" ---\n";
    std::cout << "add(2,3)      = " << add(2, 3) << "  (goi ban int)\n";
    std::cout << "add(2.5,3.5)  = " << add(2.5, 3.5) << " (goi ban double)\n";
    std::cout << "ham_kieu_c(21) = " << ham_kieu_c(21)
              << " (symbol khong mangle, C link duoc)\n";
}

// ---------------------------------------------------------------
// 3. Sections: biến nằm ở .text/.rodata/.data/.bss tuỳ cách khai báo
//    (kiểm chứng bằng: objdump -h / nm / size trên file build ra)
// ---------------------------------------------------------------
const int hang_rodata = 12345;      // const -> .rodata (MCU: nằm ở FLASH)
int bien_data = 777;                // init != 0 -> .data (copy flash->RAM luc boot)
int bien_bss;                       // không init -> .bss (zero-fill, KHONG ton flash)
static int bss_lon[1000];           // 4000 byte .bss: file exe KHÔNG to thêm 4KB

void demo_sections() {
    std::cout << "\n--- 3. Sections .text/.rodata/.data/.bss ---\n";
    std::cout << ".text  : ma lenh, vi du dia chi ham demo_sections = "
              << reinterpret_cast<void*>(&demo_sections) << "\n";
    std::cout << ".rodata: hang_rodata = " << hang_rodata
              << " tai " << static_cast<const void*>(&hang_rodata) << "\n";
    std::cout << ".data  : bien_data = " << bien_data
              << " tai " << static_cast<void*>(&bien_data) << "\n";
    std::cout << ".bss   : bien_bss = " << bien_bss << " (tu zero-fill boi startup)"
              << " tai " << static_cast<void*>(&bien_bss) << "\n";
    std::cout << ".bss   : bss_lon[999] = " << bss_lon[999]
              << " (4000B RAM nhung 0B trong file/flash)\n";
}

// ---------------------------------------------------------------
// 4. ODR & linkage: static/inline kiểm soát tầm nhìn symbol giữa các TU
// ---------------------------------------------------------------
static int chi_trong_tu_nay = 1;   // internal linkage: TU khác không thấy
                                    // -> 2 file .cpp cùng tên biến này vẫn OK
inline int dinh_nghia_trong_header_ok() {  // inline: được định nghĩa ở nhiều TU
    return 42;                              // (miễn là các bản GIỐNG HỆT nhau)
}

void demo_odr_linkage() {
    std::cout << "\n--- 4. ODR & linkage ---\n";
    std::cout << "static (internal linkage): " << chi_trong_tu_nay
              << " — TU khac khong dung duoc ten nay\n";
    std::cout << "inline function: " << dinh_nghia_trong_header_ok()
              << " — dat trong header van khong vi pham ODR\n";
    // VI PHẠM ODR (minh hoạ, không làm):
    //   int x; trong header include vao 2 .cpp -> "multiple definition of x"
    //   Sua: extern int x; trong header + int x; trong DUY NHAT 1 .cpp
}

// ---------------------------------------------------------------
// 5. Tóm tắt pipeline bằng output
// ---------------------------------------------------------------
void demo_pipeline() {
    std::cout << "\n--- 5. Pipeline bien dich ---\n";
    std::cout << "  main.cpp -(g++ -E)-> TU  -(g++ -S)-> main.s\n";
    std::cout << "  main.s   -(as)-----> main.o (symbol chua co dia chi cuoi)\n";
    std::cout << "  main.o + libs -(ld)-> executable (PE/ELF, dia chi da chot)\n";
    std::cout << "  MCU: linker script .ld quyet dinh FLASH@0x0, RAM@0x20000000\n";
    std::cout << "  Loi 'undefined reference' = loi LINKER (thieu dinh nghia)\n";
    std::cout << "  Loi 'was not declared'    = loi COMPILER (thieu header)\n";
}

int main() {
    demo_preprocessor();
    demo_mangling();
    demo_sections();
    demo_odr_linkage();
    demo_pipeline();
    return 0;
}
