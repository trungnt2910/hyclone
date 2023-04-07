#include <algorithm>
#include <cassert>
#include <cstring>
#include <dlfcn.h>
#include <filesystem>
#include <link.h>
#include <iostream>
#include <vector>

#include "haiku_image.h"
#include "loader_runtime.h"
#include "loader_servercalls.h"
#include "loader_vchroot.h"

runtime_loader_info gRuntimeLoaderInfo;

static bool sRuntimeLoaderLoaded = false;

// On ELF-bases Sys-V systems, a simple dlopen is enough.
bool loader_load_runtime()
{
    if (sRuntimeLoaderLoaded)
    {
        return true;
    }

    auto info = &gRuntimeLoaderInfo;

    auto path = std::filesystem::canonical("/proc/self/exe");
    path = path.parent_path() / "runtime_loader";

    info->handle = dlopen(path.c_str(), RTLD_LAZY);
    if (info->handle == NULL)
    {
        std::cerr << "loader_load_runtime: dlopen failed: " << dlerror() << std::endl;
        return false;
    }
    info->entry_point = (runtime_loader_main_func)dlsym(info->handle, "runtime_loader");
    if (info->entry_point == NULL)
    {
        std::cerr << "loader_load_runtime: failed to find entry point: " << dlerror() << std::endl;
        return false;
    }
    info->gRuntimeLoaderPtr = (rld_export**)dlsym(info->handle, "__gRuntimeLoader");
    if (info->gRuntimeLoaderPtr == NULL)
    {
        std::cerr << "loader_load_runtime: failed to find __gRuntimeLoaderPtr: " << dlerror() << std::endl;
        return false;
    }

    Dl_info dl_info;
    if (!dladdr((const void*)info->entry_point, &dl_info))
    {
        std::cerr << "loader_load_runtime: dladdr failed: " << dlerror() << std::endl;
        return false;
    }

    haiku_extended_image_info haiku_info;
    char hostpath[sizeof(haiku_info.basic_info.name)];
    if (!realpath(dl_info.dli_fname, hostpath))
    {
        std::cerr << "loader_load_runtime: realpath failed: " << strerror(errno) << std::endl;
        return false;
    }

    loader_vchroot_unexpand(hostpath, haiku_info.basic_info.name, sizeof(haiku_info.basic_info.name));

    const ElfW(Ehdr)* header = (const ElfW(Ehdr)*)dl_info.dli_fbase;
    const ElfW(Phdr)* programHeadersPtr = (const ElfW(Phdr)*)((const char*)header + header->e_phoff);

    assert(header->e_phentsize == sizeof(ElfW(Phdr)));
    std::vector<ElfW(Phdr)> phdrs(programHeadersPtr, programHeadersPtr + header->e_phnum);

    std::sort(phdrs.begin(), phdrs.end(), [](const ElfW(Phdr)& a, const ElfW(Phdr)& b)
    {
        if (a.p_type != b.p_type && a.p_type == PT_LOAD)
        {
            return true;
        }
        else if (b.p_type == PT_LOAD)
        {
            return false;
        }
        else
        {
            return a.p_vaddr < b.p_vaddr;
        }
    });

    while (phdrs.back().p_type != PT_LOAD)
    {
        phdrs.pop_back();
    }

    haiku_info.basic_info.text = (void*)header;
    haiku_info.basic_info.text_size = 0;
    haiku_info.basic_info.data = NULL;
    haiku_info.basic_info.data_size = 0;

    assert(phdrs.front().p_vaddr == 0);
    assert(phdrs.front().p_type == PT_LOAD);

    ElfW(Addr) lastAddress = 0;
    size_t i = 0;
    while (i < phdrs.size() && !(phdrs[i].p_flags & PF_W))
    {
        if (phdrs[i].p_vaddr != lastAddress)
        {
            break;
        }
        lastAddress += phdrs[i].p_memsz;
        haiku_info.basic_info.text_size += phdrs[i].p_memsz;
        ++i;
    }

    // Any more non-contiguous text segments?
    while (i < phdrs.size() && !(phdrs[i].p_flags & PF_W))
    {
        ++i;
    }

    if (i < phdrs.size())
    {
        lastAddress = phdrs[i].p_vaddr;
        haiku_info.basic_info.data = (void*)((char*)header + lastAddress);
        haiku_info.basic_info.data_size = phdrs[i].p_memsz;
        ++i;

        while (i < phdrs.size() && (phdrs[i].p_flags & PF_W))
        {
            if (phdrs[i].p_vaddr != lastAddress)
            {
                break;
            }
            lastAddress += phdrs[i].p_memsz;
            haiku_info.basic_info.text_size += phdrs[i].p_memsz;
            ++i;
        }
    }

    status_t status = loader_hserver_call_register_image(&haiku_info, sizeof(haiku_info));
    if (status < 0)
    {
        std::cerr << "loader_load_runtime: failed to register runtime_loader image: " << status << std::endl;
        return false;
    }

    return true;
}

void loader_unload_runtime()
{
    dlclose(gRuntimeLoaderInfo.handle);
    sRuntimeLoaderLoaded = false;
}

void* loader_runtime_symbol(const char* name)
{
    return dlsym(gRuntimeLoaderInfo.handle, name);
}