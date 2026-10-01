CXX=g++
CXXFLAGS=-std=c++17 -Iinclude -pthread -Wall -Wextra
LDFLAGS=

SRC=src/sensor_swc.cpp src/controller_swc.cpp src/diagnostic_swc.cpp src/alarm_state.cpp src/ecu_config.cpp src/execution_manager.cpp src/service_registry.cpp src/lifecycle.cpp
TESTS=test_message_queue test_shutdown test_hysteresis_alarm test_config_v2 test_diagnostic_buffer

all: ecu

build:
	mkdir -p build

ecu: build src/main.cpp $(SRC)
	$(CXX) $(CXXFLAGS) -o build/ecu src/main.cpp $(SRC) $(LDFLAGS)

build/test_message_queue: build tests/test_message_queue.cpp include/message_queue.hpp
	$(CXX) $(CXXFLAGS) -o $@ tests/test_message_queue.cpp $(LDFLAGS)

build/test_%: build tests/test_%.cpp $(SRC)
	$(CXX) $(CXXFLAGS) -o $@ tests/test_$*.cpp $(SRC) $(LDFLAGS)

.PHONY: test
test: $(addprefix build/,$(TESTS))
	@set -e; for t in $(TESTS); do ./build/$$t; done

.PHONY: clean
clean:
	rm -f build/ecu $(addprefix build/,$(TESTS)) sensor_log.txt controller_log.txt
	rm -rf diagnostics
