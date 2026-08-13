// Bài 13: STL Containers — array, vector, deque, list, map, unordered_map, set
// Biên dịch: g++ -std=c++17 -Wall -Wextra -O2 main.cpp -o bai13_stl_containers.exe
#include <algorithm>
#include <array>
#include <chrono>
#include <cstdio>
#include <deque>
#include <list>
#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

// ---------- 1. std::array: size cố định, không heap ----------
static void demo_array() {
    std::printf("--- demo_array ---\n");
    std::array<int, 5> regs{10, 20, 30, 40, 50}; // nằm trên stack
    std::printf("  size = %zu, front = %d, back = %d\n",
                regs.size(), regs.front(), regs.back());
    // regs.at(9);  // .at() kiểm tra biên (ném exception) — operator[] thì không
}

// ---------- 2. vector: capacity, reallocation, iterator invalidation ----------
static void demo_vector() {
    std::printf("--- demo_vector ---\n");
    std::vector<int> v;
    std::size_t last_cap = 0;
    for (int i = 1; i <= 17; ++i) {
        v.push_back(i);
        if (v.capacity() != last_cap) { // capacity thay đổi = vừa reallocate
            std::printf("  size = %2zu -> REALLOCATE, capacity = %zu\n",
                        v.size(), v.capacity());
            last_cap = v.capacity();
        }
    }
    // Iterator invalidation: pointer trước reallocation trở thành dangling
    std::vector<int> w;
    w.reserve(2);
    w.push_back(1);
    const int* p_truoc = &w[0];
    w.push_back(2);
    w.push_back(3); // vượt capacity -> reallocate -> p_truoc DANGLING
    std::printf("  dia chi truoc: %p, sau reallocate: %p (%s)\n",
                static_cast<const void*>(p_truoc), static_cast<const void*>(&w[0]),
                p_truoc == &w[0] ? "giong nhau" : "KHAC -> con tro cu dangling!");
}

// ---------- 3. deque và list ----------
static void demo_deque_list() {
    std::printf("--- demo_deque_list ---\n");
    std::deque<int> dq;
    dq.push_back(2);
    dq.push_front(1); // O(1) hai đầu — vector không làm được push_front O(1)
    dq.push_back(3);
    std::printf("  deque: %d %d %d\n", dq[0], dq[1], dq[2]);

    std::list<int> ls{1, 2, 4};
    auto it = std::next(ls.begin(), 2);
    ls.insert(it, 3); // O(1) khi đã có iterator; iterator khác KHÔNG bị invalidate
    std::printf("  list sau insert: ");
    for (int x : ls) std::printf("%d ", x);
    std::printf("\n");
}

// ---------- 4. map vs unordered_map ----------
static void demo_maps() {
    std::printf("--- demo_maps ---\n");
    std::map<std::string, int> cfg{{"baud", 115200}, {"addr", 0x76}, {"freq", 32}};
    std::printf("  map (tu dong sap xep theo key):\n");
    for (const auto& [key, val] : cfg) {
        std::printf("    %-5s = %d\n", key.c_str(), val);
    }

    std::unordered_map<int, std::string> id2err{
        {0, "OK"}, {1, "TIMEOUT"}, {2, "CRC"}};
    auto found = id2err.find(1); // O(1) trung bình
    if (found != id2err.end()) {
        std::printf("  unordered_map: loi %d = %s\n", found->first,
                    found->second.c_str());
    }
    // Bẫy: operator[] TẠO phần tử mới nếu key chưa có
    std::printf("  truoc [9]: size = %zu; ", id2err.size());
    std::printf("id2err[9] = \"%s\"; ", id2err[9].c_str()); // chèn chuỗi rỗng!
    std::printf("sau: size = %zu\n", id2err.size());
}

// ---------- 5. set + erase-remove idiom ----------
static void demo_set_erase() {
    std::printf("--- demo_set_erase ---\n");
    std::set<int> s{5, 1, 3, 1, 5}; // tự loại trùng + sắp xếp
    std::printf("  set: ");
    for (int x : s) std::printf("%d ", x);
    std::printf("(trung bi loai, co thu tu)\n");

    std::vector<int> v{1, 0, 2, 0, 3, 0, 4};
    v.erase(std::remove(v.begin(), v.end(), 0), v.end()); // erase-remove idiom
    std::printf("  vector sau khi xoa moi so 0: ");
    for (int x : v) std::printf("%d ", x);
    std::printf("\n");
}

// ---------- 6. Cache locality: vector vs list (duyệt + cộng dồn) ----------
static void demo_cache_locality() {
    std::printf("--- demo_cache_locality ---\n");
    constexpr int N = 200000;
    std::vector<int> v(N, 1);
    std::list<int> ls(N, 1);
    using clk = std::chrono::steady_clock;

    auto t0 = clk::now();
    long long sv = 0;
    for (int x : v) sv += x;      // bộ nhớ liên tục -> prefetch hiệu quả
    auto t1 = clk::now();
    long long sl = 0;
    for (int x : ls) sl += x;     // nhảy con trỏ khắp heap -> cache miss
    auto t2 = clk::now();

    auto us = [](auto a, auto b) {
        return std::chrono::duration_cast<std::chrono::microseconds>(b - a).count();
    };
    std::printf("  duyet %d phan tu: vector = %lld us, list = %lld us (sum %lld/%lld)\n",
                N, static_cast<long long>(us(t0, t1)),
                static_cast<long long>(us(t1, t2)), sv, sl);
}

int main() {
    std::printf("=== Bai 13: STL Containers ===\n");
    demo_array();
    demo_vector();
    demo_deque_list();
    demo_maps();
    demo_set_erase();
    demo_cache_locality();
    std::printf("=== Ket thuc ===\n");
    return 0;
}
