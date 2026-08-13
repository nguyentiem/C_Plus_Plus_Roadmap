// Bài 15: Modern C++ — lambda, auto/decltype, structured bindings,
//         optional/variant/any, if constexpr, concepts (C++20)
// Biên dịch: g++ -std=c++20 -Wall -Wextra -O2 main.cpp -o bai15_modern_cpp.exe
#include <any>
#include <concepts>
#include <cstdio>
#include <map>
#include <optional>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>

// ---------- 1. Lambda: capture value/ref, mutable, generic ----------
static void demo_lambda() {
    std::printf("--- demo_lambda ---\n");
    int base = 10;
    auto by_val = [base](int x) { return base + x; };   // copy base lúc TẠO lambda
    auto by_ref = [&base](int x) { return base + x; };  // tham chiếu base "sống"
    base = 100;
    std::printf("  by_val(1) = %d (giu ban copy cu = 10)\n", by_val(1));
    std::printf("  by_ref(1) = %d (thay gia tri moi = 100)\n", by_ref(1));

    // mutable: sửa BẢN COPY trong closure, biến gốc không đổi
    int dem = 0;
    auto counter = [dem]() mutable { return ++dem; };
    counter(); counter();
    std::printf("  counter goi 2 lan -> trong closure = %d, bien goc dem = %d\n",
                counter(), dem);

    // generic lambda: operator() là template
    auto mul = [](auto a, auto b) { return a * b; };
    std::printf("  generic: mul(3,4) = %d, mul(2.5,2.0) = %.1f\n",
                mul(3, 4), mul(2.5, 2.0));
    // BẪY (không chạy): lambda [&] giao cho thread/timer chạy sau khi
    // scope này kết thúc -> tham chiếu dangling. Callback sống lâu: capture by value!
}

// ---------- 2. auto & decltype + structured bindings ----------
static void demo_auto_bindings() {
    std::printf("--- demo_auto_bindings ---\n");
    std::vector<int> v{1, 2, 3};
    auto n = v.size();                    // suy ra size_t
    decltype(v)::value_type first = v[0]; // decltype lấy kiểu chính xác -> int
    static_assert(std::is_same_v<decltype(n), std::size_t>);
    std::printf("  n = %zu, first = %d\n", n, first);

    std::map<std::string, int> cfg{{"baud", 115200}};
    auto [it, inserted] = cfg.insert({"addr", 0x76}); // bung pair<iterator,bool>
    std::printf("  insert \"%s\": inserted = %s\n", it->first.c_str(),
                inserted ? "true" : "false");
    for (const auto& [key, val] : cfg) {
        std::printf("    %-4s = %d\n", key.c_str(), val);
    }
}

// ---------- 3. optional: thay magic value ----------
static std::optional<int> parse_int(const std::string& s) {
    if (s.empty() || s.find_first_not_of("0123456789") != std::string::npos) {
        return std::nullopt; // "không có kết quả" — rõ ràng hơn trả -1
    }
    return std::stoi(s);
}

static void demo_optional() {
    std::printf("--- demo_optional ---\n");
    for (const auto* s : {"42", "abc"}) {
        if (auto r = parse_int(s)) {
            std::printf("  parse(\"%s\") = %d\n", s, *r);
        } else {
            std::printf("  parse(\"%s\") = nullopt (value_or(-1) = %d)\n", s,
                        parse_int(s).value_or(-1));
        }
    }
}

// ---------- 4. variant + visit (pattern overloaded) và any ----------
template <typename... Ts> struct overloaded : Ts... { using Ts::operator()...; };
// (C++20: không cần deduction guide)

static void demo_variant_any() {
    std::printf("--- demo_variant_any ---\n");
    using Event = std::variant<int, double, std::string>;
    std::vector<Event> events{42, 3.14, std::string("BOOT")};
    for (const auto& e : events) {
        std::visit(overloaded{
                       [](int i) { std::printf("  Event int: %d\n", i); },
                       [](double d) { std::printf("  Event double: %.2f\n", d); },
                       [](const std::string& s) {
                           std::printf("  Event string: %s\n", s.c_str());
                       }},
                   e); // compiler BẮT BUỘC đủ nhánh cho mọi kiểu trong variant
    }

    std::any box = 123;                       // chứa kiểu bất kỳ (type-erased)
    std::printf("  any giu int: %d; ", std::any_cast<int>(box));
    box = std::string("now a string");
    std::printf("doi sang string: %s\n", std::any_cast<std::string&>(box).c_str());
}

// ---------- 5. if constexpr: rẽ nhánh compile-time ----------
template <typename T>
static void print_typed(T v) {
    if constexpr (std::is_floating_point_v<T>) {
        std::printf("  float: %.3f\n", static_cast<double>(v));
    } else if constexpr (std::is_integral_v<T>) {
        std::printf("  integer: %lld\n", static_cast<long long>(v));
    } else {
        std::printf("  string: %s\n", v.c_str()); // nhánh này bị LOẠI khi T là số
    }
}

static void demo_if_constexpr() {
    std::printf("--- demo_if_constexpr ---\n");
    print_typed(7);
    print_typed(2.718);
    print_typed(std::string("zephyr"));
}

// ---------- 6. Concepts (C++20): ràng buộc template có tên ----------
template <typename T>
concept Arithmetic = std::is_arithmetic_v<T>;

template <Arithmetic T>            // dạng gọn
static T twice(T v) { return v + v; }

template <typename T>
    requires std::integral<T>      // requires-clause + concept chuẩn
static T half(T v) { return v / 2; }

static void demo_concepts() {
    std::printf("--- demo_concepts ---\n");
    std::printf("  twice(21) = %d, twice(1.5) = %.1f\n", twice(21), twice(1.5));
    std::printf("  half(10)  = %d (chi nhan kieu integral)\n", half(10));
    // twice(std::string("x"));  // loi ro rang: constraint 'Arithmetic' not satisfied
}

int main() {
    std::printf("=== Bai 15: Modern C++ (C++20) ===\n");
    demo_lambda();
    demo_auto_bindings();
    demo_optional();
    demo_variant_any();
    demo_if_constexpr();
    demo_concepts();
    std::printf("=== Ket thuc ===\n");
    return 0;
}
