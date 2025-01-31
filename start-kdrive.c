/* xinit wrapper that searces for keyboard and mouse event devices and starts Xfbdev */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define SIZE_MAX 4096

#define STR_(x) #x
#define STR(x) STR_(x)

#define XSERVER "/usr/bin/Xfbdev"

#define DEV1POS 6
#define DEV2POS 8

#define EVDEV_ "evdev,,device=/dev/input/event"
#define EVDEV EVDEV_ "xxx"
#define EVDEVPOS (sizeof(EVDEV_) - 1)

#define PROC_DEVICES "/proc/bus/input/devices"

char mouseevdev[] = EVDEV;
char kbdevdev[] = EVDEV;

#define MOUSE_SET (mouseevdev[EVDEVPOS] != 'x')
#define KBD_SET (kbdevdev[EVDEVPOS] != 'x')

int main(int argc, char **argv)
{
    if (argc >= 2) {
        *argv = "xinit";
        execvp("xinit", argv);
    }

    FILE *f = fopen(PROC_DEVICES, "r");
    if (!f) {
        fprintf(stderr, "Could not open " PROC_DEVICES "\n"
                        "Is /proc supported?\n");
        return 0;
    }

    char p[SIZE_MAX + 1];
    char error;
    while (fscanf(f, "%" STR(SIZE_MAX) "[^\n]", p) != EOF && (!MOUSE_SET || !KBD_SET)) {
        /* read extra '\n' */
        if (fscanf(f, "%c", &error) == EOF) {
            return 0; /* should never happen */
        }
        char *ptr = strstr(p, "Handlers");
        if (!ptr) {
            continue;
        }
        if (!KBD_SET && strstr(ptr, "sysrq") && strstr(ptr, "kbd")) {
            char *q = strstr(ptr, "event");
            if (q) {
                q += sizeof("event") - 1;
                char *dst = kbdevdev + EVDEVPOS;
                while ('0' <= *q && *q <= '9') {
                    *dst = *q;
                    dst++;
                    q++;
                }
                *dst = '\0';
            }
        }
        if (!MOUSE_SET && strstr(ptr, "mouse")) {
            char *q = strstr(ptr, "event");
            if (q) {
                q += sizeof("event") - 1;
                char *dst = mouseevdev + EVDEVPOS;
                while ('0' <= *q && *q <= '9') {
                    *dst = *q;
                    dst++;
                    q++;
                }
                *dst = '\0';
            }
        }
    }

    error = (MOUSE_SET << 1) | KBD_SET;

    switch (error) {
    case 0:
        fprintf(stderr, "Could not find any event devices\n"
                        "Is CONFIG_INPUT_EVDEV enabled in the kernel?\n");
        argv = (char*[]){"xinit", "--", XSERVER, ":1", "vt8", NULL};
        execvp("xinit", argv);
    return 0;
    case 1:
        fprintf(stderr, "Could not find mouse event device\n"
                        "Is a mouse plugged in?\n");
        argv = (char*[]){"xinit", "--", XSERVER, ":1", "vt8", "-keybd", kbdevdev, NULL};
        execvp("xinit", argv);
    return 0;
    case 2:
        fprintf(stderr, "Could not find keyboard event device\n"
                        "Is a keyboard plugged in?\n");
        argv = (char*[]){"xinit", "--", XSERVER, ":1", "vt8", "-mouse", mouseevdev, NULL};
        execvp("xinit", argv);
    return 0;
    case 3:
        argv = (char*[]){"xinit", "--", XSERVER, ":1", "vt8", "-mouse", mouseevdev, "-keybd", kbdevdev, NULL};
        execvp("xinit", argv);
    return 0;
    }

    /* never reached */
    return 0;
}
