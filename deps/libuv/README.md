# libuv [![Build Status](https://secure.travis-ci.org/joyent/libuv.png)](http://travis-ci.org/joyent/libuv)

libuv is a new platform layer for Node. Its purpose is to abstract IOCP on
Windows and libev on Unix systems. We intend to eventually contain all
platform differences in this library.

http://nodejs.org/

## Features

 * Non-blocking TCP sockets

 * Non-blocking named pipes

 * UDP

 * Timers

 * Child process spawning

 * Asynchronous DNS via `uv_getaddrinfo`.

 * Asynchronous file system APIs `uv_fs_*`

 * High resolution time `uv_hrtime`

 * Current executable path look up `uv_exepath`

 * Thread pool scheduling `uv_queue_work`

 * ANSI escape code controlled TTY `uv_tty_t`

 * File system events Currently supports inotify, `ReadDirectoryChangesW`
   and kqueue. Event ports in the near future.
   `uv_fs_event_t`

 * IPC and socket sharing between processes `uv_write2`

## Community

 * [Mailing list](http://groups.google.com/group/libuv)

## Documentation

 * [include/uv.h](https://github.com/joyent/libuv/blob/master/include/uv.h)
   &mdash; API documentation in the form of detailed header comments.
 * [An Introduction to libuv](http://nikhilm.github.com/uvbook/) &mdash; An
   overview of libuv with tutorials.
 * [LXJS 2012 talk](http://www.youtube.com/watch?v=nGn60vDSxQ4) - High-level
   introductory talk about libuv.
 * [Tests and benchmarks](https://github.com/joyent/libuv/tree/master/test) -
   API specification and usage examples.

## Build Instructions

For GCC (including MinGW) there are two methods building: via normal
makefiles or via GYP. GYP is a meta-build system which can generate MSVS,
Makefile, and XCode backends. It is best used for integration into other
projects.  The old (more stable) system is using Makefiles.

To build via Makefile simply execute:

    make

To build with Visual Studio run the vcbuilds.bat file which will
checkout the GYP code into build/gyp and generate the uv.sln and
related files.

Windows users can also build from cmd-line using msbuild.  This is
done by running vcbuild.bat from Visual Studio command prompt.

To have GYP generate build script for another system you will need to
checkout GYP into the project tree manually:

    svn co http://gyp.googlecode.com/svn/trunk build/gyp

Unix users run

    ./gyp_uv -f make
    make

Macintosh users run

    ./gyp_uv -f xcode
    xcodebuild -project uv.xcodeproj -configuration Release -target All

Note for Linux users: compile your project with `-D_GNU_SOURCE` when you
include `uv.h`. GYP builds take care of that automatically. If you use
autotools, add a `AC_GNU_SOURCE` declaration to your `configure.ac`.

## Supported Platforms

Microsoft Windows operating systems since Windows XP SP2. It can be built
with either Visual Studio or MinGW.

Linux 2.6 using the GCC toolchain.

MacOS using the GCC or XCode toolchain.

Solaris 121 and later using GCC toolchain.
