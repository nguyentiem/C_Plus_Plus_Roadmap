# Bài 18: SOLID, Dependency Injection, PIMPL và CRTP trong C++ Embedded

## Định nghĩa & Khái niệm

- **SOLID**: 5 nguyên tắc thiết kế hướng đối tượng giúp code dễ sửa, dễ test, dễ mở rộng:
  - **S**ingle Responsibility: một lớp = một lý do để thay đổi.
  - **O**pen/Closed: mở để mở rộng, đóng để sửa đổi.
  - **L**iskov Substitution: lớp con thay được lớp cha mà không phá hợp đồng.
  - **I**nterface Segregation: nhiều interface nhỏ chuyên biệt thay vì một interface béo.
  - **D**ependency Inversion: module cấp cao phụ thuộc *abstraction*, không phụ thuộc chi tiết.
- **Dependency Injection (DI)**: "tiêm" phụ thuộc từ ngoài vào (qua constructor/tham số) thay vì tự tạo bên trong → thay thật bằng mock khi test.
- **PIMPL (Pointer to IMPLementation)**: giấu chi tiết hiện thực sau con trỏ opaque trong header → cắt phụ thuộc biên dịch, ổn định ABI.
- **CRTP (Curiously Recurring Template Pattern)**: `class Con : public Cha<Con>` — đa hình *tĩnh*, resolve lúc compile, không vtable.

## Giải thích chi tiết

### SOLID — vi phạm → sửa (mỗi nguyên tắc một ví dụ)

**S — SRP.** Vi phạm: lớp `BoGhiNhietDo` vừa đọc I2C, vừa format chuỗi, vừa ghi flash. Đổi định dạng log → sửa lớp sensor?! Sửa: tách `CamBien` / `DinhDangLog` / `BoLuu` — ba lý do thay đổi, ba lớp.

**O — OCP.** Vi phạm: `switch (loaiCamBien)` rải khắp codebase; thêm sensor mới phải sửa N chỗ. Sửa: interface `ICamBien` + factory (bài 17) — thêm sensor mới = thêm MỘT lớp mới, code cũ không đổi.

**L — LSP.** Vi phạm kinh điển: `HinhVuong : HinhChuNhat` — set chiều rộng làm đổi chiều cao, phá hợp đồng của lớp cha. Embedded: `UartKhongChan : IUart` mà `doc()` trả ngay 0 byte trong khi hợp đồng nói "block đến khi có dữ liệu" → caller vỡ. Bài học: kế thừa là kế thừa **hợp đồng hành vi**, không chỉ chữ ký hàm.

**I — ISP.** Vi phạm: `IThietBi` có 12 hàm (đọc, ghi, cấu hình DMA, ngủ, OTA...) — mock để test phải cài đủ 12. Sửa: tách `IDoc`, `IGhi`, `INguon` — mỗi client chỉ phụ thuộc phần nó dùng.

**D — DIP.** Vi phạm: `BoGiamSat` gọi thẳng `nrf_uarte_tx()` — logic nghiệp vụ dính chặt HAL, chỉ test được trên board. Sửa: `BoGiamSat` nhận `IUart&`; production tiêm `UartNrf52`, unit test tiêm `MockUart`. Đây chính là **DI cho testability** — kỹ thuật đáng giá nhất bài này:

```
   Production:                          Unit test (tren host, khong can board):
   ┌───────────┐    ┌────────────┐      ┌───────────┐    ┌───────────┐
   │ BoGiamSat │───►│  IUart     │◄─────│ BoGiamSat │───►│ MockUart  │
   └───────────┘    └─────┬──────┘      └───────────┘    │ (ghi lai  │
                          │                              │  moi byte)│
                    ┌─────┴──────┐                       └───────────┘
                    │ UartNrf52  │
                    └────────────┘
```

### PIMPL

```cpp
// driver.h — KHONG include header HAL nang ne
class Driver {
public:
    Driver(); ~Driver();
    void gui(int v);
private:
    struct Impl;                 // khai bao truoc, opaque
    std::unique_ptr<Impl> p_;    // moi chi tiet nam trong .cpp
};
```
Lợi: đổi ruột `.cpp` không recompile client; giấu phụ thuộc vendor SDK. Giá: một lần cấp phát heap + một lần gián tiếp con trỏ — cân nhắc trên MCU (biến thể: buffer tĩnh `aligned_storage`).

### CRTP — đa hình tĩnh

```cpp
template <typename D>
struct GpioBase {
    void bat()  { static_cast<D*>(this)->ghiMuc(true); }   // resolve compile-time
    void tat()  { static_cast<D*>(this)->ghiMuc(false); }
};
struct GpioNrf : GpioBase<GpioNrf> { void ghiMuc(bool m); };
```
So với virtual: không vtable pointer/đối tượng (tiết kiệm 4-8 byte/instance), không indirect call, **inline được** → với hàm 1-2 lệnh (bật GPIO) chênh lệch có thể 5-10 lần. Giá: mỗi driver là một *kiểu khác nhau* — không nhét chung vào mảng `Base*` được, code template phình binary nếu nhiều instantiation. Quy tắc: hot-path gọi triệu lần/giây → CRTP; cấu hình lúc boot, gọi thưa → virtual cho gọn.

## Cách dùng

Xem `main.cpp`: `IUart` + `MockUart` unit-test logic đóng gói frame không cần phần cứng; `GpioBase<D>` CRTP so với `IGpioAo` virtual.

## Tips & Tricks

- Constructor injection (tham chiếu/con trỏ) là dạng DI đơn giản nhất và đủ dùng 95% trường hợp — không cần "DI framework".
- Interface cho DI nên **nhỏ** (ISP): mock 2 hàm viết trong 10 dòng, mock 12 hàm không ai viết.
- Template parameter cũng là DI (compile-time): `template<class Uart> class BoGiamSat` — zero-cost, hợp CRTP.
- PIMPL: luôn định nghĩa destructor trong `.cpp` (nơi `Impl` hoàn chỉnh), nếu không `unique_ptr` không compile.
- Đừng thần thánh hoá SOLID: áp máy móc sinh ra lớp vụn và indirection thừa. Mục tiêu là *dễ đổi, dễ test*, không phải đếm interface.

## Lỗi thường gặp / Bẫy

1. **Interface trả về/nhận kiểu của vendor SDK** → abstraction rò rỉ, mock không viết nổi.
2. **LSP ngầm vỡ**: override ném exception/trả lỗi mà lớp cha hứa không bao giờ lỗi.
3. **CRTP gọi hàm chưa định nghĩa ở lớp con** → lỗi template khó đọc; C++20 concepts làm rõ hợp đồng.
4. **PIMPL quên `~Driver()` ở .cpp** → lỗi "incomplete type" khó hiểu từ `unique_ptr`.
5. **DI qua singleton toàn cục** — vẫn là phụ thuộc ẩn, test không thay được. Tiêm tường minh qua constructor.
6. **Mock quá thông minh** — mock chứa logic thì ai test mock? Mock chỉ ghi lại lời gọi + trả giá trị cài sẵn.

## Ghi chú Embedded

- DI + MockUart cho phép chạy unit test **trên host** (CI, không cần board) — nền tảng của TDD firmware; chỉ lớp HAL mỏng nhất cần test trên target.
- Cắt lớp chuẩn: `logic nghiệp vụ → interface HAL → HAL vendor (nRF SDK/Zephyr driver)`. Logic không bao giờ include header vendor.
- CRTP hợp driver GPIO/SPI hot-path; virtual hợp điểm cấu hình boot-time. Đo trước khi tối ưu.
- Zephyr device driver model bản chất là DIP bằng C: struct con trỏ hàm (`api`) = vtable thủ công.
- Trên MCU cấm heap: thay PIMPL bằng buffer tĩnh, hoặc chấp nhận lộ header (trade-off có ý thức).

## Bài tập tự luyện

1. Viết `ICamBienNhiet { doc() }` + lớp `BaoVeQuaNhiet` (ngắt relay khi >85°C, có hysteresis 5°C). Viết `MockCamBien` và 4 ca test: dưới ngưỡng, vượt ngưỡng, vùng hysteresis khi đang bật/đang tắt.
2. Lấy một lớp "God class" tự viết (đọc sensor + lọc + log + gửi UART) và refactor theo SRP + DIP thành 4 đơn vị, vẽ sơ đồ phụ thuộc trước/sau.
3. Viết `SpiBase<D>` CRTP với `ghiThanhGhi(addr, val)` dùng `D::chuyenByte()`; viết 2 driver giả và in kích thước đối tượng so với phiên bản virtual (`sizeof`).

## Tóm tắt

- SOLID = code dễ đổi và dễ test; DIP là nguyên tắc sinh lời nhất trong firmware.
- DI qua constructor + interface nhỏ (ISP) → unit test logic trên host với mock, không cần phần cứng.
- PIMPL cắt phụ thuộc biên dịch, giấu vendor SDK; nhớ destructor trong .cpp; cân nhắc heap trên MCU.
- CRTP = đa hình tĩnh zero-cost cho hot-path; virtual vẫn hợp lý cho cấu hình boot-time; chọn theo số liệu đo, không theo giáo điều.
