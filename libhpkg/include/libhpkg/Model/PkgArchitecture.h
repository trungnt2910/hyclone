#ifndef __LIBHPKG_MODEL_PKGARCHITECTURE_H__
#define __LIBHPKG_MODEL_PKGARCHITECTURE_H__

namespace LibHpkg::Model
{
    /// <summary>
    /// <para>In the HPKR file format, the architecture is a numerical value.  These numerical
    /// values correlate to this enum.</para>
    /// </summary>
    enum class PkgArchitecture
    {
        ANY,        // 0
        X86,        // 1
        X86_GCC2,   // 2
        SOURCE,     // 3
        X86_64,     // 4
        PPC,        // 5
        ARM,        // 6
        M68K,       // 7
        SPARC,      // 8
        ARM64,      // 9
        RISCV64     // 10
    };
}

#endif // __LIBHPKG_MODEL_PKGARCHITECTURE_H__