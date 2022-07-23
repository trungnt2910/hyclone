#ifndef __LIBHPKG_HPKGHEADER_H__
#define __LIBHPKG_HPKGHEADER_H__

#include "Heap/HeapCompression.h"

namespace LibHpkg
{
	/// <summary>
	/// Converted from Java source by replacing:
	/// <code>public ([a-z]*) get([A-Za-z_]*)\(\)[\s\n]*{[\s\n]*return ([A-Za-z]*);[\s\n]*}[\s\n]*public void set[A-Za-z_]*\([^\)]*\)[\s\n]*{[\s\n]*.*[\s\n]*\}</code>
	/// with <code>public $1 $2\n{\nget => $3;\nset => $3 = value;\n}</code>
	/// Converted from C# source by removing all properties and making all fields public :)
	/// </summary>
	struct HpkgHeader
	{
		long HeaderSize;
		int Version;
		long TotalSize;
		int MinorVersion;

		// heap
		Heap::HeapCompression HeapCompression;
		long HeapChunkSize;
		long HeapSizeCompressed;
		long HeapSizeUncompressed;

		long PackageAttributesLength;
		long PackageAttributesStringsLength;
		long PackageAttributesStringsCount;

		long TocLength;
		long TocStringsLength;
		long TocStringsCount;
	};
}

#endif // __LIBHPKG_HPKGHEADER_H__