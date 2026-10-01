# CRP-2.0 Requirement Analysis — CR-2026-042

| | |
|---|---|
| Ticket | AE-5 (T-01, Requirement analysis) — parent workstream AE-4 |
| Input package | `docs/requirements/CRP-2.0.md` (CR-01 … CR-05) |
| Code baseline analysed | `main` @ `c8ca105` + PR #2 (`devin/1789087131-stage4-ecu-demo` @ `b88704e`) |
| Author / approver | Devin (draft) / System Engineer (approval) |
| Baseline | **B2.0** — see §6 |
| Date | 2026-10-01 |

## 1. Current runtime (as analysed)

| Element | Behaviour relevant to CRP-2.0 | Location |
|---|---|---|
| `ExecutionManager` | Reads `config.json` once at start-up, launches `sensorApp` and `controllerApp` threads, closes the queue after the sensor exits. Only flat keys (`sensor.*`, `controller.warningThreshold`, `controller.periodMs`; PR #2 adds `controller.pressureWarningThreshold`). No validation. | `src/execution_manager.cpp` |
| `sensorApp` | Publishes `SensorData{temperature, pressure}` every `sensor.periodMs` (default 1000 ms). Monotonic ramps: `T = startTemp + i·tempStep`, `P = 1.0 + 0.1·i`. Samples carry **no timestamp**. | `src/sensor_swc.cpp`, `include/sensor_types.hpp` |
| `MessageQueue<SensorData>` | Unbounded FIFO, `receiveFor` timeout, `close()` wakes receivers. | `include/message_queue.hpp` |
| `ServiceRegistry` | Singleton name → `void*` lookup; `SensorDataService` registered by the sensor. | `src/service_registry.cpp` |
| `controllerApp` | Waits up to `controller.periodMs` for one sample, then **sleeps a further `periodMs`**; evaluates only when the temperature changed vs. the previous sample; `temperature > warningThreshold` → `[WARNING: High Temp!]` (PR #2 adds `pressure > pressureWarningThreshold` → `[WARNING: High Pressure!]`). Stateless — the warning is printed on every sample above threshold, never "cleared". | `src/controller_swc.cpp` |
| Lifecycle | `INIT → RUNNING → SHUTDOWN` via SIGINT/SIGTERM atomics. | `src/lifecycle.cpp` |
| Calibration defaults | `main`: T_warn 25 °C, controller period 2000 ms. PR #2 demo: `calibrated.json` T_warn 35 °C / P_warn 3.0 bar / 1000 ms; `baseline.json` 60 °C / 10 bar / 1000 ms. | `config.json`, `demo/config/*.json` (PR #2) |

### Feasibility findings against the current runtime

| ID | Finding | Impact | Resolved by |
|---|---|---|---|
| F-1 | Controller sleeps `controller.periodMs` after every received sample, so it consumes at most one sample per controller period while the sensor publishes one per `sensor.periodMs`. With defaults (1000 / 2000 ms) the queue backlog grows by one sample every 2 s, so detection latency grows without bound. Observed on `main` (13 s run, defaults): sensor at sample T = 32 °C while the controller was still evaluating T = 26 °C, i.e. ~6 s lag. | CR-05 **not met** by `main`. | Controller must evaluate every queued sample as it arrives; `periodMs` becomes an upper bound on the wait, not a sleep (AE-9 / AE-10). |
| F-2 | Evaluation is skipped when `temperature` is unchanged, so a pressure crossing with a constant temperature is never evaluated. | CR-01 / CR-05 not met. | Evaluate every sample on every channel (AE-10). |
| F-3 | Warnings are stateless (`>` compare per sample) — no raise/clear notion. | CR-02 / CR-03 not met. | Per-channel hysteresis state machine (AE-20 / AE-10). |
| F-4 | `SensorData` has no publish timestamp. | CR-05 cannot be measured from sensor publish (A-12). | Add a publish timestamp to the sample, or stamp it in the integration harness (AE-15 decides; AE-7 must update "sensorApp interface unchanged"). |
| F-5 | Synthetic sensor ramps are monotonic increasing — a clear transition never happens in a run. | CR-02 clear path is unverifiable with the built-in generator. | Replayable sensor profiles (AE-16 / AE-12). |
| F-6 | No config validation; missing keys throw. | CR-04 robustness. | Schema v2 + validation (AE-19 / AE-10). |
| F-7 | No event storage, no shutdown dump. | CR-03 not met. | New `DiagnosticEventService` SWC (AE-21 / AE-19 / AE-10). |
| F-8 | Temperature default differs across artefacts: `main` 25 °C, PR #2 calibrated 35 °C, AE-6 draft SYS-REQ-101 35 °C. CRP-2.0 gives no temperature threshold default. | Needs a single baselined default (Q-10 / A-04). | — |

None of the findings makes a CR infeasible; all are addressable inside the existing SWC / queue / registry architecture.

## 2. Analysis sheet

Disposition values: **accepted**, **accepted-with-assumption**, **rejected/deferred**.

### CR-01 Over-pressure warning

| | |
|---|---|
| Interpretation | Controller raises a *High Pressure* warning when the coolant pressure signal crosses `P_warn` (calibratable, default 3.0 bar). Warning only — no shutdown or safe-state request. |
| Assumptions | A-01, A-02, A-03 |
| Open questions | Q-01, Q-02, Q-03 |
| Feasibility | Feasible. Pressure is already in `SensorData`; PR #2 has a first stateless pressure check. Blocked by F-2, F-3. |
| Affected components | `controllerApp`, `config.json` / `ExecutionManager` config load, demo configs |
| Disposition | **accepted-with-assumption** |

### CR-02 Alarm hysteresis

| | |
|---|---|
| Interpretation | Per channel, two-state machine: Normal → Alarm when the value reaches the threshold; Alarm → Normal only when the value is strictly below `threshold − hysteresis`. Values inside the dead band keep the current state. One raise and one clear per excursion — no chatter. Defaults: temperature 2.0 °C, pressure 0.3 bar. |
| Assumptions | A-03, A-05, A-06, A-07 |
| Open questions | Q-04, Q-05, Q-06 |
| Feasibility | Feasible; pure logic unit, no threading impact. Clear path needs non-monotonic inputs (F-5). |
| Affected components | `controllerApp` (new alarm unit), config |
| Disposition | **accepted-with-assumption** |

### CR-03 Diagnostic event recording

| | |
|---|---|
| Interpretation | Each raise and each clear produces exactly one diagnostic event `{timestamp, channel, value, threshold, kind}` that can be read after the ECU process has exited. |
| Assumptions | A-08, A-09, A-10, A-11 |
| Open questions | Q-07, Q-08, Q-09 |
| Feasibility | Feasible. Fits the existing pattern: new SWC registered in `ServiceRegistry`, fed by a second `MessageQueue<DiagnosticEvent>`, launched/stopped by `ExecutionManager`. |
| Affected components | new `diagnosticApp` / `DiagnosticEventService`, `controllerApp` (producer), `ExecutionManager` (start/stop order), config |
| Disposition | **accepted-with-assumption**; UDS/ISO 14229 DTC numbering **deferred** (A-10) |

### CR-04 Calibration via `config.json`

| | |
|---|---|
| Interpretation | `P_warn`, `T_warn` and both hysteresis values are read from `config.json` at ECU start; changing them requires a restart but no rebuild. Invalid values are rejected at start-up. |
| Assumptions | A-04, A-13, A-14, A-15 |
| Open questions | Q-10, Q-11, Q-12 |
| Feasibility | Feasible — config is already read at start-up (calibration-by-config demonstrated in PR #2). |
| Affected components | `ExecutionManager` config load (schema v2 + validation), `config.json`, `demo/config/*.json`, README |
| Disposition | **accepted-with-assumption**; runtime hot-reload and ARXML import/export **deferred** (A-14, A-15) |

### CR-05 Reaction time ≤ 1 controller period

| | |
|---|---|
| Interpretation | For every crossing, the time from the sensor publishing the first sample that satisfies the raise (or clear) condition to the controller emitting the corresponding warning/clear (log line and diagnostic event handed to the queue) is ≤ `controller.periodMs` (default 2000 ms). |
| Assumptions | A-12, A-16, A-17 |
| Open questions | Q-13, Q-14 |
| Feasibility | **Not met by `main`** (F-1, F-2). Feasible once the controller evaluates every sample on arrival — expected latency is then queue hand-off only (≪ 1 period). Requires a publish timestamp for measurement (F-4). |
| Affected components | `controllerApp` loop, `SensorData` (timestamp), config validation (period rule), integration harness |
| Disposition | **accepted-with-assumption** |

### Existing behaviour carried into B2.0

| ID | Behaviour | Disposition |
|---|---|---|
| EX-01 | High Temperature warning (`controller.warningThreshold`) — becomes the temperature channel of the CR-02 state machine. | **accepted** (re-baselined with default per A-04; traced as SYS-REQ-101 in AE-6) |

## 3. Clarification questions to the customer

| Q | CR | Question | Proposed assumption |
|---|---|---|---|
| Q-01 | CR-01 | Is 3.0 bar absolute or relative to ambient (gauge)? | A-01 |
| Q-02 | CR-01 | Is over-pressure a warning only, or a shutdown / safe-state trigger? | A-02 |
| Q-03 | CR-01/02 | "Exceeds" — is the raise condition `>` or `≥` threshold? | A-03 |
| Q-04 | CR-02 | Does hysteresis apply to both temperature and pressure, and can it be calibrated independently per channel? | A-05 |
| Q-05 | CR-02 | Is the clear condition strictly below `threshold − hysteresis`? | A-06 |
| Q-06 | CR-02 | If the first sample after start-up is already above threshold, must an alarm be raised immediately? | A-07 |
| Q-07 | CR-03 | Retention: in-memory ring buffer for the run, or persisted file? Required capacity? | A-08 |
| Q-08 | CR-03 | Required fields of an event (timestamp, channel, value, threshold, raise/clear)? Timestamp format / clock? | A-09 |
| Q-09 | CR-03 | "DTC-style": are UDS/ISO 14229 DTC numbers, status bytes or freeze frames required? | A-10 |
| Q-10 | CR-04 | What is the default temperature threshold (current code 25 °C, demo 35 °C)? | A-04 |
| Q-11 | CR-04 | Is a restart acceptable after recalibration, or is runtime hot-reload required? | A-14 |
| Q-12 | CR-04 | Does "parity with ARXML calibration parameters" require ARXML import/export, or only equivalent calibratability? | A-15 |
| Q-13 | CR-05 | Is the ≤ 1 period reaction time measured from sensor publish or from controller receive? | A-12 |
| Q-14 | CR-05 | Does the budget also apply to the clear transition? Which configurations must meet it (e.g. sensor period > controller period)? | A-16, A-17 |

## 4. Q&A log

Questions issued to the customer on 2026-10-01 (AE-5 comment). Agreed answer date: baseline freeze of B2.0. No customer answers were received by that date, so every question is closed by the recorded assumption in §5. A later customer answer that contradicts an assumption is a change point (new package CRP-2.x via AE-17), not an edit to B2.0.

| Q | Customer answer | Closed by |
|---|---|---|
| Q-01 … Q-14 | none received by 2026-10-01 | A-01 … A-17 (§5) |

## 5. Baselined assumptions

| A | CR | Assumption | Rationale |
|---|---|---|---|
| A-01 | CR-01 | `P_warn` applies to the pressure signal value exactly as reported in `SensorData.pressure` (bar); no ambient compensation. | The ECU has no ambient pressure input; the simulated signal starts at 1.0 bar, i.e. behaves as an absolute reading. |
| A-02 | CR-01 | Over-pressure is a **warning** (log line + diagnostic event); it does not stop the ECU or request a safe state. | CR-01 says "warning"; a safe-state request is new scope (expected CR-06 in CRP-2.1, AE-17). |
| A-03 | CR-01/02 | Raise condition is `value ≥ threshold` on both channels. | Conservative (raises at the boundary); matches AE-6 SYS-REQ-100, AE-8 SWR-202 and AE-11 TC-202-a. AE-20 currently says `>` and must be aligned. |
| A-04 | CR-04/EX-01 | Default `T_warn` = 35 °C (replaces 25 °C in `main`). | Matches the PR #2 calibrated set and AE-6 SYS-REQ-101; keeps the 2.0 °C hysteresis meaningful. |
| A-05 | CR-02 | Hysteresis applies to both channels and is calibrated **independently per channel** (defaults 2.0 °C / 0.3 bar). | CR-02 gives a distinct default per channel; CR-04 makes all hysteresis values calibratable. Consequence: the CR-02 change expected in CRP-2.1 (AE-17, "independently calibratable per channel") is already covered by B2.0 and would be no-impact. |
| A-06 | CR-02 | Clear condition is `value < threshold − hysteresis` (strict); inside the dead band the state is held. | Customer wording "falls below". |
| A-07 | CR-02 | Each channel starts in Normal; a first sample at/above threshold raises immediately. NaN/non-finite samples do not change state. | Fail-towards-warning at start; invalid samples carry no information. |
| A-08 | CR-03 | Events are held in a bounded in-memory ring buffer (default capacity 256, calibratable; overwrite-oldest with a dropped-event counter) and written to `diagnostics/events.json` at orderly shutdown (SIGINT/SIGTERM or normal exit). Events are lost on SIGKILL/crash. | "Retrievable after the run" requires persistence; dump-at-shutdown keeps the hot path free of file I/O. |
| A-09 | CR-03 | Event fields: `timestamp` (ISO-8601 UTC, ms resolution, system clock), `channel` (`temperature` \| `pressure`), `value`, `threshold`, `kind` (`raise` \| `clear`). | Field set from the ticket's question; sufficient to reconstruct every excursion. |
| A-10 | CR-03 | "DTC-style" means one structured, timestamped record per transition; no UDS/ISO 14229 DTC numbers, status bytes or freeze frames in B2.0. | No diagnostic protocol exists in the simulator; **deferred** to a future package if required. |
| A-11 | CR-03 | Exactly one event per raise and one per clear; repeated samples in the same state produce none. | Consistent with CR-02 "no chatter". |
| A-12 | CR-05 | Reaction time is measured **from sensor publish** of the first sample satisfying the condition **to controller emission** (log line written and event enqueued). | End-to-end view is what the customer observes; includes queueing delay, the only material latency in the runtime (F-1). Requires a publish timestamp (F-4). |
| A-13 | CR-04 | Configuration is validated at start-up; negative/non-finite values, `hysteresis ≥ threshold`, or non-positive periods are rejected and the ECU refuses to start with an error. v1 flat keys (`warningThreshold`, `pressureWarningThreshold`) remain accepted as deprecated aliases. | Safe failure on bad calibration; backward compatibility with existing configs. |
| A-14 | CR-04 | Calibration is read once at start-up; a restart is required to apply a change. Runtime hot-reload is **deferred**. | "Without recompiling" ≠ "without restarting"; matches current runtime. |
| A-15 | CR-04 | "Parity with ARXML calibration parameters" means equivalent calibratability through JSON; ARXML import/export is **deferred**. | Project uses JSON instead of ARXML by design (README). |
| A-16 | CR-05 | The budget applies to both raise and clear transitions. | Clear is an alarm emission of the same state machine. |
| A-17 | CR-05 | CR-05 is guaranteed only for `sensor.periodMs ≤ controller.periodMs`; other combinations are rejected or warned by config validation (rule defined in AE-15). | A sensor slower than the controller period cannot guarantee a sample-to-alarm path within one controller period. |

## 6. Baseline B2.0 record

| Item | Value |
|---|---|
| Baseline ID | B2.0 |
| Customer package | `docs/requirements/CRP-2.0.md` |
| Package SHA-256 | `49b629f60b01cc88a8a6d6c35a34baec1b0d0b88fa4e4a4326acc8723100b5c4` |
| Frozen on | 2026-10-01 |
| Code baseline | `main` @ `c8ca105` + PR #2 @ `b88704e` |
| Content | CR-01 … CR-05 + EX-01, dispositions §2, assumptions A-01 … A-17 §5 |
| Verification | `sha256sum -c docs/requirements/B2.0.sha256` |
| Approval | System Engineer approval on AE-5 / this PR |

From this point no requirement text or assumption in B2.0 changes without the change-point process (AE-17 → re-open AE-5 → CRP-2.0 → 2.x diff on AE-4).

## 7. Work slicing (V-cycle chain)

All tickets are children of AE-4 and carry `baseline-B2.0` (AE-17 carries `baseline-B2.1`). Aliases are the ones used inside the ticket texts.

| T | Ticket | Stage | Blocked by | V-pair (Relates) |
|---|---|---|---|---|
| T-01 | AE-5 | Requirement analysis (this) | — | AE-14 |
| T-02 | AE-6 | System requirements SYS-REQ-100…105 | AE-5 | AE-14 |
| T-03 | AE-7 | System architecture | AE-6 | AE-12 |
| └ A-01 | AE-18 | Context, interfaces, allocation | — | — |
| └ A-02 | AE-21 | DiagnosticEventService spec + ICD | AE-18 | AE-12 |
| └ A-03 | AE-15 | Timing budget (CR-05) | AE-18 | AE-14 |
| └ A-04 | AE-24 | ADRs + architecture review gate | AE-21, AE-15 | — |
| T-04 | AE-8 | Software requirements SWR-200…212 | AE-7 | AE-13 |
| T-05 | AE-9 | Software design | AE-8 | AE-12 |
| └ B-01 | AE-20 | HysteresisAlarm design | — | AE-11 |
| └ B-02 | AE-19 | DiagnosticEvent / ring buffer / config v2 design | — | AE-11, AE-8 |
| └ B-03 | AE-22 | EM ordering, sequences, design review gate | AE-20, AE-19 | — |
| T-06 | AE-10 | Implementation (PR #18) | AE-9 | AE-11 |
| T-07 | AE-11 | Unit test | AE-10 | AE-10 |
| T-08 | AE-12 | Integration test | AE-11, AE-10, AE-16 | AE-7, AE-9 |
| T-09 | AE-13 | Software verification | AE-12 | AE-8 |
| T-10 | AE-14 | System verification + acceptance | AE-13, AE-16 | AE-5, AE-6 |
| — | AE-16 | Test environment / virtual HIL | — | — |
| — | AE-23 | Verification bug template | — | AE-11…14 |
| C | AE-17 | CRP-2.1 change-request intake (parked) | — | AE-4, AE-5 |

Link audit (2026-10-01): every Blocks and Relates link above exists in Jira; no link is missing.

### Items for downstream tickets

| To | Item |
|---|---|
| AE-6 | SYS-REQ-100/101 use `≥` (A-03), SYS-REQ-101 default 35 °C (A-04), SYS-REQ-105 measured per A-12/A-16. |
| AE-7 / AE-15 | F-4: a publish timestamp in `SensorData` (or harness stamping) contradicts "sensorApp → unchanged interface"; decide and record in an ADR. Period rule A-17. |
| AE-20 | Align raise condition to `≥` (A-03); NaN handling per A-07. |
| AE-19 | Event fields/format per A-09; v1 aliases per A-13. |
| AE-10 / PR #18 | Implementation started before B2.0 was frozen; reconcile against A-01 … A-17 through the AE-22 gate. |
| AE-17 | A-05 makes the expected CRP-2.1 CR-02 change no-impact; re-check when the real package arrives. |
