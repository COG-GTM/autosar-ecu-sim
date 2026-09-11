#!/usr/bin/env python3
"""Parse ECU stdout logs and plot temperature / pressure vs. calibrated thresholds."""
import argparse
import json
import re
from pathlib import Path

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt  # noqa: E402

LINE_RE = re.compile(
    r"\[Controller\] Temp = (?P<temp>[-\d.]+), Pressure = (?P<pres>[-\d.]+)(?P<flags>.*)"
)


def parse(log_path, cfg_path):
    cfg = json.loads(Path(cfg_path).read_text())
    period_s = cfg["controller"]["periodMs"] / 1000.0
    rows = []
    for line in Path(log_path).read_text().splitlines():
        m = LINE_RE.search(line)
        if not m:
            continue
        rows.append(
            dict(
                t=len(rows) * period_s,
                temp=float(m["temp"]),
                pres=float(m["pres"]),
                temp_alarm="High Temp" in m["flags"],
                pres_alarm="High Pressure" in m["flags"],
            )
        )
    return cfg, rows


def draw(ax_t, ax_p, name, cfg, rows):
    t = [r["t"] for r in rows]
    temp = [r["temp"] for r in rows]
    pres = [r["pres"] for r in rows]
    t_thr = cfg["controller"]["warningThreshold"]
    p_thr = cfg["controller"].get("pressureWarningThreshold")

    ax_t.plot(t, temp, "-o", ms=3, color="tab:red", label="temperature (sensorApp)")
    ax_t.axhline(t_thr, ls="--", color="k", label=f"warningThreshold = {t_thr}")
    ta = [r for r in rows if r["temp_alarm"]]
    if ta:
        ax_t.scatter([r["t"] for r in ta], [r["temp"] for r in ta], marker="x", s=80,
                     color="k", zorder=5, label=f"[WARNING: High Temp!] x{len(ta)}")
        ax_t.axvspan(ta[0]["t"], t[-1], color="red", alpha=0.08)
    ax_t.set_ylabel("Temp [°C]")
    ax_t.set_title(f"{name}: warningThreshold={t_thr}, pressureWarningThreshold={p_thr}",
                   fontsize=10)
    ax_t.legend(loc="upper left", fontsize=8)
    ax_t.grid(alpha=0.3)

    ax_p.plot(t, pres, "-o", ms=3, color="tab:blue", label="pressure (sensorApp)")
    if p_thr is not None:
        ax_p.axhline(p_thr, ls="--", color="k", label=f"pressureWarningThreshold = {p_thr}")
    pa = [r for r in rows if r["pres_alarm"]]
    if pa:
        ax_p.scatter([r["t"] for r in pa], [r["pres"] for r in pa], marker="x", s=80,
                     color="k", zorder=5, label=f"[WARNING: High Pressure!] x{len(pa)}")
        ax_p.axvspan(pa[0]["t"], t[-1], color="blue", alpha=0.08)
    ax_p.set_ylabel("Pressure [bar]")
    ax_p.set_xlabel("time [s]")
    ax_p.legend(loc="upper left", fontsize=8)
    ax_p.grid(alpha=0.3)
    return len(ta), len(pa)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--run", nargs=3, action="append", metavar=("NAME", "LOG", "CONFIG"),
                    required=True)
    ap.add_argument("--out", required=True)
    args = ap.parse_args()
    out = Path(args.out)
    out.mkdir(parents=True, exist_ok=True)

    runs = [(name, *parse(log, cfg)) for name, log, cfg in args.run]

    fig, axes = plt.subplots(2, len(runs), figsize=(6.5 * len(runs), 7), sharex="col")
    if len(runs) == 1:
        axes = [[axes[0]], [axes[1]]]
    summary = []
    for col, (name, cfg, rows) in enumerate(runs):
        n_t, n_p = draw(axes[0][col], axes[1][col], name, cfg, rows)
        summary.append((name, len(rows), n_t, n_p))
        print(f"[{name}] samples={len(rows)} tempAlarms={n_t} pressureAlarms={n_p}")
    fig.suptitle("AUTOSAR-style ECU — same binary, calibration changed via config.json only",
                 fontsize=13, fontweight="bold")
    fig.tight_layout()
    fig.savefig(out / "ecu_telemetry.png", dpi=130)

    # Alarm-count bar chart
    fig2, ax = plt.subplots(figsize=(6, 3.5))
    names = [s[0] for s in summary]
    x = range(len(names))
    ax.bar([i - 0.2 for i in x], [s[2] for s in summary], 0.4, color="tab:red", label="High Temp alarms")
    ax.bar([i + 0.2 for i in x], [s[3] for s in summary], 0.4, color="tab:blue", label="High Pressure alarms")
    ax.set_xticks(list(x))
    ax.set_xticklabels(names)
    ax.set_ylabel("alarm count")
    ax.set_title("Alarms raised by controllerApp per calibration")
    ax.legend()
    ax.grid(axis="y", alpha=0.3)
    fig2.tight_layout()
    fig2.savefig(out / "alarm_counts.png", dpi=130)

    (out / "summary.md").write_text(
        "| run | samples | High Temp alarms | High Pressure alarms |\n|---|---|---|---|\n"
        + "".join(f"| {n} | {s} | {t} | {p} |\n" for n, s, t, p in summary)
    )
    print(f"wrote {out/'ecu_telemetry.png'}, {out/'alarm_counts.png'}, {out/'summary.md'}")


if __name__ == "__main__":
    main()
