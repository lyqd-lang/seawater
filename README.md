# seawater
seawater is a Lyqd compiler written entirely in C.<br/>
It can compile any Lyqd code compliant with the latest standard, and any code that was written for RustLyqd.<br/>
# Compiling with seawater
To compile a file with seawater, you simply run the command:<br/>
`lyqd <file.lqd>`<br/>
The seawater compiler takes in a few arguments:<br/>
```
-v                 Print the compiler version
-o <file>          Specify output file
-k                 Keep the assembly code
-l <file>          Link an object file
-pb                Whether to link the object file into a binary
```
# Examples
seawater as of right now is really barebones. Tho it's capable of compile a few programs.<br/>
## Linux
Ye idfk how windows does kernel calls 'n shit so here's a linux section:<br/>
Before you get started compiling lyqd code, you must compile `syscalls.asm`. It's a file containing some simple I/O functions. To compile it, run:<br/>
`nasm -o syscalls.o examples/linux/syscalls.asm -felf64`<br/>
Now you can compile very simple programs like:
```
// Here tell the compiler that we got a function defined in another file
extern puts(str text, num length);

// Main entry point of program, called from _start
fn main() {
    // Call the Linux kernel and tell it to write "Hello, world!"
    // to the standard output. The backend of this function is
    // defined in examples/linux/syscalls.asm
    puts("Hello, world!\n", 14);
    // By default, seawater will exit with returncode 0, so no
    // need for a `return 0;`
}
```
Assuming you save the above code into a file called `helloworld.lqd`, you can run:<br/>
`bin/lyqd helloworld.lqd -o helloworld -l syscalls.o -pb`<br/>
So the end result will look something like:
```
$ nasm -o syscalls.o examples/linux/syscalls.asm -felf64
$ bin/lyqd helloworld.lqd -o helloworld -l syscalls.o -pb
$ ./helloworld
Hello, world!
$
```

# Compiling seawater
## Dependencies
* Preffered C compiler
* nasm
## Compiling
Simply run `make` and it'll detect os/compiler and just work. If it doesn't work, follow instructions below:<br/>
### Compiling on Linux
#### GCC / G++
It's recommended you compile with the GNU toolchain, hence you should be able to simply run:<br/>
`python3 build.py`<br/>
And it'll compile seawater for you using gcc.<br/>
#### clang
seawater does compile and run using clang. Please not that if you have both clang and gcc installed, the builder script will proritize gcc. If you wish to compile using clang, you can run:<br/>
`python3 build.py -c clang`<br/>
### Compiling on Windows
#### GCC / G++
These are the recommended compilers when compiling on windows, thus you can just run:<br/>
`py build.py`<br/>
And it will by default use gcc with MinGW<br/>
#### MSVC
NOTE: MSVC support isn't fully tested, and fails to compiel on 32bit platforms<br/><br/>
If you're compiling on Visual Studio or wish to use the MSVC compilers, you can run:<br/>
`py build.py -c cl`<br/>
Which'll use the MSVC C++ compiler.<br/>
If you want to compile on Visual Studio, I will ship a .vsxproj file, but until then, you're gonna have to manually add compiler flags.<br/>
### Compiling on Android
Yes. seawater both compiles and runs on android.<br/>
Android ships with clang, so you can just follow above instructions and it *should* function properly.<br/>