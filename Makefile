CXX=g++
CXXFLAGS=-std=c++17 -Iinclude -pthread -Wall -Wextra
LDFLAGS=

SRC=src/sensor_swc.cpp src/controller_swc.cpp src/execution_manager.cpp src/service_registry.cpp src/lifecycle.cpp

all: ecu build/stub_interfaces stub-check

build:
	mkdir -p build

ecu: build src/main.cpp $(SRC)
	$(CXX) $(CXXFLAGS) -o build/ecu src/main.cpp $(SRC) $(LDFLAGS)

# AE-9 design gate: the B2.0 interface headers must compile as stubs.
# stub-check verifies each header is self-contained in isolation (a combined
# translation unit could mask a missing include in a later header).
DESIGN_HEADERS=include/alarm_state.hpp include/diagnostic_types.hpp \
	include/diagnostic_swc.hpp include/ecu_config.hpp include/controller_swc.hpp

.PHONY: stub-check
stub-check:
	@for h in $(DESIGN_HEADERS); do \
		echo "stub-check $$h"; \
		echo "#include <$$(basename $$h)>" | $(CXX) $(CXXFLAGS) -fsyntax-only -x c++ - || exit 1; \
	done

build/stub_interfaces: build tests/stub_interfaces.cpp
	$(CXX) $(CXXFLAGS) -o $@ tests/stub_interfaces.cpp $(LDFLAGS)

build/test_message_queue: build tests/test_message_queue.cpp include/message_queue.hpp
	$(CXX) $(CXXFLAGS) -o $@ tests/test_message_queue.cpp $(LDFLAGS)

build/test_shutdown: build tests/test_shutdown.cpp $(SRC)
	$(CXX) $(CXXFLAGS) -o $@ tests/test_shutdown.cpp $(SRC) $(LDFLAGS)

.PHONY: test
test: build/test_message_queue build/test_shutdown
	./build/test_message_queue
	./build/test_shutdown

.PHONY: clean
clean:
	rm -f build/ecu build/test_message_queue build/test_shutdown sensor_log.txt controller_log.txt
