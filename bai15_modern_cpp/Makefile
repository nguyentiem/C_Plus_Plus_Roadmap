CXX ?= g++
CXXFLAGS ?= -std=c++20 -Wall -Wextra -O2
TARGET ?= bai15_modern_cpp.exe

all: $(TARGET)

$(TARGET): main.cpp
	$(CXX) $(CXXFLAGS) main.cpp -o $(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: all clean
