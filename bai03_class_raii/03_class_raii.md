# Bài 03: Class cơ bản và RAII

## Định nghĩa & Khái niệm

- **Class**: kiểu dữ liệu do người dùng định nghĩa, gói **dữ liệu (member)** và **hành vi (method)** vào một đơn vị; kiểm soát truy cập bằng `public/private/protected`.
- **Constructor (ctor)**: hàm đặc biệt chạy khi đối tượng được **tạo** — nơi thiết lập bất biến (invariant) của đối tượng.
- **Destructor (dtor)**: hàm chạy **tự động** khi đối tượng hết vòng đời — nơi trả tài nguyên.
- **Encapsulation**: che giấu dữ liệu bên trong, chỉ lộ giao diện — để bất biến của class không bị phá từ bên ngoài.
- **RAII (Resource Acquisition Is Initialization)**: kỹ thuật gắn **vòng đời tài nguyên** (file, bộ nhớ, mutex, ngoại vi) vào **vòng đời đối tượng**: ctor xin tài nguyên, dtor trả tài nguyên. Đây là idiom quan trọng nhất của C++.

## Giải thích chi tiết

### Các loại constructor

```cpp
class Diem {
    int x_, y_;
    static int dem_;                 // static member: dùng chung mọi đối tượng
public:
    Diem() : Diem(0, 0) {}           // delegating ctor (C++11): gọi ctor khác
    Diem(int x, int y) : x_(x), y_(y) { ++dem_; }  // parameterized + init list
    Diem(const Diem& o) : x_(o.x_), y_(o.y_) { ++dem_; } // copy ctor
    explicit Diem(int c) : Diem(c, c) {}  // explicit: chặn chuyển đổi ngầm
    ~Diem() { --dem_; }              // destructor
    static int dem() { return dem_; }
};
```

- **Initialization list** (`: x_(x)`) khởi tạo member **trực tiếp**, trước khi thân ctor chạy. Bắt buộc với member `const`, tham chiếu, và class không có default ctor. Gán trong thân ctor là "khởi tạo mặc định rồi gán đè" — kém hiệu quả và đôi khi không hợp lệ.
- **`explicit`**: không có nó, `Diem d = 5;` hợp lệ (chuyển ngầm int → Diem) — thường là bug tiềm ẩn. Quy tắc: ctor 1 tham số mặc định nên `explicit`.
- **`= default`**: yêu cầu compiler sinh phiên bản mặc định (rõ ràng hơn là để ngầm).
- **`= delete`**: cấm hẳn một thao tác. Ví dụ cấm copy với class quản lý tài nguyên:
  ```cpp
  FileRAII(const FileRAII&) = delete;
  FileRAII& operator=(const FileRAII&) = delete;
  ```
  Tại sao? Nếu copy được, hai đối tượng cùng "sở hữu" một `FILE*` → dtor chạy 2 lần → **double-close**.

### this pointer

Trong method, `this` là con trỏ tới chính đối tượng đang được gọi. Dùng khi: phân biệt member với tham số trùng tên (`this->x = x`), trả về chính mình để chain (`return *this;`).

### static members

Thuộc về **class**, không thuộc đối tượng nào — như biến toàn cục có namespace là class. Dùng đếm số instance, cấu hình chung. Phải định nghĩa ngoài class (hoặc `inline static` từ C++17).

### friend

Cho phép hàm/class ngoài truy cập `private`. Dùng tiết chế — chủ yếu cho operator (`operator<<`) cần đọc nội bộ. Lạm dụng friend là phá encapsulation.

### RAII — trọng tâm

```
Không RAII:                        RAII:
  f = fopen(...)                     { FileRAII f("log.txt");
  ... xử lý ...                        ... xử lý ...
  if (loi) return;   // QUÊN fclose!   if (loi) return;  // dtor tự fclose
  fclose(f);                         } // ra khỏi scope -> dtor -> fclose
```

**Tại sao RAII mạnh?** Vì destructor chạy trên **mọi đường thoát khỏi scope**: return sớm, break, exception. Con người quên `fclose`/`free`/`unlock`; compiler thì không. Toàn bộ `std::string`, `std::vector`, `std::unique_ptr`, `std::lock_guard` đều là RAII.

## Cách dùng

```cpp
class FileRAII {
    FILE* f_ = nullptr;
public:
    explicit FileRAII(const char* path, const char* mode)
        : f_(std::fopen(path, mode)) {}
    ~FileRAII() { if (f_) std::fclose(f_); }        // tự đóng
    FileRAII(const FileRAII&) = delete;             // cấm copy
    FileRAII& operator=(const FileRAII&) = delete;
    bool ok() const { return f_ != nullptr; }
    FILE* get() const { return f_; }
};

void ghi_log() {
    FileRAII f("log.txt", "w");
    if (!f.ok()) return;              // không rò rỉ gì cả
    std::fprintf(f.get(), "hello\n");
}                                     // dtor đóng file tại đây
```

## Tips & Tricks

- Thứ tự khởi tạo member theo **thứ tự khai báo trong class**, không theo thứ tự trong init list — viết init list đúng thứ tự khai báo để tránh cảnh báo `-Wreorder`.
- Rule of Zero: nếu class không quản lý tài nguyên thô, đừng viết dtor/copy/move — để compiler sinh. Rule of Five: nếu viết một trong (dtor, copy ctor, copy=, move ctor, move=), hãy cân nhắc cả năm.
- Method không sửa trạng thái → đánh dấu `const` (`int dem() const;`) — bắt buộc để gọi được trên `const T&`.
- Dùng `explicit` mặc định cho ctor 1 tham số; chỉ bỏ khi thật sự muốn chuyển ngầm.
- Destructor **không được ném exception**.

## Lỗi thường gặp / Bẫy

1. **Quên `= delete` copy cho class giữ tài nguyên** → double-free/double-close khi copy vô tình.
2. **Gán trong thân ctor thay vì init list** với member `const`/tham chiếu → lỗi biên dịch; với object nặng → khởi tạo 2 lần.
3. **Trông cậy thứ tự init list** trong khi thứ tự thật là thứ tự khai báo member.
4. **Trả về tham chiếu tới member của đối tượng tạm** → dangling.
5. **Quên định nghĩa static member** ngoài class (trước C++17) → lỗi link "undefined reference".
6. **Dtor ném exception** trong lúc stack unwinding → `std::terminate`.

## Ghi chú Embedded

- RAII cực hợp với ngoại vi MCU: ctor bật clock/cấu hình pin, dtor tắt — không bao giờ quên tắt ngoại vi khi thoát hàm sớm:
  ```cpp
  class SpiGuard {
  public:
      SpiGuard()  { /* nrfx_spim_init(...); bật CS */ }
      ~SpiGuard() { /* kéo CS lên, nrfx_spim_uninit() -> tiết kiệm điện */ }
  };
  ```
- `std::lock_guard`-style RAII cho critical section: ctor tắt ngắt (`__disable_irq()`), dtor bật lại — mọi đường return đều bật lại ngắt.
- Ctor/dtor của biến **toàn cục** chạy trước/sau `main()` (qua `.init_array`) — trên firmware cần startup code hỗ trợ; tránh logic phức tạp/phụ thuộc thứ tự giữa các global (static initialization order fiasco).
- Trên MCU không dùng exception (`-fno-exceptions`): ctor không báo lỗi được bằng throw → dùng cờ `ok()`/factory function kiểm tra sau khi tạo.

## Bài tập tự luyện

1. Viết class `Timer` RAII: ctor lưu thời điểm bắt đầu (`std::chrono`), dtor in ra thời gian tồn tại của scope. Dùng nó đo một vòng lặp.
2. Viết class `Buffer` quản lý `new char[n]`: đủ ctor/dtor, cấm copy bằng `= delete`. Sau đó thêm move ctor (tự tìm hiểu `&&`).
3. Viết class `IrqLock` mô phỏng: ctor in "IRQ disabled", dtor in "IRQ enabled"; chứng minh mọi đường `return`/exception đều "bật lại ngắt".

## Tóm tắt

- Ctor thiết lập bất biến, dtor trả tài nguyên — cả hai chạy tự động theo vòng đời đối tượng.
- Init list khởi tạo trực tiếp; `explicit` chặn chuyển ngầm; `= default`/`= delete` kiểm soát các hàm compiler sinh.
- Static member thuộc class; `friend` dùng tiết chế; `this` trỏ tới đối tượng hiện tại.
- RAII = tài nguyên sống chết cùng đối tượng → không rò rỉ trên mọi đường thoát. Class giữ tài nguyên thô phải cấm (hoặc định nghĩa đúng) copy.
- Embedded: RAII cho ngoại vi/ngắt; cẩn thận ctor global và môi trường không exception.
