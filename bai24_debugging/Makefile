CXX ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -g -O0 -pthread
TARGET ?= bai24_debugging.exe

all: $(TARGET)

$(TARGET): main.cpp
	$(CXX) $(CXXFLAGS) main.cpp -o $(TARGET)

# ASan + UBSan (MinGW co the chua ho tro ASan — dung WSL/Linux neu loi)
asan: main.cpp
	$(CXX) $(CXXFLAGS) -fsanitize=address,undefined -fno-omit-frame-pointer main.cpp -o $(TARGET)

# ThreadSanitizer — build RIENG, khong tron voi ASan
tsan: main.cpp
	$(CXX) $(CXXFLAGS) -fsanitize=thread main.cpp -o $(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: all asan tsan clean
