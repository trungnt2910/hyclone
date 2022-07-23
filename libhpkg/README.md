# libhpkg - Standalone, cross-platform `.hpkg` package reader

This package contains controller, model and helper objects for reading package files for the Haiku project. Pkg files come in two types. HPKR is a file format for providing a kind of catalogue of what is in a repository. HPKG format is a file that describes a particular package. At the time of writing, this library only supports HPKR although there is enough supporting material to easily provide a reader for HPKG.

Note that this library (currently) only supports (signed) 32bit addressing in the HPKR files.

The C++ version has been ported from the .NET [HpkgReader](https://github.com/trungnt2910/HpkgReader) library. It currently lacks many classes from the .NET port, such as the `Pkg` and `BetterPkg` classes, the `HpkrFileExtractor` class, and some more.