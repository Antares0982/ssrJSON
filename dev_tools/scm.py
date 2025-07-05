import os
import subprocess


def generate_scm_version_and_copy(
    repo: str, template_file: str, version: "str | None" = None
):
    if not template_file.endswith(".in"):
        raise ValueError("Template file should ends with '.in'")
    if not version:
        scm_raw = (
            subprocess.check_output(["git", "describe", "--tags", "--always"], cwd=repo)
            .decode()
            .strip()
        )
    else:
        scm_raw = version
    if "-" in scm_raw:
        version_part, gitver = scm_raw.split("-", 1)
    else:
        version_part, gitver = scm_raw, "0"
    nums = version_part.split(".")
    if len(nums) != 3:
        major, minor, patch = "0", "0", "0"
        gitver = scm_raw
        scm_raw = f"0.0.0-{gitver}"
    else:
        major, minor, patch = nums
        if any(not x.isdigit() for x in [major, minor, patch]):
            raise ValueError(
                f"SCM version should be digits, got '{major}', '{minor}' and '{patch}'"
            )
    new_file = template_file[: -len(".in")]
    with open(template_file, "r", encoding="utf-8") as f:
        content = f.read()
    new_content = (
        content.replace("@VERSION@", scm_raw)
        .replace("@MAJOR@", major)
        .replace("@MINOR@", minor)
        .replace("@PATCH@", patch)
        .replace("@GITVER@", gitver)
    )
    new_content = "// This file is generated. DO NOT EDIT.\n" + new_content
    if os.path.exists(new_file):
        with open(new_file, "r", encoding="utf-8") as f:
            if f.read() == new_content:
                return
    with open(new_file, "w", encoding="utf-8") as f:
        f.write(new_content)


def main():
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument("template", type=str)
    parser.add_argument("--directory", default=".")
    parser.add_argument("--version", default=None, help="Specify a version.")
    args = parser.parse_args()

    generate_scm_version_and_copy(args.directory, args.template, version=args.version)


if __name__ == "__main__":
    main()
