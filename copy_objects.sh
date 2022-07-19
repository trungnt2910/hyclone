#!/bin/bash
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

HAIKU_ARCH=${HAIKU_ARCH:-"x86_64"}
HAIKU_BUILD_OUTPUT_ROOT=${HAIKU_BUILD_OUTPUT_ROOT:-"$SCRIPT_DIR/../haiku/generated.$HAIKU_ARCH"}
HAIKU_RUNTIME_LOADER="$HAIKU_BUILD_OUTPUT_ROOT/objects/haiku/$HAIKU_ARCH/release/system/runtime_loader"
HAIKU_RUNTIME_LOADER_ARCH="$HAIKU_RUNTIME_LOADER/arch/$HAIKU_ARCH"
HAIKU_LIBROOT="$HAIKU_BUILD_OUTPUT_ROOT/objects/haiku/$HAIKU_ARCH/release/system/libroot"
HAIKU_SYSLIBS=$(echo $HAIKU_BUILD_OUTPUT_ROOT/build_packages/gcc_syslibs-*$HAIKU_ARCH)"/lib"
HAIKU_SYSLIBS_DEVEL=$(echo $HAIKU_BUILD_OUTPUT_ROOT/build_packages/gcc_syslibs_devel*$HAIKU_ARCH)"/develop/lib"
HAIKU_GLUE="$HAIKU_BUILD_OUTPUT_ROOT/objects/haiku/$HAIKU_ARCH/release/system/glue"
HAIKU_GLUE_ARCH="$HAIKU_GLUE/arch/$HAIKU_ARCH"
HAIKU_GLUE_GCC=$(echo $HAIKU_BUILD_OUTPUT_ROOT/cross-tools-$HAIKU_ARCH/lib/gcc/$HAIKU_ARCH-unknown-haiku/**)

# Some useless objects may be copied, ignore them.
# They'll be handled by CMake.

mkdir -p $SCRIPT_DIR/libroot/os/arch/$HAIKU_ARCH

cp -fv $HAIKU_RUNTIME_LOADER_ARCH/*.a $SCRIPT_DIR/runtime_loader
cp -fv $HAIKU_RUNTIME_LOADER/*.a $SCRIPT_DIR/runtime_loader
cp -fv $HAIKU_RUNTIME_LOADER/*.o $SCRIPT_DIR/runtime_loader
cp -fv $HAIKU_SYSLIBS_DEVEL/*-kernel.a $SCRIPT_DIR/runtime_loader
cp -fv $HAIKU_LIBROOT/posix/posix*.o $SCRIPT_DIR/libroot/posix
cp -fv $HAIKU_LIBROOT/posix/**/posix*.o $SCRIPT_DIR/libroot/posix
cp -fv $HAIKU_LIBROOT/posix/glibc/**/posix*.o $SCRIPT_DIR/libroot/posix
cp -fv $HAIKU_LIBROOT/posix/arch/$HAIKU_ARCH/posix*.o $SCRIPT_DIR/libroot/posix
cp -fv $HAIKU_LIBROOT/posix/**/arch/$HAIKU_ARCH/posix*.o $SCRIPT_DIR/libroot/posix
cp -fv $HAIKU_LIBROOT/os/arch/$HAIKU_ARCH/*.o $SCRIPT_DIR/libroot/os/arch/$HAIKU_ARCH
cp -fv $HAIKU_LIBROOT/os/*.o $SCRIPT_DIR/libroot/os
cp -fv $HAIKU_LIBROOT/libroot_init.o $SCRIPT_DIR/libroot
cp -fv $HAIKU_SYSLIBS_DEVEL/*.a $SCRIPT_DIR/libroot
cp -fv $HAIKU_SYSLIBS/libgcc_s.so.1 $SCRIPT_DIR/libroot
cp -fv $HAIKU_GLUE/init_term_dyn.o $SCRIPT_DIR/libroot
cp -fv $HAIKU_GLUE/start_dyn.o $SCRIPT_DIR/libroot
cp -fv $HAIKU_GLUE_ARCH/crti.o $SCRIPT_DIR/libroot
cp -fv $HAIKU_GLUE_ARCH/crtn.o $SCRIPT_DIR/libroot
cp -fv $HAIKU_GLUE_GCC/crtbeginS.o $SCRIPT_DIR/libroot
cp -fv $HAIKU_GLUE_GCC/crtendS.o $SCRIPT_DIR/libroot