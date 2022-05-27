#!/usr/bin/python3

from os import listdir, chmod, getcwd;
from os.path import exists;
from pathlib import Path;
from subprocess import Popen, PIPE, STDOUT
import json

info = lambda x: print(f"\033[96m[INFO] {x}\033[00m")
warn = lambda x: print(f"\033[93m[WARN] {x}\033[00m");
err  = lambda x: print(f"\033[91m[ERR] {x}\033[00m")

LC = f"{getcwd()}/bin/clyqd";
ARGS = ["-pb"]

def run_test(test: str) -> bool:
    test_should_error = False;
    f = open(f"tests/{test}/test.json");
    metadata = json.load(f);
    f.close();
    entry_file = f"tests/{test}/{metadata['entry']}";
    if not exists(entry_file): warn("Skipping test {test} because the entry file didn't exist.");
    
    # check compiler output
    compiler = Popen([LC, f"tests/{test}/{metadata['entry']}", "-o", f"tests/bin/{test}", *ARGS, *metadata["compiler_args"]], stdout = PIPE, stderr = STDOUT);
    stdout, stderr = compiler.communicate();
    compiler_output = stdout.decode("utf-8");
    
    if not metadata["expected_compiler_output"] == "":
        f = open(f"tests/{test}/{metadata['expected_compiler_output']}")
        expected_compiler_output = f.read();
        f.close();
        if not compiler_output == expected_compiler_output:
            err(f"Test {test}'s compiler output didn't match the expected output.\n--- Compiler output ---\n{compiler_output}");
            return False;
    
    if not compiler.returncode == metadata["compiler_exit_code"]:
        err(f"Test {test}'s compiler exit code didnt match the expected exit code. (code {compiler.returncode})");
        return False;
    
    # check program output
    program = Popen([f"{getcwd()}/tests/bin/{test}", *metadata["program_args"]], stdout = PIPE, stderr = STDOUT);
    stdout, stderr = program.communicate();
    program_output = stdout.decode("utf-8");
    if not metadata["expected_output"] == "":
        f = open(f"tests/{test}/{metadata['expected_output']}")
        expected_program_output = f.read();
        f.close();
        if not program_output == expected_program_output:
            err(f"Test {test}'s output didnt match the expected output.\n--- output ---\n{program_output}");
            return False;
    
    if not program.returncode == metadata["program_exit_code"]:
        err(f"Test {test}'s exit code didnt match the expected exit code. (code {program.returncode})");
        return False;
        

    return True;



def main():
    tests = listdir("tests");
    success_tests = 0;
    failed_tests = 0;
    Path("tests/bin").mkdir(parents = True, exist_ok = True);
    for test in tests:
        if test == "bin": continue; # skip binaries directory
        res = run_test(test);
        if res: success_tests += 1;
        else: failed_tests += 1;
    
    if failed_tests == 0:
        info("All tests succeeded!");
        exit(0);
    elif success_tests == 0:
        err(f"All tests failed.");
        exit(1);
    else:
        warn(f"{success_tests} succeeded, {failed_tests} failed.")
        exit(1);

if __name__ == "__main__":
    main();
    