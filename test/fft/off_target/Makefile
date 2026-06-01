CXX      = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -Werror -I../../../fft -I../../../interfaces -I../../../ -DCOMPILING_FFT
SRCS     = ../../../fft/fft.cpp test_fft.cpp
TARGET   = test_fft

.PHONY: test clean

test: $(TARGET)
	./$(TARGET)

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) -o $@ $^

clean:
	rm -f $(TARGET)
