# Customer Requirement Package CRP-2.0

Customer change request: **CR-2026-042** — thermal/pressure controller of the Adaptive-AUTOSAR-style ECU simulator (`COG-GTM/autosar-ecu-sim`).

Source: transcription of the customer package as recorded on the parent workstream AE-4. This file is the frozen input of baseline B2.0; its SHA-256 is recorded in `B2.0.sha256` and in `CRP-2.0_analysis.md` §6. Do not edit — a new package version gets its own file (e.g. `CRP-2.1.md`) and goes through the change-point process (AE-17).

| ID | Requirement (customer wording) |
|----|--------------------------------|
| CR-01 | Over-pressure warning when coolant pressure exceeds a calibratable threshold (default 3.0 bar). |
| CR-02 | Alarm hysteresis: an alarm, once raised, must not clear until the signal falls below `threshold − hysteresis` (default 2.0 °C / 0.3 bar) to prevent chatter. |
| CR-03 | Every alarm raise/clear is recorded as a timestamped diagnostic event (DTC-style) retrievable after the run. |
| CR-04 | All thresholds/hysteresis values are calibratable via `config.json` without recompiling (parity with AUTOSAR ARXML calibration parameters). |
| CR-05 | Controller reaction time from threshold crossing to alarm emission ≤ 1 controller period (2000 ms default). |
