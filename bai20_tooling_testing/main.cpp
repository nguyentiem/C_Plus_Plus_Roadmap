// Bai 20: Tooling & Testing — micro test framework (~30 dong) + test ring buffer
// + mocking hardware layer. Bien dich: g++ -std=c++17 -Wall -Wextra -O2
#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>

// ================================================================
// Micro test framework (~30 dong): minh hoa ruot cua GoogleTest/Catch2
// = macro kiem tra + danh sach test + runner dem pass/fail
// ================================================================
namespace microtest {

struct KetQua {
    int pass = 0;
    int fail = 0;
};
inline KetQua g_kq;
inline const char* g_ten_test = "";

// Macro kiem tra: in ro file:dong khi fail — gia tri cot loi cua framework
#define TEST_CHECK(dieu_kien)                                              \
    do {                                                                   \
        if (dieu_kien) {                                                   \
            ++microtest::g_kq.pass;                                        \
        } else {                                                           \
            ++microtest::g_kq.fail;                                        \
            std::cout << "  [FAIL] " << microtest::g_ten_test << " tai "   \
                      << __FILE__ << ":" << __LINE__ << "  (" #dieu_kien   \
                      << ")\n";                                            \
        }                                                                  \
    } while (0)

inline void chay(const char* ten, void (*ham)()) {
    g_ten_test = ten;
    const int fail_truoc = g_kq.fail;
    ham();
    std::cout << "  [" << (g_kq.fail == fail_truoc ? "PASS" : "FAIL")
              << "] " << ten << "\n";
}

inline int tongKet() {
    std::cout << "\nTong ket: " << g_kq.pass << " check pass, "
              << g_kq.fail << " check fail\n";
    return g_kq.fail == 0 ? 0 : 1;
}

} // namespace microtest

// ================================================================
// Doi tuong duoc test: ring buffer tinh (khong heap — phong cach MCU)
// ================================================================
template <typename T, std::size_t N>
class RingBuffer {
public:
    bool push(const T& v) {
        if (soPhanTu_ == N) return false; // day
        buf_[dau_] = v;
        dau_ = (dau_ + 1) % N;
        ++soPhanTu_;
        return true;
    }
    bool pop(T& out) {
        if (soPhanTu_ == 0) return false; // rong
        out = buf_[duoi_];
        duoi_ = (duoi_ + 1) % N;
        --soPhanTu_;
        return true;
    }
    std::size_t size() const { return soPhanTu_; }
    bool empty() const { return soPhanTu_ == 0; }
    bool full() const { return soPhanTu_ == N; }

private:
    std::array<T, N> buf_{};
    std::size_t dau_ = 0;   // vi tri ghi tiep theo
    std::size_t duoi_ = 0;  // vi tri doc tiep theo
    std::size_t soPhanTu_ = 0;
};

// ================================================================
// Cac test cho ring buffer — moi test MOT hanh vi, co test bien
// ================================================================
void test_moi_tao_thi_rong() {
    RingBuffer<int, 4> rb;
    TEST_CHECK(rb.empty());
    TEST_CHECK(rb.size() == 0);
    int v = 0;
    TEST_CHECK(!rb.pop(v)); // pop tu buffer rong phai that bai
}

void test_push_roi_pop_dung_thu_tu_fifo() {
    RingBuffer<int, 4> rb;
    TEST_CHECK(rb.push(10));
    TEST_CHECK(rb.push(20));
    int v = 0;
    TEST_CHECK(rb.pop(v) && v == 10); // FIFO: ra dung thu tu vao
    TEST_CHECK(rb.pop(v) && v == 20);
    TEST_CHECK(rb.empty());
}

void test_day_thi_push_that_bai() {
    RingBuffer<int, 2> rb;
    TEST_CHECK(rb.push(1));
    TEST_CHECK(rb.push(2));
    TEST_CHECK(rb.full());
    TEST_CHECK(!rb.push(3)); // day -> tu choi, khong ghi de
    int v = 0;
    TEST_CHECK(rb.pop(v) && v == 1); // phan tu cu con nguyen
}

void test_wrap_around() {
    RingBuffer<int, 3> rb;
    int v = 0;
    // Day chi so dau/duoi vuot qua bien mang nhieu lan
    for (int vong = 0; vong < 5; ++vong) {
        TEST_CHECK(rb.push(vong * 2));
        TEST_CHECK(rb.push(vong * 2 + 1));
        TEST_CHECK(rb.pop(v) && v == vong * 2);
        TEST_CHECK(rb.pop(v) && v == vong * 2 + 1);
    }
    TEST_CHECK(rb.empty());
}

// ================================================================
// Mocking hardware layer: test logic doc cam bien co retry
// ================================================================
class II2c { // interface HAL mong (bai 18: DIP + ISP)
public:
    virtual ~II2c() = default;
    virtual bool docThanhGhi(uint8_t diaChi, uint8_t& giaTri) = 0;
};

// Mock: cai san kich ban loi/thanh cong, ghi lai so lan bi goi
class MockI2c : public II2c {
public:
    explicit MockI2c(int soLanLoiDau) : conLoi_(soLanLoiDau) {}
    bool docThanhGhi(uint8_t diaChi, uint8_t& giaTri) override {
        ++soLanGoi_;
        diaChiCuoi_ = diaChi;
        if (conLoi_ > 0) { --conLoi_; return false; } // gia lap bus loi
        giaTri = 0x42;
        return true;
    }
    int soLanGoi() const { return soLanGoi_; }
    uint8_t diaChiCuoi() const { return diaChiCuoi_; }

private:
    int conLoi_;
    int soLanGoi_ = 0;
    uint8_t diaChiCuoi_ = 0;
};

// Logic can test: doc cam bien, retry toi da 3 lan
class BoDocCamBien {
public:
    explicit BoDocCamBien(II2c& i2c) : i2c_(i2c) {}
    bool doc(uint8_t& giaTri) {
        for (int lan = 0; lan < 3; ++lan)
            if (i2c_.docThanhGhi(0x76, giaTri)) return true;
        return false;
    }

private:
    II2c& i2c_;
};

void test_doc_thanh_cong_ngay_lan_dau() {
    MockI2c mock(0); // khong loi
    BoDocCamBien bo(mock);
    uint8_t v = 0;
    TEST_CHECK(bo.doc(v));
    TEST_CHECK(v == 0x42);
    TEST_CHECK(mock.soLanGoi() == 1);      // khong retry thua
    TEST_CHECK(mock.diaChiCuoi() == 0x76); // dung dia chi thanh ghi
}

void test_retry_khi_bus_loi_tam_thoi() {
    MockI2c mock(2); // loi 2 lan dau, lan 3 thanh cong
    BoDocCamBien bo(mock);
    uint8_t v = 0;
    TEST_CHECK(bo.doc(v));
    TEST_CHECK(mock.soLanGoi() == 3); // dung 3 lan goi
}

void test_bo_cuoc_sau_3_lan_loi() {
    MockI2c mock(99); // loi mai
    BoDocCamBien bo(mock);
    uint8_t v = 0;
    TEST_CHECK(!bo.doc(v));
    TEST_CHECK(mock.soLanGoi() == 3); // khong retry vo han
}

// ================================================================
int main() {
    std::cout << "Bai 20: Tooling & Testing — micro test framework demo\n\n";
    std::cout << "=== Test ring buffer ===\n";
    microtest::chay("moi tao thi rong", test_moi_tao_thi_rong);
    microtest::chay("push/pop dung thu tu FIFO", test_push_roi_pop_dung_thu_tu_fifo);
    microtest::chay("day thi push that bai", test_day_thi_push_that_bai);
    microtest::chay("wrap-around nhieu vong", test_wrap_around);

    std::cout << "\n=== Test logic cam bien voi MockI2c (khong can phan cung) ===\n";
    microtest::chay("doc thanh cong lan dau", test_doc_thanh_cong_ngay_lan_dau);
    microtest::chay("retry khi bus loi tam thoi", test_retry_khi_bus_loi_tam_thoi);
    microtest::chay("bo cuoc sau 3 lan loi", test_bo_cuoc_sau_3_lan_loi);

    return microtest::tongKet();
}
