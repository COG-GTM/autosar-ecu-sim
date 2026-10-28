CXX=g++
CXXFLAGS=-std=c++17 -Iinclude -pthread -Wall -Wextra
LDFLAGS=

SRC=src/sensor_swc.cpp src/controller_swc.cpp src/execution_manager.cpp src/service_registry.cpp src/lifecycle.cpp

all: ecu

build:
	mkdir -p build

ecu: build src/main.cpp $(SRC)
	$(CXX) $(CXXFLAGS) -o build/ecu src/main.cpp $(SRC) $(LDFLAGS)

build/test_message_queue: build tests/test_message_queue.cpp include/message_queue.hpp
	$(CXX) $(CXXFLAGS) -o $@ tests/test_message_queue.cpp $(LDFLAGS)

build/test_shutdown: build tests/test_shutdown.cpp $(SRC)
	$(CXX) $(CXXFLAGS) -o $@ tests/test_shutdown.cpp $(SRC) $(LDFLAGS)

build/test_controller_backlog: build tests/test_controller_backlog.cpp $(SRC)
	$(CXX) $(CXXFLAGS) -o $@ tests/test_controller_backlog.cpp $(SRC) $(LDFLAGS)

TESTS=build/test_message_queue build/test_shutdown build/test_controller_backlog

.PHONY: test
test: $(TESTS)
	for t in $(TESTS); do $(RUN) ./$$t || exit 1; done

# Re-run the suite under ThreadSanitizer, then ASan+UBSan. setarch -R disables ASLR,
# which TSan needs on kernels with high mmap entropy. GCC 11's libtsan does not intercept
# pthread_cond_clockwait (used by condition_variable::wait_for), giving false races:
# use GCC >= 12 or clang, e.g. `make sanitize TSAN_CXX=g++-12`.
SAN_FLAGS=-g -O1 -fno-omit-frame-pointer
TSAN_CXX?=$(CXX)
.PHONY: sanitize
sanitize:
	$(MAKE) clean
	$(MAKE) test CXX=$(TSAN_CXX) CXXFLAGS="$(CXXFLAGS) $(SAN_FLAGS) -fsanitize=thread" \
		RUN="setarch $$(uname -m) -R"
	$(MAKE) clean
	$(MAKE) test CXXFLAGS="$(CXXFLAGS) $(SAN_FLAGS) -fsanitize=address,undefined -fno-sanitize-recover=undefined"
	$(MAKE) clean

.PHONY: clean
clean:
	rm -f build/ecu $(TESTS) sensor_log.txt controller_log.txt
