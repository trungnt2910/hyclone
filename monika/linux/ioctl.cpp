// Use this instead of the POSIX termios for setting custom baud.
#include <asm/termios.h>
#include <algorithm>
#include <cstddef>
#include <cstring>

#include "BeDefs.h"
#include "errno_conversion.h"
#include "export.h"
#include "extended_commpage.h"
#include "haiku_errors.h"
#include "haiku_ioctl.h"
#include "linux_debug.h"
#include "linux_syscall.h"

typedef struct ktermios linux_termios;

static void TermiosLinuxToB(const linux_termios& linuxTermios, haiku_termios& haikuTermios);
static void TermiosBToLinux(const haiku_termios& haikuTermios, linux_termios& linuxTermios);

enum
{
    B_GET_DEVICE_SIZE = 1,          /* get # bytes - returns size_t in *data */
    B_SET_DEVICE_SIZE,              /* set # bytes - passes size_t in *data */
    B_SET_NONBLOCKING_IO,           /* set to non-blocking i/o */
    B_SET_BLOCKING_IO,              /* set to blocking i/o */
    B_GET_READ_STATUS,              /* check if can read w/o blocking */
                                    /* returns bool in *data */
    B_GET_WRITE_STATUS,             /* check if can write w/o blocking */
                                    /* returns bool in *data */
    B_GET_GEOMETRY,                 /* get info about device geometry */
                                    /* returns struct geometry in *data */
    B_GET_DRIVER_FOR_DEVICE,        /* get the path of the executable serving */
                                    /* that device */
    B_GET_PARTITION_INFO,           /* get info about a device partition */
                                    /* returns struct partition_info in *data */
    B_SET_PARTITION,                /* obsolete, will be removed */
    B_FORMAT_DEVICE,                /* low-level device format */
    B_EJECT_DEVICE,                 /* eject the media if supported */
    B_GET_ICON,                     /* return device icon (see struct below) */
    B_GET_BIOS_GEOMETRY,            /* get info about device geometry */
                                        /* as reported by the bios */
                                        /*   returns struct geometry in *data */
    B_GET_MEDIA_STATUS,             /* get status of media. */
                                        /* return status_t in *data: */
                                        /* B_NO_ERROR: media ready */
                                        /* B_DEV_NO_MEDIA: no media */
                                        /* B_DEV_NOT_READY: device not ready */
                                        /* B_DEV_MEDIA_CHANGED: media changed */
                                        /*  since open or last B_GET_MEDIA_STATUS */
                                        /* B_DEV_MEDIA_CHANGE_REQUESTED: user */
                                        /*  pressed button on drive */
                                        /* B_DEV_DOOR_OPEN: door open */
    B_LOAD_MEDIA,                   /* load the media if supported */
    B_GET_BIOS_DRIVE_ID,            /* get bios id for this device */
    B_SET_UNINTERRUPTABLE_IO,       /* prevent cntl-C from interrupting i/o */
    B_SET_INTERRUPTABLE_IO,         /* allow cntl-C to interrupt i/o */
    B_FLUSH_DRIVE_CACHE,            /* flush drive cache */
    B_GET_PATH_FOR_DEVICE,          /* get the absolute path of the device */
    B_GET_ICON_NAME,                /* get an icon name identifier */
    B_GET_VECTOR_ICON,              /* retrieves the device's vector icon */
    B_GET_DEVICE_NAME,              /* get name, string buffer */
    B_TRIM_DEVICE,                  /* trims blocks, see fs_trim_data */

    B_GET_NEXT_OPEN_DEVICE = 1000,  /* obsolete, will be removed */
    B_ADD_FIXED_DRIVER,             /* obsolete, will be removed */
    B_REMOVE_FIXED_DRIVER,          /* obsolete, will be removed */

    B_AUDIO_DRIVER_BASE = 8000,     /* base for codes in audio_driver.h */
    B_MIDI_DRIVER_BASE = 8100,      /* base for codes in midi_driver.h */
    B_JOYSTICK_DRIVER_BASE = 8200,  /* base for codes in joystick.h */
    B_GRAPHIC_DRIVER_BASE = 8300,   /* base for codes in graphic_driver.h */

    B_DEVICE_OP_CODES_END = 9999    /* end of Be-defined control ids */
};

extern "C"
{

status_t MONIKA_EXPORT _kern_ioctl(int fd, uint32 op, void* buffer, size_t length)
{
#define STUB_IOCTL(name)                     \
    case HAIKU_##name:                       \
        trace("Unimplemented ioctl: "#name); \
        break;

    switch (op)
    {
        case HAIKU_TCGETA:
        {
            // struct termio and struct termios are basically the same.
            // Haiku does not support struct termio but does not support TCGETS.
            // Using TCGETA on WSL1 returns ENOTTY, so we use TCGETS instead.
            linux_termios linux_termios;
            int result = LINUX_SYSCALL3(__NR_ioctl, fd, TCGETS, &linux_termios);
            if (result < 0)
            {
                return LinuxToB(-result);
            }
            struct haiku_termios* haiku_termios = (struct haiku_termios*)buffer;
            TermiosLinuxToB(linux_termios, *haiku_termios);
            return B_OK;
        }
        case HAIKU_TCSETAW:
        {
            const struct haiku_termios* haiku_termios = (const struct haiku_termios*)buffer;
            linux_termios linux_termios;
            TermiosBToLinux(*haiku_termios, linux_termios);
            int result = LINUX_SYSCALL3(__NR_ioctl, fd, TCSETSW, &linux_termios);
            if (result < 0)
            {
                return LinuxToB(-result);
            }
            return B_OK;
        }
        case HAIKU_TCSETAF:
        {
            const struct haiku_termios* haiku_termios = (const struct haiku_termios*)buffer;
            linux_termios linux_termios;
            TermiosBToLinux(*haiku_termios, linux_termios);
            int result = LINUX_SYSCALL3(__NR_ioctl, fd, TCSETSF, &linux_termios);
            if (result < 0)
            {
                return LinuxToB(-result);
            }
            return B_OK;
        }
        case HAIKU_TCXONC:
        {
            int arg = *(int*)buffer;
            int linuxArg;
            switch (arg)
            {
                case HAIKU_TCOOFF:
                    linuxArg = TCIOFF;
                    break;
                case HAIKU_TCOON:
                    linuxArg = TCOON;
                    break;
                case HAIKU_TCIOFF:
                    linuxArg = TCIOFF;
                    break;
                case HAIKU_TCION:
                    linuxArg = TCION;
                    break;
                default:
                    return B_BAD_VALUE;
            }
            int result = LINUX_SYSCALL3(__NR_ioctl, fd, TCXONC, linuxArg);
            if (result < 0)
            {
                return LinuxToB(-result);
            }
            return B_OK;
        }
        case HAIKU_TIOCGWINSZ:
        {
            struct winsize linux_winsize;
            int result = LINUX_SYSCALL3(__NR_ioctl, fd, TIOCGWINSZ, &linux_winsize);
            if (result < 0)
            {
                return LinuxToB(-result);
            }
            // The length parameter seems to always be 0 in this case.
            // If it faults, it faults. We can't check the length of the
            // struct passed here.
            if (buffer == NULL)
            {
                return B_BAD_VALUE;
            }
            struct haiku_winsize* haiku_winsize = (struct haiku_winsize*)buffer;
            haiku_winsize->ws_row = linux_winsize.ws_row;
            haiku_winsize->ws_col = linux_winsize.ws_col;
            haiku_winsize->ws_xpixel = linux_winsize.ws_xpixel;
            haiku_winsize->ws_ypixel = linux_winsize.ws_ypixel;
            return B_OK;
        }
        case HAIKU_TIOCSWINSZ:
        {
            struct winsize linux_winsize;
            if (buffer == NULL)
            {
                return B_BAD_VALUE;
            }
            struct haiku_winsize* haiku_winsize = (struct haiku_winsize*)buffer;
            linux_winsize.ws_row = haiku_winsize->ws_row;
            linux_winsize.ws_col = haiku_winsize->ws_col;
            linux_winsize.ws_xpixel = haiku_winsize->ws_xpixel;
            linux_winsize.ws_ypixel = haiku_winsize->ws_ypixel;
            int result = LINUX_SYSCALL3(__NR_ioctl, fd, TIOCSWINSZ, &linux_winsize);
            if (result < 0)
            {
                return LinuxToB(-result);
            }
            return B_OK;
        }
        case HAIKU_TIOCGPGRP:
        {
            int result = LINUX_SYSCALL3(__NR_ioctl, fd, TIOCGPGRP, buffer);
            if (result < 0)
            {
                return LinuxToB(-result);
            }
            return B_OK;
        }
        case HAIKU_TIOCSPGRP:
        {
            int result = LINUX_SYSCALL3(__NR_ioctl, fd, TIOCSPGRP, buffer);
            if (result < 0)
            {
                return LinuxToB(-result);
            }
            return B_OK;
        }
        case B_GET_PATH_FOR_DEVICE:
        {
            char* path = (char*)buffer;
            long realLength =
                GET_HOSTCALLS()->vchroot_unexpandat(fd, "", path, length);

            if (realLength > length)
            {
                return B_BUFFER_OVERFLOW;
            }

            if (realLength < 0)
            {
                return B_BAD_VALUE;
            }

            if (realLength > 1 && path[realLength - 1] == '/')
            {
                path[realLength - 1] = '\0';
                --realLength;
            }

            return realLength;
        }
        STUB_IOCTL(FIONBIO);
        STUB_IOCTL(FIONREAD);
        STUB_IOCTL(FIOSEEKDATA);
        STUB_IOCTL(FIOSEEKHOLE);
        //STUB_IOCTL(TCGETA);
        STUB_IOCTL(TCSETA);
        //STUB_IOCTL(TCSETAF);
        //STUB_IOCTL(TCSETAW);
        STUB_IOCTL(TCWAITEVENT);
        STUB_IOCTL(TCSBRK);
        STUB_IOCTL(TCFLSH);
        //STUB_IOCTL(TCXONC);
        STUB_IOCTL(TCQUERYCONNECTED);
        STUB_IOCTL(TCGETBITS);
        STUB_IOCTL(TCSETDTR);
        STUB_IOCTL(TCSETRTS);
        //STUB_IOCTL(TIOCGWINSZ);
        //STUB_IOCTL(TIOCSWINSZ);
        STUB_IOCTL(TCVTIME);
        //STUB_IOCTL(TIOCGPGRP);
        //STUB_IOCTL(TIOCSPGRP);
        STUB_IOCTL(TIOCSCTTY);
        STUB_IOCTL(TIOCMGET);
        STUB_IOCTL(TIOCMSET);
        STUB_IOCTL(TIOCSBRK);
        STUB_IOCTL(TIOCCBRK);
        STUB_IOCTL(TIOCMBIS);
        STUB_IOCTL(TIOCMBIC);
        STUB_IOCTL(TIOCGSID);
        default:
            // Trace this, until we have better tools
            // such as strace on Hyclone.
            trace("Unknown ioctl.");
            return B_BAD_VALUE;
    }

    return B_OK;
}

}

#if !defined(VSWTCH) && defined(VSWTC)
#define VSWTCH VSWTC
#endif

static void TermiosLinuxToB(const linux_termios& linuxTermios, haiku_termios& haikuTermios)
{
    // c_iflag
    haikuTermios.c_iflag = 0;
#define SUPPORTED_IFLAG(name)                   \
    if (linuxTermios.c_iflag & name)            \
        haikuTermios.c_iflag |= HAIKU_##name;
#include "ioctl_values.h"
#undef SUPPORTED_IFLAG

    // c_oflag
    haikuTermios.c_oflag = 0;
#define SUPPORTED_OFLAG(name)                   \
    if (linuxTermios.c_oflag & name)            \
        haikuTermios.c_oflag |= HAIKU_##name;
#include "ioctl_values.h"
#undef SUPPORTED_OFLAG

    // c_cflag
    haikuTermios.c_cflag = 0;
#define SUPPORTED_CFLAG(name)                   \
    if (linuxTermios.c_cflag & name)            \
        haikuTermios.c_cflag |= HAIKU_##name;
#include "ioctl_values.h"
#undef SUPPORTED_CFLAG

    // c_lflag
    haikuTermios.c_lflag = 0;
#define SUPPORTED_LFLAG(name)                   \
    if (linuxTermios.c_lflag & name)            \
        haikuTermios.c_lflag |= HAIKU_##name;
#include "ioctl_values.h"
#undef SUPPORTED_LFLAG

    haikuTermios.c_line = linuxTermios.c_line;

    // c_cc
#define SUPPORTED_CC(name)                      \
    haikuTermios.c_cc[HAIKU_##name] = linuxTermios.c_cc[name];
#include "ioctl_values.h"
#undef SUPPORTED_CC

    // c_ispeed
    haikuTermios.c_ispeed = linuxTermios.c_ispeed;
    // c_ospeed
    haikuTermios.c_ospeed = linuxTermios.c_ospeed;

    if ((linuxTermios.c_cflag & BOTHER) && linuxTermios.c_ispeed == 31250 && linuxTermios.c_ospeed == 31250)
    {
        haikuTermios.c_cflag |= HAIKU_B31250;
    }
}

static void TermiosBToLinux(const haiku_termios& haikuTermios, linux_termios& linuxTermios)
{
    // c_iflag
    linuxTermios.c_iflag = 0;
#define SUPPORTED_IFLAG(name)                   \
    if (haikuTermios.c_iflag & HAIKU_##name)    \
        linuxTermios.c_iflag |= name;
#include "ioctl_values.h"
#undef SUPPORTED_IFLAG

    // c_oflag
    linuxTermios.c_oflag = 0;
#define SUPPORTED_OFLAG(name)                   \
    if (haikuTermios.c_oflag & HAIKU_##name)    \
        linuxTermios.c_oflag |= name;
#include "ioctl_values.h"
#undef SUPPORTED_OFLAG

    // c_cflag
    linuxTermios.c_cflag = 0;
#define SUPPORTED_CFLAG(name)                   \
    if (haikuTermios.c_cflag & HAIKU_##name)    \
        linuxTermios.c_cflag |= name;
#define UNSUPPORTED_CFLAG(name)                 \
    if (haikuTermios.c_cflag & HAIKU_##name)    \
        trace("Unsupported c_cflag: "#name);
#include "ioctl_values.h"
#undef SUPPORTED_CFLAG
#undef UNSUPPORTED_CFLAG

    // c_lflag
    linuxTermios.c_lflag = 0;
#define SUPPORTED_LFLAG(name)                   \
    if (haikuTermios.c_lflag & HAIKU_##name)    \
        linuxTermios.c_lflag |= name;
#include "ioctl_values.h"
#undef SUPPORTED_LFLAG

    linuxTermios.c_line = haikuTermios.c_line;

    // c_cc
#define SUPPORTED_CC(name)                      \
    linuxTermios.c_cc[name] = haikuTermios.c_cc[HAIKU_##name];
#include "ioctl_values.h"
#undef SUPPORTED_CC

    // c_ispeed
    linuxTermios.c_ispeed = haikuTermios.c_ispeed;
    // c_ospeed
    linuxTermios.c_ospeed = haikuTermios.c_ospeed;

    if (haikuTermios.c_cflag & HAIKU_B31250)
    {
        linuxTermios.c_cflag &= ~CBAUD;
        linuxTermios.c_cflag |= BOTHER;

        linuxTermios.c_ispeed = 31250;
        linuxTermios.c_ospeed = 31250;
    }

    // Linux has no separate RTS and CTS flow control.
    if ((haikuTermios.c_cflag & HAIKU_RTSFLOW) &&
        !(haikuTermios.c_cflag & HAIKU_CTSFLOW))
    {
        trace("Unsupported c_cflag: RTSFLOW without CTSFLOW");
    }

    if ((haikuTermios.c_cflag & HAIKU_CTSFLOW) &&
        !(haikuTermios.c_cflag & HAIKU_RTSFLOW))
    {
        trace("Unsupported c_cflag: CTSFLOW without RTSFLOW");
    }
}
