// Use this instead of the POSIX termios for setting custom baud.
#include <asm/termios.h>
#include <algorithm>
#include <cstddef>
#include <cstring>
#include <net/if.h>
#include <sys/mman.h>

// Internal header, but we have to use it to prevent clashing with
// names from <asm/termios.h>
#define _SYS_IOCTL_H
#include <bits/ioctls.h>

// Annoying macros that breaks Haiku headers
#undef ifr_name
#undef ifr_addr
#undef ifr_dstaddr
#undef ifr_broadaddr
#undef ifr_flags
#undef ifr_metric
#undef ifr_mtu
#undef ifr_data
#undef ifc_buf
#undef ifc_req

#include "BeDefs.h"
#include "errno_conversion.h"
#include "extended_commpage.h"
#include "haiku_drivers.h"
#include "haiku_errors.h"
#include "haiku_ioctl.h"
#include "haiku_net_if.h"
#include "haiku_sockio.h"
#include "linux_debug.h"
#include "linux_syscall.h"
#include "socket_conversion.h"
#include "stringutils.h"

#define B_IOCTL_GET_TTY_INDEX (HAIKU_TCGETA + 32) /* param is int32* */
#define B_IOCTL_GRANT_TTY     (HAIKU_TCGETA + 33) /* no param (cf. grantpt()) */

typedef struct ktermios linux_termios;

static void TermiosLinuxToB(const linux_termios& linuxTermios, haiku_termios& haikuTermios);
static void TermiosBToLinux(const haiku_termios& haikuTermios, linux_termios& linuxTermios);
static int InterfaceFlagsLinuxToB(int flags);

extern "C"
{

status_t _moni_ioctl(int fd, uint32 op, void* buffer, size_t length)
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
            linux_termios linuxTermios;
            int result = LINUX_SYSCALL3(__NR_ioctl, fd, TCGETS, &linuxTermios);
            if (result < 0)
            {
                return LinuxToB(-result);
            }
            struct haiku_termios* haikuTermios = (struct haiku_termios*)buffer;
            TermiosLinuxToB(linuxTermios, *haikuTermios);
            return B_OK;
        }
        case HAIKU_TCSETA:
        {
            const struct haiku_termios* haikuTermios = (const struct haiku_termios*)buffer;
            linux_termios linuxTermios;
            TermiosBToLinux(*haikuTermios, linuxTermios);
            int result = LINUX_SYSCALL3(__NR_ioctl, fd, TCSETS, &linuxTermios);
            if (result < 0)
            {
                return LinuxToB(-result);
            }
            return B_OK;
        }
        case HAIKU_TCSETAW:
        {
            const struct haiku_termios* haikuTermios = (const struct haiku_termios*)buffer;
            linux_termios linuxTermios;
            TermiosBToLinux(*haikuTermios, linuxTermios);
            int result = LINUX_SYSCALL3(__NR_ioctl, fd, TCSETSW, &linuxTermios);
            if (result < 0)
            {
                return LinuxToB(-result);
            }
            return B_OK;
        }
        case HAIKU_TCSETAF:
        {
            const struct haiku_termios* haikuTermios = (const struct haiku_termios*)buffer;
            linux_termios linuxTermios;
            TermiosBToLinux(*haikuTermios, linuxTermios);
            int result = LINUX_SYSCALL3(__NR_ioctl, fd, TCSETSF, &linuxTermios);
            if (result < 0)
            {
                return LinuxToB(-result);
            }
            return B_OK;
        }
        case HAIKU_TCXONC:
        {
            int arg = (int)(intptr_t)buffer;
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
        case HAIKU_SIOCGIFDSTADDR:
        case HAIKU_SIOCGIFFLAGS:
        case HAIKU_SIOCGIFNETMASK:
        {
            if (buffer == NULL)
            {
                return B_BAD_ADDRESS;
            }
            struct haiku_ifreq* haiku_ifreq = (struct haiku_ifreq*)buffer;
            struct ifreq linux_ifreq;
            strncpy(linux_ifreq.ifr_ifrn.ifrn_name, haiku_ifreq->ifr_name, std::min(IFNAMSIZ, HAIKU_IFNAMSIZ));

            int linuxOp;
            switch (op)
            {
                case HAIKU_SIOCGIFDSTADDR:
                    linuxOp = SIOCGIFDSTADDR;
                    break;
                case HAIKU_SIOCGIFFLAGS:
                    linuxOp = SIOCGIFFLAGS;
                    break;
                case HAIKU_SIOCGIFNETMASK:
                    linuxOp = SIOCGIFNETMASK;
                    break;
            }

            int result = LINUX_SYSCALL3(__NR_ioctl, fd, linuxOp, &linux_ifreq);
            if (result < 0)
            {
                return LinuxToB(-result);
            }

            switch (op)
            {
                case HAIKU_SIOCGIFDSTADDR:
                    SocketAddressLinuxToB((sockaddr*)&linux_ifreq.ifr_ifru.ifru_dstaddr,
                        (haiku_sockaddr_storage*)&haiku_ifreq->ifr_dstaddr);
                break;
                case HAIKU_SIOCGIFFLAGS:
                    haiku_ifreq->ifr_flags = InterfaceFlagsLinuxToB(linux_ifreq.ifr_ifru.ifru_flags);
                break;
                case HAIKU_SIOCGIFNETMASK:
                    SocketAddressLinuxToB((sockaddr*)&linux_ifreq.ifr_ifru.ifru_netmask,
                        (haiku_sockaddr_storage*)&haiku_ifreq->ifr_mask);
                break;
            }

            return B_OK;
        }
        case HAIKU_SIOCGIFCOUNT:
        {
            if (buffer == NULL)
            {
                return B_BAD_ADDRESS;
            }
            struct haiku_ifconf* haiku_ifconf = (struct haiku_ifconf*)buffer;
            if (haiku_ifconf->ifc_len < (int)sizeof(int))
            {
                return B_BAD_VALUE;
            }
            struct ifconf linux_ifconf;
            linux_ifconf.ifc_ifcu.ifcu_buf = NULL;
            int result = LINUX_SYSCALL3(__NR_ioctl, fd, SIOCGIFCONF, &linux_ifconf);
            if (result < 0)
            {
                return LinuxToB(-result);
            }
            haiku_ifconf->ifc_value = linux_ifconf.ifc_len / sizeof(struct ifreq);
            return B_OK;
        }
        case HAIKU_SIOCGIFCONF:
        {
            if (buffer == NULL)
            {
                return B_BAD_ADDRESS;
            }
            struct haiku_ifconf* haiku_ifconf = (struct haiku_ifconf*)buffer;
            struct ifconf linux_ifconf;

            size_t count = haiku_ifconf->ifc_len / sizeof(struct haiku_ifreq);
            size_t bufferSize = (count * sizeof(struct ifreq) + B_PAGE_SIZE - 1) & ~(B_PAGE_SIZE - 1);
            long result = LINUX_SYSCALL6(__NR_mmap, NULL, bufferSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
            if (result < 0)
            {
                return LinuxToB(-result);
            }

            linux_ifconf.ifc_len = count * sizeof(struct ifreq);
            linux_ifconf.ifc_ifcu.ifcu_buf = (char*)result;

            result = LINUX_SYSCALL3(__NR_ioctl, fd, SIOCGIFCONF, &linux_ifconf);
            if (result < 0)
            {
                LINUX_SYSCALL2(__NR_munmap, (void*)linux_ifconf.ifc_ifcu.ifcu_buf, bufferSize);
                return LinuxToB(-result);
            }

            for (size_t i = 0; i < count; ++i)
            {
                struct haiku_ifreq& haiku_ifreq = haiku_ifconf->ifc_req[i];
                struct ifreq& linux_ifreq = linux_ifconf.ifc_ifcu.ifcu_req[i];

                strncpy(haiku_ifreq.ifr_name, linux_ifreq.ifr_ifrn.ifrn_name, std::min(IFNAMSIZ, HAIKU_IFNAMSIZ));

                struct sockaddr_in* haiku_addr = (struct sockaddr_in*)&haiku_ifreq.ifr_addr;
                struct sockaddr_in* linux_addr = (struct sockaddr_in*)&linux_ifreq.ifr_ifru.ifru_addr;

                SocketAddressLinuxToB((sockaddr*)linux_addr, (haiku_sockaddr_storage*)haiku_addr);
            }

            LINUX_SYSCALL2(__NR_munmap, (void*)linux_ifconf.ifc_ifcu.ifcu_buf, bufferSize);
            return B_OK;
        }
        case B_IOCTL_GET_TTY_INDEX:
        {
            long result = LINUX_SYSCALL3(__NR_ioctl, fd, TIOCGPTN, buffer);
            if (result < 0)
            {
                return LinuxToB(-result);
            }
            return B_OK;
        }
        case B_IOCTL_GRANT_TTY:
        {
            // This is what musl's grantpt() does.
            // https://elixir.bootlin.com/musl/latest/source/src/misc/pty.c#L15
            return 0;
        }
        case B_GET_PATH_FOR_DEVICE:
        {
            char* path = (char*)buffer;
            long realLength =
                GET_HOSTCALLS()->vchroot_unexpandat(fd, "", path, length);

            if (realLength < 0)
            {
                return B_BAD_VALUE;
            }

            if ((size_t)realLength > length)
            {
                return B_BUFFER_OVERFLOW;
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
        //STUB_IOCTL(TCSETA);
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
        {
            // These are driver-specific ioctls.
            if (op > B_DEVICE_OP_CODES_END)
            {
                return GET_SERVERCALLS()->ioctl(fd, op, buffer, length);
            }
            // Trace this, until we have better tools
            // such as strace on Hyclone.
            GET_HOSTCALLS()->printf("Unknown ioctl: %d\n", op);
            trace("Unknown ioctl.");
            return B_BAD_VALUE;
        }
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

static int InterfaceFlagsLinuxToB(int linuxFlags)
{
    int haikuFlags = 0;

    if (linuxFlags & IFF_UP)
        haikuFlags |= HAIKU_IFF_UP;
    if (linuxFlags & IFF_BROADCAST)
        haikuFlags |= HAIKU_IFF_BROADCAST;
    if (linuxFlags & IFF_LOOPBACK)
        haikuFlags |= HAIKU_IFF_LOOPBACK;
    if (linuxFlags & IFF_POINTOPOINT)
        haikuFlags |= HAIKU_IFF_POINTOPOINT;
    if (linuxFlags & IFF_NOARP)
        haikuFlags |= HAIKU_IFF_NOARP;
    if (linuxFlags & IFF_PROMISC)
        haikuFlags |= HAIKU_IFF_PROMISC;
    if (linuxFlags & IFF_ALLMULTI)
        haikuFlags |= HAIKU_IFF_ALLMULTI;
    if (linuxFlags & IFF_MULTICAST)
        haikuFlags |= HAIKU_IFF_MULTICAST;

    return haikuFlags;
}