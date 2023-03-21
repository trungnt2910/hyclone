/*-*- mode:c;indent-tabs-mode:nil;c-basic-offset:2;tab-width:8;coding:utf-8 -*-│
│vi: set net ft=c ts=2 sts=2 sw=2 fenc=utf-8                                :vi│
╞══════════════════════════════════════════════════════════════════════════════╡
│ Copyright 2022 Justine Alexandra Roberts Tunney                              │
│                                                                              │
│ Permission to use, copy, modify, and/or distribute this software for         │
│ any purpose with or without fee is hereby granted, provided that the         │
│ above copyright notice and this permission notice appear in all copies.      │
│                                                                              │
│ THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL                │
│ WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED                │
│ WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE             │
│ AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL         │
│ DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR        │
│ PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER               │
│ TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR             │
│ PERFORMANCE OF THIS SOFTWARE.                                                │
╚─────────────────────────────────────────────────────────────────────────────*/
#include <algorithm>
#include <cinttypes>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "loader_elf.h"
#include "loader_sysinfo.h"

static inline long CheckedAdd(int64_t x, int64_t y, int64_t *z) {
#if (defined(__GNUC__) && __GNUC__ >= 5 && !defined(__ICC)) || \
    __has_builtin(__builtin_add_overflow)
  if (!__builtin_add_overflow(x, y, z)) {
    return 0;
  }
#else
  uint64_t a, b, c;
  a = x, b = y, c = a + b;
  if (!(((c ^ a) & (c ^ b)) >> 63)) {
    return *z = c, 0;
  }
#endif
  return -1;
}

static inline long CheckedSub(int64_t x, int64_t y, int64_t *z) {
#if (defined(__GNUC__) && __GNUC__ >= 5 && !defined(__ICC)) || \
    __has_builtin(__builtin_sub_overflow)
  if (!__builtin_sub_overflow(x, y, z)) {
    return 0;
  }
#else
  uint64_t a, b, c;
  a = x, b = y, c = a - b;
  if (!(((c ^ a) & (a ^ b)) >> 63)) {
    return *z = c, 0;
  }
#endif
  return -1;
}

int64_t GetElfMemorySize(const Elf64_Ehdr_ *ehdr, size_t size, int64_t *base) {
  size_t off;
  unsigned i;
  int64_t x, y, lo, hi, res;
  const Elf64_Phdr_ *phdr;
  lo = INT64_MAX;
  hi = INT64_MIN;
  if (Read64(ehdr->phoff) < size) {
    for (i = 0; i < Read16(ehdr->phnum); ++i) {
      off = Read64(ehdr->phoff) + Read16(ehdr->phentsize) * i;
      if (off + Read16(ehdr->phentsize) > size) return -1;
      phdr = (const Elf64_Phdr_ *)((const uint8_t *)ehdr + off);
      if (Read32(phdr->type) == PT_LOAD_) {
        x = Read64(phdr->vaddr);
        if (CheckedAdd(x, Read64(phdr->memsz), &y) == -1) return -1;
        lo = std::min(x, lo);
        hi = std::max(y, hi);
      }
    }
  }
  lo &= get_page_size();
  if (CheckedSub(hi, lo, &res) == -1) return -1;
  *base = lo;
  return res;
}

void CheckElfAddress(const Elf64_Ehdr_ *elf, size_t mapsize, uintptr_t addr,
                     size_t addrsize) {
  if (addr < (uintptr_t)elf || addr + addrsize > (uintptr_t)elf + mapsize) {
    exit(202);
  }
}

char *GetElfString(const Elf64_Ehdr_ *elf, size_t mapsize, const char *strtab,
                   uint32_t rva) {
  uintptr_t addr = (uintptr_t)strtab + rva;
  CheckElfAddress(elf, mapsize, addr, 0);
  CheckElfAddress(elf, mapsize, addr,
                  strnlen((char *)addr, (uintptr_t)elf + mapsize - addr) + 1);
  return (char *)addr;
}

Elf64_Phdr_ *GetElfSegmentHeaderAddress(const Elf64_Ehdr_ *elf, size_t mapsize,
                                        uint64_t i) {
  uintptr_t addr = ((uintptr_t)elf + (uintptr_t)Read64(elf->phoff) +
                    (uintptr_t)Read16(elf->phentsize) * i);
  CheckElfAddress(elf, mapsize, addr, Read16(elf->phentsize));
  return (Elf64_Phdr_ *)addr;
}

void *GetElfSectionAddress(const Elf64_Ehdr_ *elf, size_t mapsize,
                           const Elf64_Shdr_ *shdr) {
  uintptr_t addr, size;
  addr = (uintptr_t)elf + (uintptr_t)Read64(shdr->offset);
  size = (uintptr_t)Read64(shdr->size);
  CheckElfAddress(elf, mapsize, addr, size);
  return (void *)addr;
}

char *GetElfSectionNameStringTable(const Elf64_Ehdr_ *elf, size_t mapsize) {
  if (!Read64(elf->shoff) || !Read16(elf->shentsize)) return NULL;
  return (char *)GetElfSectionAddress(
      elf, mapsize,
      GetElfSectionHeaderAddress(elf, mapsize, Read16(elf->shstrndx)));
}

const char *GetElfSectionName(const Elf64_Ehdr_ *elf, size_t mapsize,
                              Elf64_Shdr_ *shdr) {
  if (!elf || !shdr) return NULL;
  return GetElfString(elf, mapsize, GetElfSectionNameStringTable(elf, mapsize),
                      Read32(shdr->name));
}

Elf64_Shdr_ *GetElfSectionHeaderAddress(const Elf64_Ehdr_ *elf, size_t mapsize,
                                        uint16_t i) {
  uintptr_t addr;
  addr = ((uintptr_t)elf + (uintptr_t)Read64(elf->shoff) +
          (uintptr_t)Read16(elf->shentsize) * i);
  CheckElfAddress(elf, mapsize, addr, Read16(elf->shentsize));
  return (Elf64_Shdr_ *)addr;
}

static char *GetElfStringTableImpl(const Elf64_Ehdr_ *elf, size_t mapsize,
                                   const char *kind) {
  int i;
  const char *name;
  Elf64_Shdr_ *shdr;
  for (i = 0; i < Read16(elf->shnum); ++i) {
    shdr = GetElfSectionHeaderAddress(elf, mapsize, i);
    if (Read32(shdr->type) == SHT_STRTAB_) {
      name = GetElfSectionName(elf, mapsize,
                               GetElfSectionHeaderAddress(elf, mapsize, i));
      if (name && !strcmp(name, kind)) {
        return (char *)GetElfSectionAddress(elf, mapsize, shdr);
      }
    }
  }
  return 0;
}

char *GetElfStringTable(const Elf64_Ehdr_ *elf, size_t mapsize) {
  char *res;
  if (!(res = GetElfStringTableImpl(elf, mapsize, ".strtab"))) {
    res = GetElfStringTableImpl(elf, mapsize, ".dynstr");
  }
  return res;
}

static Elf64_Sym_ *GetElfSymbolTableImpl(const Elf64_Ehdr_ *elf, size_t mapsize,
                                         int *out_count, uint32_t kind) {
  int i;
  Elf64_Shdr_ *shdr;
  for (i = Read16(elf->shnum); i > 0; --i) {
    shdr = GetElfSectionHeaderAddress(elf, mapsize, i - 1);
    if (Read32(shdr->type) == kind) {
      if (Read64(shdr->entsize) != sizeof(Elf64_Sym_)) continue;
      if (out_count) {
        *out_count = Read64(shdr->size) / Read64(shdr->entsize);
      }
      return (Elf64_Sym_ *)GetElfSectionAddress(elf, mapsize, shdr);
    }
  }
  return 0;
}

Elf64_Sym_ *GetElfSymbolTable(const Elf64_Ehdr_ *elf, size_t mapsize,
                              int *out_count) {
  Elf64_Sym_ *res;
  if (!(res = GetElfSymbolTableImpl(elf, mapsize, out_count, SHT_SYMTAB_))) {
    res = GetElfSymbolTableImpl(elf, mapsize, out_count, SHT_DYNSYM_);
  }
  return res;
}
