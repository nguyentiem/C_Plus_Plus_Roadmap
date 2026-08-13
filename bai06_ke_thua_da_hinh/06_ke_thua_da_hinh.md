# Bài 06: Kế thừa & Đa hình (Inheritance & Polymorphism)

## Định nghĩa & Khái niệm

- **Kế thừa (inheritance)**: cơ chế cho phép một lớp (derived/lớp con) tái sử dụng và mở rộng
  giao diện + hiện thực của một lớp khác (base/lớp cha). Quan hệ "is-a" (là một).
- **Đa hình (polymorphism)**: cùng một lời gọi hàm nhưng hành vi khác nhau tùy vào kiểu
  *thực tế* của đối tượng tại runtime. Trong C++ đạt được qua **virtual function**.
- **Virtual function**: hàm thành viên được đánh dấu `virtual`, cho phép lớp con override
  và được gọi qua con trỏ/tham chiếu lớp cha (dynamic dispatch).
- **Pure virtual** (`= 0`): hàm không có hiện thực bắt buộc lớp con phải override.
  Lớp chứa ít nhất một pure virtual là **abstract class** — không thể tạo object trực tiếp.
- **Interface**: abstract class chỉ chứa pure virtual + virtual destructor (C++ không có
  từ khóa `interface` riêng như Java).
- **vtable/vptr**: cơ chế phổ biến compiler dùng để hiện thực dynamic dispatch.

## Giải thích chi tiết

### 1. Ba mức kế thừa: public / protected / private

| Kiểu kế thừa | public của base thành | protected của base thành | Ý nghĩa |
|---|---|---|---|
| `public`    | public    | protected | "is-a" — dùng đa hình được |
| `protected` | protected | protected | chỉ lớp con thấy được |
| `private`   | private   | private   | "implemented-in-terms-of" |

**Tại sao**: chỉ `public` inheritance mới cho phép chuyển đổi ngầm `Derived*` → `Base*`.
Kế thừa `private` gần với composition hơn — che giấu hoàn toàn quan hệ với bên ngoài.
`private` của base **không bao giờ** truy cập được từ lớp con (dù kế thừa kiểu gì).

### 2. Cơ chế vtable/vptr

Khi một lớp có (hoặc kế thừa) virtual function, compiler:
1. Tạo **một bảng vtable duy nhất cho mỗi lớp** (mảng con trỏ hàm, đặt trong ROM/.rodata).
2. Thêm **một con trỏ ẩn vptr vào mỗi object** (thường ở đầu object), trỏ tới vtable
   của lớp thực tế. vptr được gán trong constructor.

```
   Base b;                      Derived d;
   +-----------+                +-----------+
   | vptr ─────┼──► vtable_Base | vptr ─────┼──► vtable_Derived
   | data...   |   [0] &Base::f | base data |   [0] &Derived::f   (override)
   +-----------+   [1] &Base::g | own data  |   [1] &Base::g      (kế thừa)
                                +-----------+   [2] &Derived::h   (mới)
```

Lời gọi `p->f()` với `Base* p` được dịch thành (giả mã):
`(*(p->vptr)[index_cua_f])(p);` — 2 lần đọc bộ nhớ + 1 indirect call.

**Chi phí runtime** (quan trọng với embedded):
- +1 pointer (4 byte trên Cortex-M) cho **mỗi object**.
- Mỗi lời gọi virtual: thêm ~2 lần load + indirect branch (cản trở branch prediction,
  và **không inline được** trừ khi compiler devirtualize).
- vtable chiếm flash. Với class nhỏ, overhead tương đối lớn.

### 3. Virtual destructor — tại sao bắt buộc

Nếu xóa object qua con trỏ base (`delete basePtr;`) mà destructor của base **không**
virtual → **undefined behavior**: destructor của lớp con không được gọi (rò rỉ tài nguyên),
và với multiple inheritance, địa chỉ truyền cho `operator delete` có thể sai.

Quy tắc: *lớp nào được thiết kế làm base đa hình thì destructor phải `virtual`*
(hoặc `protected` non-virtual nếu cấm delete qua base).

### 4. Object slicing (cắt xén đối tượng)

Gán/copy một `Derived` vào một biến `Base` **theo giá trị** → chỉ phần Base được copy,
phần dữ liệu của Derived bị "cắt" mất, vptr trở thành của Base → mất đa hình.

```cpp
void ham(Base b);      // SAI: slicing, mất đa hình
void ham(const Base& b); // ĐÚNG: tham chiếu giữ nguyên kiểu động
```

### 5. Multiple inheritance & diamond problem

```
      A            Kim cương: B và C cùng kế thừa A,
     / \           D kế thừa cả B và C
    B   C          → D chứa HAI bản sao của A (mơ hồ, tốn RAM)
     \ /
      D
```

Giải pháp: **virtual inheritance** — `class B : virtual public A`, `class C : virtual public A`.
Khi đó D chỉ chứa **một** bản A duy nhất; B và C giữ con trỏ/offset tới bản chung đó.
Chi phí: mỗi object thêm dữ liệu định vị (vbase offset), lời gọi phức tạp hơn,
và **lớp dẫn xuất cuối cùng (most-derived) chịu trách nhiệm gọi constructor của A**.

### 6. Composition vs Inheritance

- Kế thừa = "is-a" (Motor **là** Device). Composition = "has-a" (Robot **có** Motor).
- Ưu tiên **composition** khi chỉ cần tái sử dụng code: ít ràng buộc, không phơi bày
  giao diện base, dễ thay đổi. Kế thừa public chỉ khi thực sự cần đa hình/thay thế
  (Liskov Substitution Principle: chỗ nào dùng Base được thì dùng Derived cũng phải đúng).

## Cách dùng

```cpp
class Sensor {                      // interface
public:
    virtual ~Sensor() = default;    // BẮT BUỘC virtual destructor
    virtual int read() = 0;         // pure virtual
};

class TempSensor final : public Sensor {
public:
    int read() override { return 25; }  // luôn dùng 'override'
};

void log(Sensor& s) { /* dynamic dispatch */ (void)s.read(); }
```

## Tips & Tricks

- Luôn viết `override` khi ghi đè — compiler bắt lỗi sai chữ ký ngay lập tức.
- Dùng `final` cho lớp/hàm không cho override — giúp compiler **devirtualize** (gọi trực tiếp, inline được).
- Interface: `virtual ~I() = default;` + toàn pure virtual, không data member.
- Không gọi virtual function trong constructor/destructor — lúc đó vptr đang trỏ vào
  vtable của lớp *đang được khởi tạo*, không phải lớp cuối cùng.
- Truyền đối tượng đa hình bằng `Base&` hoặc `Base*`, không bao giờ theo giá trị.

## Lỗi thường gặp / Bẫy

1. **Quên virtual destructor** → UB khi `delete` qua con trỏ base.
2. **Object slicing** khi truyền theo giá trị hoặc chứa trong `std::vector<Base>`.
3. **Ghi đè sai chữ ký** (thiếu `const`, sai kiểu tham số) → tạo hàm mới thay vì override
   — `override` phát hiện lỗi này.
4. **Default argument với virtual**: default argument lấy theo **kiểu tĩnh** của con trỏ,
   còn thân hàm theo kiểu động → hành vi khó lường. Tránh dùng.
5. **Diamond không virtual inheritance** → hai bản base, gọi hàm mơ hồ.
6. Gọi virtual trong constructor → dispatch không như mong đợi.

## Ghi chú Embedded

- vptr + vtable tốn RAM/flash; với MCU nhỏ (nRF52840 có 256KB RAM thì thoải mái,
  nhưng driver nhiều instance nhỏ thì đáng cân nhắc).
- Indirect call cản trở tối ưu, không inline → tránh virtual trong ISR / vòng lặp nóng.
- Thay thế compile-time: **CRTP** (Curiously Recurring Template Pattern) hoặc
  `if constexpr` — đa hình tĩnh, zero overhead.
- Tuy nhiên virtual rất hữu ích cho HAL: `class IUart { virtual void write(...) = 0; }`
  cho phép mock khi unit test trên host — chi phí thường chấp nhận được.
- `-fno-rtti` (thường bật trên embedded) không ảnh hưởng virtual dispatch, chỉ tắt
  `dynamic_cast`/`typeid`.

## Bài tập tự luyện

1. Viết interface `IDisplay` với `virtual void draw(char c) = 0;` và hai lớp
   `ConsoleDisplay`, `BufferedDisplay`. Chứng minh dynamic dispatch bằng mảng `IDisplay*`.
2. Tạo diamond A→B,C→D không dùng virtual inheritance, in `sizeof(D)`; sau đó thêm
   `virtual` và so sánh kích thước + số lần constructor A được gọi.
3. Viết hàm nhận `Base` theo giá trị và theo `const Base&`, quan sát object slicing
   qua output của virtual function.

## Tóm tắt

- Kế thừa public = "is-a", cho phép đa hình; private inheritance ≈ composition.
- Virtual function → vtable (mỗi lớp) + vptr (mỗi object); chi phí: bộ nhớ + indirect call.
- Base đa hình **phải** có virtual destructor; luôn dùng `override`/`final`.
- Truyền đa hình bằng reference/pointer để tránh slicing.
- Diamond problem giải bằng virtual inheritance (có chi phí).
- Ưu tiên composition; chỉ kế thừa khi cần thay thế đa hình thật sự.
