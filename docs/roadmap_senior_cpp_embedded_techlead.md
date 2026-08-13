# Roadmap Senior C++ / Embedded → Tech Lead

> Mục tiêu: xây dựng nền tảng C++ sâu, Embedded/System vững, có khả năng
> thiết kế kiến trúc, debug vấn đề khó và dẫn dắt kỹ thuật.

## 1. C++ Core

-   [ ] Value, reference, pointer
-   [ ] `const`, `constexpr`, `consteval`
-   [ ] Scope và storage duration
-   [ ] Stack / Heap / Static storage
-   [ ] Function overloading
-   [ ] Default arguments
-   [ ] Namespace
-   [ ] `auto`, `decltype`
-   [ ] Lambda
-   [ ] Enum / `enum class`
-   [ ] Type casting
    -   [ ] `static_cast`
    -   [ ] `dynamic_cast`
    -   [ ] `const_cast`
    -   [ ] `reinterpret_cast`
-   [ ] Undefined Behavior (UB)
-   [ ] Alignment / Padding
-   [ ] Strict aliasing
-   [ ] Object lifetime
-   [ ] C++17 / C++20 / C++23

## 2. OOP --- Học thật sâu

### 2.1 Class & Object

-   [ ] Class / Object
-   [ ] Access modifiers: `public`, `private`, `protected`
-   [ ] Member variables / member functions
-   [ ] Static members
-   [ ] `this` pointer
-   [ ] Friend class / friend function
-   [ ] Nested class

### 2.2 Constructor & Destructor

-   [ ] Default constructor
-   [ ] Parameterized constructor
-   [ ] Copy constructor
-   [ ] Move constructor
-   [ ] Destructor
-   [ ] Constructor initialization list
-   [ ] Delegating constructor
-   [ ] `explicit`
-   [ ] `= default`
-   [ ] `= delete`
-   [ ] Thứ tự constructor/destructor trong inheritance

### 2.3 Copy / Move Semantics

-   [ ] Lvalue / Rvalue
-   [ ] Lvalue reference
-   [ ] Rvalue reference
-   [ ] `std::move`
-   [ ] `std::forward`
-   [ ] Copy assignment
-   [ ] Move assignment
-   [ ] Rule of 3
-   [ ] Rule of 5
-   [ ] Rule of 0
-   [ ] Perfect forwarding

### 2.4 Encapsulation

-   [ ] Data hiding
-   [ ] Public interface
-   [ ] Invariant của object
-   [ ] Getter/setter khi nào cần và khi nào không
-   [ ] PImpl idiom

### 2.5 Inheritance

-   [ ] Base / Derived class
-   [ ] Public/protected/private inheritance
-   [ ] Single inheritance
-   [ ] Multiple inheritance
-   [ ] Diamond problem
-   [ ] Virtual inheritance
-   [ ] Object slicing
-   [ ] Composition vs inheritance
-   [ ] "Favor composition over inheritance"

### 2.6 Polymorphism

-   [ ] Function overriding
-   [ ] Virtual function
-   [ ] Pure virtual function
-   [ ] Abstract class
-   [ ] Interface
-   [ ] Virtual destructor
-   [ ] Dynamic dispatch
-   [ ] Static polymorphism
-   [ ] vtable / vptr
-   [ ] Chi phí runtime của virtual function
-   [ ] CRTP

### 2.7 Abstraction & Interface Design

-   [ ] Thiết kế interface nhỏ, rõ ràng
-   [ ] Dependency inversion
-   [ ] Dependency injection
-   [ ] Interface segregation
-   [ ] API stability
-   [ ] ABI considerations

### 2.8 SOLID

-   [ ] Single Responsibility Principle
-   [ ] Open/Closed Principle
-   [ ] Liskov Substitution Principle
-   [ ] Interface Segregation Principle
-   [ ] Dependency Inversion Principle

> Không học SOLID theo kiểu thuộc định nghĩa. Hãy áp dụng vào driver,
> service và application thực tế.

## 3. Memory & Resource Management

-   [ ] Stack / Heap
-   [ ] `malloc/free`
-   [ ] `new/delete`
-   [ ] `new[]/delete[]`
-   [ ] RAII
-   [ ] Ownership / Borrowing
-   [ ] `std::unique_ptr`
-   [ ] `std::shared_ptr`
-   [ ] `std::weak_ptr`
-   [ ] Circular reference
-   [ ] Dangling pointer/reference
-   [ ] Memory leak
-   [ ] Use-after-free
-   [ ] Double-free
-   [ ] Stack overflow
-   [ ] Heap fragmentation
-   [ ] Placement `new`
-   [ ] Memory pool
-   [ ] Arena allocator
-   [ ] Custom allocator
-   [ ] Cache locality
-   [ ] Virtual memory
-   [ ] MMU / MPU
-   [ ] Page / Page fault

Luôn đặt các câu hỏi:

-   Ai sở hữu object?
-   Lifetime của object bao lâu?
-   Ai destroy?
-   Có thể `nullptr` không?
-   Thread/ISR nào được truy cập?
-   Pointer/reference có thể dangling không?

## 4. STL & Modern C++

-   [ ] `std::array`
-   [ ] `std::vector`
-   [ ] `std::deque`
-   [ ] `std::list`
-   [ ] `std::map`
-   [ ] `std::unordered_map`
-   [ ] `std::set`
-   [ ] `std::string`
-   [ ] `std::string_view`
-   [ ] `std::span`
-   [ ] `std::optional`
-   [ ] `std::variant`
-   [ ] `std::tuple`
-   [ ] Iterators
-   [ ] Algorithms
-   [ ] Ranges
-   [ ] Chrono
-   [ ] Container complexity
-   [ ] Iterator invalidation
-   [ ] Allocation behavior
-   [ ] Cache locality

## 5. Templates & Generic Programming

-   [ ] Function template
-   [ ] Class template
-   [ ] Template specialization
-   [ ] Partial specialization
-   [ ] Variadic templates
-   [ ] Fold expressions
-   [ ] Type traits
-   [ ] SFINAE
-   [ ] Concepts
-   [ ] `requires`
-   [ ] Compile-time programming
-   [ ] `constexpr`
-   [ ] CRTP

## 6. Concurrency & C++ Memory Model

-   [ ] Process vs Thread
-   [ ] `std::thread`
-   [ ] Mutex
-   [ ] Recursive mutex
-   [ ] Condition variable
-   [ ] Semaphore
-   [ ] Atomic
-   [ ] Race condition
-   [ ] Deadlock
-   [ ] Livelock
-   [ ] Starvation
-   [ ] Priority inversion
-   [ ] Producer/Consumer
-   [ ] Thread pool
-   [ ] Lock-free structures
-   [ ] False sharing
-   [ ] Happens-before
-   [ ] `memory_order_relaxed`
-   [ ] `memory_order_acquire`
-   [ ] `memory_order_release`
-   [ ] `memory_order_seq_cst`

## 7. Data Structures & Algorithms

-   [ ] Big-O
-   [ ] Array
-   [ ] Linked list
-   [ ] Stack / Queue
-   [ ] Hash table
-   [ ] Tree / BST
-   [ ] Heap
-   [ ] Graph
-   [ ] Sorting
-   [ ] Searching
-   [ ] BFS / DFS
-   [ ] Recursion
-   [ ] Dynamic programming cơ bản
-   [ ] Bit manipulation

## 8. Linux / System Programming

-   [ ] Process
-   [ ] Thread
-   [ ] Scheduler
-   [ ] Context switch
-   [ ] Virtual memory
-   [ ] System call
-   [ ] File descriptor
-   [ ] Pipe
-   [ ] Signal
-   [ ] Shared memory
-   [ ] IPC
-   [ ] Socket
-   [ ] TCP / UDP
-   [ ] `mmap`
-   [ ] Dynamic library

### Tools

-   [ ] `gdb`
-   [ ] `strace`
-   [ ] `ltrace`
-   [ ] `perf`
-   [ ] `valgrind`
-   [ ] `readelf`
-   [ ] `objdump`
-   [ ] `nm`
-   [ ] `addr2line`

## 9. Compiler / Linker / Build System

Hiểu toàn bộ pipeline:

``` text
.cpp
 ↓
Preprocessor
 ↓
Compiler
 ↓
Assembly
 ↓
Object (.o)
 ↓
Linker
 ↓
ELF
```

-   [ ] Preprocessor
-   [ ] Compiler optimization
-   [ ] Symbol
-   [ ] Linkage
-   [ ] Name mangling
-   [ ] ODR
-   [ ] Relocation
-   [ ] Static library
-   [ ] Shared library
-   [ ] ABI
-   [ ] ELF
-   [ ] Linker script
-   [ ] `.text`
-   [ ] `.rodata`
-   [ ] `.data`
-   [ ] `.bss`
-   [ ] Stack / Heap
-   [ ] LTO

### Build & Quality Tools

-   [ ] GCC
-   [ ] Clang
-   [ ] CMake
-   [ ] Make
-   [ ] Ninja
-   [ ] GDB
-   [ ] AddressSanitizer
-   [ ] UndefinedBehaviorSanitizer
-   [ ] ThreadSanitizer
-   [ ] clang-tidy
-   [ ] clang-format

## 10. Embedded C++

### MCU / Hardware

-   [ ] ARM Cortex-M architecture
-   [ ] Memory map
-   [ ] Register
-   [ ] Memory-mapped I/O
-   [ ] Interrupt
-   [ ] NVIC
-   [ ] DMA
-   [ ] Timer
-   [ ] GPIO
-   [ ] UART
-   [ ] SPI
-   [ ] I2C
-   [ ] CAN
-   [ ] ADC
-   [ ] Flash
-   [ ] Cache
-   [ ] MPU
-   [ ] Alignment
-   [ ] `volatile`
-   [ ] Atomic operations
-   [ ] ISR safety
-   [ ] HardFault debugging

### RTOS

-   [ ] Task
-   [ ] Scheduler
-   [ ] Queue
-   [ ] Semaphore
-   [ ] Mutex
-   [ ] Event/Event Group
-   [ ] Software timer
-   [ ] ISR → Task synchronization
-   [ ] Priority inversion
-   [ ] Deadlock
-   [ ] Stack sizing
-   [ ] Heap strategies
-   [ ] Watchdog

### Embedded C++ Design

-   [ ] Zero-cost abstraction
-   [ ] Static allocation
-   [ ] Hạn chế/loại bỏ dynamic allocation khi cần
-   [ ] RAII cho peripheral/resource
-   [ ] Compile-time configuration
-   [ ] Template-based drivers
-   [ ] Hardware abstraction

Kiến trúc nên hướng tới:

``` text
Hardware
   ↓
Low-level Driver
   ↓
HAL / BSP
   ↓
Device Driver
   ↓
Service
   ↓
Application
```

## 11. Software Architecture

-   [ ] Module boundaries
-   [ ] Separation of concerns
-   [ ] Dependency management
-   [ ] Layered architecture
-   [ ] Event-driven architecture
-   [ ] State machine
-   [ ] Message queue
-   [ ] Publish/Subscribe
-   [ ] Dependency injection
-   [ ] Error handling strategy
-   [ ] Logging architecture
-   [ ] Configuration management
-   [ ] API design
-   [ ] Backward compatibility

### Design Patterns

-   [ ] Factory
-   [ ] Strategy
-   [ ] Observer
-   [ ] State
-   [ ] Command
-   [ ] Adapter
-   [ ] Facade
-   [ ] Builder
-   [ ] Template Method
-   [ ] Dependency Injection

> Pattern là công cụ giải quyết vấn đề, không phải mục tiêu để nhét càng
> nhiều pattern vào code càng tốt.

## 12. Testing & Code Quality

-   [ ] Unit testing
-   [ ] Integration testing
-   [ ] Hardware-in-the-loop testing
-   [ ] Mock / Fake / Stub
-   [ ] GoogleTest / GoogleMock
-   [ ] Static analysis
-   [ ] Dynamic analysis
-   [ ] Sanitizers
-   [ ] Code coverage
-   [ ] Regression testing
-   [ ] CI/CD
-   [ ] Coding standard
-   [ ] MISRA C++ / CERT C++ cơ bản

## 13. Debugging --- Kỹ năng Senior bắt buộc

Phải có khả năng điều tra:

``` text
Crash
 ↓
Core dump / Fault context
 ↓
Backtrace
 ↓
Registers
 ↓
Memory
 ↓
Assembly
 ↓
Source
 ↓
Root Cause
```

-   [ ] Segmentation fault
-   [ ] HardFault
-   [ ] BusFault
-   [ ] UsageFault
-   [ ] Memory corruption
-   [ ] Stack corruption
-   [ ] Heap corruption
-   [ ] Race condition
-   [ ] Deadlock
-   [ ] Timing bug
-   [ ] Release-only bug
-   [ ] Optimization-related UB
-   [ ] DMA/cache coherency issues

Mục tiêu: xử lý được bug kiểu:

> "Thiết bị thỉnh thoảng crash sau vài ngày, chỉ khi UART + DMA +
> network chạy đồng thời và chỉ xảy ra ở Release build."

## 14. Networking & Protocol Design

-   [ ] TCP/IP fundamentals
-   [ ] TCP vs UDP
-   [ ] Socket programming
-   [ ] Client/server architecture
-   [ ] Binary protocol
-   [ ] Framing
-   [ ] CRC/checksum
-   [ ] Timeout
-   [ ] Retry
-   [ ] Reconnection
-   [ ] State machine
-   [ ] Serialization
-   [ ] Endianness
-   [ ] Protocol versioning
-   [ ] Robust parser design

## 15. Git & Development Workflow

-   [ ] Branch
-   [ ] Merge
-   [ ] Rebase
-   [ ] Cherry-pick
-   [ ] Bisect
-   [ ] Tag
-   [ ] Submodule
-   [ ] Conflict resolution
-   [ ] Pull Request
-   [ ] Code review
-   [ ] Conventional commits
-   [ ] CI/CD

Đặc biệt nên thành thạo:

``` bash
git bisect
```

để tìm commit gây regression.

## 16. Từ Senior lên Tech Lead

### Technical Leadership

-   [ ] Requirement analysis
-   [ ] System design
-   [ ] Architecture design
-   [ ] Technical specification
-   [ ] Design document
-   [ ] API/interface definition
-   [ ] Technical risk analysis
-   [ ] Trade-off analysis
-   [ ] Performance analysis
-   [ ] Memory budget
-   [ ] CPU budget
-   [ ] Technical debt management

### Code Review

Không chỉ hỏi:

> "Code có chạy không?"

Mà phải đánh giá:

-   Ownership có rõ không?
-   Lifetime có an toàn không?
-   Thread-safe không?
-   ISR-safe không?
-   Có UB không?
-   API có dễ misuse không?
-   Test được không?
-   Maintain được 5 năm không?
-   Có scale được không?
-   Có phá backward compatibility không?

### Team Leadership

-   [ ] Chia task
-   [ ] Estimate
-   [ ] Mentoring
-   [ ] Review design
-   [ ] Review code
-   [ ] Root-cause analysis
-   [ ] Incident/postmortem
-   [ ] Technical decision
-   [ ] Giải quyết technical disagreement
-   [ ] Giao tiếp với hardware/software/test/product teams
-   [ ] Viết tài liệu kỹ thuật rõ ràng

## 17. Thứ tự ưu tiên học

``` text
                    TECH LEAD
                       ▲
              Leadership / Mentoring
                       ▲
             Architecture / Design
                       ▲
           Embedded Systems + Linux
                       ▲
        Concurrency / C++ Memory Model
                       ▲
       Compiler / Linker / Debugging
                       ▲
       Modern C++ / STL / Templates
                       ▲
        OOP / RAII / Design Principles
                       ▲
     Memory / Lifetime / Ownership / UB
                       ▲
                 C / C++ Core
```

## 18. Các project nên tự làm

### Project 1 --- Modern C++ Library

-   RAII
-   Smart pointer
-   Move semantics
-   Templates
-   Unit test
-   CMake

### Project 2 --- Linux Multithreaded Server

-   TCP
-   Thread pool
-   Queue
-   Mutex / condition variable
-   Atomic
-   Logging
-   Graceful shutdown

### Project 3 --- Embedded C++ Driver Framework

``` text
Application
   ↓
Service
   ↓
Device Driver
   ↓
HAL/BSP
   ↓
STM32
```

Áp dụng:

-   Interface
-   Dependency injection
-   RAII
-   Template
-   Static allocation
-   Mock driver để unit test trên PC

### Project 4 --- RTOS Communication System

-   UART DMA
-   Ring buffer
-   Multiple tasks
-   Queue
-   Event
-   Timeout
-   Protocol parser
-   CRC
-   Error recovery

### Project 5 --- Production-grade Embedded System

Kết hợp:

-   STM32
-   FreeRTOS
-   C++
-   UART/SPI/I2C/CAN
-   DMA
-   Bootloader
-   External Flash
-   Logging
-   Watchdog
-   Configuration
-   Firmware update
-   Unit test
-   HIL test

## 19. Tiêu chuẩn tự đánh giá

### Junior

> "Tôi biết viết code C++."

### Middle

> "Tôi biết tổ chức code và giải quyết feature độc lập."

### Senior

> "Tôi hiểu tại sao hệ thống hoạt động, tìm được root cause của bug khó
> và thiết kế được subsystem ổn định."

### Tech Lead

> "Tôi có thể đưa ra kiến trúc, phân tích trade-off, dự đoán rủi ro, dẫn
> dắt implementation và giúp cả team đưa hệ thống đến production."

------------------------------------------------------------------------

## Nguyên tắc quan trọng

Đừng chỉ học **C++ syntax**.

Hãy học theo chuỗi:

``` text
Language
  ↓
Memory
  ↓
Object Lifetime
  ↓
OOP / Generic Programming
  ↓
Concurrency
  ↓
Operating System
  ↓
Hardware
  ↓
Architecture
  ↓
Production Debugging
  ↓
Technical Leadership
```

Một Senior/Tech Lead C++ giỏi không nhất thiết là người nhớ nhiều syntax
nhất. Đó là người hiểu hệ thống đủ sâu để **thiết kế đúng, phát hiện rủi
ro sớm, debug vấn đề khó và giúp team đưa ra quyết định kỹ thuật tốt**.
