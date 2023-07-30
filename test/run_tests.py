#!/usr/bin/env python3

import os
import subprocess
import sys

any_failed = False


def eprint(*args, **kwargs):
    print(*args, file=sys.stderr, **kwargs)


def run_test(path, test_case):
    result = subprocess.run([path, test_case], capture_output=True, text=True)

    if result == 3221225781:
        print("Failed to run test executable due to missing dependency")
        return False

    print(result.stdout)
    if result.returncode != 0:
        eprint(result.stderr)
        return False
    else:
        return True


def run_tests_for(module, test_cases):
    global any_failed

    bin_suffix = ".exe" if os.name == "nt" else ""
    exe_name = "%s%s" % (module, bin_suffix)

    base_path = "%s/test_binaries/%s" % (bin_dir, exe_name)

    if not os.path.isfile(base_path):
        raise ValueError("Invalid executable path %s" % exe_name)

    for test_case in test_cases:
        print("Running test case %s in executable %s" % (test_case, exe_name))

        res = run_test(base_path, test_case)

        if not res:
            any_failed = True
            eprint("Test case %s in executable %s failed" % (test_case, exe_name))


if len(sys.argv) != 2:
    raise ValueError("Invalid arguments! Usage: ./run_tests.py <binary dir>")

bin_dir = sys.argv[1]

run_tests_for("libs/lowlevel/test_lowlevel", [
    "[Dirtiable]",
    "[Vector]",
    "[Matrix]"
])

exit(1 if any_failed else 0)
