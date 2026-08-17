Giai đoạn 1: Lập trình hướng đối tượng (OOP) (3–4 tuần)
Mục tiêu: Nắm vững class, kế thừa, đa hình, RAII – nền tảng của C++.

Kiến thức cần nắm:
Class & Struct
Khác biệt: class mặc định private, struct mặc định public.

Constructors: default, parameterized, copy, move.

Destructor, initializer list.

this pointer.

static members (biến/hàm static).

Copy & Move (giới thiệu)
Copy constructor, copy assignment operator.

Rule of Three (hiểu trước, sau nâng lên Rule of Five).

Operator Overloading
Nạp chồng toán tử: +, -, <<, >>, [], (), ==, <.

Friend functions.

Inheritance (Kế thừa)
public, protected, private inheritance.

Virtual functions, override, final.

Pure virtual functions, abstract class.

Virtual destructor (quan trọng!).

Vtable cơ bản (hiểu cơ chế).

Polymorphism (Đa hình)
Con trỏ/tham chiếu base class trỏ tới derived class.

Gọi hàm ảo, dynamic binding.

RAII (Resource Acquisition Is Initialization)
Tài nguyên được giải phóng tự động khi đối tượng ra khỏi phạm vi.

Ứng dụng: quản lý file, memory, mutex.

Composition vs Inheritance
Ưu tiên composition khi có thể.

Bài tập:
Xây dựng hệ thống quản lý thư viện: lớp Book, Member, Library.

Xây dựng hệ thống quản lý nhân viên: Employee (abstract), Manager, Engineer, Intern.

Viết lớp String đơn giản (quản lý bộ nhớ động) để hiểu copy constructor, destructor, assignment.

Tài nguyên:
Sách: C++ Primer (chương về class, OOP).

Website: learncpp.com (chương 13–18).

📦 Giai đoạn 2: STL và Modern C++ nền tảng (4–5 tuần)
Mục tiêu: Sử dụng thành thạo thư viện chuẩn, lambda, smart pointers, move semantics.

Kiến thức cần nắm:
STL Containers
Sequence: std::vector, std::deque, std::list, std::forward_list, std::array.

Associative: std::map, std::multimap, std::set, std::multiset.

Unordered: std::unordered_map, std::unordered_set.

Adapters: std::stack, std::queue, std::priority_queue.

Iterators
Các loại iterator: input, output, forward, bidirectional, random access.

Sử dụng với algorithms.

Algorithms
std::sort, std::find, std::transform, std::accumulate, std::copy, std::remove_if, std::for_each.

Lambda expressions: [capture](params) -> ret { body }.

Function objects, std::function.

Smart Pointers (C++11)
std::unique_ptr, std::shared_ptr, std::weak_ptr.

std::make_unique, std::make_shared.

Tránh con trỏ trần sở hữu tài nguyên.

Move Semantics (C++11)
Rvalue references (T&&).

Move constructor, move assignment operator.

std::move, std::forward.

Rule of Five (hoặc Rule of Zero).

Hiểu tại sao move giúp tăng hiệu năng (tránh copy).

Exceptions
try, catch, throw.

noexcept, exception safety.

Tương tác với destructors (RAII).

std::string, std::string_view
Xử lý chuỗi hiện đại.

Bài tập:
Viết chương trình đếm tần suất từ trong file văn bản: dùng std::unordered_map, std::string, std::ifstream.

Viết lớp MyVector đơn giản giống std::vector, hỗ trợ move semantics.

Viết chương trình quản lý sinh viên dùng vector, sort, lambda.

Viết hàm trả về std::unique_ptr để quản lý đối tượng.

Tài nguyên:
Sách: Effective Modern C++ (Scott Meyers) – đọc dần.

Website: cppreference.com (tra cứu STL).

The Cherno (YouTube) – series C++.

🚀 Giai đoạn 3: Templates, Move Semantics nâng cao & C++20 (3–4 tuần)
Mục tiêu: Hiểu sâu templates, type traits, concepts, và các tính năng hiện đại.

Kiến thức cần nắm:
Templates nâng cao
Function templates, class templates.

Template specialization (full, partial).

Variadic templates (C++11).

SFINAE (Substitution Failure Is Not An Error).

if constexpr (C++17).

Concepts (C++20) – ràng buộc template.

Type Traits
<type_traits>: std::is_integral, std::is_same, std::enable_if, std::decay, std::remove_reference.

Ứng dụng trong template metaprogramming.

constexpr & Compile-time
constexpr functions, variables.

consteval (C++20), constinit (C++20).

static_assert.

C++17/20 Features
Structured bindings: auto [a, b] = pair;.

std::optional, std::variant, std::any.

std::string_view, std::span (C++20).

Ranges (C++20): std::ranges::sort, views, adaptors.

Modules (C++20) – tìm hiểu tổng quan.

Coroutines (C++20) – tìm hiểu tổng quan.

Bài tập:
Viết hàm template tính tổng các phần tử của bất kỳ container nào.

Viết std::enable_if để giới hạn template chỉ nhận kiểu số nguyên.

Sử dụng std::variant để lưu các kiểu khác nhau.

Tìm hiểu và chạy thử ví dụ về concepts (C++20).

Tài nguyên:
Sách: C++ Templates: The Complete Guide (Vandevoorde, Josuttis).

Blog: C++ Stories, Fluent C++.

🧵 Giai đoạn 4: Đa luồng & Hiệu năng (3–4 tuần)
Mục tiêu: Viết chương trình concurrency an toàn, hiểu memory model.

Kiến thức cần nắm:
Threading cơ bản
std::thread, std::async, std::future, std::promise, std::packaged_task.

Truyền tham số cho thread, nhận kết quả.

Đồng bộ hóa
std::mutex, std::lock_guard, std::unique_lock, std::scoped_lock (C++17).

std::condition_variable.

std::atomic và các thao tác atomic.

Data races, deadlock, livelock.

Memory Model
Hiểu khái niệm happens-before, sequential consistency.

Tránh data race.

Parallel Algorithms (C++17)
std::execution::par, std::execution::par_unseq.

Hiệu năng
Move semantics giúp tránh copy.

Cache locality, data-oriented design.

Benchmark, profiling (dùng std::chrono, Google Benchmark).

Bài tập:
Viết thread pool đơn giản.

Viết chương trình tính tổng mảng song song với std::async.

Viết producer-consumer sử dụng std::mutex và std::condition_variable.

So sánh hiệu năng copy vs move.

Tài nguyên:
Sách: C++ Concurrency in Action (Anthony Williams).

🛠️ Giai đoạn 5: Công cụ, Testing & Dự án thực tế (liên tục)
Mục tiêu: Sử dụng thành thạo công cụ phát triển, viết code chất lượng, có dự án thực tế.

Công cụ
Build system: CMake (quan trọng nhất), Makefile.

Package manager: vcpkg, Conan.

Testing: Google Test, Catch2, doctest.

Debugging: gdb, lldb, Valgrind, AddressSanitizer, UndefinedBehaviorSanitizer.

Code quality: clang-format, clang-tidy, cppcheck.

IDE: Visual Studio, CLion, VSCode + C/C++ extension.

Dự án gợi ý (theo độ khó tăng dần)
CLI tool: xử lý file, thống kê từ, tìm kiếm.

Hệ thống quản lý (thư viện, sinh viên, nhân viên) – OOP + STL.

Custom containers: viết MyVector, MyString, MyUniquePtr để hiểu sâu.

Chat server/client dùng TCP socket + multithreading.

Game 2D đơn giản với SFML/SDL.

Web server nhỏ với HTTP/1.1.

Tham gia open-source trên GitHub.

Design Patterns trong C++
Singleton, Factory, Observer, Strategy, Adapter, RAII-based patterns.