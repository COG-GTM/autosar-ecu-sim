# Adaptive AUTOSAR ECU Simulation (C++)

This project simulates an Adaptive AUTOSAR-style ECU using open-source C++ on Linux/macOS. It progressively evolves from Classic AUTOSAR concepts into Adaptive architecture — all without needing any hardware or licensed tools.

---

## 🚗 Features Implemented

- **Sensor, Controller & Diagnostic SWCs** running in parallel POSIX threads
- **ExecutionManager** that validates config, launches and stops apps in a fixed order
- **ServiceRegistry** discovery (`SensorDataService`, `DiagnosticEventService`) over thread-safe `MessageQueue<T>`
- **Hysteresis alarms** on temperature and pressure (`HysteresisAlarm`): raise at `value >= threshold`, clear only at `value < threshold - hysteresis`
- **Diagnostic event log**: bounded ring buffer of raise/clear events, dumped to `diagnostics/events.json` at shutdown
- **JSON configuration schema v2** (instead of ARXML) for runtime calibration, with v1 aliases

---

## 📁 Project Structure

```
include/  alarm_state.hpp  controller_swc.hpp  diagnostic_swc.hpp  diagnostic_types.hpp
          ecu_config.hpp   execution_manager.hpp  lifecycle.hpp  message_queue.hpp
          sensor_swc.hpp   sensor_types.hpp  service_registry.hpp  nlohmann/
src/      main.cpp  execution_manager.cpp  ecu_config.cpp  sensor_swc.cpp
          controller_swc.cpp  alarm_state.cpp  diagnostic_swc.cpp  service_registry.cpp  lifecycle.cpp
tests/    test_message_queue.cpp  test_shutdown.cpp  test_hysteresis_alarm.cpp
          test_config_v2.cpp  test_diagnostic_buffer.cpp
config/schema_v2.json   config.json   demo/   Makefile
```

Start-up order: `diagnosticApp` (registers `DiagnosticEventService`) → `sensorApp` → `controllerApp`.
Shutdown (SIGINT/SIGTERM): `sensorApp` → `controllerApp` → `diagnosticApp`, which writes `diagnostics/events.json` last.

---

## ⚙️ Configuration (schema v2)

`build/ecu [config.json]` reads `./config.json` by default. Every key is optional; missing keys take the default.
Invalid values are reported as `[Execution Manager] Config error: ...` and the ECU refuses to start (exit code 1).
JSON Schema: [`config/schema_v2.json`](config/schema_v2.json).

| Key | Type | Default | Validation / notes |
|---|---|---|---|
| `schemaVersion` | int | `1` | `1` or `2` |
| `sensor.startTemp` | number | `20.0` | °C at sample 0 |
| `sensor.tempStep` | number | `1.0` | °C per sample |
| `sensor.startPressure` | number | `1.0` | bar at sample 0 |
| `sensor.pressureStep` | number | `0.1` | bar per sample |
| `sensor.rampSteps` | int | `0` | `>= 0`; `0` = monotonic ramp, `N` = triangle wave (N samples up, N down) |
| `sensor.tempNoise` / `sensor.pressureNoise` | number | `0.0` | alternating ± offset per sample |
| `sensor.periodMs` | int | `1000` | `> 0` |
| `controller.thresholds.temperature` | number | `35.0` | °C, finite, `>= 0` |
| `controller.thresholds.pressure` | number | `3.0` | bar, finite, `>= 0` |
| `controller.hysteresis.temperature` | number | `2.0` | °C, finite, `>= 0`, `< thresholds.temperature` |
| `controller.hysteresis.pressure` | number | `0.3` | bar, finite, `>= 0`, `< thresholds.pressure` |
| `controller.periodMs` | int | `1000` | `> 0`; max wait for a sample before re-checking lifecycle state |
| `diagnostics.capacity` | int | `256` | `>= 1`; oldest event dropped (and counted) on overflow |
| `diagnostics.outputFile` | string | `diagnostics/events.json` | non-empty; parent directories are created |
| `controller.warningThreshold` | number | — | **deprecated** v1 alias of `controller.thresholds.temperature` |
| `controller.pressureWarningThreshold` | number | — | **deprecated** v1 alias of `controller.thresholds.pressure` |

Deprecated aliases log a `Config warning`; if both the alias and the v2 key are set, the v2 key wins.

`diagnostics/events.json`:

```json
{
  "capacity": 256,
  "dropped": 0,
  "events": [
    {"channel": "pressure", "kind": "raise", "state": "ALARM", "threshold": 3.0,
     "timestamp": "2026-10-01T14:17:29.621Z", "value": 3.08}
  ]
}
```

---

## 🛠 Build, Test & Run

```bash
make clean && make
make test
./build/ecu                                 # ./config.json
./build/ecu demo/config/calibrated.json     # hysteresis demo signal
./demo/run_demo.sh                          # baseline vs calibrated, plots in demo/out/
```
