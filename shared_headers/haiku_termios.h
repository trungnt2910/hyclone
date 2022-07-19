/*
 * Copyright 2004-2012 Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef __HAIKU_TERMIOS_H__
#define __HAIKU_TERMIOS_H__

#include <cstdint>

typedef uint32_t haiku_tcflag_t;
typedef unsigned char haiku_speed_t;
typedef unsigned char haiku_cc_t;

#define HAIKU_NCCS 11 /* number of control characters */

struct haiku_termios
{
    haiku_tcflag_t  c_iflag;    /* input modes */
    haiku_tcflag_t  c_oflag;    /* output modes */
    haiku_tcflag_t  c_cflag;    /* control modes */
    haiku_tcflag_t  c_lflag;    /* local modes */
    char            c_line;     /* line discipline */
    haiku_speed_t   c_ispeed;   /* custom input baudrate */
    haiku_speed_t   c_ospeed;   /* custom output baudrate */
    haiku_cc_t      c_cc[HAIKU_NCCS]; /* control characters */
};

/* control characters */
#define HAIKU_VINTR 0
#define HAIKU_VQUIT 1
#define HAIKU_VERASE 2
#define HAIKU_VKILL 3
#define HAIKU_VEOF 4
#define HAIKU_VEOL 5
#define HAIKU_VMIN 4
#define HAIKU_VTIME 5
#define HAIKU_VEOL2 6
#define HAIKU_VSWTCH 7
#define HAIKU_VSTART 8
#define HAIKU_VSTOP 9
#define HAIKU_VSUSP 10

/* c_iflag - input control modes */
#define HAIKU_IGNBRK 0x01  /* ignore break condition */
#define HAIKU_BRKINT 0x02  /* break sends interrupt */
#define HAIKU_IGNPAR 0x04  /* ignore characters with parity errors */
#define HAIKU_PARMRK 0x08  /* mark parity errors */
#define HAIKU_INPCK 0x10   /* enable input parity checking */
#define HAIKU_ISTRIP 0x20  /* strip high bit from characters */
#define HAIKU_INLCR 0x40   /* maps newline to CR on input */
#define HAIKU_IGNCR 0x80   /* ignore carriage returns */
#define HAIKU_ICRNL 0x100  /* map CR to newline on input */
#define HAIKU_IUCLC 0x200  /* map all upper case to lower */
#define HAIKU_IXON 0x400   /* enable input SW flow control */
#define HAIKU_IXANY 0x800  /* any character will restart input */
#define HAIKU_IXOFF 0x1000 /* enable output SW flow control */

/* c_oflag - output control modes */
#define HAIKU_OPOST 0x01  /* enable postprocessing of output */
#define HAIKU_OLCUC 0x02  /* map lowercase to uppercase */
#define HAIKU_ONLCR 0x04  /* map NL to CR-NL on output */
#define HAIKU_OCRNL 0x08  /* map CR to NL on output */
#define HAIKU_ONOCR 0x10  /* no CR output when at column 0 */
#define HAIKU_ONLRET 0x20 /* newline performs CR function */
#define HAIKU_OFILL 0x40  /* use fill characters for delays */
#define HAIKU_OFDEL 0x80  /* Fills are DEL, otherwise NUL */
#define HAIKU_NLDLY 0x100 /* Newline delays: */
#define HAIKU_NL0 0x000
#define HAIKU_NL1 0x100
#define HAIKU_CRDLY 0x600 /* Carriage return delays: */
#define HAIKU_CR0 0x000
#define HAIKU_CR1 0x200
#define HAIKU_CR2 0x400
#define HAIKU_CR3 0x600
#define HAIKU_TABDLY 0x1800 /* Tab delays: */
#define HAIKU_TAB0 0x0000
#define HAIKU_TAB1 0x0800
#define HAIKU_TAB2 0x1000
#define HAIKU_TAB3 0x1800
#define HAIKU_BSDLY 0x2000 /* Backspace delays: */
#define HAIKU_BS0 0x0000
#define HAIKU_BS1 0x2000
#define HAIKU_VTDLY 0x4000 /* Vertical tab delays: */
#define HAIKU_VT0 0x0000
#define HAIKU_VT1 0x4000
#define HAIKU_FFDLY 0x8000 /* Form feed delays: */
#define HAIKU_FF0 0x0000
#define HAIKU_FF1 0x8000

/* c_cflag - control modes */
#define HAIKU_CBAUD 0x1F /* line speed definitions */

#define HAIKU_B0 0x00  /* hang up */
#define HAIKU_B50 0x01 /* 50 baud */
#define HAIKU_B75 0x02
#define HAIKU_B110 0x03
#define HAIKU_B134 0x04
#define HAIKU_B150 0x05
#define HAIKU_B200 0x06
#define HAIKU_B300 0x07
#define HAIKU_B600 0x08
#define HAIKU_B1200 0x09
#define HAIKU_B1800 0x0A
#define HAIKU_B2400 0x0B
#define HAIKU_B4800 0x0C
#define HAIKU_B9600 0x0D
#define HAIKU_B19200 0x0E
#define HAIKU_B38400 0x0F
#define HAIKU_B57600 0x10
#define HAIKU_B115200 0x11
#define HAIKU_B230400 0x12
#define HAIKU_B31250 0x13 /* for MIDI */

#define HAIKU_CSIZE 0x20 /* character size */
#define HAIKU_CS5 0x00   /* only 7 and 8 bits supported */
#define HAIKU_CS6 0x00   /* Note, it was not very wise to set all of these */
#define HAIKU_CS7 0x00   /* to zero, but there is not much we can do about it*/
#define HAIKU_CS8 0x20
#define HAIKU_CSTOPB 0x40    /* send 2 stop bits, not 1 */
#define HAIKU_CREAD 0x80     /* enable receiver */
#define HAIKU_PARENB 0x100   /* parity enable */
#define HAIKU_PARODD 0x200   /* odd parity, else even */
#define HAIKU_HUPCL 0x400    /* hangs up on last close */
#define HAIKU_CLOCAL 0x800   /* indicates local line */
#define HAIKU_XLOBLK 0x1000  /* block layer output ?*/
#define HAIKU_CTSFLOW 0x2000 /* enable CTS flow */
#define HAIKU_RTSFLOW 0x4000 /* enable RTS flow */
#define HAIKU_CRTSCTS (HAIKU_RTSFLOW | HAIKU_CTSFLOW)

/* c_lflag - local modes */
#define HAIKU_ISIG 0x01    /* enable signals */
#define HAIKU_ICANON 0x02  /* Canonical input */
#define HAIKU_XCASE 0x04   /* Canonical u/l case */
#define HAIKU_ECHO 0x08    /* Enable echo */
#define HAIKU_ECHOE 0x10   /* Echo erase as bs-sp-bs */
#define HAIKU_ECHOK 0x20   /* Echo nl after kill */
#define HAIKU_ECHONL 0x40  /* Echo nl */
#define HAIKU_NOFLSH 0x80  /* Disable flush after int or quit */
#define HAIKU_TOSTOP 0x100 /* stop bg processes that write to tty */
#define HAIKU_IEXTEN 0x200 /* implementation defined extensions */
#define HAIKU_ECHOCTL 0x400
#define HAIKU_ECHOPRT 0x800
#define HAIKU_ECHOKE 0x1000
#define HAIKU_FLUSHO 0x2000
#define HAIKU_PENDIN 0x4000

/* options to tcsetattr() */
#define HAIKU_TCSANOW 0x01   /* make change immediate */
#define HAIKU_TCSADRAIN 0x02 /* drain output, then change */
#define HAIKU_TCSAFLUSH 0x04 /* drain output, flush input */

/* actions for tcflow() */
#define HAIKU_TCOOFF 0x01 /* suspend output */
#define HAIKU_TCOON 0x02  /* restart output */
#define HAIKU_TCIOFF 0x04 /* transmit STOP character, intended to stop input data */
#define HAIKU_TCION 0x08  /* transmit START character, intended to resume input data */

/* values for tcflush() */
#define HAIKU_TCIFLUSH 0x01  /* flush pending input */
#define HAIKU_TCOFLUSH 0x02  /* flush untransmitted output */
#define HAIKU_TCIOFLUSH 0x03 /* flush both */

/* ioctl() identifiers to control the TTY */
#define HAIKU_TCGETA 0x8000
#define HAIKU_TCSETA (HAIKU_TCGETA + 1)
#define HAIKU_TCSETAF (HAIKU_TCGETA + 2)
#define HAIKU_TCSETAW (HAIKU_TCGETA + 3)
#define HAIKU_TCWAITEVENT (HAIKU_TCGETA + 4)
#define HAIKU_TCSBRK (HAIKU_TCGETA + 5)
#define HAIKU_TCFLSH (HAIKU_TCGETA + 6)
#define HAIKU_TCXONC (HAIKU_TCGETA + 7)
#define HAIKU_TCQUERYCONNECTED (HAIKU_TCGETA + 8)
#define HAIKU_TCGETBITS (HAIKU_TCGETA + 9)
#define HAIKU_TCSETDTR (HAIKU_TCGETA + 10)
#define HAIKU_TCSETRTS (HAIKU_TCGETA + 11)
#define HAIKU_TIOCGWINSZ (HAIKU_TCGETA + 12) /* pass in a struct winsize */
#define HAIKU_TIOCSWINSZ (HAIKU_TCGETA + 13) /* pass in a struct winsize */
#define HAIKU_TCVTIME (HAIKU_TCGETA + 14)    /* pass in bigtime_t, old value saved */
#define HAIKU_TIOCGPGRP (HAIKU_TCGETA + 15)  /* Gets the process group ID of the TTY device */
#define HAIKU_TIOCSPGRP (HAIKU_TCGETA + 16)  /* Sets the process group ID ('pgid' in BeOS) */
#define HAIKU_TIOCSCTTY (HAIKU_TCGETA + 17)  /* Become controlling TTY */
#define HAIKU_TIOCMGET (HAIKU_TCGETA + 18)   /* get line state, like TCGETBITS */
#define HAIKU_TIOCMSET (HAIKU_TCGETA + 19)   /* does TCSETDTR/TCSETRTS */
#define HAIKU_TIOCSBRK (HAIKU_TCGETA + 20)   /* set txd pin */
#define HAIKU_TIOCCBRK (HAIKU_TCGETA + 21)   /* both are a frontend to TCSBRK */
#define HAIKU_TIOCMBIS (HAIKU_TCGETA + 22)   /* set bits in line state */
#define HAIKU_TIOCMBIC (HAIKU_TCGETA + 23)   /* clear bits in line state */
#define HAIKU_TIOCGSID (HAIKU_TCGETA + 24)   /* get session leader process group ID */

/* Event codes.  Returned from TCWAITEVENT */
#define HAIKU_EV_RING           0x0001
#define HAIKU_EV_BREAK          0x0002
#define HAIKU_EV_CARRIER        0x0004
#define HAIKU_EV_CARRIERLOST    0x0008

/* for TIOCGWINSZ */
struct haiku_winsize
{
    unsigned short ws_row;
    unsigned short ws_col;
    unsigned short ws_xpixel;
    unsigned short ws_ypixel;
};

/* Bits for the TCGETBITS control */
#define HAIKU_TCGB_CTS  0x01
#define HAIKU_TCGB_DSR  0x02
#define HAIKU_TCGB_RI   0x04
#define HAIKU_TCGB_DCD  0x08

/* Bits for the TIOCMGET / TIOCMSET control */
#define HAIKU_TIOCM_CTS     HAIKU_TCGB_CTS  /* clear to send */
#define HAIKU_TIOCM_CD      HAIKU_TCGB_DCD  /* carrier detect */
#define HAIKU_TIOCM_CAR     HAIKU_TIOCM_CD
#define HAIKU_TIOCM_RI      HAIKU_TCGB_RI   /* ring indicator */
#define HAIKU_TIOCM_DSR     HAIKU_TCGB_DSR  /* dataset ready */
#define HAIKU_TIOCM_DTR     0x10            /* data terminal ready */
#define HAIKU_TIOCM_RTS     0x20            /* request to send */

#endif /* __HAIKU_TERMIOS_H__ */