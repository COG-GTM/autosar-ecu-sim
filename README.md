# Adaptive AUTOSAR ECU Simulation (C++)

This project simulates an Adaptive AUTOSAR-style ECU using open-source C++ on Linux/macOS. It progressively evolves from Classic AUTOSAR concepts into Adaptive architecture — all without needing any hardware or licensed tools.

---

## 🚗 Features Implemented

- **Sensor & Controller SWCs** running in parallel POSIX threads
- **ExecutionManager** that launches and manages apps like Adaptive AUTOSAR
- **JSON configuration** (instead of ARXML) for runtime control
- **Config-driven timing** and behavior
- **Logging** of sensor data and controller decisions
- **Shared Memory IPC** (to be replaced by messaging)

---

## 📁 Project Structure
autosar_ecu_sim/ ├── include/ │ ├── shared_memory.hpp │ ├── sensor_swc.hpp │ ├── controller_swc.hpp │ └── execution_manager.hpp ├── src/ │ ├── shared_memory.cpp │ ├── sensor_swc.cpp │ ├── controller_swc.cpp │ └── execution_manager.cpp ├── config.json ├── main.cpp ├── Makefile └── README.md


---

## 🧩 Next Steps (Planned)

- Replace shared memory with **message queues or sockets**
- Implement **service discovery and registration**
- Introduce **lifecycle state machine**
- Simulate **platform deployment**

---

## 🛠 Build & Run

### Prerequisites

- A C++17 compiler (`g++`) and GNU `make`. On Debian/Ubuntu: `sudo apt-get install -y build-essential`. On macOS: `xcode-select --install`.
- No other dependencies: `nlohmann/json` is vendored under `include/nlohmann/`.

### Build and run

Run these from the repository root:

```bash
make clean && make
./build/ecu
```

The ECU reads `config.json` from the current directory, so start it from the repository root (from anywhere else it prints `Failed to open config.json` and exits). It runs until you stop it with `Ctrl+C` (or `SIGTERM`), then prints `[Execution Manager] All apps have shut down.`

While running, the sensor and controller print their readings to the terminal and append them to `sensor_log.txt` and `controller_log.txt` in the current directory. The controller adds `[WARNING: High Temp!]` once the temperature passes `controller.warningThreshold` in `config.json`. `make clean` deletes both log files.

### Tests

```bash
make test
```

This builds and runs the host tests in `tests/` (message queue and shutdown); each prints `PASSED (0 failures)`.
