import json
import os
import subprocess
import sys
import tempfile

CUR_FILE = os.path.abspath(__file__)
CUR_DIR = os.path.dirname(CUR_FILE)


class MockArgs(dict):
    def __setattr__(self, name, value):
        self[name] = value

    def __getattr__(self, name):
        return self.get(name)


def run_all_versions_test(args) -> int:
    with open("./dev_tools/pyver.json", "rb") as f:
        _o = json.load(f)
        vers = list(range(_o["minSupportVer"], _o["maxSupportVer"] + 1))
    exec_command_str = f"""
import importlib.util
spec = importlib.util.spec_from_file_location("autotest", "{CUR_FILE}")
mod = importlib.util.module_from_spec(spec)
spec.loader.exec_module(mod)
args = mod.MockArgs()
args.build_type = "{args.build_type}"
args.build_only = {bool(args.build_only)}
args.asan = {bool(args.asan)}
args.pyver = $TO_INSERT
mod.run_test(args)
"""
    print(exec_command_str)

    tempfiles = []
    for ver in vers:
        exec_command = exec_command_str.replace("$TO_INSERT", str(ver))
        file = tempfile.mktemp()
        with open(file, "w", encoding="utf-8") as f:
            f.write(exec_command)
        tempfiles.append(file)

    processes = []
    for file in tempfiles:
        process = subprocess.Popen([sys.executable, file])
        processes.append(process)

    has_error = False
    for i, process in enumerate(processes):
        process.wait()
        if process.returncode != 0:
            has_error = True
            print(f"Error when testing pyver: {vers[i]}")

    if not has_error:
        print("All tests passed.")

    # clean up
    for file in tempfiles:
        os.remove(file)

    return 1 if has_error else 0


def run_test(args):
    if args.asan:
        print("NOTE: use asan check will suppress `build-type` option.")

    if args.all_ver:
        return run_all_versions_test(args)

    if sys.platform in ["linux", "darwin"]:
        # use nix
        test_entrance = os.path.join(CUR_DIR, "asan_check.sh" if args.asan else "autotest.sh")
        envs = os.environ.copy()
        envs.update({
            "BUILD_PY_VER": str(args.pyver),
            "ISOLATE_BUILD": "1"
        })
        if not args.asan:
            build_type = args.build_type
            envs["TARGET_BUILD_TYPE"] = build_type
        if args.build_only:
            envs["SKIP_TEST"] = "1"
        if "IN_NIX_SHELL" in os.environ:
            print("Already in nix shell")
            os.execvpe("bash", ["bash", test_entrance], envs)
        os.execvpe("nix", ["nix", "develop", "--command", test_entrance], envs)
    else:
        # Windows: not support --all-ver, use current python version
        import shutil
        from os.path import dirname
        from sysconfig import get_config_h_filename, get_config_var
        new_env = os.environ.copy()
        new_env["Python3_EXECUTABLE"] = sys.executable
        new_env["Python3_INCLUDE_DIR"] = dirname(get_config_h_filename())
        ldlib = get_config_var("LDLIBRARY")
        if ldlib is not None:
            new_env["Python3_LIBRARY"] = get_config_var("prefix") + os.path.sep + ldlib
        if os.path.exists("build"):
            shutil.rmtree("build")
        os.makedirs("build")
        if args.asan:
            build_type = "Debug"
        else:
            build_type = args.build_type
        configure_cmd = ["cmake", "-T", "ClangCL", "-S", ".", "-B", "build", "-DCMAKE_BUILD_TYPE=" + build_type]
        if args.asan:
            configure_cmd += ["-DASAN_ENABLED=ON"]
        subprocess.run(configure_cmd, check=True, env=new_env)
        subprocess.run(["cmake", "--build", "build", "--config", build_type], check=True, env=new_env)
        target_file = f"build/{build_type}/ssrjson.pyd"
        os.rename(target_file, "build/ssrjson.pyd")
        if args.asan:
            os.rename(f"build/{build_type}/clang_rt.asan_dynamic-x86_64.dll", "build/clang_rt.asan_dynamic-x86_64.dll")
        if args.build_only:
            return 0
        # run test
        new_env["PYTHONPATH"] = os.path.join(os.curdir, "build")
        exe_base_name = os.path.basename(sys.executable)
        cmd = [exe_base_name, "-m", "pytest", "python-test"]
        subprocess.run(cmd, check=True, env=new_env)
    return 0


def main():
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument("--build-type", help="CMake Build type", default="Debug")
    parser.add_argument("--pyver", help="Specify Python version, default to 14", default="14")
    parser.add_argument("--all-ver", help="Test with all versions", action="store_true")
    parser.add_argument("--build-only", help="Build without running tests", action="store_true")
    parser.add_argument("--asan", help="Run asan check", action="store_true")

    args = parser.parse_args()

    exit(run_test(args))


if __name__ == "__main__":
    main()
