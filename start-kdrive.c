/* xinit wrapper that searces for keyboard and mouse event devices */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include "config.h"

#define EVDEV_ "evdev,,device=/dev/input/event"
#define EVDEV EVDEV_ "xxx"
#define EVDEVPOS (sizeof(EVDEV_) - 1)

#define PROC_DEVICES "/proc/bus/input/devices"

static char mouseevdev[] = EVDEV;
static char kbdevdev[] = EVDEV;

#define MOUSE_SET (mouseevdev[EVDEVPOS] != 'x')
#define KBD_SET (kbdevdev[EVDEVPOS] != 'x')

#define KBD_EV (0x120013)

enum {
    EVDEV_NONE = 0,
    EVDEV_KEYBOARD = 1 << 0,
    EVDEV_MOUSE = 1 << 1,
};

#define EVDEV_STRCPY(dst, src) \
                              do { \
                                  while ('0' <= *src && *src <= '9') { \
                                      *dst = *src; \
                                      dst++; \
                                      src++; \
                                  } \
                                  *dst = '\0'; \
                              } while (0); \

static int fd;

static char BUF[SIZE_MAX + 1];
static char* buf = BUF;

static int _getline(char *out)
{
    int n = read(fd, buf, SIZE_MAX + BUF - buf);
    if (n >= 0) {
        buf[n] = '\0';
    }
    if (n < 0 || (n == 0 && buf == BUF)) {
        return EOF;
    }
    char *p = strchr(BUF, '\n');
    if (!p) {
        fprintf(stderr, "SIZE_MAX too low\n");
        return EOF;
    }
    memcpy(out, BUF, p - BUF);
    out[p - BUF] = '\0';
    int len = strlen(p + 1);
    memmove(BUF, p + 1, len + 1);
    buf = BUF + len;
    return 0;
}

static int read_evdev(char *evdev)
{
    char p[SIZE_MAX + 1];
    char ignore_kbd = 0;
    char ignore_mouse = 0;
    for (;;) {
        if (_getline(p) == EOF) { /* read all evdev devices */
            return EOF;
        }
        if (*p == '\0') { /* finished reading the evdev device but found nothing */
            return EVDEV_NONE;
        }
        if (*p == 'N') { /* N: Name="..." */
            int i;
            for (i = 0; ignore_keyoards[0] && i < sizeof(ignore_keyoards)/sizeof(*ignore_keyoards); i++) {
                if (ignore_keyoards[i] && strstr(p, ignore_keyoards[i])) {
                    ignore_kbd = 1;
                    break;
                }
            }
            for (i = 0; ignore_mice[0] && i < sizeof(ignore_mice)/sizeof(*ignore_mice); i++) {
                if (ignore_mice[i] && strstr(p, ignore_mice[i])) {
                    ignore_mouse = 1;
                    break;
                }
            }
        }
        if (*p == 'H') { /* H: Handlers=... */
            char *q = strstr(p, "event");
            if (q) {
                q += sizeof("event") - 1;
                EVDEV_STRCPY(evdev, q);
            }
            if (strstr(p, "mouse")) {
                return ignore_mouse ? EVDEV_NONE : EVDEV_MOUSE;
            }
            continue;
        }
        char *ptr = strstr(p, "EV=");
        if (ptr && ((strtol(ptr + sizeof("EV=") - 1, NULL, 16) & KBD_EV) == KBD_EV)) {
            return ignore_kbd ? EVDEV_NONE : EVDEV_KEYBOARD;
        }
    }
    /* never reached */
    return EOF;
}

int main(int argc, char **argv)
{

    fd = open(PROC_DEVICES, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "Could not open " PROC_DEVICES "\n"
                        "Is /proc supported?\n");
        return 0;
    }

    char evdev[sizeof("xxx") - 1];
    int evdev_type=EVDEV_NONE;

    while ((evdev_type != EOF) && (!MOUSE_SET || !KBD_SET)) {
        evdev_type = read_evdev(evdev);
        char *src = evdev;
        char *dst;
        switch (evdev_type & (~KBD_SET) & (~(MOUSE_SET << 1))) {
        case EVDEV_KEYBOARD:
            dst = kbdevdev + EVDEVPOS;
            EVDEV_STRCPY(dst, src);
        break;
        case EVDEV_MOUSE:
            dst = mouseevdev + EVDEVPOS;
            EVDEV_STRCPY(dst, src);
        break;
        }
    }

    /* free()'d at return from main() */
    char **av = malloc((argc + 1) * sizeof(char*) + sizeof((char*[]){"xinit", "--", "-mouse", mouseevdev, "-keybd", kbdevdev, NULL}) + sizeof(char*));
    av[0] = "xinit";
    argc = argc ? argc - 1 : 0;
    memcpy(av + 1, argv + 1, argc * sizeof(*av));
    argv = av + 1 + argc;

    switch (KBD_SET | (MOUSE_SET << 1)) {
    case 0:
        fprintf(stderr, "Could not find any event devices\n"
                        "Is CONFIG_INPUT_EVDEV enabled in the kernel?\n");
        argv[0] = NULL;
        execvp("xinit", av);
    return 0;
    case 1:
        fprintf(stderr, "Could not find mouse event device\n"
                        "Is a mouse plugged in?\n");
        argv[0] = "-keybd";
        argv[1] = kbdevdev;
        argv[2] = NULL;
        execvp("xinit", av);
    return 0;
    case 2:
        fprintf(stderr, "Could not find keyboard event device\n"
                        "Is a keyboard plugged in?\n");
        argv[0] = "-mouse";
        argv[1] = mouseevdev;
        argv[2] = NULL;
        execvp("xinit", av);
    return 0;
    case 3:
        argv[0] = "-mouse";
        argv[1] = mouseevdev;
        argv[2] = "-keybd";
        argv[3] = kbdevdev;
        argv[4] = NULL;
        execvp("xinit", av);
    return 0;
    }

    /* never reached */
    return 0;
}
