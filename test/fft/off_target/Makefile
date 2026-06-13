CXX      = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -Werror -I. -I../../../fft -I../../../interfaces -I../../../ -DCOMPILING_FFT -include progmem_shim.h
LDFLAGS  = -lm
SRCS     = ../../../fft/fft.cpp test_fft.cpp
TARGET   = test_fft

PERF_SRCS   = ../../../fft/fft.cpp perf_fft.cpp
PERF_TARGET = perf_fft

.PHONY: test perf clean

test: $(TARGET)
	./$(TARGET)

perf: $(PERF_TARGET)
	./$(PERF_TARGET)

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

$(PERF_TARGET): $(PERF_SRCS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(TARGET) $(PERF_TARGET)
