#!/usr/bin/env python3
"""
Emit assembly files from compile_commands.json.

Reads build/compile_commands.json and generates assembly (.s) files
for all compilation units, preserving all original compilation flags.
"""

import json
import os
import re
import shlex
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor, as_completed
from pathlib import Path


SPILL_RELOAD_PATTERN = re.compile(r"#\s+\d+-byte\s+.*(?:Spill|Reload)")


def find_project_root() -> Path:
    """Find the project root directory (where compile_commands.json is located)."""
    script_dir = Path(__file__).resolve().parent
    # dev_tools is directly under project root
    return script_dir.parent


def load_compile_commands(project_root: Path) -> list[dict]:
    """Load and parse compile_commands.json."""
    compile_commands_path = project_root / "build" / "compile_commands.json"
    if not compile_commands_path.exists():
        print(f"Error: {compile_commands_path} not found.", file=sys.stderr)
        print("Please run CMake configuration first.", file=sys.stderr)
        sys.exit(1)

    with open(compile_commands_path, "r", encoding="utf-8") as f:
        return json.load(f)


def convert_to_asm_command(entry: dict) -> tuple[list[str], Path, str]:
    """
    Convert a compile command to an assembly output command.

    Returns:
        tuple of (command_args, output_path, source_file)
    """
    command_str = entry["command"]
    directory = Path(entry["directory"])
    source_file = entry["file"]

    # Determine output path from 'output' field or parse from command
    if "output" in entry:
        obj_output = entry["output"]
        if not os.path.isabs(obj_output):
            obj_output = directory / obj_output
        else:
            obj_output = Path(obj_output)
    else:
        # Try to parse -o from command
        args = shlex.split(command_str)
        obj_output = None
        for i, arg in enumerate(args):
            if arg == "-o" and i + 1 < len(args):
                obj_output = Path(args[i + 1])
                if not obj_output.is_absolute():
                    obj_output = directory / obj_output
                break
        if obj_output is None:
            # Fallback: use source filename with .s extension in directory
            obj_output = directory / (Path(source_file).stem + ".o")

    # Convert .o path to .s path
    asm_output = obj_output.with_suffix(".s")

    # Parse and modify the command
    args = shlex.split(command_str)
    new_args = []
    skip_next = False

    for i, arg in enumerate(args):
        if skip_next:
            skip_next = False
            continue

        if arg == "-o":
            # Replace output path
            new_args.append("-o")
            new_args.append(str(asm_output))
            skip_next = True
        elif arg == "-c":
            # Remove -c flag (not needed with -S)
            continue
        elif arg in ("-flto", "-flto=thin", "-flto=full") or arg.startswith("-flto="):
            # Remove LTO flags (incompatible with assembly output)
            continue
        else:
            new_args.append(arg)

    # Add -S flag after compiler if not present
    if "-S" not in new_args:
        # Insert -S after the compiler (first argument)
        new_args.insert(1, "-S")

    return new_args, asm_output, source_file


def run_asm_command(
    args: list[str], output_path: Path, source_file: str, directory: Path
) -> tuple[bool, str, Path, int, str]:
    """
    Run the assembly generation command.

    Returns:
        tuple of (success, source_file, output_path, spill_reload_count, error_message)
    """
    try:
        # Ensure output directory exists
        output_path.parent.mkdir(parents=True, exist_ok=True)

        result = subprocess.run(
            args,
            cwd=directory,
            capture_output=True,
            text=True,
            timeout=300,  # 5 minute timeout
        )

        if result.returncode != 0:
            return False, source_file, output_path, 0, result.stderr

        spill_reload_count = count_spill_reload_lines(output_path)

        return True, source_file, output_path, spill_reload_count, ""

    except subprocess.TimeoutExpired:
        return False, source_file, output_path, 0, "Timeout expired"
    except Exception as e:
        return False, source_file, output_path, 0, str(e)


def count_spill_reload_lines(asm_path: Path) -> int:
    """Count spill/reload comment lines in a generated assembly file."""
    count = 0
    with open(asm_path, "r", encoding="utf-8", errors="ignore") as asm_file:
        for line in asm_file:
            if SPILL_RELOAD_PATTERN.search(line):
                count += 1
    return count


def main():
    """Main entry point."""
    project_root = find_project_root()
    print(f"Project root: {project_root}")

    # Load compile commands
    compile_commands = load_compile_commands(project_root)
    total = len(compile_commands)
    print(f"Found {total} compilation units")

    # Prepare all commands
    tasks = []
    for entry in compile_commands:
        try:
            args, output_path, source_file = convert_to_asm_command(entry)
            directory = Path(entry["directory"])
            tasks.append((args, output_path, source_file, directory))
        except Exception as e:
            print(
                f"Warning: Failed to parse command for {entry.get('file', 'unknown')}: {e}",
                file=sys.stderr,
            )

    # Execute in parallel
    success_count = 0
    failure_count = 0
    failures = []

    # Use number of CPUs as worker count
    max_workers = os.cpu_count() or 4

    print(f"Generating assembly files using {max_workers} workers...")

    with ThreadPoolExecutor(max_workers=max_workers) as executor:
        futures = {
            executor.submit(run_asm_command, args, output, source, directory): source
            for args, output, source, directory in tasks
        }

        for future in as_completed(futures):
            success, source_file, output_path, spill_reload_count, error = (
                future.result()
            )
            source_name = Path(source_file).name
            # Make output path relative to project root
            try:
                rel_output = output_path.relative_to(project_root)
            except ValueError:
                rel_output = output_path

            if success:
                success_count += 1
                print(
                    f"  [{success_count + failure_count}/{total}] ✓ {source_name} -> ./{rel_output} | spill/reload: {spill_reload_count}"
                )
            else:
                failure_count += 1
                print(
                    f"  [{success_count + failure_count}/{total}] ✗ {source_name} -> ./{rel_output} | spill/reload: {spill_reload_count}"
                )
                failures.append((source_file, error))

    # Summary
    print()
    print(f"Completed: {success_count} succeeded, {failure_count} failed")

    if failures:
        print("\nFailures:")
        for source_file, error in failures:
            print(f"  {source_file}:")
            for line in error.strip().split("\n")[:5]:  # Limit error output
                print(f"    {line}")
        sys.exit(1)

    sys.exit(0)


if __name__ == "__main__":
    main()
