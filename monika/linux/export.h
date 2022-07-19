#ifndef __MONIKA_EXPORT_H__
#define __MONIKA_EXPORT_H__

#if __GNUC__ >= 4
#define MONIKA_EXPORT __attribute__((visibility("default")))
#else
#define MONIKA_EXPORT
#endif

#endif // __MONIKA_EXPORT_H__