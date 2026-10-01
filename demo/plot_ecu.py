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


def thresholds(cfg):
    """(temperature, pressure) thresholds from config v2, falling back to v1 aliases."""
    ctrl = cfg.get("controller", {})
    nested = ctrl.get("thresholds", {})
    return (nested.get("temperature", ctrl.get("warningThreshold", 35.0)),
            nested.get("pressure", ctrl.get("pressureWarningThreshold", 3.0)))


def hysteresis(cfg):
    h = cfg.get("controller", {}).get("hysteresis", {})
    return h.get("temperature", 2.0), h.get("pressure", 0.3)


def parse(log_path, cfg_path):
    cfg = json.loads(Path(cfg_path).read_text())
    period_s = cfg.get("sensor", {}).get("periodMs", 1000) / 1000.0
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
                temp_raise="High Temp" in m["flags"],
                temp_clear="CLEARED: Temp" in m["flags"],
                pres_raise="High Pressure" in m["flags"],
                pres_clear="CLEARED: Pressure" in m["flags"],
            )
        )
    return cfg, rows


def alarm_spans(rows, key):
    """[(t_raise, t_clear)] intervals during which the controller held the alarm."""
    spans, t0 = [], None
    for r in rows:
        if r[f"{key}_raise"]:
            t0 = r["t"]
        elif r[f"{key}_clear"] and t0 is not None:
            spans.append((t0, r["t"]))
            t0 = None
    if t0 is not None and rows:
        spans.append((t0, rows[-1]["t"]))
    return spans


def alarm_state(rows, key):
    """Per-sample controller alarm state (1 while raised, 0 after clear)."""
    state, out = 0, []
    for r in rows:
        if r[f"{key}_raise"]:
            state = 1
        elif r[f"{key}_clear"]:
            state = 0
        out.append(state)
    return out


def draw_channel(ax, rows, key, value, thr, hyst, color, label, unit):
    t = [r["t"] for r in rows]
    ax.plot(t, [r[value] for r in rows], "-o", ms=3, color=color, label=f"{label} (sensorApp)")
    ax.axhline(thr, ls="--", color="k", label=f"threshold = {thr}")
    ax.axhline(thr - hyst, ls=":", color="gray", label=f"clear < {thr - hyst:g}")
    raises = [r for r in rows if r[f"{key}_raise"]]
    clears = [r for r in rows if r[f"{key}_clear"]]
    if raises:
        ax.scatter([r["t"] for r in raises], [r[value] for r in raises], marker="x", s=80,
                   color="k", zorder=5, label=f"raise x{len(raises)}")
    if clears:
        ax.scatter([r["t"] for r in clears], [r[value] for r in clears], marker="o", s=60,
                   facecolors="none", edgecolors="k", zorder=5, label=f"clear x{len(clears)}")
    for t0, t1 in alarm_spans(rows, key):
        ax.axvspan(t0, t1, color=color, alpha=0.08)
    ax.set_ylabel(unit)
    ax.legend(loc="upper left", fontsize=8)
    ax.grid(alpha=0.3)
    return len(raises)


def draw(ax_t, ax_p, name, cfg, rows):
    t_thr, p_thr = thresholds(cfg)
    t_hyst, p_hyst = hysteresis(cfg)
    n_t = draw_channel(ax_t, rows, "temp", "temp", t_thr, t_hyst, "tab:red", "temperature", "Temp [°C]")
    ax_t.set_title(f"{name}: temperature {t_thr} (hyst {t_hyst}), pressure {p_thr} (hyst {p_hyst})",
                   fontsize=10)
    n_p = draw_channel(ax_p, rows, "pres", "pres", p_thr, p_hyst, "tab:blue", "pressure", "Pressure [bar]")
    ax_p.set_xlabel("time [s]")
    return n_t, n_p


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
