#!/usr/bin/env bash
set -euo pipefail

if [ "$#" -lt 1 ]; then
  echo "[test] uso: linux_gui_smoke.sh <ruta-aymc>" >&2
  exit 1
fi

compiler="$1"
repo_root="$(cd "$(dirname "$0")/.." && pwd)"
tmp_root="${repo_root}/build/tmp/linux_gui_smoke"
sample_path="${tmp_root}/gui_smoke.aym"
binary_path="${tmp_root}/gui_smoke"

mkdir -p "$tmp_root"

if [ ! -x "$compiler" ]; then
  echo "[test] compilador no ejecutable: $compiler" >&2
  exit 1
fi

if ! command -v xvfb-run >/dev/null 2>&1 && [ -z "${DISPLAY:-}" ]; then
  echo "[test] linux gui smoke skipped: no DISPLAY/xvfb-run"
  exit 0
fi

cat > "$sample_path" <<'EOF'
qallta
ukaxa(!uja_qallta(220, 160, "Aym GUI Smoke")) {
  qillqa("OPEN_FAIL");
} maysatxa {
  uja_pichha(12, 18, 26);
  uja_suyu(18, 20, 72, 44, 255, 92, 92);
  uja_qillqa("smoke", 22, 96, 245, 245, 245);
  uja_ustaya();
  sleep(60);
  qillqa(uja_utji() ? "OPEN_OK" : "OPEN_CLOSED");
  uja_tukuya();
  qillqa("CLOSED");
}
tukuya
EOF

"$compiler" "$sample_path" -o "$binary_path" >/dev/null

if command -v xvfb-run >/dev/null 2>&1; then
  output="$(xvfb-run -a "$binary_path")"
else
  output="$("$binary_path")"
fi

grep -q "OPEN_OK" <<<"$output"
grep -q "CLOSED" <<<"$output"

echo "[test] linux gui smoke test passed"
