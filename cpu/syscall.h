// cpu/syscall.h
#ifndef SYSCALL_H
#define SYSCALL_H

#include <lib/lib.h>
#include <fs/fs.h>
#include <drivers/keyboard.h>

// Syscall numbers
#define SYS_EXIT    0
#define SYS_PRINT   1
#define SYS_READ    2
#define SYS_EXEC_CMD 3
#define SYS_COLOR_PRINT 4

#define ENTER_KEY  '\n'
#define BACK_SPACE '\b'
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
static void cmd_create_file(const char *args);
static void cmd_cat(const char *args);
static void cmd_meminfo(const char *args);
int process_cmd(const char *buff);

// Syscall handler
u32 syscall_dispatcher(u32 syscall_num, u32 arg1, u32 arg2, u32 arg3, u32 arg4, u32 arg5);

// Individual syscall implementations
u32 sys_exit(u32 code);
u32 sys_print(const char *str, int color);
u32 sys_read(u32 fd, char *buf, u32 count);

#endif