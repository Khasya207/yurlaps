# Unit-test host untuk firmware YurFFB (tanpa hardware / tanpa Arduino IDE).
# Pakai:  make -C ffb/firmware/test
CXX ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -I.
FW = ../ffb_wheel
BIN = /tmp/yurffb_test

all: run_as5600 run_quadrature run_pot

# Varian utama (AS5600): full runtime test
run_as5600:
	$(CXX) $(CXXFLAGS) -D__AVR_ATmega328P__ test_main.cpp $(FW)/link.cpp $(FW)/motor.cpp $(FW)/encoder.cpp -o $(BIN)_as5600 && $(BIN)_as5600

# Varian quadrature & potensiometer: runtime test ringkas + cek kompilasi
run_quadrature:
	$(CXX) $(CXXFLAGS) -D__AVR_ATmega328P__ -DENCODER_TYPE=2 test_main.cpp $(FW)/link.cpp $(FW)/motor.cpp $(FW)/encoder.cpp -o $(BIN)_quad && $(BIN)_quad

run_pot:
	$(CXX) $(CXXFLAGS) -D__AVR_ATmega328P__ -DENCODER_TYPE=3 test_main.cpp $(FW)/link.cpp $(FW)/motor.cpp $(FW)/encoder.cpp -o $(BIN)_pot && $(BIN)_pot

# Cek sintaks varian driver PWM+DIR (Cytron) & opsi PWM non-20kHz
build_variants:
	@for def in "-DDRIVER_TYPE=2" "-DPWM_20KHZ=0" "-DDRIVER_TYPE=2 -DPWM_20KHZ=0"; do \
		echo "-- $$def"; \
		$(CXX) $(CXXFLAGS) -D__AVR_ATmega328P__ $$def -fsyntax-only test_main.cpp $(FW)/link.cpp $(FW)/motor.cpp $(FW)/encoder.cpp || exit 1; \
	done
	@echo "-- ESP32 (syntax only)"
	@$(CXX) $(CXXFLAGS) -DARDUINO_ARCH_ESP32 -fsyntax-only $(FW)/link.cpp || exit 1

.PHONY: all run_as5600 run_quadrature run_pot build_variants
