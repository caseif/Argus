#!/usr/bin/env python3

import os
import subprocess
import sys


def eprint(*args, **kwargs):
    print(*args, file=sys.stderr, **kwargs)


def run_test(path):
    result = subprocess.run(path, capture_output=True, text=True)

    if result == 3221225781:
        print("Failed to run test executable due to missing dependency")
        return False

    print(result.stdout)
    if result.returncode != 0:
        eprint(result.stderr)
        return False
    else:
        return True


def run_tests_for(module):
    print("Running tests for module %s" % module)

    base_path = "%s/test_binaries/%s" % (bin_dir, module)

    if not os.path.isdir(base_path):
        raise ValueError("Invalid module path %s" % module)

    success_count = 0
    total_count = 0

    for file in os.listdir(base_path):
        test_name = file[:-4] if file.endswith(".exe") else file
        res = run_test("%s/%s" % (base_path, file))

        if res:
            success_count += 1
        else:
            eprint("Test %s in module %s failed" % (test_name, module))

        total_count += 1

    print("%d/%d tests passed for module %s" % (success_count, total_count, module))


if len(sys.argv) != 2:
    raise ValueError("Invalid arguments! Usage: ./run_tests.py <binary dir>")

bin_dir = sys.argv[1]

run_tests_for("libs/lowlevel")
#run_tests_for("static/scripting")
