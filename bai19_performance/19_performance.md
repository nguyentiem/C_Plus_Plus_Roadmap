# Bài 19: Performance — zero-cost abstraction, layout/padding, cache locality, benchmark đúng cách

## Định nghĩa & Khái niệm

- **Zero-cost abstraction**: nguyên tắc thiết kế của C++ — "cái bạn không dùng thì không trả giá; cái bạn dùng thì không viết tay tốt hơn được". `unique_ptr`, template, `constexpr`, iterator... biên dịch ra mã máy tương đương code C viết tay.
- **Padding/Alignment**: mỗi kiểu có yêu cầu căn chỉnh (alignment) — `int` 4 byte phải nằm ở địa chỉ chia hết cho 4. Compiler chèn byte đệm (padding) giữa các member để thoả yêu cầu đó.
- **Cache locality**: CPU đọc bộ nhớ theo **cache line** (thường 64 byte). Dữ liệu dùng cùng nhau nằm gần nhau → ít cache miss → nhanh.
- **AoS vs SoA**: Array of Structs (`Hat hat[N]`) vs Struct of Arrays (`x[N], y[N], ...`) — hai cách bố trí dữ liệu, hiệu năng khác nhau lớn khi chỉ chạm một phần trường.
- **LTO (Link-Time Optimization)**: tối ưu xuyên đơn vị dịch tại bước link — inline được hàm nằm ở `.cpp` khác.

## Giải thích chi tiết

### Padding — tiền mất vô hình

```cpp
struct Xau  { char a; int b; char c; };   // 1 +3pad+ 4 + 1 +3pad = 12 byte
struct Tot  { int b; char a; char c; };   // 4 + 1 + 1 + 2pad     = 8  byte
```

```
Xau:  [a][.][.][.][b b b b][c][.][.][.]     12 byte (6 byte la padding!)
Tot:  [b b b b][a][c][.][.]                  8 byte
```

Quy tắc: **sắp xếp member từ lớn xuống nhỏ** (hoặc gom theo alignment). Với mảng 10.000 phần tử, chênh lệch 4 byte/phần tử = 40 KB — trên nRF52840 (256 KB RAM) là con số nghiêm túc. `alignas(N)` ép căn chỉnh lớn hơn (ví dụ `alignas(64)` chống false sharing — bài 16; `alignas(4)` cho buffer DMA).

### Cache locality & data-oriented design

Bài toán: 100.000 "hạt", mỗi frame chỉ cập nhật vị trí `x`.

- **AoS**: mỗi struct 32+ byte nhưng chỉ dùng 4 byte `x` → mỗi cache line 64 byte tải về chỉ dùng ~4-8 byte hữu ích, còn lại vứt.
- **SoA**: mảng `x[]` liền mạch → mỗi cache line 64 byte chứa 16 giá trị `x`, dùng 100%; prefetcher đoán được pattern; vectorize (SIMD) dễ.

```
AoS:  [x y z vx vy vz m t][x y z vx vy vz m t]...   ← line 64B chua 2 "x"
SoA:  [x x x x x x x x x x x x x x x x]...          ← line 64B chua 16 "x"
```

Đây là tư duy **data-oriented design**: thiết kế theo *cách dữ liệu được truy cập*, không theo "đối tượng ngoài đời". Không phải lúc nào SoA cũng thắng — nếu luôn dùng TẤT CẢ trường của một phần tử cùng lúc, AoS lại tốt hơn. Đo, đừng đoán.

### Inline, LTO và cách đọc assembly

- `inline` ngày nay chủ yếu là ngữ nghĩa linkage (ODR); compiler tự quyết inline theo heuristic. Ép bằng `__attribute__((always_inline))` chỉ khi có số liệu.
- LTO (`-flto`): tối ưu lúc link, inline xuyên file `.cpp`. Firmware thường bật cùng `-Os`/`-O2` — giảm cả size lẫn thời gian chạy; giá là link chậm và debug khó hơn.
- **Godbolt (compiler explorer)**: dán hàm C++ vào godbolt.org, chọn `ARM gcc` + `-O2 -mcpu=cortex-m4`, xem mã máy thật. Kỹ năng senior: xác nhận "abstraction này có zero-cost thật không?" bằng mắt, không bằng niềm tin. Mẹo đọc: tìm `bl` (call chưa inline), đếm lệnh trong vòng lặp, xem có load/store thừa.

### Benchmark đúng cách — chống optimizer

Optimizer XOÁ code có kết quả không dùng (dead code elimination) và tính trước kết quả (constant folding). Benchmark ngây thơ đo được... 0 ns. Chống:

1. **Volatile sink**: ghi kết quả vào biến `volatile` — compiler buộc phải thực hiện phép tính.
2. Dữ liệu đầu vào không phải hằng biết trước lúc compile (đọc runtime, ví dụ từ `argc` hoặc sinh ngẫu nhiên).
3. Đo nhiều lần, lấy min/median; warm-up trước lần đo (nạp cache, ổn định tần số CPU).
4. Dùng `std::chrono::steady_clock` (không bị chỉnh giờ hệ thống làm sai).

## Cách dùng

```cpp
volatile float sink;                       // "ho den" nuot ket qua
auto t0 = std::chrono::steady_clock::now();
for (...) tong += ...;
auto t1 = std::chrono::steady_clock::now();
sink = tong;                               // ep compiler giu vong lap
double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
```

## Tips & Tricks

- `static_assert(sizeof(GoiTin) == 12)` khoá layout của struct giao thức — vỡ ngay lúc compile khi ai đó thêm trường.
- `#pragma pack(1)` loại padding cho struct on-wire, nhưng truy cập lệch địa chỉ (unaligned) có giá — trên Cortex-M0 còn HardFault. Cân nhắc memcpy từng trường.
- Trên MCU không có cache vẫn hưởng lợi từ struct nhỏ: ít RAM, ít lệnh load; nRF52840 có ICACHE cho flash — code locality vẫn có ý nghĩa.
- Đo trên **target thật với cờ tối ưu thật** (`-Os` khác `-O2`!); trên MCU dùng DWT cycle counter (CYCCNT) thay chrono.
- Ưu tiên thuật toán và cấu trúc dữ liệu trước; micro-optimization là bước cuối, có profiler dẫn đường.

## Lỗi thường gặp / Bẫy

1. **Benchmark bị optimizer xoá sạch** → kết quả "nhanh khó tin". Luôn có volatile sink + kiểm tra kết quả in ra hợp lý.
2. Đo bản `-O0` rồi kết luận cho production `-O2` — vô nghĩa, thứ tự nhanh chậm có thể đảo ngược.
3. So sánh hai phép đo khác điều kiện (một bản cache nóng, một bản nguội) — luôn warm-up cả hai.
4. Giả định layout struct giống nhau giữa compiler/ABI — sai; dùng `static_assert` và serialize tường minh.
5. `reinterpret_cast` buffer byte sang struct — vi phạm strict aliasing + unaligned access. Dùng `memcpy`.
6. Tối ưu chỗ không nóng: 97% thời gian nằm ở 3% code — profile trước, sửa sau.

## Ghi chú Embedded

- nRF52840: Cortex-M4 @64 MHz, có ICACHE flash, không DCACHE — cache locality dạng "SoA vs AoS" ít kịch tính hơn desktop, nhưng **RAM footprint** (padding) và **số lệnh** lại quan trọng hơn nhiều.
- Buffer DMA (EasyDMA trên nRF52) phải nằm ở RAM và đúng alignment — `alignas` là công cụ đúng.
- `-Os` là mặc định firmware; kiểm tra size bằng `arm-none-eabi-size`, xem map file để biết cái gì chiếm flash.
- Cấm/thu hẹp: exception và RTTI thường tắt (`-fno-exceptions -fno-rtti`) — đây là hai abstraction KHÔNG zero-cost về size.
- Đo chu kỳ trên Cortex-M: DWT->CYCCNT, hoặc bật GPIO quanh đoạn code và xem bằng logic analyzer/oscilloscope.

## Bài tập tự luyện

1. Lấy struct `GoiTinCamBien` 7 trường trộn `uint8_t/uint32_t/uint16_t`, tính tay sizeof trước, kiểm chứng bằng code, rồi sắp xếp lại để nhỏ nhất và khoá bằng `static_assert`.
2. Dán vòng lặp cộng mảng của bài này vào godbolt với `ARM gcc -O2 -mcpu=cortex-m4` và `-O0`; đếm số lệnh trong thân vòng lặp ở hai bản, xác định bản `-O2` có auto-vectorize/unroll không.
3. Viết benchmark so sánh duyệt `std::vector<int>` tuần tự vs nhảy ngẫu nhiên (cùng tổng số phần tử truy cập). Giải thích chênh lệch bằng cache line và prefetcher.

## Tóm tắt

- Zero-cost abstraction là lời hứa có điều kiện — xác nhận bằng godbolt, không bằng niềm tin.
- Padding: sắp member lớn→nhỏ, khoá layout bằng `static_assert`; `alignas` cho DMA/false-sharing.
- Cache line 64 byte quyết định: dữ liệu dùng cùng nhau đặt gần nhau; AoS vs SoA chọn theo access pattern.
- Benchmark: steady_clock, warm-up, volatile sink chống dead-code elimination, đo đúng cờ tối ưu, đúng target.
- Embedded: RAM/flash footprint thường quan trọng hơn cycle; đo trên target bằng DWT/GPIO.
