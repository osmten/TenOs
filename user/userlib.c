#include "userlib.h"

// Low-level syscall function
// This does the actual INT 0x80
static inline int syscall0(int num) {
    int ret;
    asm volatile(
        "int $0x80"
        : "=a"(ret)           // Output: EAX -> ret
        : "a"(num)            // Input: num -> EAX
    );
    return ret;
}

static inline int syscall1(int num, unsigned int arg1) {
    int ret;
    asm volatile(
        "int $0x80"
        : "=a"(ret)           // Output: EAX -> ret
        : "a"(num), "b"(arg1) // Input: num -> EAX, arg1 -> EBX
    );
    return ret;
}

static inline int syscall2(int num, unsigned int arg1, unsigned int arg2) {
    int ret;
    asm volatile(
        "int $0x80"
        : "=a"(ret)
        : "a"(num), "b"(arg1), "c"(arg2)
    );
    return ret;
}

static inline int syscall3(int num, unsigned int arg1, unsigned int arg2, unsigned int arg3) {
    int ret;
    asm volatile(
        "int $0x80"
        : "=a"(ret)
        : "a"(num), "b"(arg1), "c"(arg2), "d"(arg3)
    );
    return ret;
}

// User-facing functions

void exit(int code) {
    syscall1(SYS_EXIT, code);
}

int print(const char *str) {
    return syscall1(SYS_PRINT, (unsigned int)str);
}

int read(int fd, char *buf, unsigned int count) {
    return syscall3(SYS_READ, fd, (unsigned int)buf, count);
}

int print_color(const char *str, int color)
{
    return syscall2(SYS_COLOR_PRINT, (unsigned int)str, color);
}

int sys_exec_command(const char *cmd) {
    return syscall1(SYS_EXEC_CMD, (unsigned int)cmd);
}
