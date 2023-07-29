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


def run_tests_for(module, test_case):
    global any_failed

    print("Running test case %s for module %s" % (test_case, module))

    base_path = "%s/test_binaries/%s" % (bin_dir, module)

    if not os.path.isdir(base_path):
        raise ValueError("Invalid module path %s" % module)

    for file in os.listdir(base_path):
        test_name = file[:-4] if file.endswith(".exe") else file
        res = run_test("%s/%s" % (base_path, file), test_case)

        if not res:
            any_failed = True
            eprint("Test case %s in module %s failed" % (test_case, module))


if len(sys.argv) != 2:
    raise ValueError("Invalid arguments! Usage: ./run_tests.py <binary dir>")

bin_dir = sys.argv[1]

run_tests_for("libs/lowlevel", "[Dirtiable]")
run_tests_for("libs/lowlevel", "[Vector2]")
#run_tests_for("static/scripting")

exit(1 if any_failed else 0)
