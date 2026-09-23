CXX=g++
CXXFLAGS=-std=c++17 -Iinclude -pthread
LDFLAGS=

SRC=src/sensor_swc.cpp src/controller_swc.cpp src/execution_manager.cpp src/service_registry.cpp src/lifecycle.cpp

all: ecu

ecu: src/main.cpp $(SRC)
	$(CXX) $(CXXFLAGS) -o build/ecu src/main.cpp $(SRC) $(LDFLAGS)

clean:
	rm -f build/ECU sensor_log.txt controller_log.txt
