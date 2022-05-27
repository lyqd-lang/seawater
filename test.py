from os import listdir;
from os.path import exists;
from pathlib import Path;
import subprocess;

info = lambda x: print(f"\033[96m[INFO] {x}\033[00m")
warn = lambda x: print(f"\033[93m[WARN] {x}\033[00m");
err  = lambda x: print(f"\033[91m[ERR] {x}\033[00m")

LC = "bin/clyqd";
ARGS = "-l bin/runtime/runtime.o -pb"

def run_test(name: str) -> bool:
    return True;

def main():
    tests = listdir("tests");
    success_tests = 0;
    failed_tests = 0;
    Path("tests/bin").mkdir(parents = True, exist_ok = True);
    for test in tests:
        test_should_error = False;

        if test == "bin": continue;
        if not exists(f"tests/{test}/main.lqd"):
            warn(f"skipping test {test}!");
            continue;
        
        with open(f"tests/{test}/main.lqd") as file:
            if file.readline().rstrip() == "// SHOULD_ERROR": test_should_error = True;
        
        try:
            lyqd_proc = subprocess.run(f"{LC} tests/{test}/main.lqd {ARGS} -o tests/bin/{test}", capture_output = True);
            success_tests += 1;
        except:
            if test_should_error: success_tests += 1;
            else:
                failed_tests += 1;
                err(f"test {test} failed.")
    
    if failed_tests == 0:
        info("All tests succeeded!");
        exit(0);
    elif success_tests == 0:
        err(f"All tests failed.");
        exit(1);
    else:
        warn(f"{success_tests} succeeded, {failed_tests} failed.")

if __name__ == "__main__":
    main();
    