#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd -- "$SCRIPT_DIR/.." && pwd)"

OUTPUT_SVG="${1:-$REPO_ROOT/build/test-flamegraph.svg}"
PERF_DATA="${2:-$REPO_ROOT/build/test-flamegraph.perf.data}"
# Sampling frequency in Hz (higher = more detail, more overhead)
PERF_FREQUENCY="${PERF_FREQUENCY:-9999}"
# Stack size in bytes for dwarf-based call-graph unwinding
PERF_STACK_SIZE="${PERF_STACK_SIZE:-16384}"
FLAMEGRAPH_TITLE="${FLAMEGRAPH_TITLE:-ssrJSON test.py flamegraph}"

check_required_tools() {
	local missing=0

	if ! command -v python3 >/dev/null 2>&1; then
		echo "error: python3 not found" >&2
		missing=1
	fi
	if ! command -v perf >/dev/null 2>&1; then
		echo "error: perf not found" >&2
		missing=1
	fi
	if ! command -v stackcollapse-perf.pl >/dev/null 2>&1; then
		echo "error: stackcollapse-perf.pl not found" >&2
		missing=1
	fi
	if ! command -v flamegraph.pl >/dev/null 2>&1; then
		echo "error: flamegraph.pl not found" >&2
		missing=1
	fi

	if [ "$missing" -ne 0 ]; then
		exit 1
	fi
}

main() {
	check_required_tools

	mkdir -p "$(dirname -- "$OUTPUT_SVG")" "$(dirname -- "$PERF_DATA")"
	rm -f "$OUTPUT_SVG" "$PERF_DATA"

	set -euo pipefail
	cd "$REPO_ROOT"
	echo "[1/2] Sampling: $REPO_ROOT/test.py"
	env PYTHONPATH=build perf record \
		-F "$PERF_FREQUENCY" \
		-g \
		--call-graph "dwarf,$PERF_STACK_SIZE" \
		-o "$PERF_DATA" \
		-- python3 test.py

	echo "[2/2] Generating flamegraph: $OUTPUT_SVG"
	perf script -i "$PERF_DATA" \
		| stackcollapse-perf.pl \
		| flamegraph.pl --title "$FLAMEGRAPH_TITLE" > "$OUTPUT_SVG"

	echo
	echo "Output: $OUTPUT_SVG"
	echo "If perf sampling fails, check perf_event_paranoid (e.g. kernel.perf_event_paranoid, /proc/sys/kernel/perf_event_paranoid)."
}

main "$@"
