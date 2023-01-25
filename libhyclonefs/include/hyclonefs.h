#ifndef __LIBHYCLONEFS_HYCLONEFS_H__
#define __LIBHYCLONEFS_HYCLONEFS_H__

extern "C"
{
    struct HycloneFSArgs
    {
        const char* HyclonePrefix;
        const char* MountPoint;
    };


    int hyclonefs_system_init(const HycloneFSArgs* args);
}

#endif // __LIBHYCLONEFS_HYCLONEFS_H__