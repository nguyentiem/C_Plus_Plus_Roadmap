# Mục lục khóa học C++ — Cơ bản → Nâng cao → Senior (thiên hướng Embedded)

> Xây dựng theo `docs/roadmap-cpp-senior.md` và `docs/roadmap_senior_cpp_embedded_techlead.md`.
> Mỗi bài là 1 folder gồm: file `.md` (lý thuyết, định nghĩa, giải thích, tips & tricks, ghi chú embedded, bài tập)
> và `main.cpp` (ví dụ chạy được) + `Makefile` (build bằng `make`, chạy `./baiXX_ten.exe`).

## Cấp độ 1 — Cơ bản (Foundation)

| Bài | Folder | Nội dung chính |
|-----|--------|----------------|
| 01 | `bai1_kieu_du_lieu` | Kiểu dữ liệu, khác biệt C vs C++, namespace, overloading, ép kiểu tổng quan |
| 02 | `bai02_con_tro_tham_chieu` | Con trỏ, tham chiếu, mảng, const pointer vs pointer to const, dangling, nullptr |
| 03 | `bai03_class_raii` | Class, constructor/destructor, encapsulation, `this`, static/friend, **RAII** |
| 04 | `bai04_stack_heap_lifetime` | Stack/Heap/Static storage, object lifetime, new/delete, leak, fragmentation |
| 05 | `bai05_compile_process` | Preprocessor → Compiler → Linker, include guard, ODR, name mangling, .text/.data/.bss |

## Cấp độ 2 — Trung cấp (OOP, Templates, Memory, STL)

| Bài | Folder | Nội dung chính |
|-----|--------|----------------|
| 06 | `bai06_ke_thua_da_hinh` | Kế thừa, virtual/pure virtual, vtable/vptr, virtual destructor, slicing, diamond problem |
| 07 | `bai07_operator_const_correctness` | Operator overloading, const correctness, mutable, static members |
| 08 | `bai08_casting_pod` | 4 loại cast, POD/trivially-copyable, map struct vào register/frame, strict aliasing |
| 09 | `bai09_templates` | Function/class template, specialization, non-type param, variadic + fold expressions |
| 10 | `bai10_constexpr_traits_sfinae` | constexpr/consteval, type traits, SFINAE, static_assert, bảng CRC compile-time |
| 11 | `bai11_smart_pointers` | unique_ptr/shared_ptr/weak_ptr, ownership, circular reference, câu hỏi lifetime |
| 12 | `bai12_move_semantics` | lvalue/rvalue, std::move/forward, Rule of 3/5/0, copy elision/RVO |
| 13 | `bai13_stl_containers` | array/vector/map/unordered_map..., Big-O, iterator invalidation, cache locality |
| 14 | `bai14_algorithms_string` | Algorithms, erase-remove idiom, string/SSO, string_view, span, chrono |
| 15 | `bai15_modern_cpp` | Lambda, auto, structured bindings, optional/variant/any, if constexpr, concepts (C++20) |

## Cấp độ 3 — Senior (Concurrency, Kiến trúc, Performance, Tooling)

| Bài | Folder | Nội dung chính |
|-----|--------|----------------|
| 16 | `bai16_concurrency` | thread/mutex/atomic, condition_variable, memory order, deadlock, false sharing, so sánh FreeRTOS |
| 17 | `bai17_design_patterns` | Factory, Strategy, Observer, **State Machine** (rất hợp firmware), Command |
| 18 | `bai18_solid_di_crtp` | SOLID, Dependency Injection + mock hardware, PIMPL, **CRTP** static polymorphism |
| 19 | `bai19_performance` | Padding/alignment, cache locality, AoS vs SoA benchmark, zero-cost abstraction |
| 20 | `bai20_tooling_testing` | CMake, clang-tidy/cppcheck, sanitizers, unit test + mock, MISRA C++ |

## Cách học đề xuất

1. Mỗi bài: đọc `.md` → đọc `main.cpp` → `make` và chạy → tự sửa code thí nghiệm → làm bài tập cuối bài.
2. Thứ tự ưu tiên (theo roadmap): Core → Memory/Lifetime/Ownership → OOP/RAII → Templates/STL → Compiler/Linker → Concurrency → Embedded → Architecture.
3. Sau bài 20: làm các project trong mục 18 của `docs/roadmap_senior_cpp_embedded_techlead.md` (Modern C++ Library, Embedded Driver Framework, RTOS Communication System...).

## Build

```bash
cd baiXX_ten
make          # build
./baiXX_ten.exe
make clean
```

Yêu cầu: g++ (MinGW/MSYS2), bài 15 cần C++20, bài 16 dùng -pthread.
