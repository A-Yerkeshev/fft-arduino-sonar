CXX      = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -Werror -I. -I../../../fft -I../../../interfaces -I../../../ -DCOMPILING_FFT -include progmem_shim.h
LDFLAGS  = -lm
SRCS     = ../../../fft/fft.cpp test_fft.cpp
TARGET   = test_fft

.PHONY: test clean

test: $(TARGET)
	./$(TARGET)

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(TARGET)
