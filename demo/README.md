# Stage 4 demo — AUTOSAR-style ECU integration

Single entry point, < 2 min:

```bash
./demo/run_demo.sh            # 30 s per run (override with DURATION=10)
```

It will:

1. `make` → `build/ecu` (C++17, POSIX threads, nlohmann/json).
2. Run the ECU for 30 s with `demo/config/baseline.json` (thresholds 60 °C / 10 bar) — live log, no alarms.
3. Run the **same binary** for 30 s with `demo/config/calibrated.json` (35 °C / 3 bar) — controller trips `[WARNING: High Temp!]` / `[WARNING: High Pressure!]`.
4. `demo/plot_ecu.py` parses the stdout logs and writes `demo/out/ecu_telemetry.png`, `alarm_counts.png`, `summary.md`.
5. If `DD_API_KEY` (and optionally `DD_SITE`) is set, `demo/ship_datadog.py` posts the series as custom metrics `aptiv.ecu.*` (tags `run:baseline|calibrated`). Skipped silently otherwise.

## What is real vs. simulated

| Element | Status |
|---|---|
| Execution Manager, Service Registry, MessageQueue, sensor/controller SWCs | **Real C++ code, compiled and executed on the host** (Linux, g++) |
| Threshold calibration | Real: read from `config.json` at start-up, no rebuild |
| Sensor values | Host-simulated ramp (`startTemp + i*tempStep`, pressure `1 + 0.1 i`) standing in for the plant/HIL feed from Stages 1–3 |
| Datadog metrics | Real HTTP POST to `api.<DD_SITE>/api/v2/series` when the key is present |
| Target hardware | None — this stage is the platform-independent SWC layer; the same SWCs would be deployed on the ECU target in Stage 5/6 |

## Architecture

```mermaid
flowchart LR
    CFG[config.json<br/>calibration: thresholds, periods] --> EM

    subgraph ECU["build/ecu — Adaptive-AUTOSAR-style runtime (C++17)"]
        EM[Execution Manager<br/>runExecutionManager]
        EM -->|std::thread| S[sensorApp SWC]
        EM -->|std::thread| C[controllerApp SWC]
        EM -->|SIGINT → SHUTDOWN| LC[Lifecycle<br/>INIT / RUNNING / SHUTDOWN]

        S -->|registerService<br/>"SensorDataService"| SR[(Service Registry<br/>singleton)]
        C -->|discoverService| SR
        S -->|send SensorData| MQ[[MessageQueue&lt;SensorData&gt;]]
        MQ -->|receive| C
    end

    C -->|stdout / controller_log.txt<br/>WARNING flags| LOG[ECU log]
    LOG --> PY[demo/plot_ecu.py] --> PNG[ecu_telemetry.png]
    LOG --> DD[demo/ship_datadog.py] --> DDG[(Datadog<br/>aptiv.ecu.*)]
```

## Artifacts (`demo/out/`)

![telemetry](out/ecu_telemetry.png)
![alarms](out/alarm_counts.png)

## Source changes made for the demo

- `controllerApp` gained an optional `pressureWarningThreshold` (defaults to ∞ if absent from `config.json`).
- SWC log lines are composed in a `std::ostringstream` and written with a single `<<` so the two threads no longer interleave characters on stdout (needed for parsing).
- `Makefile`: `mkdir -p build`; `clean` removes `build/ecu` (was `build/ECU`).
