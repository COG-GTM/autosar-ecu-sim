#!/usr/bin/env bash
# Stage 4 demo: build the AUTOSAR-style ECU, run it twice with different
# calibrations (no recompile), parse the live log and plot it.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DEMO="$ROOT/demo"
OUT="$DEMO/out"
DURATION="${DURATION:-30}"   # seconds per ECU run

mkdir -p "$OUT"

banner() { printf '\n\033[1;36m== %s ==\033[0m\n' "$*"; }

banner "1/4  Build ECU (make -> build/ecu)"
make -C "$ROOT" clean >/dev/null
make -C "$ROOT"
ls -l "$ROOT/build/ecu"

run_ecu() {  # run_ecu <name> <config>
  local name="$1" cfg="$2" wd="$OUT/run_$1"
  rm -rf "$wd"; mkdir -p "$wd"
  cp "$cfg" "$wd/config.json"
  banner "Run '$name' for ${DURATION}s  (config: $(basename "$cfg"))"
  cat "$wd/config.json"
  echo
  # ECU reads ./config.json; SIGINT triggers the ExecutionManager's graceful shutdown.
  local rc
  (cd "$wd" && timeout -s INT "$DURATION" "$ROOT/build/ecu" | tee "$OUT/ecu_$name.log"; exit "${PIPESTATUS[0]}") && rc=0 || rc=$?
  # 124 = timeout delivered SIGINT and the ECU shut down gracefully.
  if [[ $rc -ne 0 && $rc -ne 124 ]]; then
    echo "ECU run '$name' failed (exit $rc)" >&2
    exit "$rc"
  fi
  # DiagnosticEventService dumps its ring buffer at shutdown.
  cp "$wd/diagnostics/events.json" "$OUT/events_$name.json"
  echo "diagnostics: $(python3 -c 'import json,sys; d=json.load(open(sys.argv[1])); print(len(d["events"]), "events,", d["dropped"], "dropped")' "$OUT/events_$name.json") -> demo/out/events_$name.json"
}

banner "2/4  Baseline calibration"
run_ecu baseline "$DEMO/config/baseline.json"

banner "3/4  Re-calibrate thresholds/hysteresis via config.json ONLY (no rebuild)"
diff --color=always "$DEMO/config/baseline.json" "$DEMO/config/calibrated.json" || true
run_ecu calibrated "$DEMO/config/calibrated.json"

banner "4/4  Parse logs -> plots"
python3 "$DEMO/plot_ecu.py" \
  --run baseline   "$OUT/ecu_baseline.log"   "$DEMO/config/baseline.json" \
  --run calibrated "$OUT/ecu_calibrated.log" "$DEMO/config/calibrated.json" \
  --out "$OUT"

if [[ -n "${DD_API_KEY:-}" ]]; then
  banner "Optional: ship telemetry to Datadog"
  python3 "$DEMO/ship_datadog.py" \
    --run baseline   "$OUT/ecu_baseline.log"   "$DEMO/config/baseline.json" \
    --run calibrated "$OUT/ecu_calibrated.log" "$DEMO/config/calibrated.json" || echo "(Datadog upload skipped)"
fi

banner "Done — artifacts in demo/out/"
ls -1 "$OUT"
