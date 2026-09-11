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
  (cd "$wd" && timeout -s INT "$DURATION" "$ROOT/build/ecu" | tee "$OUT/ecu_$name.log") || true
}

banner "2/4  Baseline calibration"
run_ecu baseline "$DEMO/config/baseline.json"

banner "3/4  Re-calibrate thresholds via config.json ONLY (no rebuild)"
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
