# hCraft-2
A custom Minecraft server written in C++, written from the grounds up to
supersede the previous [hCraft](https://github.com/BizarreCake/hCraft).

Protocol Version
----------------

hCraft 2 currently supports 1.8 (protocol version: 47).

Features
--------

This is a new project so there's not a lot implemented yet!
*  Players can connect and walk around.
*  More will implemented soon!

Building
--------

It is possible to build hCraft 2 both on Linux and on Windows.
In both platforms, you will need a working copy of [CMake](http://www.cmake.org/)
and C++11-compatible compiler (Visual Studio 2013+ has been tested on Windows).

## On Linux

Enter source directory and type `cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release .`
followed by `make`. After compilation has ended, the executable could then be
found inside the newlycreated build directory.

## On Windows

Enter source directory using Visual Studio's x64 Native Tools Command Prompt
and type `cmake -G "Visual Studio 12 2013 Win64" -DCMAKE_BUILD_TYPE=Release .`
(Replace Visual Studio 12 2013 with whatever version you have, but don't forget
the Win64!); proceed by opening the generated .sln file, switch to Release mode
within Visual Studio and build the project. The executable should be in
build/Release/ afterwards.

License
-------

hCraft 2 is released under GNU's general public license (GPLv3), more information
can be found [here](http://www.gnu.org/licenses/gpl.html).

