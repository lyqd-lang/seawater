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
Before you get started compiling lyqd code, you must compile `syscalls.asm`. It's a file containing some simple I/O functions. To compile it, run:<br/>
`nasm -o syscalls.o examples/linux/syscalls.asm -felf64`<br/>
Now you can compile a helloworld program!<br/>
`bin/clyqd examples/linux/test.lqd -o helloworld -l syscalls.o -pb`<br/>
So the end result will look something like:
```
$ nasm -o syscalls.o examples/linux/syscalls.asm -felf64
$ bin/clyqd helloworld.lqd -o helloworld -l syscalls.o -pb
$ ./helloworld
Hello, world!
$
```
## Windows
To compile on windows you're gonna need hese chocolatey packages:
 - nasm
 - visualstudio2017buildtools
 - visualstudio2017-workload-vctools
Once you've got these, you're gonna create a file called `vcexec.bat`<br/>
in this file put:
```
call "C:\\Program Files (x86)\Microsoft Visual Studio\2017\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" amd64 %*
```
Then run `nasm .\examples\windows\console.asm -o console.o -fwin64`
<br/><br/>
Alright now you're ready to start compiling some programs!<br/>
You can (mostly) follow the steps above.
But to compile a lyqd program on windows you're gonna have to do it manually:<br/>
`.\bin\clyqd.exe .\examples\linux\test.lqd`<br/><br/>
`powershell -Command "~\\vsexec.bat link /entry:start /subsystem:console a.o console.o kernel32.lib"`<br/>
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