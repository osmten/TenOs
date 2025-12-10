// userlib.h - User mode library (include this in user programs)
#ifndef USERLIB_H
#define USERLIB_H

// Syscall numbers (must match kernel!)
#define SYS_EXIT    0
#define SYS_PRINT   1
#define SYS_READ    2
#define SYS_EXEC_CMD 3

// User-facing syscall wrappers

// Exit the program
void exit(int code);

// Print a string
int print(const char *str);

// Read from file descriptor
int read(int fd, char *buf, unsigned int count);

// Execute Command from user shell
int sys_exec_command(const char *cmd);

#endif
