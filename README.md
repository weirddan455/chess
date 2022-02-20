# chess

Chess game written in C99 with AI opponent.  Currently supports Linux and Windows.  Linux build probably will also work on BSD but un-tested.

Most everything has been written from scratch.  The only libraries used are Xlib (Linux only) and stb_truetype (header only library, comes included in source).
The Windows build uses the Windows API directly.

## Requirements

To build, you will need a C compiler and CMake.  Build has been tested with GCC, Clang, and MSVC.  Linux users will additonally need GNU Make and "-devel" packages of Xorg if your distribution seperates them.

On Windows, Visual Studio will work as a compiler or you can build with a Unix-like environment with MSYS2.

## Building

Two bash scripts are provided "clean-build.sh" and "win-build.sh".  They're mostly for my personal use but you may find them helpful.  They're meant to be used in Unix-like environments.  I use win-build.sh for cross compiling a Windows binary from Linux.

To manually build, create a build directory, cd into it, and run cmake.

```
mkdir build
cd build
cmake ..
```

On Windows, if you have Visual Studio installed, this should create Visual Studio solution/project files.  On Linux, this will create Makefiles (simply type "make" inside the build directory to compile).

Lastly, the game will look for an "assets" folder in the build directory.  You will need to either make a symlink or copy-paste it into the build directory.  For a symlink:

Linux:
```
ln -s ../assets assets
```

Windows:
```
mklink /D assets ..\assets
```

## Running

You can simply double-click the chess binary or run from console.  On Linux, debug output will be printed to the console.  On Windows, it uses OutputDebugString so you will need to run inside a debugger to view debug output.

Command-line arguments:

-test or -test -verbose: Calculates number of positions from some test FEN strings.  Compares to known good results.  Used for bug testing.

-ai: Has the AI play a game against itself.
