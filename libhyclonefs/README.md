# libhyclonefs

This file contains `libhyclonefs` - a library which will replace `hyclone_server`'s `packagefs` emulation as well as `haiku_loader`'s `vchroot` mechanism.

Currently, nothing works yet. The library is WIP.

## How it works

`libhyclonefs` uses code from Haiku's portable `fs_shell` to implement a basic VFS subsystem right inside `hyclone_server`. This allows emulating special Haiku-specific filesystems
all from the Hyclone host's userspace.

These file systems are intended to be implemented within `libhyclonefs`:
- `devfs`: Device file system. Only `/dev/random`, `/dev/null`, and `/dev/zero` are intended to be supported in the short term.
- `hostfs`: Host machine's file system. This file system mirrors everthing from Hyclone's host machine.
- `packagefs`: Hyclone's new `packagefs` emulation. This file system also uses `libhpkg` as its backend. However, it does not extract every packages to the disk but lazily extracts files when data is requested.

In the long term, images of any file system supported by Haiku's `fs_shell` utility (for instance, `bfs` with its `bfs_shell`) can be mounted by this library, as `libhyclonefs` exposes the same API as the one used by `fs_shell`.