# Bài 17: Design Patterns cho C++ Embedded (Factory, Strategy, Observer, State Machine, Command, Template Method)

## Định nghĩa & Khái niệm

- **Design pattern**: giải pháp mẫu, có tên gọi, cho một lớp vấn đề thiết kế lặp lại. Pattern là **từ vựng chung** giữa kỹ sư — "chỗ này dùng Observer" nói được nhiều hơn 10 dòng mô tả.
- **Factory (Method)**: tách logic *tạo* đối tượng khỏi logic *dùng* đối tượng — caller chỉ biết interface.
- **Strategy**: đóng gói một họ thuật toán sau interface, hoán đổi được lúc runtime (hoặc compile-time bằng template).
- **Observer**: một chủ thể (subject) thông báo cho danh sách người nghe (listeners) khi có sự kiện — tách nguồn sự kiện khỏi nơi xử lý.
- **State Machine**: hành vi của hệ thống phụ thuộc *trạng thái hiện tại*; sự kiện gây *chuyển trạng thái*. Trái tim của hầu hết firmware.
- **Command**: gói một yêu cầu thành đối tượng — hàng đợi lệnh, undo, defer từ ISR sang task.
- **Template Method**: lớp cha định khung thuật toán, lớp con lấp các bước tuỳ biến.

## Giải thích chi tiết

### State Machine — trọng tâm của bài

Firmware thực chất là tập các máy trạng thái: kết nối BLE, giao thức UART, quản lý nguồn... Hai cách hiện thực chính:

**Cách 1 — enum class + bảng chuyển trạng thái (table-driven).** Trạng thái là dữ liệu, chuyển trạng thái là tra bảng. Ưu: nhỏ gọn, không cấp phát động, toàn bộ FSM nhìn thấy trong MỘT bảng — dễ review, dễ sinh từ tài liệu đặc tả, rất hợp MCU. Nhược: hành động phức tạp theo trạng thái phải nhét vào switch/callback.

```
            ket_noi            thanh_cong
   [Idle] ──────────► [Connecting] ─────────► [Connected]
     ▲                     │  that_bai             │ mat_ket_noi
     │      reset          ▼                       ▼
     └───────────────── [Error] ◄──────────────────┘
```

**Cách 2 — State pattern (OOP).** Mỗi trạng thái là một lớp cài interface `ITrangThai` với `vao()/ra()/xuLy(suKien)`. Ưu: hành vi mỗi trạng thái gói gọn một chỗ, entry/exit action tự nhiên, mở rộng thêm trạng thái không sửa code cũ (Open-Closed). Nhược: nhiều lớp, vtable, nếu `new` mỗi lần chuyển thì tốn heap (khắc phục: mỗi state là singleton tĩnh — như demo).

Chọn cách nào? FSM nhỏ, phẳng, ít hành động → bảng. FSM có entry/exit action phức tạp, hành vi khác nhau nhiều theo trạng thái → State pattern. FSM phân cấp (hierarchical) → cân nhắc thư viện (Boost.SML, QP framework).

### Observer trong embedded

Sensor đọc xong → thông báo logger, màn hình, bộ điều khiển. Không có Observer, driver sensor phải `#include` cả ba module đó — phụ thuộc ngược, không test được. Với Observer, driver chỉ biết interface `IListener`. Lưu ý embedded: dùng mảng/`std::array` con trỏ cố định thay vì `std::vector` nếu cấm cấp phát động; cẩn thận lifetime — listener phải hủy đăng ký trước khi chết.

### Các pattern còn lại — khi nào dùng

- **Factory**: chọn driver theo cấu hình/board ID lúc khởi động (`taoCamBien(loai)` trả `unique_ptr<ICamBien>`).
- **Strategy**: thuật toán lọc tín hiệu hoán đổi (trung bình trượt vs Kalman), chế độ điều khiển (PID vs bang-bang).
- **Command**: hàng đợi lệnh từ ISR/BLE sang main loop — chính là mẫu "defer work" của FreeRTOS.
- **Template Method**: khung `khoiTao() → doc() → xuLy()` chung cho mọi driver sensor, lớp con chỉ viết `doc()`.

## Cách dùng

```cpp
// Strategy: hoan doi thuat toan luc runtime
struct ILoc { virtual float loc(float x) = 0; virtual ~ILoc() = default; };
class BoDieuKhien {
    ILoc* loc_;                       // khong so huu — DI tu ngoai
public:
    explicit BoDieuKhien(ILoc* l) : loc_(l) {}
    float capNhat(float raw) { return loc_->loc(raw); }
};

// Bang chuyen trang thai: (trang_thai, su_kien) -> trang_thai_moi
struct ChuyenTT { TrangThai tu; SuKien sk; TrangThai den; };
constexpr ChuyenTT BANG[] = {
    {TrangThai::Idle, SuKien::KetNoi, TrangThai::Connecting},
    // ...
};
```

## Tips & Tricks

- Pattern là **công cụ, không phải mục tiêu**. Code 50 dòng không cần Factory + Strategy + Visitor. Áp pattern khi có *biến thiên thực tế* cần cô lập.
- Ưu tiên composition hơn inheritance; Strategy/State qua interface nhỏ gọn hơn cây kế thừa sâu.
- Embedded: state là **singleton tĩnh** hoặc dùng `std::variant` + `std::visit` để tránh heap và vtable.
- Bảng chuyển trạng thái để `constexpr` → nằm trong flash, không tốn RAM.
- Observer: quyết định rõ notify đồng bộ (gọi ngay, cẩn thận reentrancy) hay bất đồng bộ (đẩy vào queue).
- Log mọi chuyển trạng thái ở mức debug — công cụ chẩn đoán field-issue rẻ nhất bạn có.

## Lỗi thường gặp / Bẫy

1. **Nhồi pattern (over-engineering)**: AbstractSingletonProxyFactoryBean cho bài toán 3 dòng if. Senior biết KHÔNG dùng pattern khi nào.
2. **Observer quên hủy đăng ký** → subject gọi vào đối tượng đã chết (dangling). Dùng RAII cho việc đăng ký.
3. **Notify trong khi đang giữ khoá** → listener gọi ngược lại subject → deadlock/reentrancy.
4. **FSM "chui"**: cờ bool `dangKetNoi`, `daLoi`, `dangCho` rải rác — thực chất là state machine ẩn với 2^n trạng thái bất hợp lệ. Gom về MỘT enum trạng thái.
5. **State pattern cấp phát `new` mỗi lần chuyển trạng thái** trên MCU → phân mảnh heap.
6. **Quên xử lý sự kiện không hợp lệ** trong FSM — phải quyết định rõ: bỏ qua, log, hay vào Error.

## Ghi chú Embedded

- Bảng chuyển trạng thái `constexpr` + enum class là dạng FSM chuẩn công nghiệp cho firmware (Zephyr có sẵn `smf` — State Machine Framework với entry/exit/run action).
- ISR nên chỉ **sinh sự kiện** (đẩy vào queue), FSM chạy trong context task/main loop — không bao giờ chuyển trạng thái phức tạp trong ISR.
- Observer đồng bộ trong ISR = gọi hàm dài trong ISR = bug. Defer qua queue (chính là Command pattern).
- Virtual dispatch tốn ~vài ns và một vtable pointer/đối tượng — thường chấp nhận được cả trên Cortex-M; chỉ tối ưu (CRTP, bài 18) khi hot-path đo được.
- BLE stack (SoftDevice/nRF Connect SDK) bản chất là FSM sự kiện: mô hình Idle→Connecting→Connected→Error trong demo ánh xạ thẳng vào thực tế.

## Bài tập tự luyện

1. Mở rộng FSM demo: thêm trạng thái `Sleeping` (từ Idle khi sự kiện `HetGio`, thoát về Idle khi `DanhThuc`), thêm entry/exit action in log cho cả hai cách hiện thực.
2. Viết Command queue: ISR giả (một hàm) đẩy đối tượng lệnh vào ring buffer, main loop lấy ra thực thi. So sánh với cách gọi hàm trực tiếp từ ISR.
3. Refactor: cho đoạn code dùng 3 cờ bool điều khiển lẫn nhau (tự viết), chuyển thành enum FSM và chứng minh số trạng thái bất hợp lệ đã bị loại.

## Tóm tắt

- Pattern = từ vựng thiết kế; dùng khi có biến thiên cần cô lập, không nhồi nhét.
- State machine là pattern quan trọng nhất trong firmware: bảng chuyển trạng thái (gọn, flash-friendly) vs State pattern OOP (entry/exit, mở rộng tốt).
- Observer tách nguồn sự kiện khỏi xử lý; cẩn thận lifetime và reentrancy.
- Factory tách khâu tạo; Strategy hoán đổi thuật toán; Command defer công việc (ISR→task); Template Method định khung driver.
- Embedded: tránh heap trong pattern (state tĩnh, mảng listener cố định), ISR chỉ sinh sự kiện.
