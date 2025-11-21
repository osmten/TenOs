#ifndef _SHELL_H
#define _SHELL_H

#include <lib/lib.h>

#define ENTER_KEY  0x1C
#define BACK_SPACE 0x0E
#define CMD_BUFFER_SIZE 256

struct shell_cmd {
    char *name;
    char *descr;
};

int shell_init();
int shell_process();
int process_cmd();

#endif