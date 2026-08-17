# Bài 03: Lập trình hướng đối tượng trong C++ — Class, Lifetime, RAII

## Định nghĩa & Khái niệm

- **Class** là kiểu dữ liệu do người dùng định nghĩa, gói dữ liệu (**data members**) và hành vi (**member functions**) thành một đơn vị.
- **Object** là một instance cụ thể của class. Mỗi object có trạng thái riêng, nhưng cùng giao diện do class định nghĩa.
- **Encapsulation** che giấu representation nội bộ và chỉ công bố API cần thiết. Mục tiêu là bảo vệ **invariant**: điều kiện luôn đúng của object.
- **RAII** (*Resource Acquisition Is Initialization*) gắn thời gian sống của tài nguyên với object: ctor nhận tài nguyên, dtor trả nó.
- **Polymorphism** cho phép sử dụng nhiều loại object qua cùng một interface. Trong C++ có đa hình runtime với `virtual`, và đa hình compile-time với template.
- **Ownership** trả lời “ai có trách nhiệm giải phóng tài nguyên”. Đây là cơ sở để thiết kế copy/move, RAII và smart pointer đúng.

Bài này cung cấp bức tranh OOP hoàn chỉnh. Các chủ đề có bài chuyên sâu vẫn được đào sâu hơn ở Bài 06 (kế thừa/đa hình), Bài 09 (template), Bài 11 (smart pointer) và Bài 12 (move semantics).

## Giải thích chi tiết

### 1. Class, object và `struct`

```cpp
class DongCoDC {
    int toc_do_ = 0;              // private mặc định với class
public:
    explicit DongCoDC(int toc_do) : toc_do_(toc_do) {}
    void datTocDo(int toc_do) { toc_do_ = toc_do; }
    int tocDo() const { return toc_do_; }
};

DongCoDC dong_co(1200);           // object
```

`class` và `struct` có cùng khả năng trong C++. Khác biệt mặc định là `class` có member/base `private`, còn `struct` là `public`. Dùng `struct` cho dữ liệu đơn giản, không invariant phức tạp. Dùng `class` khi cần che giấu representation và bảo vệ API.

Không để client tự sửa dữ liệu nếu việc đó có thể phá invariant. Ví dụ một motor không được nhận tốc độ âm; kiểm tra đó nên nằm trong `datTocDo`, không để client ghi trực tiếp member.

> **Kinh nghiệm thực chiến (senior)**
> - Trước khi viết class, viết ra invariant bằng một câu. Nếu không nêu được invariant, có lẽ chỉ cần `struct` + free function; class "getter/setter cho mọi field" là struct trá hình và tốn công bảo trì.
> - Trong dự án driver thực tế, class hay nhất là class nhỏ: một class = một trách nhiệm (một UART port, một ring buffer). Class "Manager" 2000 dòng luôn là nơi phát sinh bug khó tái hiện.
> - Đặt tên member có hậu tố `_` (hoặc theo convention của team) ngay từ đầu; đổi convention giữa dự án gây diff review khổng lồ.
> - Khi review, câu hỏi đầu tiên nên là "trạng thái nào của object là không hợp lệ và ai chặn nó?" — trả lời được là thiết kế đã ổn 80%.

### 2. `public`, `private`, `protected` và `friend`

| Mức truy cập | Client bên ngoài | Lớp dẫn xuất | Bản thân class |
|---|---:|---:|---:|
| `public` | Có | Có | Có |
| `protected` | Không | Có | Có |
| `private` | Không | Không | Có |

- `public` là contract của class.
- `private` là implementation detail, có thể thay đổi mà ít làm hỏng code gọi.
- `protected` thường nên dành cho hàm extension point, không phải data. Derived class sửa trực tiếp `protected` data rất dễ phá invariant của base class.
- `friend` cho phép hàm/class ngoài đọc private state. Trường hợp hợp lý phổ biến là `operator<<`; không dùng `friend` thay cho API thiết kế kém.

> **Kinh nghiệm thực chiến (senior)**
> - `protected` data là món nợ kỹ thuật kinh điển: sau 2-3 lớp kế thừa, không ai còn dám sửa base vì không biết derived nào đang ghi trực tiếp. Quy tắc thực dụng: data luôn `private`, derived truy cập qua `protected` getter/hook.
> - Nếu bạn phải thêm `friend` để unit test class, đó là dấu hiệu class làm quá nhiều việc. Tách phần cần test thành class/hàm riêng thay vì mở private ra.
> - Với thư viện dùng chung nhiều team, mọi thứ để `public` "cho tiện" sẽ thành API contract vĩnh viễn — thu hẹp access sau này là breaking change. Bắt đầu chặt, nới dần khi có nhu cầu thật.

### 3. Constructor

Constructor chạy khi object được tạo và phải tạo object ở trạng thái hợp lệ.

```cpp
class Diem {
    int x_;
    int y_;
public:
    Diem() : Diem(0, 0) {}               // delegating ctor
    Diem(int x, int y) : x_(x), y_(y) {}
    explicit Diem(int ca) : Diem(ca, ca) {}
    Diem(const Diem&) = default;
};
```

#### **Default ctor** nhận không tham số.
#### **Parameterized ctor** nhận dữ liệu khởi tạo.
#### **Converting constructor** : chuyển kiểu cái mà cần explicit để loại bỏ

- Ctor một tham số nên dùng `explicit` trừ khi chuyển đổi ngầm thật sự có nghĩa. Không có `explicit`, `Diem d = 5;` có thể hợp lệ ngoài ý muốn. khi truyền tham số vào hàm nó sẽ tự convert thành obj và gọi lại constructor không mong muốn 
  
#### **Delegating ctor** gọi ctor khác cùng class.

#### **Copy ctor** có dạng `T(const T&)`, tạo object mới từ object có sẵn.

- `= default` yêu cầu compiler sinh implementation chuẩn. `= delete` cấm hẳn thao tác như copy.

```
Person p1;
Person p2 = p1; => copy constructor 
khác với 
Person p1;
Person p2;
p2 = p1; => copy assignment

```

#### Move contructor
 
 cứ khi gán là các phần trong move consturtor sẽ được chuyển quyền sở hữu cho object mới ứng dụng trong việc quản lsy cấp phát memory 

```
Buffer(Buffer&& other)
    : data(other.data),
      size(other.size)
{
    other.data = nullptr;
    other.size = 0;
}

Buffer createBuffer() {
    Buffer b{1000};

    return b;
}
hoặc 
Buffer buffer = createBuffer();
Buffer b = std::move(a); 
```


Nếu ctor ném exception, destructor của object đang xây **không** chạy vì object chưa tồn tại hoàn chỉnh. Tuy nhiên, các base/member đã xây thành công vẫn bị hủy. Vì vậy member nên là RAII object, không phải raw resource cần tự cleanup thủ công.

NOTE
```
void func(A a);        // truyền value → copy, hoặc move nếu cấm copy mà truyền func(A) lỗi 
void func(A& a);       // reference → không copy
void func(const A& a); // const reference → không copy
void func(A&& a);      // rvalue reference → không tự copy/move
///////////////
nếu thuộc tính là con trỏ mà không nên cùng trỏ đến một phần tử thì cần khai báo RAII tránh deconstruct 2 lần 
nếu cần trỏ thực sự thì cần std::shared_ptr

```


> **Kinh nghiệm thực chiến (senior)**
> - Bug chuyển đổi ngầm do thiếu `explicit` cực khó truy: `guiLenh(1000)` compile được vì `Lenh` có ctor `Lenh(int timeout)`. Bật thói quen: mọi ctor một tham số viết `explicit` trước, xóa sau nếu có lý do.
> - Tránh ctor làm việc nặng (mở socket, đọc flash, calibrate sensor). Ctor nặng khiến tạo object trong test rất đắt và khó mock. Mẫu hay dùng: ctor chỉ gán state, thêm `init()`/factory trả `optional<T>` cho phần có thể fail.
> - `= delete` là công cụ giao tiếp, không chỉ là cấm: `void ghi(bool) = delete;` chặn caller truyền nhầm bool vào overload `int`. Đọc code thấy `= delete` là thấy ngay ý đồ người thiết kế.
> - Delegating ctor giúp gom validation về một chỗ; nếu thấy 3 ctor cùng lặp một đoạn kiểm tra, refactor về một ctor "gốc".

### 4. Member initializer list

```cpp
class CauHinh {
    const int ma_;
    int& thanh_ghi_;
public:
    CauHinh(int ma, int& thanh_ghi) : ma_(ma), thanh_ghi_(thanh_ghi) {}
};
```

Initializer list (`: ma_(ma)`) khởi tạo member trực tiếp, trước thân constructor. **Nó bắt buộc cho**:

- `const` member
- reference member
- base class
- member type không có default constructor

Gán trong thân ctor không tương đương: member đã được default-initialize trước rồi mới assignment. Với object nặng, đây là một lần làm việc thừa.

**Thứ tự khởi tạo luôn là** base class → member theo **thứ tự khai báo trong class** → thân ctor. Nó không theo thứ tự bạn viết trong initializer list. Hãy khai báo và viết init-list cùng thứ tự để tránh `-Wreorder`.

> **Kinh nghiệm thực chiến (senior)**
> - Bug thực tế hay gặp: `Buffer(size_t n) : data_(new int[size_]), size_(n)` — `data_` khai báo trước `size_` nên dùng `size_` khi nó còn rác. Chạy được trên máy dev, crash trên target. Luôn bật `-Werror=reorder` trong CI.
> - Member phụ thuộc member khác là mùi thiết kế; nếu buộc phải có, thêm comment `// PHU THUOC THU TU:` ngay tại khai báo để người sau không sắp xếp lại "cho đẹp".
> - Từ C++11, ưu tiên default value ngay tại khai báo (NSDMI: `int retry_ = 3;`) — mọi ctor tự nhận giá trị này, đỡ lặp init-list và đỡ quên khi thêm ctor mới.
> - Reference member khiến class không assign được và khó test; cân nhắc pointer non-null hoặc `std::reference_wrapper` nếu class cần copy/assign.

### 5. Destructor

Destructor có dạng `~T()`. Nó chạy khi object automatic rời scope, dynamic object bị `delete`, smart pointer reset, hoặc static object kết thúc chương trình.

```cpp
~FileRAII() {
    if (file_ != nullptr) {
        std::fclose(file_);
    }
}
```

Khi hủy, thứ tự ngược với lúc xây: thân destructor → members theo thứ tự ngược khai báo → base. Destructor không được ném exception. Một exception thứ hai trong lúc stack unwinding sẽ dẫn đến `std::terminate`.

Với base class dùng đa hình, destructor phải là `public virtual` hoặc `protected` non-virtual. Thực tế interface public thường dùng `virtual ~Base() = default;`.

> **Kinh nghiệm thực chiến (senior)**
> - Dtor là nơi tệ nhất để có logic phức tạp: nó chạy trong stack unwinding, trong shutdown, đôi khi sau khi hệ thống con khác đã chết. Dtor lý tưởng chỉ trả tài nguyên, không gọi service khác, không log qua đường có thể fail.
> - Case thật: dtor gửi gói "goodbye" qua UART — treo toàn bộ shutdown khi UART đã bị tắt clock trước đó. Cleanup có thứ tự phụ thuộc phải điều phối tường minh, đừng giấu trong dtor.
> - Nếu cleanup có thể fail và caller cần biết (flush file, close transaction), cung cấp hàm `close()` trả lỗi; dtor chỉ là lưới an toàn gọi `close()` best-effort.
> - Đừng viết dtor rỗng `~T() {}` — nó user-declared, tắt implicit move của class. Hoặc không viết gì, hoặc `= default` kèm lý do.

### Default và delete

```
class Buffer {
public:
    Buffer(const Buffer&) = delete; // cấm copy constuctor 
    Buffer& operator=(const Buffer&) = delete;// cấm copy assignment
};
```
khi nào nên cấm : nếu 2 đối tượng cùng trỏ đến một resource thì cần cấm tránh deconstructor 2 lần 
delete còn để báo hiệu không cho dùng sai tham số trong overloading

### 6. `this` pointer

Trong non-static member function, `this` là con trỏ tới object hiện tại. Trong hàm `const`, kiểu hiệu dụng là `const T* const`, nên không thể sửa data member non-`mutable`.

```cpp
DongCoDC& datTocDo(int toc_do) {
    this->toc_do_ = toc_do;
    return *this;
}
```

`this->` hữu ích khi parameter trùng tên member hoặc trong template. `return *this` trả chính object, cho phép chain `dong_co.datTocDo(100).datTocDo(200)`. Static member function không có `this`, vì nó thuộc class chứ không thuộc một object.

> **Kinh nghiệm thực chiến (senior)**
> - Bug khó nhất liên quan `this` là callback: đăng ký `this` vào ISR/timer/event loop rồi object chết trước khi callback bị hủy đăng ký → dangling `this`, crash ngẫu nhiên. Quy tắc: ai đăng ký thì dtor của nó phải hủy đăng ký, hoặc dùng token/`weak_ptr`.
> - Fluent API (`return *this`) đẹp cho builder/config, nhưng đừng lạm dụng cho hàm có side effect quan trọng — chain dài che mất điểm fail ở giữa.
> - Trong lambda, `[this]` capture con trỏ chứ không copy object; với code async hãy tự hỏi "object còn sống khi lambda chạy không?". C++17 có `[*this]` để copy khi cần.

### 7. Const data và const member function

```cpp
class BoDem {
    int gia_tri_ = 0;
    mutable int so_lan_doc_ = 0;
public:
    int giaTri() const { ++so_lan_doc_; return gia_tri_; }
};
```

- `int giaTri() const` cam kết không thay đổi observable state của object. nên dùng cho getter
- `const T&` chỉ gọi được const member function.
- `mutable` cho phép thay đổi dữ liệu không thuộc logical state, ví dụ cache hoặc mutex. Dùng tiết chế.
- Const data member phải được khởi tạo bằng initializer list và làm assignment operator mặc định khó dùng hơn. Thường `const` API quan trọng hơn const data member.

Có thể overload theo constness:

```cpp
T& operator[](std::size_t i);
const T& operator[](std::size_t i) const;
```

> **Kinh nghiệm thực chiến (senior)**
> - Const correctness phải làm từ ngày đầu. Thêm `const` vào codebase cũ là "const lan truyền": sửa một hàm kéo theo hàng chục hàm khác, nên nhiều dự án bỏ cuộc. Mọi member function mới viết `const` trước, bỏ đi khi compiler bắt.
> - `const` là tài liệu đáng tin nhất: reviewer thấy `int doc() const` là biết hàm không đổi state mà không cần đọc body. Comment có thể nói dối, `const` thì không.
> - `mutable` hợp lệ cho mutex và cache, nhưng khi cache `mutable` xuất hiện thì hàm `const` không còn thread-safe hiển nhiên — hai thread cùng gọi getter "read-only" vẫn đua nhau ghi cache. Đi kèm `mutable` cache thường phải có `mutable std::mutex`.
> - Nhận tham số bằng `const T&` mặc định với type to hơn 2 thanh ghi; nó cũng ép bạn viết API const-correct từ phía class.

### 8. Static data và static member function

Static member thuộc về class, không thuộc một object.

```cpp
class KetNoi {
    inline static int so_dang_song_ = 0; // C++17
public:
    static int soDangSong() { return so_dang_song_; }
};
```

Trước C++17, static data member thường cần một definition ngoài class. `inline static` C++17 tránh lỗi linker đó. Static function không có `this`, chỉ truy cập được static member trực tiếp. Static-local trong function được khởi tạo một lần, thread-safe từ C++11, nhưng cần cân nhắc chi phí/đồng bộ trên embedded.

> **Kinh nghiệm thực chiến (senior)**
> - Static mutable data là global variable đội lốt class: nó phá unit test (test sau thấy state của test trước) và phá multi-instance (ngày nào đó cần 2 UART thay vì 1). Trước khi thêm static counter/config, hỏi "tại sao đây không phải member của một object được inject?".
> - Singleton qua static-local (`static T inst; return inst;`) tiện nhưng trên bare-metal, guard thread-safe của compiler có thể kéo theo code đồng bộ không mong muốn; một số dự án firmware bật `-fno-threadsafe-statics` và tự đảm bảo init một lần.
> - Static member function tốt cho named constructor (`static Ket_qua taoTuFlash()`) và stateless helper gắn với class; nếu hàm static không đụng gì tới class, chuyển thành free function trong namespace.
> - Debug tip: static data không hiện trong watch của object → khi dump state để phân tích lỗi field, static hay bị bỏ sót.

### 9. Object lifetime và storage duration

| Storage duration | Ví dụ | Khi bắt đầu/kết thúc |
|---|---|---|
| Automatic | local variable | vào/rời scope |
| Dynamic | `new`, `unique_ptr` | cấp phát đến khi owner giải phóng |
| Static | global, `static` local | trước `main` đến sau `main` |
| Thread | `thread_local` | theo thread |

```text
Xây: base -> member 1 -> member 2 -> than ctor
Hủy: than dtor -> member 2 -> member 1 -> base
```

Temporary thường chết cuối full-expression. Binding tạm vào `const T&` local có thể kéo dài lifetime, nhưng đừng trả reference tới temporary/local object.

Tránh global object có dependency lẫn nhau: thứ tự init giữa các translation unit không được bảo đảm (*static initialization order fiasco*). Firmware còn cần startup code hỗ trợ `.init_array` cho global ctor.

```
void func()
{
    int a = 10;

    {
        Buffer b(1000);
        // b sống
    } // ← b chết ở đây

    // b không còn tồn tại
}


void func()
{
    auto p = std::make_unique<Buffer>(1000);

} // unique_ptr chết
  // → delete Buffer tự động

STACK                       HEAP

┌─────────────┐             ┌───────────────┐
│ unique_ptr p│ ──────────► │ Buffer        │
└─────────────┘             └───────────────┘
      │
 automatic                     dynamic  

 thread duration 
 thread_local int counter = 0;
```

> **Kinh nghiệm thực chiến (senior)**
> - Lỗi lifetime kinh điển trong sản phẩm: trả `const std::string&` từ hàm tạo string tạm, lưu `string_view`/pointer vào container đã bị resize, giữ reference vào phần tử `vector` rồi `push_back`. Cả ba đều chạy "được" cho tới ngày dữ liệu lớn hơn — luôn chạy ASan/UBSan trong CI cho phần code host-testable.
> - Static initialization order fiasco từng làm hệ thống log crash trước `main`: logger global dùng config global ở file khác, thứ tự link thay đổi sau khi thêm file mới. Fix chuẩn: hàm `logger()` trả static-local, hoặc init tường minh trong `main()`.
> - Trên firmware, quy ước dễ sống: mọi object tầng hệ thống được tạo và wire trong một hàm `khoiTaoHeThong()` theo thứ tự viết tay — thứ tự nhìn thấy được, không phụ thuộc linker.
> - Khi nghi ngờ lifetime, thêm log vào ctor/dtor như demo `TheoDoiDoiSong` — rẻ và trả lời ngay "ai chết trước ai".

### 10. Copy constructor, copy assignment, shallow và deep copy

Copy constructor tạo object mới. Copy assignment ghi đè object đã tồn tại.

```cpp
T b(a); hoặc  T b = a;     // copy constructor
b = a;        // copy assignment
```

**Shallow copy** chỉ copy pointer/handle. Hai object cùng “sở hữu” một buffer sẽ double-free/double-close.

```text
Shallow: a.ptr ----+--> [buffer]
         b.ptr ----+

Deep:    a.ptr --------> [buffer A]
         b.ptr --------> [buffer B]
```

**Deep copy** cấp phát resource mới và copy nội dung. Với raw owner, copy assignment an toàn thường tạo bản tạm trước rồi `swap`:

```cpp
T& operator=(const T& rhs) {
    if (this != &rhs) {
        T temp(rhs);       // nếu ném, *this chưa đổi
        swap(temp);
    }
    return *this;
}

class Buffer {
private:
    int* data;
    std::size_t size;

public:
    Buffer(std::size_t n)
        : data(new int[n]), size(n) {}

    Buffer(const Buffer& other)
        : data(new int[other.size]),
          size(other.size) {
        std::copy(
            other.data,
            other.data + size,
            data
        );
    }

    ~Buffer() {
        delete[] data;
    }
};
```

Đây là **copy-and-swap**, cho strong exception guarantee. Demo dùng `ShallowView` non-owning để chỉ in địa chỉ chung an toàn; không chạy double-free có chủ ý.

> **Kinh nghiệm thực chiến (senior)**
> - Double-free do copy ngầm hiếm khi lộ tại chỗ copy; nó nổ ở dtor, cách xa hàng nghìn dòng. Khi viết class ôm raw handle, việc ĐẦU TIÊN là quyết định copy: `= delete` hay deep copy — đừng để compiler quyết hộ.
> - Copy đắt bị giấu là kẻ ăn CPU thầm lặng: truyền `std::vector` by value vào hàm gọi trong vòng lặp, `for (auto item : container)` thiếu `&`. Profile một firmware từng thấy 30% CPU nằm ở copy không chủ ý kiểu này. hãy lưu ý truyền reference của class kể cả `for (auto & item : container)`
> - Phân biệt rõ class **owning** và **viewing** ngay trong tên (`Buffer` vs `BufferView`); trộn lẫn hai vai là nguồn use-after-free khi view sống lâu hơn owner.
> - Kiểm tra `if (this != &rhs)` không chỉ là hình thức: self-assignment thật sự xảy ra qua alias (`a[i] = a[j]` khi `i == j`, hai reference cùng object qua hai đường gọi).

### 11. Move constructor và move assignment

Move chuyển ownership từ rvalue thay vì copy resource.

```cpp
Buffer(Buffer&& other) noexcept
    : data_(other.data_) {
    other.data_ = nullptr;
}
```

Sau move, source phải ở trạng thái **valid but unspecified**: có thể hủy, gán lại, gọi API có precondition phù hợp; không nên dựa vào giá trị cũ. Đánh dấu move/swap `noexcept` khi đúng vì container như `std::vector` ưu tiên move `noexcept` lúc reallocate.

`std::move(x)` không tự di chuyển dữ liệu, chỉ cast `x` thành rvalue. Move thực sự chỉ xảy ra khi overload move được chọn. Dùng `std::move` lên `const` object thường vẫn copy.

> **Kinh nghiệm thực chiến (senior)**
> - Quên `noexcept` trên move là lỗi hiệu năng vô hình: `std::vector` sẽ COPY toàn bộ phần tử khi reallocate thay vì move. Kiểm nhanh bằng `static_assert(std::is_nothrow_move_constructible_v<T>);` ngay dưới class — vừa là test vừa là tài liệu.
> - `std::move` lên `const` object compile êm ru nhưng âm thầm copy — từng thấy trong code review một hàm "tối ưu bằng move" mà tham số là `const T&`. Reviewer nên grep `std::move` và soi kiểu nguồn.
> - Đừng `return std::move(local);` — nó chặn RVO và thường chậm hơn `return local;`.
> - Dùng lại object sau move là bug logic phổ biến: an toàn duy nhất là hủy hoặc gán giá trị mới. Nếu hàm lấy ownership, nhận tham số by value rồi move vào member — chữ ký hàm tự nói "tôi lấy luôn".

### 12. Rule of 3, Rule of 5 và Rule of 0

| Quy tắc | Khi áp dụng |
|---|---|
| Rule of 3 | Nếu tự viết destructor, copy ctor hoặc copy assignment cho raw resource, cần xem xét cả ba |
| Rule of 5 | C++11 bổ sung move ctor và move assignment |
| Rule of 0 | Không tự quản lý raw resource: dùng member RAII rồi để compiler sinh special members |

Nếu định nghĩa hoặc `= delete` copy/move/destructor, hãy chủ động định nghĩa hoặc xóa toàn bộ special members liên quan. User-declared destructor có thể làm compiler không sinh move ngầm như bạn mong đợi.

Ưu tiên Rule of 0. `std::vector`, `std::string`, `std::array`, `std::unique_ptr` đã quản lý lifetime đúng; class chứa chúng thường không cần viết destructor/copy/move.

> **Kinh nghiệm thực chiến (senior)**
> - Trong một codebase trưởng thành, 95% class nên là Rule of 0. Mỗi class Rule of 5 là một điểm cần review kỹ và test riêng — hãy gom chúng vào vài wrapper nhỏ (kiểu `UniqueHandle`) rồi mọi class khác compose từ đó.
> - Bẫy khi maintain: thêm một raw pointer member vào class Rule of 0 đang chạy tốt mà quên rằng copy/move compiler sinh giờ đã sai. Checklist khi thêm member: "member này copy/move mặc định có đúng nghĩa không?".
> - Khai báo dtor (kể cả `= default`) tắt implicit move — class bỗng chậm đi mà không ai đổi logic. Nếu cần virtual dtor cho base, default cả 5 special member như hướng dẫn của Core Guidelines.
> - Trò chơi "đếm dòng" khi review rất hiệu quả: bản `std::vector` của `Buffer` ngắn hơn ~40 dòng so với bản raw pointer và không có nhánh lỗi nào để test.

### 13. RAII

```text
Khong RAII:                      RAII:
FILE* f = fopen(...);            { FileRAII f(...);
if (loi) return; // leak            if (loi) return; // dtor dong file
fclose(f);                       }
```

RAII giải phóng resource trên mọi đường rời scope: return, `break`, exception. Nó áp dụng cho memory, file, socket, mutex, lock, peripheral clock và critical section.

`FileRAII` cấm copy vì một `FILE*` chỉ có một owner. `IrqLock` cấm copy để chắc chắn chỉ object tạo lock mới mở lock. `std::lock_guard` là ví dụ chuẩn RAII cho mutex.

> **Kinh nghiệm thực chiến (senior)**
> - Dấu hiệu code thiếu RAII: hàm có `goto cleanup`, hoặc mỗi `return` lặp lại 3 dòng giải phóng. Mỗi lần thêm early-return mới là một cơ hội quên cleanup — RAII xóa cả lớp bug này, không phải từng bug.
> - Bug thật đã gặp: hàm giữ mutex có 4 đường return, người sau thêm đường thứ 5 và quên `unlock` → deadlock chỉ xuất hiện dưới tải. Sau sự cố, rule của team: cấm lock/unlock tay, bắt buộc `lock_guard`.
> - Guard tên phải được đặt tên: `std::lock_guard<std::mutex>{m};` (temporary) chết ngay cuối câu lệnh — khóa được đúng 0 dòng code. Luôn `std::lock_guard<std::mutex> lock(m);`.
> - RAII cho critical section (`IrqLock`) nên giữ scope càng hẹp càng tốt: mở block `{}` riêng quanh đúng đoạn cần bảo vệ thay vì để guard sống hết hàm, tránh tắt ngắt dài làm trượt deadline ISR khác.

### 14. Composition, inheritance và polymorphism

- **Composition** là quan hệ *has-a*: `Xe` có một `DongCo`. Đây là lựa chọn mặc định vì ownership rõ và giảm coupling.
chứa object, chưa referece object chứa contro objetc 

- **Inheritance** là *is-a*: `CamBien` là một `IThietBi`. Chỉ dùng khi derived thực sự thay thế được base theo Liskov Substitution Principle.
- **Virtual function** dispatch runtime qua base reference/pointer. Luôn viết `override`; dùng `final` khi không cho override/derive thêm.
- **Abstract class** có ít nhất một pure virtual function (`virtual void f() = 0;`) và không tạo object trực tiếp.
không gọi virtula fucntion trong constructor 
không nên kế thừa nhiều tầng tăng coupling 
tạo composition chỉ kết thừ khi lớp con thay thế hoàn toàn lớp cha, nhưng nơi truyền vào lớp cha * hoặc & có hoạt động đúng với lớp con không ? 

vector object nên để là unique_tr thay vì object hoặc con trỏ 

final ngăn overide / ngăn kế thừa 
vitural tránh việc kế thừa hình thoi 

```cpp
class IThietBi {
public:
    virtual ~IThietBi() = default;
    virtual void khoiDong() const = 0;
};
```

**Virtual destructor** là bắt buộc nếu xóa derived object qua `Base*` hoặc `std::unique_ptr<Base>`. Không gọi virtual function để dựa vào override trong ctor/dtor: lúc đó phần derived chưa xây xong hoặc đã hủy.

**Object slicing** xảy ra khi copy `Derived` vào `Base` by value; phần derived bị cắt. Nhận `const Base&` hoặc `Base*` cho đa hình.

**Multiple inheritance** có thể hợp lý khi kết hợp interface nhỏ, độc lập. Diamond tạo hai base subobject; `virtual` inheritance tạo một shared base nhưng tăng độ phức tạp/layout. Ưu tiên composition hoặc interface nhỏ. Xem Bài 06 để thực hành chi tiết hơn.

> **Kinh nghiệm thực chiến (senior)**
> - Câu hỏi quyết định is-a: "mọi hàm nhận `Base&` có chạy đúng khi đưa `Derived` vào không?". Ví dụ kinh điển fail: `HinhVuong : HinhChuNhat` — set width khác height phá invariant của vuông. Không chắc chắn → composition.
> - Hierarchy sâu quá 2-3 tầng gần như luôn hối hận: sửa base là chạy regression toàn bộ derived. Interface phẳng (`IThietBi` + N implementation) dễ sống hơn cây kế thừa nhiều tầng.
> - Bug slicing hay lọt qua review vì trông vô hại: `std::vector<Hinh>` chứa `HinhTron` — mỗi phần tử đã bị cắt từ lúc `push_back`. Container đa hình phải là `vector<unique_ptr<Hinh>>`. C++ Core Guidelines còn khuyên delete copy của polymorphic class để compiler bắt hộ.
> - Luôn viết `override`: khi base đổi chữ ký (thêm `const`, đổi tham số), mọi override cũ thành hàm mới im lặng — `override` biến bug runtime thành lỗi compile. Bật thêm `-Wsuggest-override` nếu toolchain hỗ trợ.
> - Multiple inheritance trong thực tế firmware chủ yếu ổn ở dạng "1 base có data + N interface thuần ảo". Diamond có data hầu như luôn là dấu hiệu cần thiết kế lại thay vì thêm `virtual` inheritance.

### 15. Class template

```cpp
template <typename T, std::size_t N>
class Stack {
    std::array<T, N> data_{};
    std::size_t size_ = 0;
};
```

`Stack<int, 4>` và `Stack<std::string, 4>` là hai type khác nhau. `T` là type parameter, `N` là non-type template parameter. Static member của mỗi instantiation cũng riêng biệt.

Compiler cần nhìn thấy definition của template tại điểm instantiate, nên class-template implementation thường nằm trong header. Template tạo đa hình compile-time, thường không có vtable nhưng có thể gây code bloat nếu instantiate nhiều kiểu. Xem Bài 09 để học specialization, variadic template và deduction.

> **Kinh nghiệm thực chiến (senior)**
> - Code bloat template là chuyện có thật trên MCU: `RingBuffer<uint8_t, 64>`, `<uint8_t, 128>`, `<uint16_t, 64>`... mỗi tổ hợp một bản mã đầy đủ. Kỹ thuật chuẩn: tách phần không phụ thuộc `T`/`N` vào base non-template, template chỉ còn lớp mỏng — soi `arm-none-eabi-nm --size-sort` để thấy hiệu quả.
> - Lỗi biên dịch template nổ ở nơi dùng, không phải nơi khai báo, và message dài hàng trang. Tự cứu mình bằng `static_assert` với message rõ ngay đầu class (`static_assert(N > 0, "Stack can dung luong > 0");`).
> - Đừng template hóa "phòng khi cần": nếu hiện tại chỉ có một kiểu dùng thật, viết class thường trước, template hóa khi có kiểu thứ hai. Template hóa sớm trả giá bằng thời gian build và khó debug.
> - Non-type parameter (`N`) đưa cấu hình về compile-time: sai kích thước là lỗi build chứ không phải overflow lúc 2h sáng — rất hợp khẩu vị firmware, dùng thay `#define BUFFER_SIZE`.

### 16. Smart pointer trong class

- `std::unique_ptr<T>`: sole ownership, move-only, là lựa chọn mặc định cho heap ownership.
- `std::shared_ptr<T>`: shared ownership, có control block và overhead atomic tiềm năng.
- `std::weak_ptr<T>`: reference không sở hữu, dùng để phá **cycle** của `shared_ptr`.

```cpp
class HaiTaiNguyen {
    std::unique_ptr<int> mot_;
    std::unique_ptr<int> hai_;
public:
    HaiTaiNguyen()
        : mot_(std::make_unique<int>(1)), hai_(std::make_unique<int>(2)) {}
};
```

Nếu khởi tạo resource thứ hai ném, `mot_` tự dọn. Class này tuân Rule of 0; compiler tự tạo destructor/move đúng và cấm copy vì `unique_ptr` không copy được. Xem Bài 11 cho ownership graph và circular reference.

> **Kinh nghiệm thực chiến (senior)**
> - Chọn smart pointer là chọn thiết kế: `unique_ptr` mặc định; thấy `shared_ptr` trong review thì hỏi "ai là owner thứ hai và tại sao?". Đa số `shared_ptr` trong code thực tế là "không nghĩ kỹ ai own" chứ không phải shared ownership thật.
> - Memory leak khó nhất tôi từng truy là cycle `shared_ptr`: `ThietBi` giữ `shared_ptr<Callback>`, callback capture `shared_ptr<ThietBi>` — cả hai bất tử, heap tăng dần qua nhiều ngày chạy. Quan hệ ngược (child → parent, observer → subject) mặc định dùng `weak_ptr` hoặc raw pointer non-owning có tài liệu lifetime.
> - `get()` chỉ để truyền xuống API không sở hữu; thấy `delete ptr.get()` hay `unique_ptr` thứ hai tạo từ `get()` là bug chắc chắn. Truyền tham số: hàm chỉ dùng → nhận `T&`/`T*`; hàm lấy ownership → nhận `unique_ptr<T>` by value.
> - Trên MCU: `unique_ptr` là zero-overhead so với raw pointer + delete tay, dùng thoải mái kể cả bare-metal; `shared_ptr` kéo theo control block và atomic — cân nhắc kỹ.

### 17. Exception safety và RAII

Ba mức guarantee thường dùng:

| Mức | Cam kết |
|---|---|
| Nothrow | Không ném exception |
| Strong | Thành công hoàn toàn hoặc state cũ không đổi |
| Basic | Invariant vẫn đúng, không leak, nhưng state có thể thay đổi |

RAII trước hết giúp **basic guarantee**: resource không leak khi exception unwind stack. Copy-and-swap hay xây object mới trước khi commit thường mang **strong guarantee**. Đừng dùng destructor để ném lỗi cleanup; log hoặc lưu trạng thái lỗi thay thế.

Nhiều firmware tắt exception với `-fno-exceptions`. RAII vẫn hoạt động cho normal scope exit, nhưng ctor cần báo lỗi qua `ok()`, error code, `std::optional`, hoặc factory function.

> **Kinh nghiệm thực chiến (senior)**
> - Đặt câu hỏi review theo mức guarantee: "hàm này ném giữa chừng thì object ở trạng thái gì?". Nếu không ai trả lời được, hàm chưa đạt cả basic guarantee. Đa số hàm chỉ cần basic; strong chỉ đáng giá ở thao tác "commit" (ghi config, apply firmware update).
> - Mẫu strong guarantee dễ nhớ: làm mọi việc có thể fail trên bản nháp/biến tạm trước, các bước cuối chỉ còn thao tác nothrow (swap, gán pointer). Copy-and-swap là trường hợp riêng của mẫu này.
> - Quyết định exception on/off phải chốt sớm cấp dự án: code viết cho exception (báo lỗi bằng throw trong ctor) chuyển sang `-fno-exceptions` là đập đi làm lại API báo lỗi. Team firmware thường chốt `-fno-exceptions` + factory trả `optional`/status ngay từ kiến trúc.
> - Đừng `catch (...)` rồi nuốt lỗi cho "ổn định": hệ thống chạy tiếp với state hỏng còn tệ hơn restart sạch. Catch ở boundary, log đủ ngữ cảnh, rồi recover hoặc reset có chủ đích.

### 18. Memory layout, padding, vptr và vtable

Member thường được đặt theo thứ tự khai báo, có padding để thỏa alignment.

```text
PlainLayout { char; int; double; }
[char][padding 3][int 4][double 8]  -> thuong la 16 byte
```

- Empty class/struct có kích thước ít nhất 1 byte để các object có địa chỉ khác nhau.
- Layout chính xác, padding và thứ tự base subobject phụ thuộc ABI/compiler; không serialize raw class bytes trừ khi type phù hợp và giao thức quy định rõ.
- Với class có virtual function, ABI phổ biến thêm hidden **vptr** vào object; vptr trỏ tới table dùng dispatch (**vtable**) thường ở read-only memory. C++ standard không bắt buộc cách triển khai này hay vị trí vptr.
- Mỗi polymorphic/multiple-inheritance object có thể lớn hơn do vptr/base layout. Chi phí này quan trọng trên MCU có RAM nhỏ.

Demo in `sizeof`, offset của `PlainLayout`, và word đầu của object polymorphic như quan sát ABI. Không suy ra portable protocol hay tự sửa vptr từ kết quả đó. Xem Bài 06 và Bài 19 để phân tích sâu hơn.

> **Kinh nghiệm thực chiến (senior)**
> - Bug giao thức kinh điển: hai bên "cùng một struct" nhưng khác packing/endianness → lệch field âm thầm. Struct đi qua UART/CAN/flash phải dùng fixed-width type, serialize tường minh (hoặc quy ước packing được cả hai bên kiểm chứng), và chốt bằng `static_assert(sizeof(Frame) == 12);` — thay compiler/flags là CI báo ngay.
> - Trên MCU RAM nhỏ, sắp member từ lớn xuống nhỏ tiết kiệm thật: struct `{u8, u32, u8, u32}` tốn 16 byte, sắp lại `{u32, u32, u8, u8}` còn 12 — nhân với mảng 1000 phần tử là 4KB. Xem nhanh bằng `-Wpadded` hoặc pahole.
> - Thêm virtual function ĐẦU TIÊN vào class đang dùng trong mảng lớn làm mỗi object phình thêm 1 pointer và đổi layout — từng phá cả vùng flash config được ghi theo layout cũ. Class dùng để lưu trữ/truyền tin thì giữ non-polymorphic.
> - Crash "nhảy vào địa chỉ rác khi gọi virtual" gần như luôn là use-after-free/memory corruption ghi đè vptr, không phải bug của compiler. Gặp nó thì bật sanitizer/soi ai ghi đè object, đừng đi đọc vtable.

## Cách dùng

Ví dụ raw owner theo Rule of Five.

```cpp
class Buffer {
    std::size_t size_ = 0;
    int* data_ = nullptr;
public:
    explicit Buffer(std::size_t n) : size_(n), data_(n ? new int[n]{} : nullptr) {}
    ~Buffer() { delete[] data_; }

    Buffer(const Buffer& rhs) : Buffer(rhs.size_) {
        std::copy(rhs.data_, rhs.data_ + size_, data_);
    }
    Buffer& operator=(const Buffer& rhs) {
        if (this != &rhs) { Buffer temp(rhs); swap(temp); }
        return *this;
    }
    Buffer(Buffer&& rhs) noexcept : size_(rhs.size_), data_(rhs.data_) {
        rhs.size_ = 0; rhs.data_ = nullptr;
    }
    Buffer& operator=(Buffer&& rhs) noexcept {
        if (this != &rhs) { delete[] data_; size_ = rhs.size_; data_ = rhs.data_;
                            rhs.size_ = 0; rhs.data_ = nullptr; }
        return *this;
    }
    void swap(Buffer& rhs) noexcept { std::swap(size_, rhs.size_); std::swap(data_, rhs.data_); }
};
```

Phiên bản ưu tiên Rule of 0 đơn giản hơn.

```cpp
class BufferTotHon {
    std::vector<int> data_;
public:
    explicit BufferTotHon(std::size_t n) : data_(n) {}
};
```

Chỉ tự viết Rule of Five khi class thật sự phải trực tiếp quản lý raw handle/resource và standard RAII type không phù hợp.

## Tips & Tricks

- Khai báo member theo thứ tự alignment lớn đến nhỏ khi layout/RAM quan trọng, nhưng ưu tiên tính đúng và đọc được trước.
- Constructor một tham số nên `explicit`; virtual override nên `override`; không cho kế thừa thêm thì dùng `final`.
- Public polymorphic base nên có virtual destructor. Không lưu polymorphic object by value.
- Dùng `std::make_unique` thay cho `new` trực tiếp trong ứng dụng hiện đại.
- Đánh dấu move ctor, move assignment và `swap` là `noexcept` nếu toàn bộ thao tác thực sự không ném.
- Không gọi virtual API dựa vào derived state trong ctor/dtor.
- Template nên ở header; giảm số tổ hợp `T`/`N` trên firmware để giới hạn code bloat.

## Lỗi thường gặp / Bẫy

1. Public data làm client phá invariant.
2. Viết initializer list sai thứ tự member rồi hiểu nhầm thứ tự khởi tạo.
3. Gán trong thân ctor cho `const`/reference member.
4. Copy raw pointer owner theo mặc định dẫn tới double-free.
5. Quên self-assignment hoặc exception safety trong copy assignment.
6. Moved-from object bị dùng như còn giữ giá trị cũ.
7. Khai báo destructor nhưng không xem lại implicit move operations.
8. Xóa object derived qua base pointer không virtual destructor.
9. Truyền polymorphic object by value gây slicing.
10. Dùng `shared_ptr` mặc định thay cho `unique_ptr`, hoặc tạo cycle `shared_ptr`.
11. Ném exception từ destructor.
12. Giả định vtable/vptr và raw memory layout là portable C++ ABI.

## Ghi chú Embedded

- RAII phù hợp cho guard của chip-select, DMA transaction, clock gate, mutex/critical section. Đảm bảo destructor không thực hiện thao tác blocking trong ISR.
- Polymorphism runtime tốn RAM cho vptr và flash cho vtable/code dispatch. Khi số loại cố định, template/CRTP hoặc `std::variant` có thể phù hợp hơn.
- Tránh heap hoặc `shared_ptr` trên hệ thống không kiểm soát fragmentation/atomic overhead; `unique_ptr` hoặc owner static rõ ràng thường dễ phân tích hơn.
- Global ctor/dtor có thể chưa được startup code firmware gọi đúng. Hạn chế global object có logic, dùng explicit initialization khi cần trình tự xác định.
- Với `-fno-rtti`, `dynamic_cast`/`typeid` có hạn chế. Với `-fno-exceptions`, thiết kế API trả status rõ ràng nhưng vẫn dùng RAII.
- Không dùng class polymorphic/non-standard-layout để map trực tiếp thanh ghi MCU hoặc CAN/UART frame. Dùng fixed-width integer và serialization rõ ràng.

## Bài tập tự luyện

1. Viết `GpioGuard`: ctor cấu hình pin output, dtor trả pin về trạng thái an toàn. Cấm copy, cho phép move nếu ownership chuyển được.
2. Viết `Array<T, N>` không cấp phát heap, có overload `operator[]` const/non-const và kiểm tra full/empty.
3. Viết `Buffer` Rule of Five, thêm counter copy/move; sau đó thay raw pointer bằng `std::vector` và so sánh số special member cần viết.
4. Tạo `ITransport` với `UartTransport` và `SpiTransport`; lưu bằng `std::unique_ptr<ITransport>`. Thử truyền by value để quan sát slicing.
5. Đo `sizeof` một struct layout khác nhau, một class virtual và diamond có/không virtual inheritance trên compiler của bạn. Giải thích tại sao số đo không phải portable guarantee.
6. Viết class quản lý hai resource; mô phỏng lỗi khi resource thứ hai không tạo được và chứng minh không rò resource bằng RAII.

## Tóm tắt

- Class bảo vệ invariant qua `private` state và public API; ctor xây state hợp lệ, dtor trả resource.
- Init-list khởi tạo trực tiếp và tuân theo thứ tự khai báo member.
- `const`, `this`, static member và access control xác định interface chính xác của object.
- Copy raw owner cần deep copy; move chuyển ownership; ưu tiên Rule of 0, chỉ dùng Rule 3/5 khi thật cần.
- RAII là nền tảng cleanup đúng trên return sớm và exception.
- Composition là mặc định; inheritance chỉ cho quan hệ is-a; polymorphic base cần virtual destructor và tránh slicing.
- Template cung cấp đa hình compile-time; smart pointer giúp ownership rõ và exception-safe.
- Vptr/vtable là implementation phổ biến nhưng không được standard đảm bảo; luôn cân nhắc RAM/flash/ABI trên embedded.
