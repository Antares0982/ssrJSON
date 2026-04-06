#!/usr/bin/env python3
"""
Analyze function call relationships in an ELF binary and generate a Graphviz DOT file.

Usage:
    python call_graph.py <binary> [-o output.dot] [--no-filter] [--tail-calls]

Dependencies:
    - objdump (from binutils or LLVM, available in the nix develop shell)

Limitations:
    - Indirect calls (via function pointers / vtables) cannot be resolved
      by static disassembly; they will be silently missing from the graph.
    - Fully inlined functions do not appear as separate nodes.
    - Tail calls (jmp/jmpq to a named function) are excluded by default;
      pass --tail-calls to include them.
"""

from __future__ import annotations

import argparse
import re
import shutil
import subprocess
import sys
from collections import defaultdict
from pathlib import Path

# -- boilerplate symbols that clutter the graph --------------------------
_BORING_FUNCTIONS: frozenset[str] = frozenset(
    {
        "_init",
        "_fini",
        "_start",
        "__do_global_dtors_aux",
        "register_tm_clones",
        "deregister_tm_clones",
        "frame_dummy",
        "__libc_csu_init",
        "__libc_csu_fini",
        "__cxa_finalize",
        "__gmon_start__",
    }
)


# helpers -------------------------------------------------------------
def _find_objdump() -> str:
    """Return the path to an available objdump binary, or exit."""
    for name in ("objdump", "llvm-objdump"):
        path = shutil.which(name)
        if path is not None:
            return path
    print(
        "Error: neither 'objdump' nor 'llvm-objdump' found in PATH.",
        file=sys.stderr,
    )
    sys.exit(1)


def _run_objdump(objdump: str, binary: str) -> str:
    """Disassemble *binary* with demangling and return stdout."""
    try:
        proc = subprocess.run(
            [objdump, "-d", "-C", binary],
            capture_output=True,
            text=True,
            check=True,
        )
    except subprocess.CalledProcessError as exc:
        print(f"Error running objdump:\n{exc.stderr}", file=sys.stderr)
        sys.exit(1)
    return proc.stdout


# -- core analysis -------------------------------------------------------
def parse_disassembly(
    disasm: str,
    *,
    include_tail_calls: bool = False,
) -> tuple[dict[str, set[str]], set[str], set[str]]:
    """Parse objdump output and return (call_graph, internal, external).

    * call_graph  - mapping  caller_name -> {callee_names}
    * internal    - set of function names *defined* inside the binary
    * external    - set of function names called only via PLT
    """
    # "0000000000001234 <func_name>:"
    re_func = re.compile(r"^[0-9a-f]+ <(.+)>:\s*$")
    # x86: call / callq,  ARM: bl / blx
    re_call = re.compile(r"\b(?:call|callq|bl|blx)\s+[0-9a-f]+\s+<(.+?)>\s*$")
    # tail-call via jmp (exclude intra-function offsets like <func+0x1a>)
    re_jmp = re.compile(r"\b(?:jmp|jmpq)\s+[0-9a-f]+\s+<([^+>]+)>\s*$")

    call_graph: dict[str, set[str]] = defaultdict(set)
    internal: set[str] = set()
    external: set[str] = set()

    in_plt = False
    cur_func: str | None = None

    for line in disasm.splitlines():
        stripped = line.strip()

        # -- section header --
        if stripped.startswith("Disassembly of section"):
            in_plt = ".plt" in stripped
            cur_func = None
            continue

        # -- function header --
        m = re_func.match(stripped)
        if m:
            name = m.group(1)
            if in_plt:
                cur_func = None  # skip PLT stubs
            else:
                cur_func = name
                internal.add(name)
            continue

        if cur_func is None:
            continue

        # -- call instruction --
        m = re_call.search(stripped)
        if m:
            target = m.group(1)
            if target.endswith("@plt"):
                clean = target[:-4]
                external.add(clean)
                call_graph[cur_func].add(clean)
            else:
                call_graph[cur_func].add(target)
            continue

        # -- optional: tail-call via jmp --
        if include_tail_calls:
            m = re_jmp.search(stripped)
            if m:
                target = m.group(1).strip()
                if target == cur_func:
                    continue  # recursive jump - skip
                if target.endswith("@plt"):
                    clean = target[:-4]
                    external.add(clean)
                    call_graph[cur_func].add(clean)
                else:
                    call_graph[cur_func].add(target)

    return dict(call_graph), internal, external


def filter_boring(
    call_graph: dict[str, set[str]],
    internal: set[str],
    external: set[str],
) -> tuple[dict[str, set[str]], set[str], set[str]]:
    """Remove standard boilerplate functions from all sets."""
    internal -= _BORING_FUNCTIONS
    external -= _BORING_FUNCTIONS
    for name in _BORING_FUNCTIONS:
        call_graph.pop(name, None)
    for caller in list(call_graph):
        call_graph[caller] -= _BORING_FUNCTIONS
        if not call_graph[caller]:
            del call_graph[caller]
    return call_graph, internal, external


# -- DOT generation ------------------------------------------------------
def _esc(name: str) -> str:
    """Escape a label for use inside DOT double-quotes."""
    return name.replace("\\", "\\\\").replace('"', '\\"')


def generate_dot(
    call_graph: dict[str, set[str]],
    internal: set[str],
    external: set[str],
    title: str = "Call Graph",
) -> str:
    """Return a Graphviz DOT string."""
    # Nodes that participate in at least one edge
    all_callees: set[str] = set()
    for targets in call_graph.values():
        all_callees |= targets
    active_nodes = set(call_graph.keys()) | all_callees

    # -- compute in-degree / out-degree --
    in_deg: dict[str, int] = defaultdict(int)
    out_deg: dict[str, int] = defaultdict(int)
    for caller, callees in call_graph.items():
        out_deg[caller] += len(callees)
        for callee in callees:
            in_deg[callee] += 1

    lines: list[str] = [
        f'digraph "{_esc(title)}" {{',
        "    rankdir=LR;",
        "    node [shape=box, style=filled, fillcolor=white, "
        'fontname="monospace", fontsize=10];',
        '    edge [color="#404040"];',
        "",
    ]

    # -- internal (black) --
    for node in sorted(active_nodes & internal):
        e = _esc(node)
        i, o = in_deg.get(node, 0), out_deg.get(node, 0)
        label = f"{e}\\nin:{i} out:{o}"
        lines.append(f'    "{e}" [label="{label}", color=black, fontcolor=black];')

    lines.append("")

    # -- external (red) --
    ext_nodes = active_nodes - internal
    for node in sorted(ext_nodes):
        e = _esc(node)
        i, o = in_deg.get(node, 0), out_deg.get(node, 0)
        label = f"{e}\\nin:{i} out:{o}"
        lines.append(
            f'    "{e}" [label="{label}", color=red, fontcolor=red, '
            f'style="filled,dashed", fillcolor="#fff0f0"];'
        )

    lines.append("")

    # -- edges --
    for caller in sorted(call_graph):
        for callee in sorted(call_graph[caller]):
            lines.append(f'    "{_esc(caller)}" -> "{_esc(callee)}";')

    lines.append("}")
    return "\n".join(lines)


# -- entry point ---------------------------------------------------------
def main() -> None:
    ap = argparse.ArgumentParser(
        description="Analyze ELF binary function call graph -> Graphviz DOT.",
    )
    ap.add_argument("binary", help="Path to the ELF binary (.so / executable)")
    ap.add_argument(
        "-o",
        "--output",
        help="Output DOT file (default: <stem>_call_graph.dot next to binary)",
    )
    ap.add_argument(
        "--no-filter",
        action="store_true",
        help="Keep standard boilerplate functions (_init, _fini, ...)",
    )
    ap.add_argument(
        "--tail-calls",
        action="store_true",
        help="Also track tail-call jumps (jmp/jmpq to named functions)",
    )
    args = ap.parse_args()

    binary = Path(args.binary)
    if not binary.exists():
        print(f"Error: '{binary}' not found.", file=sys.stderr)
        sys.exit(1)

    output = (
        Path(args.output)
        if args.output
        else binary.with_name(f"{binary.stem}_call_graph.dot")
    )

    objdump = _find_objdump()
    print(f"objdump : {objdump}")
    print(f"binary  : {binary}")
    print(f"output  : {output}")
    print()

    print("Disassembling ...")
    disasm = _run_objdump(objdump, str(binary))

    print("Parsing call graph ...")
    call_graph, internal, external = parse_disassembly(
        disasm, include_tail_calls=args.tail_calls
    )

    if not args.no_filter:
        call_graph, internal, external = filter_boring(call_graph, internal, external)

    dot = generate_dot(call_graph, internal, external, title=binary.name)
    output.write_text(dot, encoding="utf-8")

    # -- summary --
    all_callees: set[str] = set()
    for targets in call_graph.values():
        all_callees |= targets
    active = set(call_graph.keys()) | all_callees
    n_int = len(active & internal)
    n_ext = len(active - internal)
    n_edges = sum(len(v) for v in call_graph.values())

    print()
    print(f"✓  Written to {output}")
    print(f"   Internal nodes : {n_int}")
    print(f"   External nodes : {n_ext}  (red in graph)")
    print(f"   Edges (calls)  : {n_edges}")
    print()
    print("Render examples:")
    print(f"  dot -Tsvg '{output}' -o '{output.with_suffix('.svg')}'")
    print(f"  dot -Tpng '{output}' -o '{output.with_suffix('.png')}'")
    print()
    print(
        "Note: indirect calls (function pointers) and fully inlined "
        "functions are not captured by static disassembly."
    )


if __name__ == "__main__":
    main()
