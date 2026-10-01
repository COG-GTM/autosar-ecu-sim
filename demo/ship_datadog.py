#!/usr/bin/env python3
"""Optionally ship parsed ECU telemetry to Datadog as custom metrics (needs DD_API_KEY)."""
import argparse
import json
import os
import sys
import time
import urllib.request

from plot_ecu import alarm_state, parse, thresholds


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--run", nargs=3, action="append", metavar=("NAME", "LOG", "CONFIG"),
                    required=True)
    args = ap.parse_args()

    api_key = os.environ.get("DD_API_KEY")
    if not api_key:
        print("DD_API_KEY not set — skipping Datadog upload")
        return
    site = os.environ.get("DD_SITE", "datadoghq.com")
    now = int(time.time())

    series = []
    for name, log, cfg_path in args.run:
        cfg, rows = parse(log, cfg_path)
        n = len(rows)
        tags = [f"run:{name}", "stage:4-ecu-integration", "app:autosar-ecu-sim"]

        def add(metric, values, mtype=0):
            series.append({
                "metric": metric,
                "type": mtype,
                "tags": tags,
                "points": [{"timestamp": now - int(rows[-1]["t"] - r["t"]), "value": v}
                           for r, v in zip(rows, values)],
            })

        if not n:
            continue
        add("aptiv.ecu.temperature", [r["temp"] for r in rows])
        add("aptiv.ecu.pressure", [r["pres"] for r in rows])
        t_thr, p_thr = thresholds(cfg)
        add("aptiv.ecu.alarm.high_temp", alarm_state(rows, "temp"))
        add("aptiv.ecu.alarm.high_pressure", alarm_state(rows, "pres"))
        add("aptiv.ecu.threshold.temperature", [t_thr] * n)
        add("aptiv.ecu.threshold.pressure", [p_thr] * n)

    req = urllib.request.Request(
        f"https://api.{site}/api/v2/series",
        data=json.dumps({"series": series}).encode(),
        headers={"Content-Type": "application/json", "DD-API-KEY": api_key},
    )
    try:
        with urllib.request.urlopen(req, timeout=15) as resp:
            print(f"Datadog: HTTP {resp.status}, shipped {len(series)} series "
                  f"({sum(len(s['points']) for s in series)} points) to {site}")
    except Exception as e:  # network / auth problems must not fail the demo
        print(f"Datadog upload failed: {e}", file=sys.stderr)


if __name__ == "__main__":
    main()
