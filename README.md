# Adaptive AUTOSAR ECU Simulation (C++)

This project simulates an Adaptive AUTOSAR-style ECU using open-source C++ on Linux/macOS. It progressively evolves from Classic AUTOSAR concepts into Adaptive architecture — all without needing any hardware or licensed tools.

---

## 🚗 Features Implemented

- **Sensor & Controller SWCs** running in parallel POSIX threads
- **ExecutionManager** that launches and manages apps like Adaptive AUTOSAR
- **JSON configuration** (instead of ARXML) for runtime control
- **Config-driven timing** and behavior
- **Logging** of sensor data and controller decisions
- **Thread-safe message queue IPC** between the SWCs
- **Service registry** for dynamic service discovery
- **Lifecycle state machine** with graceful shutdown

---

## 📁 Project Structure
```
autosar_ecu_sim/
├── include/
│   ├── message_queue.hpp
│   ├── sensor_types.hpp
│   ├── sensor_swc.hpp
│   ├── controller_swc.hpp
│   ├── service_registry.hpp
│   ├── lifecycle.hpp
│   └── execution_manager.hpp
├── src/
│   ├── main.cpp
│   ├── sensor_swc.cpp
│   ├── controller_swc.cpp
│   ├── service_registry.cpp
│   ├── lifecycle.cpp
│   └── execution_manager.cpp
├── config.json
├── Makefile
└── README.md
```


---

## 🧩 Next Steps (Planned)

- Replace the in-process message queue with **sockets** for cross-process IPC
- Simulate **platform deployment**

---

## 🛠 Build & Run

```bash
make clean && make
./build/ecu
