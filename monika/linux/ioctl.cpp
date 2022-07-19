// Use this instead of the POSIX termios for setting custom baud.
#include <asm/termios.h>
#include <cstddef>
#include <cstring>

#include "BeDefs.h"
#include "errno_conversion.h"
#include "export.h"
#include "haiku_errors.h"
#include "haiku_ioctl.h"
#include "linux_debug.h"
#include "linux_syscall.h"

typedef struct ktermios linux_termios;

static void TermiosLinuxToB(const linux_termios& linuxTermios, haiku_termios& haikuTermios);
static void TermiosBToLinux(const haiku_termios& haikuTermios, linux_termios& linuxTermios);

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
        case HAIKU_TIOCGWINSZ:
        {
            struct winsize linux_winsize;
            int result = LINUX_SYSCALL3(__NR_ioctl, fd, TIOCGWINSZ, &linux_winsize);
            if (result < 0)
            {
                return LinuxToB(-result);
            }
            if (buffer == NULL || length != sizeof(struct haiku_winsize))
            {
                return B_BAD_VALUE;
            }
            struct haiku_winsize& haiku_winsize = *(struct haiku_winsize*)buffer;
            haiku_winsize.ws_row = linux_winsize.ws_row;
            haiku_winsize.ws_col = linux_winsize.ws_col;
            haiku_winsize.ws_xpixel = linux_winsize.ws_xpixel;
            haiku_winsize.ws_ypixel = linux_winsize.ws_ypixel;
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
        STUB_IOCTL(FIONBIO);
        STUB_IOCTL(FIONREAD);
        STUB_IOCTL(FIOSEEKDATA);
        STUB_IOCTL(FIOSEEKHOLE);
        // STUB_IOCTL(TCGETA);
        STUB_IOCTL(TCSETA);
        STUB_IOCTL(TCSETAF);
        //STUB_IOCTL(TCSETAW);
        STUB_IOCTL(TCWAITEVENT);
        STUB_IOCTL(TCSBRK);
        STUB_IOCTL(TCFLSH);
        STUB_IOCTL(TCXONC);
        STUB_IOCTL(TCQUERYCONNECTED);
        STUB_IOCTL(TCGETBITS);
        STUB_IOCTL(TCSETDTR);
        STUB_IOCTL(TCSETRTS);
        // STUB_IOCTL(TIOCGWINSZ);
        STUB_IOCTL(TIOCSWINSZ);
        STUB_IOCTL(TCVTIME);
        // STUB_IOCTL(TIOCGPGRP);
        //STUB_IOCTL(TIOCSPGRP);
        STUB_IOCTL(TIOCSCTTY);
        STUB_IOCTL(TIOCMGET);
        STUB_IOCTL(TIOCMSET);
        STUB_IOCTL(TIOCSBRK);
        STUB_IOCTL(TIOCCBRK);
        STUB_IOCTL(TIOCMBIS);
        STUB_IOCTL(TIOCMBIC);
        STUB_IOCTL(TIOCGSID);
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
}
