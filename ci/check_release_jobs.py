import json
import os
import sys


def blocking_jobs(pages, publisher, dry_run):
    """List jobs that block publication.

    >>> jobs = [{"name": "Publish"}, {"name": "macOS (11)", "conclusion": "cancelled"}]
    >>> blocking_jobs([{"total_count": 2, "jobs": jobs}], "Publish", False)
    ['macOS (11): cancelled']
    """
    jobs = [job for page in pages for job in page["jobs"]]
    if len(jobs) < 2 or len(jobs) != pages[0]["total_count"]:
        return ["Incomplete release job list"]

    blocked = []
    for job in jobs:
        name = job["name"]
        if name == publisher or job["conclusion"] == "success":
            continue
        if dry_run and name == "Run-Fuzzer" and job["conclusion"] == "skipped":
            continue
        blocked.append(f"{name}: {job['conclusion'] or job['status']}")
    return blocked


if __name__ == "__main__":
    blocked = blocking_jobs(
        json.load(sys.stdin), os.environ["GITHUB_JOB"], os.environ["DRY_RUN"] == "true"
    )
    if blocked:
        sys.exit("\n".join(blocked))
