// userlib.h - User mode library (include this in user programs)
#ifndef USERLIB_H
#define USERLIB_H

// Syscall numbers (must match kernel!)
#define SYS_EXIT    0
#define SYS_PRINT   1
#define SYS_READ    2
#define SYS_EXEC_CMD 3
#define SYS_COLOR_PRINT 4

//Screen colors for text
#define VGA_COLOR_BLACK         0
#define VGA_COLOR_BLUE          1
#define VGA_COLOR_GREEN         2
#define VGA_COLOR_CYAN          3
#define VGA_COLOR_RED           4
#define VGA_COLOR_MAGENTA       5
#define VGA_COLOR_BROWN         6
#define VGA_COLOR_LIGHT_GREY    7
#define VGA_COLOR_DARK_GREY     8
#define VGA_COLOR_LIGHT_BLUE    9
#define VGA_COLOR_LIGHT_GREEN   10
#define VGA_COLOR_LIGHT_CYAN    11
#define VGA_COLOR_LIGHT_RED     12
#define VGA_COLOR_LIGHT_MAGENTA 13
#define VGA_COLOR_YELLOW        14
#define VGA_COLOR_WHITE         15

// User-facing syscall wrappers

// Exit the program
void exit(int code);

// Print a string
int print(const char *str);

// Read from file descriptor
int read(int fd, char *buf, unsigned int count);

// Execute Command from user shell
int sys_exec_command(const char *cmd);

// Print a string
int print_color(const char *str, int color);

#endif
