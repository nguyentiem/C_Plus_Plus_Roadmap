# Roadmap C++ từ cơ bản đến Senior (thiên hướng Embedded)

> Lộ trình này thiên hướng embedded (STM32/nRF52840/ESP32) — có ghi chú riêng cho các phần đặc thù firmware (hạn chế heap, không dùng exception, v.v.) thay vì chỉ là C++ ứng dụng thuần túy.

## Giai đoạn 1 — Nền tảng (Foundation)

- Cú pháp cơ bản: kiểu dữ liệu, con trỏ, tham chiếu, mảng, struct
- Sự khác biệt C vs C++: namespace, function overloading, default arguments
- Class cơ bản: constructor/destructor, encapsulation, `this` pointer
- RAII (Resource Acquisition Is Initialization) — nền tảng của toàn bộ C++ hiện đại
- Stack vs Heap, lifetime của object
- Compile process: header/source, include guard, linker

## Giai đoạn 2 — OOP và Type System

- Kế thừa (inheritance), virtual function, polymorphism, vtable (hiểu cơ chế bên dưới, không chỉ dùng)
- Abstract class, interface pattern
- Operator overloading
- `static`/`const` correctness (const member function, const pointer vs pointer to const)
- Casting: `static_cast`, `dynamic_cast`, `const_cast`, `reinterpret_cast` — khi nào dùng cái nào
- Struct vs Class, POD types (quan trọng cho embedded khi map struct vào register/protocol)

## Giai đoạn 3 — Templates & Generic Programming

- Function template, class template
- Template specialization (full & partial)
- SFINAE cơ bản
- Variadic templates
- `constexpr`, `consteval` (tính toán compile-time — hữu ích cho embedded để giảm runtime cost)
- Type traits (`std::is_same`, `std::enable_if`...)

## Giai đoạn 4 — Memory Management hiện đại

- Smart pointers: `unique_ptr`, `shared_ptr`, `weak_ptr` — cơ chế ownership
- Rule of Three/Five/Zero
- Move semantics: lvalue/rvalue, `std::move`, move constructor/assignment
- Copy elision, RVO/NRVO
- ⚠️ Embedded: hiểu rõ để **biết khi nào không dùng** (dynamic allocation trên MCU RAM hạn chế) — nhưng vẫn cần nắm sâu vì codebase lớn hoặc unit test trên host thường dùng

## Giai đoạn 5 — STL (Standard Template Library)

- Containers: vector, array, map, unordered_map, deque, list — độ phức tạp (Big-O) của từng thao tác
- Iterators và iterator category
- Algorithms: sort, find, transform, accumulate...
- `std::string`, `std::string_view`
- Embedded: `std::array` thường thay `std::vector` (tránh heap); `etl` (Embedded Template Library) là lựa chọn thay STL

## Giai đoạn 6 — Modern C++ (C++11 → C++20)

- Lambda expressions, closures, capture by ref/value
- `auto`, structured bindings
- `std::optional`, `std::variant`, `std::any`
- Range-based for, `constexpr if`
- Concepts (C++20) — ràng buộc template rõ ràng hơn SFINAE
- Modules (C++20) — thay thế header truyền thống

## Giai đoạn 7 — Concurrency

- `std::thread`, `std::mutex`, `std::atomic`
- Memory model: happens-before, memory order (acquire/release) — quan trọng khi làm việc với ISR/RTOS
- `std::condition_variable`, futures/promises
- So sánh với FreeRTOS primitives (semaphore, queue) — nhiều khái niệm tương đồng

## Giai đoạn 8 — Kiến trúc & Design ở mức Senior

- Design Patterns áp dụng thực tế (Factory, Observer, State Machine, Strategy) — State Machine pattern rất hợp firmware
- SOLID principles áp dụng cho C++
- Dependency Injection, Interface Segregation cho testability
- PIMPL idiom (giảm compile dependency, ẩn implementation)
- CRTP (Curiously Recurring Template Pattern) — static polymorphism, tránh overhead của virtual function → dùng nhiều trong embedded để tối ưu

## Giai đoạn 9 — Performance & Low-level Understanding

- Cache locality, data-oriented design
- Object layout, padding/alignment (`alignas`, `#pragma pack`)
- Compiler optimization: inline, LTO, hiểu output assembly
- Zero-cost abstraction — triết lý cốt lõi để dùng C++ trên MCU mà không mất performance
- Profiling & benchmarking

## Giai đoạn 10 — Tooling & Engineering Practice (mức Senior thực sự)

- Build system: CMake nâng cao (targets, generator expressions)
- Static analysis: clang-tidy, cppcheck
- Sanitizers: ASan, UBSan (chạy trên host trước khi lên target)
- Unit testing: GoogleTest/Catch2, mocking hardware layer để test logic thuần
- Code review mindset: đọc code người khác viết dở, refactor an toàn
- Embedded C++ subset: Google style guide "no exceptions", MISRA C++ nếu làm automotive/medical

---

### Gợi ý bước tiếp theo

- Biến roadmap thành lịch học theo tuần, gắn với project thực tế (VD: áp CRTP/State Machine vào STM32H523 bootloader)
- Gợi ý sách/tài liệu cụ thể cho từng giai đoạn
