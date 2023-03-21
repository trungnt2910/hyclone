# Hyclone

[![Build](https://github.com/trungnt2910/hyclone/actions/workflows/build.yml/badge.svg)](https://github.com/trungnt2910/hyclone/actions/workflows/build.yml)

Hyclone is a runtime environment for [Haiku](https://github.com/haiku/haiku) applications.

Hyclone currently targets and supports Linux, but should be, with some modifications, portable to any ELF-based SysV OSes (including Haiku itself).

Currently, Hyclone only supports x86_64.

## Build instructions

### Building Haiku objects

Hyclone uses precompiled objects from a Haiku build. Therefore, the first step is to fetch a copy of Haiku sources and build it.

On an Ubuntu machine:

```
sudo apt install -y git nasm bc autoconf automake texinfo flex bison gawk build-essential unzip wget zip less zlib1g-dev libzstd-dev xorriso libtool python3

cd hyclone
./copy_objects.sh
```

`copy_objects.sh` fetches a copy of the Haiku official source code, builds the neccessary targets, and copies the required precompiled object files to the Hyclone source tree.

By default, `copy_objects.sh` stores the cloned source code (`haiku` for the OS, `buildtools` for the Haiku cross-compilers) at Hyclone's parent directory, for example:
```
CodingProjects
|
|--- hyclone
    |
    |--- copy_objects.sh
|--- haiku
|--- buildtools
```

If you want to change the Haiku source and Build tools source locations, or if you are a Haiku developer wishing to use your existing Haiku work environment, you can set these
environment variables to change where `copy_objects.sh` looks for objects:

- `HAIKU_ARCH`: The Haiku build architecture. Only the default value, `x86_64` is currently supported.
- `HAIKU_BUILD_ENVIRONMENT_ROOT`: The root folder of the Haiku build environment. By default, it is the parent directory of the Hyclone source.
- `HAIKU_BUILD_SOURCE_DIRECTORY`: The folder where Haiku's source code should be stored. Defaults to `$HAIKU_BUILD_ENVIRONMENT_ROOT/haiku`.
- `HAIKU_BUILD_TOOLS_DIRECTORY`: The folder where the source code for Haiku's build tools should be stored. Defaults to `$HAIKU_BUILD_ENVIRONMENT_ROOT/buildtools`.
- `HAIKU_BUILD_OUTPUT_ROOT`: The folder where Haiku's build output should be stored. Defaults to `$HAIKU_BUILD_SOURCE_DIRECTORY/generated.$HAIKU_ARCH`.

### Building Hyclone

```
cd hyclone
mkdir build; cd build
cmake ..
# For test builds, do this to install everything to the build directory:
# cmake .. -DCMAKE_INSTALL_PREFIX=.
sudo make install
```

The Hyclone source directory must be placed in the same directory as Haiku, so that it could detect and copy the required object files.

### Running Hyclone

In order to get Haiku apps running (`bash`, `gcc`,...), we need a Haiku installation with basic system directories, libraries, and configuration files.

Hyclone does not come with a way to set up a Haiku installation prefix yet, but you can set up one using [this](https://github.com/jessicah/cross-compiler/blob/main/build-rootfs.sh)
script by **@jessicah**:

```
export HPREFIX=/path/to/haiku/installation/that/you/choose
./build-rootfs.sh x86_64 --rootfsdir $HPREFIX --jobs $(nproc)
```

This can take quite a while.

Then, set up our environment a little bit:

```
# Path to modified libroot is usually at $CMAKE_INSTALL_PREFIX/lib
export LIBRARY_PATH=/SystemRoot/path/to/modified/libroot:/boot/system/lib
# This will be prepended to PATH by haiku_loader
export HPATH=/boot/system/bin:/boot/system/non-packaged/bin

# Set up a few links:
ln -s /dev $HPREFIX/dev

# Copy Haiku-specific bash profiles to $HPREFIX to get a Haiku shell experience
cp -rf /path/to/the/source/code/of/haiku/data/etc $HPREFIX/boot/system
```

Now, assuming that you're at `$CMAKE_INSTALL_PREFIX`, run:

```
cd bin
./hyclone_server
```

This starts the Hyclone kernel server, a process that manages some system states that should have been handled by the Haiku kernel.

After that, you can run Haiku binaries by:

```
./haiku_loader <binary name> <args>
```

A screenshot of Hyclone on WSL1:

![wsl1_hyclone](docs/bashonhaikuonubuntuonwindows.png)


### Notes

- While some basic CLI apps (`bash`, `gcc`,...) may run, most won't work on Hyclone, as this project is still in its early stages.
- The host's root is mounted on Hyclone as `/SystemRoot`. When translating calls from Haiku to the host system, Hyclone maps the Haiku root to `$HPREFIX`, and when translating the results, Hyclone appends `/SystemRoot` to the host's root. 