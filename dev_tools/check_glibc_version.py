#!/usr/bin/env -S python3 -O
#!/usr/bin/env python3
import subprocess
import sys
import re

def get_glibc_versions(lib_path):
    cmd = ['readelf', '--version-info', lib_path]
    try:
        result = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=True, text=True)
    except subprocess.CalledProcessError as e:
        print(f"Error running readelf: {e.stderr}", file=sys.stderr)
        return None
    
    version_pattern = re.compile(r'Name: GLIBC_(\d+)\.(\d+)')
    versions = set()
    for line in result.stdout.splitlines():
        match = version_pattern.search(line)
        if match:
            major, minor = match.groups()
            major = int(major)
            minor = int(minor)
            if major == 2:
                versions.add(minor)
    return versions

def main():
    if len(sys.argv) != 3:
        print("Usage: python check_glibc_version.py <path_to_shared_lib> <glibc_minor_version>", file=sys.stderr)
        print("Example: python check_glibc_version.py /lib/x86_64-linux-gnu/libc.so.6 17", file=sys.stderr)
        sys.exit(1)

    lib_path = sys.argv[1]
    try:
        given_minor = int(sys.argv[2])
    except ValueError:
        print("Invalid glibc minor version. Must be an integer like 17 for 2.17", file=sys.stderr)
        sys.exit(1)

    versions = get_glibc_versions(lib_path)
    if versions is None or len(versions) == 0:
        print(f"Could not find any GLIBC versioned symbols in {lib_path}", file=sys.stderr)
        sys.exit(1)

    max_version = max(versions)
    print(f"Maximum GLIBC version used by {lib_path}: 2.{max_version}")
    if max_version <= given_minor:
        print(f"The dynamic library's GLIBC version is not higher than 2.{given_minor} ✔️")
        sys.exit(0)
    else:
        print(f"The dynamic library's GLIBC version is higher than 2.{given_minor} ❌")
        sys.exit(2)

if __name__ == "__main__":
    main()
