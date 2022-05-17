#--------------------------------------------------#
# Imports                                          #
#--------------------------------------------------#

import os
import subprocess
import argparse
import shutil

#--------------------------------------------------#
# Compilation helper                               #
#--------------------------------------------------#

class Compiler:
    def __init__(self, compiler=None):
        self.compiler = compiler
        print("==> Detecting os...")
        self.os = self.detect_os()
        print(f"==> Detected os: {self.os}" if self.os != "unknown" else "==> Unknown os")
        if not self.test_compiler():
            self.compiler = self.detect_compiler()
        else:
            print(f"==> Passed compiler {compiler}...yes")
        print("---------------------------")
        print("==> CLYQD COMPILATION PROCESS:")
        print(f"===> OS:           {self.os}")
        print(f"===> COMPILER:     {self.compiler}")
        print(f"===> CREATE DEBUG: No")
        print("---------------------------")

    def detect_os(self):
        if "ANDROID_ARGUMENT" in os.environ:
            return "android"
        return "windows" if os.name == "nt" else \
               "linux" if os.name == "posix" else \
               "macos" if os.name == "darwin" else \
               "unknown"

    def detect_compiler(self):
        print("==> No valid compiler passed")
        print("==> Testing compiler gcc...", end="")
        proc = subprocess.run("gcc --help", shell=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        if proc.returncode != 0:
            print("no")
        else:
            print("yes")
            return "gcc"
        print("==> Testing compiler clang...", end="")
        proc = subprocess.run("clang --help", shell=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        if proc.returncode != 0:
            print("no")
        else:
            print("yes")
            return "clang"
        if self.os == "windows":
            print("==> Testing compiler MSVC (cl)...", end="")
            proc = subprocess.run("cl", shell=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            if proc.returncode != 0:
                print("no")
            else:
                print("yes")
                return "cl"
        print("==> No valid compiler detected! Aborting...")
        exit(1)

    def test_compiler(self):
        if self.compiler is None:
            return False
        proc = subprocess.run(f"{self.compiler} --help", shell=True, stderr=subprocess.DEVNULL, stdout=subprocess.DEVNULL)
        if proc.returncode == 0:
            return True
        proc = subprocess.run(self.compiler, shell=True, stderr=subprocess.DEVNULL, stdout=subprocess.DEVNULL)
        return proc.returncode == 0

    def compile(self):
        if not os.path.exists("build"):
            os.mkdir("build")
        if not os.path.exists("bin"):
            os.mkdir("bin")
        for file in os.listdir("src/modules"):
            print(" ".join([self.compiler, "-Iinclude/", "-c", "-o", f"build/{file.replace('.c','.o')}", "src/modules/" + file]))
            proc = subprocess.run([self.compiler, "-Iinclude/", "-c", "-o", f"build/{file.replace('.c','.o')}", "src/modules/" + file])
            if proc.returncode != 0:exit(1)
        for file in os.listdir("src/compilers"):
            print(" ".join([self.compiler, "-Iinclude/", "-c", "-o", f"build/{file.replace('.c','.o')}", "src/compilers/" + file]))
            proc = subprocess.run([self.compiler, "-Iinclude/", "-c", "-o", f"build/{file.replace('.c','.o')}", "src/compilers/" + file])
            if proc.returncode != 0:exit(1)
        print(" ".join([self.compiler, "src/main.c", "-Iinclude/", "-o", f"bin/clyqd{'.exe' if self.os == 'windows' else ''}", *["build/" + x for x in os.listdir("build")]]))
        proc = subprocess.run([self.compiler, "src/main.c", "-Iinclude/", "-o", f"bin/lyqd{'.exe' if self.os == 'windows' else ''}", *["build/" + x for x in os.listdir("build")]])
        if proc.returncode != 0:exit(1)

    def compile_debug(self):
        if not os.path.exists("build"):
            os.mkdir("build")
        if not os.path.exists("bin"):
            os.mkdir("bin")
        for file in os.listdir("src/modules"):
            print(" ".join([self.compiler, "-Iinclude/", "-fsanitize=undefined,address", "-g3", "-Wall", "-Werror", "-c", "-o", f"build/{file.replace('.c','.o')}", "src/modules/" + file]))
            proc = subprocess.run([self.compiler, "-Iinclude/", "-fsanitize=undefined,address", "-g3", "-Wall", "-Werror", "-c", "-o", f"build/{file.replace('.c','.o')}", "src/modules/" + file])
            if proc.returncode != 0:exit(1)
        for file in os.listdir("src/compilers"):
            print(" ".join([self.compiler, "-Iinclude/", "-c", "-o", f"build/{file.replace('.c','.o')}", "src/compilers/" + file]))
            proc = subprocess.run([self.compiler, "-Iinclude/", "-fsanitize=undefined,address", "-g3", "-Wall", "-Werror", "-c", "-o", f"build/{file.replace('.c','.o')}", "src/compilers/" + file])
            if proc.returncode != 0:exit(1)
        print(" ".join([self.compiler, "src/main.c", "-Iinclude/", "-o", f"bin/lyqd_debug{'.exe' if self.os == 'windows' else ''}", *["build/" + x for x in os.listdir("build")]]))
        proc = subprocess.run([self.compiler, "src/main.c", "-Iinclude/", "-fsanitize=undefined,address", "-g3", "-Wall", "-Werror", "-o", f"bin/clyqd{'.exe' if self.os == 'windows' else ''}", *["build/" + x for x in os.listdir("build")]])
        if proc.returncode != 0:exit(1)

def parse_args():
    ap = argparse.ArgumentParser(description="Compile clyqd")
    ap.add_argument("-c", "--compiler", nargs=1, help="The compiler to use when compiling seawater")
    ap.add_argument("-d", "--debug", action="store_const", const=True, help="Create a debug binary")
    ap.add_argument("--clean", action="store_const", const=True, help="Clean up build files")

    return ap.parse_args()

def main():
    args = parse_args()
    if args.clean:
        if os.path.exists("bin"):
            shutil.rmtree("bin")
        if os.path.exists("build"):
            shutil.rmtree("build")
        exit(0)
    compiler = Compiler(args.compiler and args.compiler[0])
    compiler.compile()
    if args.debug:
        compiler.compile_debug()

if __name__ == "__main__":
    main()