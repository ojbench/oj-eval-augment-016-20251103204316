CXX = g++
CXXFLAGS = -std=c++17 -O2 -Wall
TARGET = code

all: $(TARGET)

$(TARGET): main.cpp
	$(CXX) $(CXXFLAGS) -o $(TARGET) main.cpp

clean:
	rm -f $(TARGET) bptree.dat

.PHONY: all clean

