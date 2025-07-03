/*
 * BSD 2-Clause License
 *
 * Purpose: Declarations for terminal attribute helpers.
 */
#ifndef TERMIOS_H
#define TERMIOS_H

#include <sys/types.h>

/* types used by struct termios */
typedef unsigned int tcflag_t;
typedef unsigned char cc_t;
typedef unsigned int speed_t;

/* size of the c_cc array */
#ifndef NCCS
#define NCCS 20
#endif

struct termios {
    tcflag_t c_iflag;
    tcflag_t c_oflag;
    tcflag_t c_cflag;
    tcflag_t c_lflag;
    cc_t c_cc[NCCS];
    speed_t c_ispeed;
    speed_t c_ospeed;
};

/* input modes */
#define IGNBRK  0x00000001
#define BRKINT  0x00000002
#define IGNPAR  0x00000004
#define PARMRK  0x00000008
#define INPCK   0x00000010
#define ISTRIP  0x00000020
#define INLCR   0x00000040
#define IGNCR   0x00000080
#define ICRNL   0x00000100
#define IXON    0x00000200
#define IXOFF   0x00000400

/* output modes */
#define OPOST   0x00000001

/* control modes */
#define CSIZE   0x00000300
#define   CS5   0x00000000
#define   CS6   0x00000100
#define   CS7   0x00000200
#define   CS8   0x00000300
#define CSTOPB  0x00000400
#define CREAD   0x00000800
#define PARENB  0x00001000
#define PARODD  0x00002000
#define HUPCL   0x00004000
#define CLOCAL  0x00008000

/* local modes */
#define ECHO    0x00000008
#define ECHONL  0x00000010
#define ICANON  0x00000100
#define IEXTEN  0x00000400
#define ISIG    0x00000080

/* control characters */
#define VMIN    16
#define VTIME   17

/* tcsetattr actions */
#define TCSANOW   0
#define TCSADRAIN 1
#define TCSAFLUSH 2

/* tcflow actions */
#define TCOOFF    0
#define TCOON     1
#define TCIOFF    2
#define TCION     3

/* tcflush queue selectors */
#define TCIFLUSH  0
#define TCOFLUSH  1
#define TCIOFLUSH 2

int tcgetattr(int fd, struct termios *t);
int tcsetattr(int fd, int act, const struct termios *t);
void cfmakeraw(struct termios *t);
int tcdrain(int fd);
int tcflow(int fd, int act);
int tcflush(int fd, int qs);
int tcsendbreak(int fd, int dur);
int cfsetispeed(struct termios *t, speed_t sp);
int cfsetospeed(struct termios *t, speed_t sp);
speed_t cfgetispeed(const struct termios *t);
speed_t cfgetospeed(const struct termios *t);

#endif /* TERMIOS_H */
