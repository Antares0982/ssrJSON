#!/usr/bin/env bash
set -euo pipefail

usage() {
    echo "Usage: $0 <wheel_file> [python_executable]"
    echo "  wheel_file        Path to the .whl file to verify"
    echo "  python_executable Python to use (default: python3.14)"
    exit 1
}

[[ $# -lt 1 ]] && usage

WHEEL_FILE="$1"
PYTHON="${2:-python3.14}"

if ! command -v "$PYTHON" &>/dev/null; then
    PYTHON="/Library/Frameworks/Python.framework/Versions/3.14/bin/python3"
fi

if [[ ! -f "$WHEEL_FILE" ]]; then
    echo "Error: wheel file not found: $WHEEL_FILE"
    exit 1
fi

# Normalize wheel filename: version must use dots, not underscores
WHEEL_BASENAME=$(basename "$WHEEL_FILE")
# Replace only the version part (second field in wheel filename: name-version-...)
NORMALIZED=$(echo "$WHEEL_BASENAME" | awk -F'-' '{
    # fields: name, version, python, abi, platform
    # normalize version field (field 2) by replacing _ with .
    split($2, v, "_"); ver=""; for(i=1;i<=length(v);i++) ver=(i==1?v[i]:ver"."v[i]);
    print $1"-"ver"-"$3"-"$4"-"$5
}')

TMPDIR=$(mktemp -d)
trap 'rm -rf "$TMPDIR"' EXIT

echo "==> Temp dir: $TMPDIR"
echo "==> Python:   $($PYTHON --version)"
echo "==> Wheel:    $WHEEL_FILE"

# Copy with normalized name if needed
if [[ "$WHEEL_BASENAME" != "$NORMALIZED" ]]; then
    echo "==> Normalizing wheel filename: $WHEEL_BASENAME -> $NORMALIZED"
    cp "$WHEEL_FILE" "$TMPDIR/$NORMALIZED"
    INSTALL_WHEEL="$TMPDIR/$NORMALIZED"
else
    INSTALL_WHEEL="$WHEEL_FILE"
fi

echo ""
echo "--- [1/3] Creating venv and installing wheel ---"
"$PYTHON" -m venv "$TMPDIR/venv"
"$TMPDIR/venv/bin/pip" install --quiet "$INSTALL_WHEEL"

echo ""
echo "--- [2/3] Functional test ---"
"$TMPDIR/venv/bin/python" - <<'EOF'
import ssrjson, json

data = {"hello": "world", "num": 42, "arr": [1, 2, 3], "nested": {"a": True, "b": None}, "unicode": b'\xe4\xbd\xa0\xe5\xa5\xbd'.decode('utf-8')}
encoded = ssrjson.dumps(data)
decoded = ssrjson.loads(encoded)
assert decoded == data, f"Round-trip mismatch: {decoded}"

stdlib_encoded = json.dumps(data, ensure_ascii=False)
assert ssrjson.loads(stdlib_encoded) == data, "Failed to parse stdlib json output"
assert json.loads(encoded) == data, "ssrjson output not valid json"

print(f"  version : {ssrjson.__version__}")
print(f"  output  : {encoded}")
print("  PASS")
EOF

echo ""
echo "--- [3/3] Binary inspection (macOS) ---"
SO_FILE=$(find "$TMPDIR/venv" -name "*.so" | head -1)
if [[ -z "$SO_FILE" ]]; then
    echo "  WARNING: no .so file found in venv"
else
    echo "  file: $(basename "$SO_FILE")"
    if command -v otool &>/dev/null; then
        echo ""
        echo "  otool -L:"
        otool -L "$SO_FILE" | sed 's/^/    /'
        echo ""
        MINOS=$(otool -l "$SO_FILE" | awk '/LC_BUILD_VERSION/{found=1} found && /minos/{print $2; exit}')
        SDK=$(otool -l "$SO_FILE" | awk '/LC_BUILD_VERSION/{found=1} found && /sdk/{print $2; exit}')
        echo "  minos (min macOS): ${MINOS:-unknown}"
        echo "  sdk:               ${SDK:-unknown}"
    fi
fi

echo ""
echo "==> All checks passed."
