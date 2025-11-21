#ifndef _SHELL_H
#define _SHELL_H

#include <lib/lib.h>
#include <drivers/screen.h>

#define ENTER_KEY  0x1C
#define BACK_SPACE 0x0E
#define CMD_BUFFER_SIZE 256

// Command handler type
typedef void (*cmd_handler_t)(const char *args);

// Command structure
typedef struct {
    const char *name;
    const char *description;
    cmd_handler_t handler;
} command_t;

static void cmd_help(const char *args);
static void cmd_clear(const char *args);
static void cmd_echo(const char *args);
static void cmd_ls(const char *args);
static void cmd_cat(const char *args);
static void cmd_meminfo(const char *args);
int shell_init();
int shell_process();
int process_cmd();

#endif