# Bài 24: Debugging kỹ năng Senior — GDB, sanitizers, core dump, binutils

## Định nghĩa & Khái niệm

- **Quy trình điều tra chuẩn**: Crash → fault context (core dump / HardFault registers) → backtrace → registers → memory → assembly → source → **root cause**. Senior khác junior ở chỗ đi hết chuỗi này thay vì "thử sửa xem hết chưa".
- **GDB**: debugger chuẩn GNU — breakpoint, watchpoint, backtrace, in biến, chạy từng lệnh assembly, mổ core dump.
- **Core dump**: ảnh chụp memory + registers của process lúc chết — debug "hậu kỳ" không cần tái hiện bug.
- **Sanitizers** (đo lúc *chạy*, cần compile flag):
  - **ASan** (`-fsanitize=address`): heap/stack overflow, use-after-free, double-free.
  - **UBSan** (`-fsanitize=undefined`): signed overflow, shift quá bit, null deref, misaligned.
  - **TSan** (`-fsanitize=thread`): data race (không dùng chung với ASan).
- **Valgrind/memcheck**: như ASan nhưng không cần compile lại — chậm hơn (~20x).
- **binutils**: `nm` (symbol), `objdump -d` (disassembly), `readelf` (header/section), `addr2line` (địa chỉ → file:line), `strings`, `size`.
- **Watchpoint**: break khi *dữ liệu* thay đổi (`watch g_state`) — vũ khí chính cho memory corruption "ai ghi đè biến của tôi?".

## Giải thích chi tiết

### GDB — bộ lệnh sống còn
```text
g++ -g -O0 main.cpp -o app      # -g: debug info; điều tra bug nên thêm -O0
gdb ./app
  break main.cpp:42             # b = breakpoint theo file:line hoặc hàm
  run arg1 arg2                 # r
  next / step / finish          # n = qua dòng, s = vào hàm, finish = chạy hết hàm
  print expr / print *ptr@10    # in biến, in 10 phần tử từ con trỏ
  backtrace / frame 2 / info locals
  watch g_counter               # dừng khi giá trị đổi + chỉ ra AI đổi
  info registers / x/16xb &buf  # xem registers / hex dump memory
  disassemble                   # source không đủ thì đọc asm
```

### Core dump workflow (Linux)
```text
ulimit -c unlimited             # cho phép ghi core
./app                           # ... crash: Segmentation fault (core dumped)
gdb ./app core                  # mổ tử thi
  bt full                       # backtrace + local variables mọi frame
  frame N; print ...
```
Embedded tương đương: HardFault handler lưu `r0-r3, r12, LR, PC, xPSR` + `CFSR/HFSR/BFAR` vào flash/RAM noinit → đọc ra sau reset → `addr2line -e app.elf 0x<PC>`.

### addr2line — từ địa chỉ crash ra dòng code
```bash
addr2line -e app.exe -f -C 0x401234
# -> do_parse(char const*)
#    /src/parser.cpp:87
```
Đây là kỹ thuật số 1 khi chỉ có log "PC=0x..." từ thiết bị ngoài hiện trường.

### Sanitizers — bật trong CI, chạy test hằng ngày
```bash
g++ -g -fsanitize=address,undefined -fno-omit-frame-pointer main.cpp -o app
./app        # bug in ra: loại lỗi + stack ghi + stack cấp phát + stack free
```
ASan tốn ~2x CPU, ~3x RAM — chạy được trong unit test, KHÔNG chạy nổi trên MCU (dùng ý tưởng: canary, stack painting, MPU guard region).

### Phân loại bug khó và hướng đánh
| Triệu chứng | Nghi phạm | Công cụ |
|---|---|---|
| Chạy đúng -O0, sai -O2 | UB (aliasing, thiếu volatile, overflow) | UBSan, đọc asm |
| Crash ngẫu nhiên, chỗ chết mỗi lần một khác | Memory corruption | ASan, watchpoint |
| Treo khi tải cao | Deadlock / race | TSan, `gdb -p` + `thread apply all bt` |
| Chết sau N ngày | Leak/fragmentation/counter overflow | valgrind, heap stats, soak test |
| Chỉ chết trên field, không tái hiện | Thiếu telemetry | crash dump + addr2line |

## Cách dùng (thực hành với main.cpp của bài)

`main.cpp` có các bug *cố ý*, chọn qua argv:
```bash
make                 # build thường
./bai24_debugging.exe stack      # stack buffer overflow
./bai24_debugging.exe heap       # use-after-free
./bai24_debugging.exe ub         # signed overflow + shift UB
./bai24_debugging.exe race       # data race 2 thread

make asan            # build lại với ASan+UBSan rồi chạy lại các case trên
gdb ./bai24_debugging.exe        # tự đặt breakpoint điều tra
```

## Tips & Tricks

- Điều tra bug: build `-g -O0`. Điều tra bug *chỉ xuất hiện ở release*: build `-g -O2` (debug info vẫn dùng được, chỉ khó theo dõi hơn) — đừng đổi optimization level, bug sẽ trốn.
- `gdb -p <pid>` attach vào process đang treo, `thread apply all bt` — ra deadlock ngay.
- `rr` (record & replay) chạy lại bug race **y hệt** từng lần — đáng học nếu làm Linux.
- Bug hiếm: thêm `assert` dày đặc ở invariant + log ring buffer trong RAM — lần crash sau có dữ liệu.
- `git bisect run ./test.sh` — máy tự tìm commit gây regression (roadmap §15).
- In con trỏ nghi ngờ: căn chỉnh sai (`0x...3`), pattern quen (`0xCDCDCDCD` = MSVC uninit heap, `0xDEADBEEF` = marker của bạn).

## Lỗi thường gặp / Bẫy (của người debug)

1. Sửa triệu chứng chưa hiểu root cause — bug quay lại dưới dạng khác.
2. Debug release bug bằng build debug — bug biến mất, kết luận sai "không tái hiện được".
3. Tin 100% backtrace khi stack đã corrupt — frame rác; đối chiếu bằng registers + memory quanh SP.
4. Quên rằng thêm `printf` đổi timing → race bug "tự hết" (Heisenbug).
5. Không giữ file `.elf`/`.map` đúng version với firmware ngoài field → addr2line ra rác. **Archive elf theo release!**
6. Chạy ASan và TSan cùng lúc — không hỗ trợ, phải 2 build riêng.

## Ghi chú Embedded

- HardFault trên Cortex-M: đọc `CFSR` (nguyên nhân), `BFAR/MMFAR` (địa chỉ lỗi), stacked `PC/LR` (chỗ chết). Viết fault handler lưu chúng vào section `noinit` để sống qua reset.
- Stack overflow MCU: tô stack bằng pattern (`0xA5`) lúc boot, đo high-water mark định kỳ (FreeRTOS `uxTaskGetStackHighWaterMark`, Zephyr `CONFIG_THREAD_STACK_INFO`).
- MPU guard page dưới đáy stack → overflow thành fault ngay lập tức thay vì corruption âm thầm.
- Trên nRF52840: J-Link + GDB server (`JLinkGDBServer -device nRF52840_xxAA`), hoặc `west debug` với Zephyr. RTT cho log không chặn realtime.

## Bài tập

1. Chạy từng case bug với và không với ASan/UBSan — so sánh thông tin nhận được, tự viết "báo cáo root cause" 5 dòng cho mỗi bug.
2. Dùng GDB watchpoint tìm thủ phạm ghi đè `g_config` trong case `stack` (không nhìn source trước).
3. Lấy địa chỉ crash từ ASan output, xác nhận lại bằng `addr2line -e bai24_debugging.exe -f -C <addr>`.
4. Viết HardFault handler giả lập: struct lưu PC/LR/CFSR + hàm in "crash report" — nền cho firmware thật.
