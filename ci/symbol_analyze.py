import argparse
import json
import os
import re
import subprocess
import sys


class LibNode:
    def __init__(self, name: str):
        self.name = name
        self.children: list[LibNode] = []
        self.flag = 0

    def add_child(self, node):
        self.children.append(node)

    def __repr__(self, level=0):
        indent = "  " * level
        repr_str = f"{indent}{self.name}\n"
        for child in self.children:
            repr_str += child.__repr__(level + 1)
        return repr_str

    def walk(self):
        for child in self.children:
            yield from child.walk()
        yield self


def prune_tree(node: LibNode, pred):
    new_children = []
    for child in node.children:
        if not prune_tree(child, pred):
            # prune
            continue
        else:
            new_children.append(child)
    node.children = new_children

    if not pred(node) and len(node.children) == 0:
        # prune
        return False
    else:
        return True


def run_cmd(cmd: list[str]):
    """Run a command and return its output as string."""
    result = subprocess.run(
        cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, check=False
    )
    if result.returncode != 0:
        raise RuntimeError(f"Command {' '.join(cmd)} failed: {result.stderr.strip()}")
    return result.stdout


def _check_lib_file(lib_name: str):
    if not os.path.isfile(lib_name):
        raise RuntimeError(f"library {lib_name} not found")


def _process_first_line(line: str):
    par_index = line.find("(")
    if par_index == -1:
        filename = line.strip()
    else:
        filename = line[:par_index].strip()
    _check_lib_file(filename)
    indent_count = len(line) - len(line.lstrip())
    return LibNode(filename), indent_count


def _process_line(line: str, all_libs_dict: dict[str, str]):
    index = line.find("=>")
    if index == -1:
        filename = line.strip()
    else:
        filename = line[:index].strip()
    filename = all_libs_dict[filename]
    _check_lib_file(filename)
    indent_count = len(line) - len(line.lstrip())
    return LibNode(filename), indent_count


def parse_lddtree_output(lines: list[str], all_libs_dict: dict[str, str]):
    """
    Parse output from lddtree, construct a tree according to indentation levels.
    """
    root = None
    stack: list[LibNode] = []  # holds (level, LibNode) tuples

    first_line = True
    cur_indent = 0
    cur_node: LibNode = None  # type: ignore
    for line in lines:
        if not line.strip():
            continue

        if first_line:
            first_line = False
            cur_node, cur_indent = _process_first_line(line)
            root = cur_node
        else:
            old_indent = cur_indent
            old_node = cur_node
            cur_node, cur_indent = _process_line(line, all_libs_dict)
            if cur_indent > old_indent:
                stack.append(old_node)
                old_node.add_child(cur_node)
            elif cur_indent < old_indent:
                if len(stack) <= 1:
                    raise RuntimeError("Unreachable")
                stack.pop(-1)
                stack[-1].add_child(cur_node)
            else:
                if len(stack) == 0:
                    raise RuntimeError("Unreachable")
                stack[-1].add_child(cur_node)

    return root


def get_dependency_tree(filepath):
    """
    Call lddtree, parse output, return the root node of the dependency tree.
    """
    try:
        lddtree_lines = run_cmd(["lddtree", filepath]).splitlines()
        lddtree_listed = run_cmd(["lddtree", filepath, "-l"]).splitlines()
    except RuntimeError as e:
        try:
            run_cmd(["lddtree", "--help"])
        except RuntimeError:
            raise RuntimeError(
                "Call lddtree failed, did you forget to install it?"
            ) from e
        raise

    lddtree_filedict = {os.path.basename(x): x for x in lddtree_listed if x.strip()}

    return parse_lddtree_output(lddtree_lines, lddtree_filedict)


def strip_symbol_version(sym: str) -> str:
    """
    Strip symbol version suffixes like @GLIBC_2.2.5 or @@GLIBC_2.2.5.
    e.g. strcpy@GLIBC_2.2.5 -> strcpy
         memcpy@@GLIBC_2.2.5 -> memcpy
    """
    # symbol@version   or symbol@@version
    return re.split(r"@{1,2}", sym)[0]


def nm_defined_symbols(lib_path: str) -> set[str]:
    """Return a set containing all exported symbols from the given library."""
    try:
        output = run_cmd(["nm", "-D", "--defined-only", lib_path])
    except RuntimeError:
        # Some libraries might have no dynamic symbol table, return empty set
        return set()

    syms = set()
    for line in output.splitlines():
        # Example line: 00000000000aaaa0 T memcpy@GLIBC_2.2.5
        parts = line.strip().split()
        if len(parts) >= 3:
            sym = strip_symbol_version(parts[2])
            syms.add(sym)
    return syms


def nm_undefined_symbols(lib_path: str, need_weak=False) -> set[str]:
    """Return a set containing all undefined (external dependency) symbols in the given library."""
    output = run_cmd(["nm", "-D", "--undefined-only", lib_path])
    syms = set()
    for line in output.splitlines():
        # Example line:                  U memcpy@@GLIBC_2.2.5
        parts = line.strip().split()
        if len(parts) > 0:
            if parts[0] == "w" and not need_weak:
                continue
            sym = strip_symbol_version(parts[-1])
            syms.add(sym)
    return syms


def get_deps_tree_and_set(target_lib: str):
    deps_tree = get_dependency_tree(target_lib)
    if deps_tree is None:
        raise RuntimeError(f"No dependencies found for {target_lib}")
    all_deps_set: set[LibNode] = set()
    for node in deps_tree.walk():
        all_deps_set.add(node)
    return deps_tree, all_deps_set


def get_sym_to_lib(all_deps_set: set[LibNode]):
    sym_to_lib: dict[str, LibNode] = dict()

    for lib in all_deps_set:
        defined_syms = nm_defined_symbols(lib.name)
        for s in defined_syms:
            # If duplicates appear, later libs override previous (reasonable?)
            sym_to_lib[s] = lib
    return sym_to_lib


def analyze_symbols(target_lib: str, include_weak_symbols: bool):
    # 1. Get dependencies via ldd
    deps_tree, all_deps_set = get_deps_tree_and_set(target_lib)

    # 2. Build symbol to library path mapping
    sym_to_lib = get_sym_to_lib(all_deps_set)

    # 3. Get undefined symbols from target library
    undefined_syms = nm_undefined_symbols(target_lib, include_weak_symbols)

    # 4. Match undefined symbols to dependency libs
    result: dict[str, LibNode] = dict()
    for sym in undefined_syms:
        if sym in sym_to_lib:
            result[sym] = sym_to_lib[sym]
        else:
            if not sym.startswith("Py") and not sym.startswith("_Py"):
                raise RuntimeError(
                    f"Symbol '{sym}' not found in any dependency libraries"
                )

    return deps_tree, result


def print_needed(deps_tree: LibNode, result: dict[str, LibNode]):
    for v in result.values():
        v.flag = 1
    prune_tree(deps_tree, lambda x: x.flag == 1)
    all_libs = set(x for x in deps_tree.walk())
    for lib in all_libs:
        if lib.name != deps_tree.name:
            print(lib.name)


def print_needless(deps_tree: LibNode, result: dict[str, LibNode]):
    for v in result.values():
        v.flag = 1
    all_libs_pre = set(x for x in deps_tree.walk())
    prune_tree(deps_tree, lambda x: x.flag == 1)
    all_libs_missing = all_libs_pre - set(x for x in deps_tree.walk())
    for lib in all_libs_missing:
        if lib.name != deps_tree.name:
            print(lib.name)


def print_pretty(mapping: dict[str, LibNode]):
    kmaxlen = 0
    for k in mapping.keys():
        kmaxlen = max(kmaxlen, len(k))
    kmaxlen += 1
    for k, v in mapping.items():
        klen = len(k)
        line = k + " " * (kmaxlen - klen)
        print(line + f"=> {v.name}")


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Analyze external symbols of a shared library and map them to their source libraries."
    )
    parser.add_argument(
        "target_lib", help="Path to the target shared library (.so) file"
    )
    parser.add_argument(
        "--include-weak-symbols", help="Consider weak symbols", action="store_true"
    )
    parser.add_argument(
        "--find-needed", help="Print needed libraries", action="store_true"
    )
    parser.add_argument(
        "--find-needless", help="Print needless libraries", action="store_true"
    )
    parser.add_argument("--json", help="Print in json style", action="store_true")
    args = parser.parse_args()

    target_lib: str = args.target_lib
    include_weak_symbols: bool = bool(args.include_weak_symbols)
    find_needed: bool = bool(args.find_needed)
    find_needless: bool = bool(args.find_needless)
    print_json: bool = bool(args.json)

    if not os.path.isfile(target_lib):
        print(f"File not found: {target_lib}", file=sys.stderr)
        sys.exit(1)

    cnt = sum([find_needed, find_needless, print_json])
    if cnt > 1:
        print("Cannot mix --find-needed, --find-needless and --json options")
        sys.exit(1)

    dep_tree, mapping = analyze_symbols(target_lib, include_weak_symbols)
    if find_needed:
        print_needed(dep_tree, mapping)
    elif find_needless:
        print_needless(dep_tree, mapping)
    elif print_json:
        print(
            json.dumps(
                {x: y.name for x, y in mapping.items()}, indent=2, ensure_ascii=False
            )
        )
    else:
        print_pretty(mapping)


if __name__ == "__main__":
    main()
