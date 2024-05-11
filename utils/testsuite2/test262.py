# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# This source code is licensed under the MIT license found in the
# LICENSE file in the root directory of this source tree.

import asyncio
import os.path
import sys
import time
from asyncio import Semaphore, subprocess
from collections import defaultdict
from typing import Awaitable, List

import preprocess

import utils
from preprocess import StrictMode
from progress import (
    ProgressBar,
    SimpleProgressBar,
    TerminalController,
    TestCaseResult,
    TestingProgressDisplay,
)
from skiplist import SkipCategory, SkippedPathsOrFeatures
from typing_defs import OptNegative, PathT
from utils import Color, TestResultCode

ES6_ARGS = ["-Xes6-promise", "-Xes6-proxy"]
EXTRA_RUN_ARGS = ["-Xhermes-internal-test-methods"]
USE_MICROTASK_FLAG = ["-Xmicrotask-queue"]
EXTRA_COMPILE_FLAGS = ["-fno-static-builtins"]

TIMEOUT_COMPILER = 200
TIMEOUT_VM = 200


async def make_call(
    test_name: str,
    js_source_files: List[PathT],
    strict_mode: StrictMode,
    binary_path: PathT,
    negative: OptNegative,
    disable_handle_san: bool,
) -> TestCaseResult:
    """
    Run the generated source files with async subprocess and return the
    result.
    """
    for js_source_file in js_source_files:
        run_vm = True
        base_file_name = os.path.basename(js_source_file)
        file_to_run = f"{js_source_file}.out"
        args = [
            str(js_source_file),
            "-emit-binary",
            "-out",
            file_to_run,
        ] + EXTRA_COMPILE_FLAGS

        args.append("-O0")
        if StrictMode.Strict in strict_mode:
            args.append("-strict")
        else:
            args.append("-non-strict")

        negative_phase = negative["phase"] if negative else ""
        hermesc_exe = os.path.join(binary_path, "hermesc")
        proc = await asyncio.create_subprocess_exec(
            hermesc_exe, *args, stderr=subprocess.PIPE, stdout=subprocess.PIPE
        )
        stdout, stderr = (None, None)
        try:
            (stdout, stderr) = await asyncio.wait_for(
                proc.communicate(), timeout=TIMEOUT_COMPILER
            )
        except asyncio.TimeoutError:
            msg = f"FAIL: Compilation timed out on {js_source_file}"
            proc.kill()
            return TestCaseResult(test_name, TestResultCode.COMPILE_TIMEOUT, msg)

        output = ""
        if stdout:
            output += f"stdout:\n {stdout.decode('utf-8')}"
        if stderr:
            output += f"stderr:\n {stderr.decode('utf-8')}"

        # Check if the compilation succeeded
        # There is no CalledProcessError in asyncio, so explicitly check the
        # return code.
        if proc.returncode:
            run_vm = False
            if negative_phase != "early" and negative_phase != "parse":
                msg = f"Fail to run command: {args}"
                return TestCaseResult(
                    test_name, TestResultCode.COMPILE_FAILED, msg, output
                )
        else:
            if negative_phase == "early" or negative_phase == "parse":
                msg = f"FAIL: Compilation failure expected on {base_file_name} with Hermes"
                return TestCaseResult(
                    test_name, TestResultCode.COMPILE_FAILED, msg, output
                )

        if run_vm:
            # Run the generated bytecode/native code.
            hvm_exe = os.path.join(binary_path, "hvm")
            args = [file_to_run] + ES6_ARGS + EXTRA_RUN_ARGS + USE_MICROTASK_FLAG
            if disable_handle_san:
                args += ["-gc-sanitize-handles=0"]
            env = {"LC_ALL": "en_US.UTF-8"}
            if sys.platform == "linux":
                env["ICU_DATA"] = binary_path
            proc = await asyncio.create_subprocess_exec(
                hvm_exe, *args, env=env, stderr=subprocess.PIPE, stdout=subprocess.PIPE
            )
            stdout, stderr = (None, None)
            try:
                (stdout, stderr) = await asyncio.wait_for(
                    proc.communicate(), timeout=TIMEOUT_COMPILER
                )
            except asyncio.TimeoutError:
                msg = f"FAIL: Execution of binary timed out for {file_to_run}"
                # Kill the subprocess and all its child processes
                proc.kill()
                return TestCaseResult(test_name, TestResultCode.EXECUTE_TIMEOUT, msg)

            output = ""
            if stdout:
                output += f"stdout:\n {stdout.decode('utf-8')}"
            if stderr:
                output += f"stderr:\n {stderr.decode('utf-8')}"

            # Check if the run succeeded
            if proc.returncode:
                if negative_phase == "" or negative_phase != "runtime":
                    msg = f"FAIL: Execution of {base_file_name} threw unexpected error"
                    return TestCaseResult(
                        test_name, TestResultCode.EXECUTE_FAILED, msg, output
                    )
                else:
                    msg = f"PASS: execution of {base_file_name} threw an error as expected"
                    return TestCaseResult(test_name, TestResultCode.TEST_PASSED, msg)
            else:
                if negative_phase != "":
                    msg = f"FAIL: Expected execution to throw"
                    return TestCaseResult(
                        test_name, TestResultCode.TEST_UNEXPECTED_PASSED, msg, output
                    )

    return TestCaseResult(test_name, TestResultCode.TEST_PASSED)


async def run_test(
    test_file: PathT,
    suite_path: PathT,
    tests_home: PathT,
    work_dir: PathT,
    binary_path: PathT,
    skipped_paths_features: SkippedPathsOrFeatures,
) -> TestCaseResult:
    """
    Load and preprocess the test file, check if it's skipped. If not, run
    the preprocessed source code, return the result.
    """
    with open(test_file, "rb") as reader:
        content = reader.read().decode("utf-8")
    rel_test_path = os.path.relpath(test_file, suite_path)
    test_case = preprocess.generate_source(content, suite_path, rel_test_path)
    if "testIntl.js" in test_case.includes:
        msg = f"SKIP: no support for multiple Intl constructors in {rel_test_path}"
        return TestCaseResult(rel_test_path, TestResultCode.TEST_SKIPPED, msg)

    flags = test_case.flags
    if "async" in flags:
        return TestCaseResult(
            rel_test_path,
            TestResultCode.TEST_SKIPPED,
            "SKIP: test has `async` flag",
        )
    if "module" in flags:
        return TestCaseResult(
            rel_test_path,
            TestResultCode.TEST_SKIPPED,
            "SKIP: test has `module` flag",
        )

    base_name = os.path.basename(test_file)
    base_name_no_ext = os.path.splitext(base_name)[0]

    # Check if we need to skip this test due to unsupported features.
    for f in test_case.features:
        if skip_result := skipped_paths_features.try_skip(
            f,
            [
                SkipCategory.UNSUPPORTED_FEATURES,
                SkipCategory.PERMANENT_UNSUPPORTED_FEATURES,
            ],
            rel_test_path,
        ):
            return skip_result

    tmp_dir: PathT = os.path.join(
        work_dir, os.path.dirname(os.path.relpath(test_file, tests_home))
    )
    os.makedirs(tmp_dir, exist_ok=True)
    flags_str = ("." + "_".join(sorted(flags))) if len(flags) > 0 else ""
    js_sources = []
    if StrictMode.Strict in test_case.strict_mode:
        js_source = os.path.join(tmp_dir, f"{base_name_no_ext}.strict{flags_str}.js")
        with open(js_source, "wb") as writer:
            # Add the directive for strict mode
            writer.write("'use strict';\n".encode("utf-8"))
            writer.write(test_case.source.encode("utf-8"))
        js_sources.append(js_source)

    if StrictMode.NoStrict in test_case.strict_mode:
        js_source = os.path.join(
            tmp_dir,
            f"{base_name_no_ext}{flags_str}.js",
        )
        with open(js_source, "wb") as writer:
            writer.write(test_case.source.encode("utf-8"))
        js_sources.append(js_source)

    disable_handle_san = skipped_paths_features.should_skip_cat(
        test_file, SkipCategory.HANDLESAN_SKIP_LIST
    )

    return await make_call(
        rel_test_path,
        js_sources,
        test_case.strict_mode,
        binary_path,
        test_case.negative,
        disable_handle_san,
    )


def print_stats(stats: dict):
    """Print the result stats."""
    total = sum(stats.values())
    failed = (
        stats[TestResultCode.COMPILE_FAILED]
        + stats[TestResultCode.COMPILE_TIMEOUT]
        + stats[TestResultCode.EXECUTE_FAILED]
        + stats[TestResultCode.EXECUTE_TIMEOUT]
        + stats[TestResultCode.TEST_UNEXPECTED_PASSED]
    )
    eligible = (
        sum(stats.values())
        - stats[TestResultCode.TEST_SKIPPED]
        - stats[TestResultCode.TEST_PERMANENTLY_SKIPPED]
    )

    if eligible > 0:
        passRate = "{0:.2%}".format(stats[TestResultCode.TEST_PASSED] / eligible)
    else:
        passRate = "--"

    if (eligible - stats[TestResultCode.TEST_PASSED]) > 0:
        resultStr = "{}FAIL{}".format(Color.RED, Color.RESET)
    else:
        resultStr = "{}PASS{}".format(Color.GREEN, Color.RESET)

    # Turn off formatting so that the table looks nice in source code.
    # fmt: off
    print("-----------------------------------")
    print("| Results              |   {}   |".format(resultStr))
    print("|----------------------+----------|")
    print("| Total                | {:>8} |".format(total))
    print("| Pass                 | {:>8} |".format(stats[TestResultCode.TEST_PASSED]))
    print("| Fail                 | {:>8} |".format(failed))
    print("| Skipped              | {:>8} |".format(stats[TestResultCode.TEST_SKIPPED]))
    print("| Permanently Skipped  | {:>8} |".format(stats[TestResultCode.TEST_PERMANENTLY_SKIPPED]))
    print("| Pass Rate            | {:>8} |".format(passRate))
    print("-----------------------------------")
    print("| Failures             |          |")
    print("|----------------------+----------|")
    print("| Compile fail         | {:>8} |".format(stats[TestResultCode.COMPILE_FAILED]))
    print("| Compile timeout      | {:>8} |".format(stats[TestResultCode.COMPILE_TIMEOUT]))
    print("| Execute fail         | {:>8} |".format(stats[TestResultCode.EXECUTE_FAILED]))
    print("| Execute timeout      | {:>8} |".format(stats[TestResultCode.EXECUTE_TIMEOUT]))
    print("-----------------------------------")
    # fmt: on


def print_failed_tests(tests: dict):
    def print_test_list(cat: TestResultCode, header: str):
        if len(tests[cat]) > 0:
            print("-----------------------------------")
            print(header)
            for test in tests[cat]:
                print(test)
            print("-----------------------------------")

    print("\nDetails:")
    print_test_list(TestResultCode.COMPILE_FAILED, "Compile failed:")
    print_test_list(TestResultCode.COMPILE_TIMEOUT, "Compile timeout:")
    print_test_list(TestResultCode.EXECUTE_FAILED, "Execute failed:")
    print_test_list(TestResultCode.EXECUTE_TIMEOUT, "Execute timeout:")
    print_test_list(TestResultCode.TEST_UNEXPECTED_PASSED, "Unexpected passed:")


async def run(
    tests_paths: List[PathT],
    binary_path: PathT,
    skipped_paths_features: SkippedPathsOrFeatures,
    work_dir: PathT,
    n_jobs: int,
    verbose: bool,
) -> None:
    """
    Run all tests with async subprocess and wait for results in completion order.
    Each subprocess invokes hermes binary on a given test file, so to restrict
    resource contention, we use asyncio.Semaphore to control the maximum number
    of alive tasks (configured by n_jobs).

    We first collect all tasks for given tests_paths, then wrap each of them
    in a new task guarded by Semaphore, and pass them to asyncio.as_completed().
    Each task can be awaited in it to get earliest next result. And once a
    task is done, its Semaphore counter is released and a new task can
    continue to run.

    After all tasks are done, print the test stats and relative paths of all
    failing tests (if there are).
    """
    utils.check_hermes_exe(binary_path)

    # Get the common path of all file/directory paths to be used when creating
    # temporary files.
    tests_home = os.path.commonpath(tests_paths)
    tests_files = utils.list_all_files(tests_paths)

    header = f"-- Testing: {len(tests_files)} tests, max {n_jobs} concurrent tasks --"
    try:
        term = TerminalController()
        pb = ProgressBar(term, header)
    except ValueError:
        print(header)
        pb = SimpleProgressBar("Testing: ")
    pd = TestingProgressDisplay(len(tests_files), pb, verbose)
    current_n_tasks = Semaphore(n_jobs)

    start_time = time.time()
    tasks = []
    stats = defaultdict(int)
    for test_file in tests_files:
        suite_path = utils.get_suite(test_file)
        assert suite_path, "Test suite root directory must be found!"
        rel_test_path = os.path.relpath(test_file, suite_path)

        # Check if this file should be skipped.
        if test_result := skipped_paths_features.try_skip(
            test_file,
            [SkipCategory.SKIP_LIST, SkipCategory.PERMANENT_SKIP_LIST],
            rel_test_path,
        ):
            pd.update(test_result)
            stats[test_result.code] += 1
            continue
        elif test_result := skipped_paths_features.try_skip(
            test_file, [SkipCategory.INTL_TESTS], rel_test_path
        ):
            pd.update(test_result)
            stats[TestResultCode.TEST_SKIPPED] += 1
            continue

        tasks.append(
            run_test(
                test_file,
                suite_path,
                tests_home,
                work_dir,
                binary_path,
                skipped_paths_features,
            )
        )

    # Control maximum running tasks.
    async def wrap_task(fut: Awaitable) -> TestCaseResult:
        # Acquire the Semaphore and decrement it by one. If the counter is
        # already zero, wait until one running task is done, which will release
        # the Semaphore and increment the counter.
        async with current_n_tasks:
            return await fut

    failed_cases = defaultdict(list)
    for task in asyncio.as_completed([wrap_task(fut) for fut in tasks]):
        result = await task
        stats[result.code] += 1
        pd.update(result)
        if result.code.is_failure:
            failed_cases[result.code].append(result.test_name)
    pd.finish()
    elapsed = time.time() - start_time
    print(f"Testing time: {elapsed:.2f}")

    # Print result
    print_stats(stats)
    print_failed_tests(failed_cases)
