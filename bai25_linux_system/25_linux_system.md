# Bài 25: Linux / System Programming — process, IPC, socket, mmap, signal

> Bài này lý thuyết là chính (áp dụng cho embedded Linux / host tools).
> `main.cpp` demo phần portable; phần POSIX-only được `#ifdef` — chạy đầy đủ trên WSL/Linux.

## Định nghĩa & Khái niệm

- **Process**: chương trình đang chạy — có không gian địa chỉ ảo *riêng*, file descriptor table, PID. Cách ly lỗi tốt (chết không kéo process khác), giao tiếp phải qua IPC.
- **Thread**: dòng thực thi *trong* process — chia sẻ toàn bộ memory, chỉ riêng stack + registers. Nhẹ, giao tiếp trực tiếp, nhưng một thread hỏng memory là cả process nguy hiểm.
- **Scheduler & context switch**: kernel chia CPU theo time slice + priority; mỗi lần switch lưu/khôi phục registers, đổi page table (process switch đắt hơn thread switch vì mất TLB).
- **Virtual memory**: mỗi process thấy không gian địa chỉ liên tục; MMU dịch trang (page, thường 4KB) sang RAM vật lý. **Page fault** = truy cập trang chưa map — kernel nạp (hợp lệ) hoặc SIGSEGV (không hợp lệ).
- **System call**: cổng duy nhất vào kernel (`read`, `write`, `open`, `mmap`...) — đắt hơn function call thường (~trăm ns) → buffer I/O để giảm số lần syscall.
- **File descriptor (fd)**: số nguyên chỉ tài nguyên I/O — file, socket, pipe, timer... "Everything is a file". fd 0/1/2 = stdin/stdout/stderr.
- **Signal**: ngắt mềm gửi tới process (`SIGINT` Ctrl+C, `SIGSEGV`, `SIGTERM`). Signal handler bị giới hạn như ISR — chỉ gọi hàm *async-signal-safe*.

## Giải thích chi tiết

### Process vs Thread — chọn gì?
```text
              PROCESS                        THREAD
memory        riêng (an toàn)               chung (nhanh, nguy hiểm)
tạo           fork() ~ đắt                  pthread_create ~ rẻ
giao tiếp     IPC (pipe, shm, socket)       biến chung + mutex
chết 1 cái    cái khác sống                 cả process chết
dùng khi      cách ly (daemon, sandbox)     song song trong 1 app
```

### Các kênh IPC
| Cơ chế | Đặc điểm | Dùng khi |
|---|---|---|
| **Pipe** `pipe()` | 1 chiều, byte stream, cha↔con | lệnh nối `ls \| grep` |
| **FIFO** (named pipe) | như pipe, có tên trong filesystem | 2 process không họ hàng |
| **Shared memory** `shm_open`+`mmap` | nhanh nhất (0 copy), phải tự đồng bộ | dữ liệu lớn, tần suất cao |
| **Unix domain socket** | 2 chiều, có framing datagram | client/server cùng máy |
| **TCP/UDP socket** | qua mạng | khác máy (bài 26) |
| **Signal** | chỉ là "tiếng gõ cửa" + số hiệu | shutdown, reload config |

### Socket API — bộ xương server TCP
```c
int s = socket(AF_INET, SOCK_STREAM, 0);
bind(s, ...);          // gắn địa chỉ:port
listen(s, backlog);
int c = accept(s, ...); // block đến khi có client
read(c, buf, n); write(c, buf, n);
close(c);
```
Server thật: `epoll`/`poll` (multiplexing) hoặc thread-pool — 1 thread/client không scale.

### mmap — hai công dụng
```c
// 1) Map file vào memory: đọc file như mảng, kernel tự nạp trang
char* p = (char*)mmap(NULL, len, PROT_READ, MAP_PRIVATE, fd, 0);
// 2) Truy cập thanh ghi phần cứng từ userspace (embedded Linux!)
int fd = open("/dev/mem", O_RDWR);
volatile uint32_t* gpio = (uint32_t*)mmap(NULL, 4096, PROT_READ|PROT_WRITE,
                                          MAP_SHARED, fd, GPIO_PHYS_ADDR);
```
Cách 2 chính là "MMIO của bài 23" phiên bản Linux.

### Signal an toàn
```c
volatile sig_atomic_t g_stop = 0;            // kiểu DUY NHẤT an toàn để ghi
void on_sigint(int) { g_stop = 1; }          // KHÔNG printf/malloc trong handler!
// main loop: while (!g_stop) { ... }  → graceful shutdown
```
Pattern chuẩn hơn nữa: `signalfd`/`sigwait` — biến signal thành sự kiện đọc được, khỏi handler.

### Dynamic library
- `.so` (Linux) / `.dll` (Windows): nạp lúc chạy, chia sẻ giữa process.
- `dlopen/dlsym`: nạp plugin runtime; symbol phải `extern "C"` để khỏi bị mangling (bài 5).
- `LD_LIBRARY_PATH`, `ldd app` xem phụ thuộc; version mismatch `.so` là lỗi triển khai kinh điển.

## Tools sống còn (chạy trên Linux/WSL)

```bash
strace ./app          # xem MỌI syscall — app treo ở đâu, mở file gì, lỗi errno nào
ltrace ./app          # như strace nhưng cho library call
perf top / perf record ./app; perf report    # CPU đang đốt ở hàm nào
valgrind --leak-check=full ./app             # leak, use-after-free
gdb -p <pid>          # attach process đang treo
cat /proc/<pid>/maps  # bản đồ memory của process (thấy heap, stack, .so)
```

## Tips & Tricks

- `strace -f -e trace=network ./app` — chỉ xem syscall mạng, kèm process con.
- Đọc `/proc/<pid>/status` (VmRSS, Threads), `/proc/<pid>/fd/` (fd đang mở — tìm leak fd).
- Syscall bị signal cắt ngang trả `-1, errno=EINTR` — code bền phải retry (vòng `do {} while (errno == EINTR)`).
- `write()` có thể ghi *một phần* — luôn loop đến đủ n byte (bẫy số 1 khi viết socket code).
- Zero-copy: `sendfile`, `splice` — chuyển file → socket không qua userspace.
- Embedded Linux đọc GPIO nhanh: `mmap /dev/mem` nhanh hơn sysfs hàng trăm lần (nhưng cần root + cẩn thận).

## Lỗi thường gặp / Bẫy

1. `printf`/`malloc` trong signal handler → deadlock ngẫu nhiên (không async-signal-safe).
2. Quên `waitpid` con → zombie process chất đống.
3. Không xử lý `EINTR`, ghi thiếu byte của `write()` → mất dữ liệu lúc tải cao.
4. Share memory giữa process mà quên đồng bộ (mutex phải là *process-shared* attribute).
5. fd leak: quên `close()` — process chết ở giới hạn 1024 fd sau vài ngày chạy.
6. Giả định page = 4KB cứng — dùng `sysconf(_SC_PAGESIZE)`.

## Ghi chú Embedded

- Embedded Linux (i.MX, AM335x, Raspberry Pi) dùng nguyên bộ kỹ năng này; MCU (nRF52840) thì "process" ≈ task RTOS, "IPC" ≈ queue/msgq, "signal" ≈ software event.
- Trên Zephyr: `k_msgq` = pipe, `k_event` = signal, `k_mem_slab` chia sẻ buffer — mô hình tư duy giống hệt, API khác.
- Host tool (flasher, log parser, test rig) của dự án firmware thường là chương trình Linux — senior embedded phải viết được cả hai phía.

## Bài tập (làm trên WSL/Linux)

1. Viết chương trình cha `fork()` con, nối bằng `pipe()`: cha gửi lệnh, con trả kết quả, cha `waitpid`.
2. Viết TCP echo server (port 5000) + client; sau đó nâng cấp server dùng `poll()` phục vụ 10 client cùng lúc.
3. `mmap` một file log 100MB và đếm số dòng — so tốc độ với `ifstream::getline`.
4. Bắt `SIGINT` để graceful shutdown server ở bài 2 (đóng socket, in thống kê, thoát mã 0).
5. Dùng `strace` tìm xem `./bai25_linux_system.exe` gọi bao nhiêu lần `write` — giải thích vì sao buffering của stdio làm số này nhỏ.
