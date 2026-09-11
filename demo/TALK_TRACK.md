# Stage 4 — ECU Software Integration (AUTOSAR-style SWCs)

## Talk track (~140 words)

In the earlier stages we designed the controller in Simulink, generated C with Embedded Coder and proved it on the STM32 HIL bench. Now that algorithm has to live inside a real ECU software stack.

This is an Adaptive-AUTOSAR-style runtime in C++17. An Execution Manager boots two software components as POSIX threads: a sensor SWC publishing temperature and pressure, and a controller SWC that discovers that feed through the Service Registry and consumes it over a thread-safe MessageQueue.

Watch the live log: `make`, run, telemetry streams, no alarms. Now — same binary, no recompile — we edit `config.json`: temperature limit 60→35, pressure 10→3. Run again and the controller trips High-Temp and High-Pressure warnings exactly where the plot shows the crossing.

Behaviour is calibrated by configuration, not code — the same pattern AUTOSAR uses with ARXML parameters. Telemetry also lands in Datadog for fleet monitoring.

## What you're seeing

- `make` → `build/ecu`: Execution Manager launches `sensorApp` and `controllerApp` threads (real C++ executing on host).
- Live stdout: `[Sensor]` publishes via `MessageQueue<SensorData>`; `[Controller]` discovers `SensorDataService` from the `ServiceRegistry`.
- Run 1 (`baseline.json`): thresholds 60 °C / 10 bar → 0 alarms.
- Run 2 (`calibrated.json`): thresholds 35 °C / 3 bar → `[WARNING: High Temp!]` ×14, `[WARNING: High Pressure!]` ×9 — **no source change, no rebuild**.
- `demo/out/ecu_telemetry.png` overlays both runs against their thresholds with alarm markers; metrics also shipped to Datadog (`aptiv.ecu.*`).

## Hand-off

Next: Stage 5 drops this integrated ECU into a virtual HIL rig (BMS plant + dashboard) for system-level validation.
