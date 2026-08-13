// Bài 04: Stack, Heap, Static storage và Vòng đời đối tượng
// Biên dịch: g++ -std=c++17 -Wall -Wextra main.cpp -o bai04_stack_heap_lifetime.exe

#include <cstdlib>   // malloc/free
#include <iostream>
#include <memory>    // unique_ptr

// ---------------------------------------------------------------
// 1. Ba loại storage duration: automatic / static / dynamic
// ---------------------------------------------------------------
int bien_global = 10;        // static storage, có giá trị -> section .data
int bien_bss;                // static storage, không khởi tạo -> .bss (zero)

int dem_lan_goi() {
    static int dem = 0;      // static cục bộ: khởi tạo 1 LẦN, sống suốt chương trình
    return ++dem;            // giữ trạng thái giữa các lần gọi
}

void demo_storage_duration() {
    std::cout << "--- 1. Storage duration ---\n";
    int cuc_bo = 1;          // automatic: trên stack, hủy cuối scope

    std::cout << "dia chi bien stack : " << &cuc_bo << "\n";
    std::cout << "dia chi .data      : " << &bien_global
              << " (gia tri " << bien_global << ")\n";
    std::cout << "dia chi .bss       : " << &bien_bss
              << " (tu dong = " << bien_bss << ")\n";

    int* heap = new int(99); // dynamic: trên heap
    std::cout << "dia chi heap       : " << heap << "\n";
    delete heap;

    std::cout << "static cuc bo: goi 3 lan -> "
              << dem_lan_goi() << ", " << dem_lan_goi() << ", "
              << dem_lan_goi() << " (nho trang thai)\n";
}

// ---------------------------------------------------------------
// 2. Stack lớn XUỐNG: quan sát địa chỉ biến cục bộ qua các frame
// ---------------------------------------------------------------
void de_quy_stack(int sau) {
    int danh_dau = sau;      // mỗi lần gọi có bản riêng trên frame mới
    std::cout << "  frame sau=" << danh_dau
              << " dia chi=" << static_cast<void*>(&danh_dau) << "\n";
    if (sau < 3) {
        de_quy_stack(sau + 1);   // frame mới nằm ở địa chỉ THẤP hơn
    }
}

void demo_stack_frames() {
    std::cout << "\n--- 2. Stack lon xuong (dia chi giam dan) ---\n";
    de_quy_stack(1);
    // BẪY (không thực thi): đệ quy không có điều kiện dừng, hoặc
    //   char to[1024*1024]; trên stack -> stack overflow.
}

// ---------------------------------------------------------------
// 3. new/delete vs malloc/free — khác biệt cốt lõi: ctor/dtor
// ---------------------------------------------------------------
struct CoCtor {
    CoCtor()  { std::cout << "  CoCtor: ctor chay\n"; }
    ~CoCtor() { std::cout << "  CoCtor: dtor chay\n"; }
};

void demo_new_vs_malloc() {
    std::cout << "\n--- 3. new/delete vs malloc/free ---\n";

    std::cout << "new/delete (goi ctor/dtor):\n";
    CoCtor* a = new CoCtor;      // cấp phát + GỌI CTOR
    delete a;                    // GỌI DTOR + trả bộ nhớ

    std::cout << "malloc/free (chi cap byte tho, KHONG ctor/dtor):\n";
    void* raw = std::malloc(sizeof(CoCtor));  // không có dòng "ctor chay"
    if (raw != nullptr) {
        std::cout << "  malloc tra ve " << raw << ", khong ctor nao chay\n";
        std::free(raw);          // không dtor nào chạy
    }

    // Mảng: new[] PHẢI đi với delete[]
    int* arr = new int[5]();     // () = zero-init (new int[5] thì KHÔNG zero)
    std::cout << "  new int[5]() -> arr[4] = " << arr[4] << " (zero-init)\n";
    delete[] arr;                // dùng delete (không []) ở đây là UB!
}

// ---------------------------------------------------------------
// 4. Các lỗi bộ nhớ kinh điển — minh hoạ AN TOÀN (UB chỉ trong comment)
// ---------------------------------------------------------------
void demo_loi_bo_nho() {
    std::cout << "\n--- 4. Leak / use-after-free / double-free ---\n";

    // MEMORY LEAK (không thực thi):
    //   void leak() { int* p = new int(1); return; }  // mất con trỏ, không delete
    //
    // USE-AFTER-FREE (không thực thi):
    //   int* p = new int(1); delete p; std::cout << *p;  // đọc vùng đã trả -> UB
    //
    // DOUBLE-FREE (không thực thi):
    //   int* p = new int(1); delete p; delete p;         // phá allocator -> UB

    // Phòng thủ 1: gán nullptr sau delete
    int* p = new int(5);
    delete p;
    p = nullptr;
    delete p;    // delete nullptr là NO-OP an toàn -> double-free vô hại
    std::cout << "  delete nullptr: an toan (no-op)\n";

    // Phòng thủ 2 (chuẩn hiện đại): RAII với unique_ptr — không thể quên delete
    {
        std::unique_ptr<int> up = std::make_unique<int>(42);
        std::cout << "  unique_ptr giu " << *up << ", tu delete cuoi scope\n";
    }   // <- delete tự động tại đây, mọi đường thoát
    std::cout << "  unique_ptr da tu giai phong\n";
}

// ---------------------------------------------------------------
// 5. Heap fragmentation — mô phỏng bằng pool tự quản lý
// ---------------------------------------------------------------
void demo_phan_manh() {
    std::cout << "\n--- 5. Heap fragmentation (mo phong) ---\n";
    // Pool 16 ô, mỗi ký tự là 1 "byte": '.' trống, '#' đang dùng
    // Sau chuỗi alloc/free xen kẽ, heap thành:
    const char* heap_sau_free = "##..##..##..##..";
    std::cout << "  heap: " << heap_sau_free << "\n";
    std::cout << "  tong o trong = 8, nhung lon nhat lien ke = 2\n";
    std::cout << "  -> xin 4 byte lien tuc THAT BAI du tong trong du!\n";
    std::cout << "  day la ly do MCU chay dai ngay tranh malloc/new dong\n";
}

int main() {
    demo_storage_duration();
    demo_stack_frames();
    demo_new_vs_malloc();
    demo_loi_bo_nho();
    demo_phan_manh();
    return 0;
}
