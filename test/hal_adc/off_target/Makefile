CXX      = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -Werror -I. -I../../../hal_adc -I../../../ -DCOMPILING_HAL_ADC
SRCS     = ../../../hal_adc/hal_adc.cpp arduino_stub.cpp test_hal_adc.cpp
TARGET   = test_hal_adc

.PHONY: test clean

test: $(TARGET)
	./$(TARGET)

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) -o $@ $^

clean:
	rm -f $(TARGET)
