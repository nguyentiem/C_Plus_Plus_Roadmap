// Bài 11: Smart Pointers — unique_ptr, shared_ptr, weak_ptr
// Biên dịch: g++ -std=c++17 -Wall -Wextra -O2 main.cpp -o bai11_smart_pointers.exe
#include <cstdio>
#include <memory>
#include <utility>

// ---------- Tài nguyên có log để quan sát lifetime ----------
struct Sensor {
    int id;
    explicit Sensor(int i) : id(i) { std::printf("  [Sensor %d] constructor\n", id); }
    ~Sensor() { std::printf("  [Sensor %d] destructor\n", id); }
    void read() const { std::printf("  [Sensor %d] read() = %d\n", id, id * 100); }
};

// ---------- 1. unique_ptr: ownership độc quyền ----------
static std::unique_ptr<Sensor> make_sensor(int id) { // factory: trả ownership cho caller
    return std::make_unique<Sensor>(id);
}

static void demo_unique_ptr() {
    std::printf("--- demo_unique_ptr ---\n");
    auto a = make_sensor(1);
    a->read();
    // auto b = a;              // KHÔNG biên dịch: unique_ptr không copy được
    auto b = std::move(a);      // chuyển ownership: a trở thành nullptr
    std::printf("  sau move: a %s null, b so huu Sensor %d\n",
                a ? "khong" : "==", b->id);
    // b ra khỏi scope -> destructor tự chạy (RAII)
}

// ---------- 2. unique_ptr với custom deleter (giả lập handle C API) ----------
static int open_handle() { std::printf("  open_handle() -> 7\n"); return 7; }
static void close_handle(const int* h) { std::printf("  close_handle(%d)\n", *h); delete h; }

static void demo_custom_deleter() {
    std::printf("--- demo_custom_deleter ---\n");
    using Handle = std::unique_ptr<int, void (*)(const int*)>;
    Handle h(new int(open_handle()), &close_handle);
    std::printf("  dung handle gia tri = %d\n", *h);
    // close_handle được gọi tự động khi h ra khỏi scope
}

// ---------- 3. shared_ptr: control block + use_count ----------
static void demo_shared_ptr() {
    std::printf("--- demo_shared_ptr ---\n");
    auto s1 = std::make_shared<Sensor>(2); // 1 lần cấp phát: object + control block
    std::printf("  use_count = %ld\n", static_cast<long>(s1.use_count()));
    {
        std::shared_ptr<Sensor> s2 = s1;   // copy -> atomic increment (co chi phi!)
        std::printf("  sau copy: use_count = %ld\n", static_cast<long>(s1.use_count()));
        s2->read();
    } // s2 chết -> atomic decrement
    std::printf("  sau scope: use_count = %ld\n", static_cast<long>(s1.use_count()));
} // s1 chết -> count = 0 -> destructor Sensor chạy

// ---------- 4. Vòng shared_ptr gây LEAK, weak_ptr phá vòng ----------
struct NodeLeaky { // parent và child giữ nhau bằng shared_ptr -> vòng!
    std::shared_ptr<NodeLeaky> other;
    ~NodeLeaky() { std::printf("  ~NodeLeaky (neu khong in dong nay => LEAK)\n"); }
};

struct NodeFixed {
    std::shared_ptr<NodeFixed> child;  // chiều sở hữu: parent -> child
    std::weak_ptr<NodeFixed> parent;   // chiều ngược: KHÔNG sở hữu
    ~NodeFixed() { std::printf("  ~NodeFixed\n"); }
};

static void demo_weak_ptr_cycle() {
    std::printf("--- demo_weak_ptr_cycle ---\n");
    std::printf(" a) Vong shared_ptr (destructor KHONG chay -> leak):\n");
    {
        auto p = std::make_shared<NodeLeaky>();
        auto c = std::make_shared<NodeLeaky>();
        p->other = c;
        c->other = p; // strong count cua ca hai = 2, khong bao gio ve 0
    }
    std::printf("    (khong thay ~NodeLeaky nao duoc in -> 2 object bi leak)\n");

    std::printf(" b) Sua bang weak_ptr (destructor chay du):\n");
    {
        auto p = std::make_shared<NodeFixed>();
        auto c = std::make_shared<NodeFixed>();
        p->child = c;
        c->parent = p; // weak: khong tang strong count
        if (auto locked = c->parent.lock()) {
            std::printf("    lock() thanh cong: parent con song\n");
        }
    } // p chết -> c chết -> cả hai destructor chạy
}

// ---------- 5. weak_ptr quan sát object đã chết (chống dangling) ----------
static void demo_weak_observe() {
    std::printf("--- demo_weak_observe ---\n");
    std::weak_ptr<Sensor> w;
    {
        auto s = std::make_shared<Sensor>(3);
        w = s;
        std::printf("  trong scope: expired = %s\n", w.expired() ? "true" : "false");
    }
    // Raw pointer ở đây sẽ dangling; weak_ptr thì biết object đã chết:
    std::printf("  ngoai scope: expired = %s, lock() %s\n",
                w.expired() ? "true" : "false",
                w.lock() ? "co object" : "tra ve nullptr (an toan)");
}

int main() {
    std::printf("=== Bai 11: Smart Pointers ===\n");
    demo_unique_ptr();
    demo_custom_deleter();
    demo_shared_ptr();
    demo_weak_ptr_cycle();
    demo_weak_observe();
    std::printf("=== Ket thuc ===\n");
    return 0;
}
