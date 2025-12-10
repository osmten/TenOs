#ifndef PRINT_H
#define PRINT_H

#include "types.h"
#include <drivers/screen.h>

typedef enum {
    KERN_DEBUG = 0,
    KERN_INFO  = 1,
    KERN_WARN  = 2,
    KERN_ERR = 3,
} log_level_t;

void printk(const char *fmt, ...);
void print_core(int level, const char *module, const char *fmt, ...);

#define pr_debug(mod, ...) printk_core(KERN_DEBUG, mod, __VA_ARGS__)
#define pr_info(mod, ...)  printk_core(KERN_INFO,  mod, __VA_ARGS__)
#define pr_warn(mod, ...)  printk_core(KERN_WARN,  mod, __VA_ARGS__)
#define pr_err(mod, ...)   printk_core(KERN_ERR,   mod, __VA_ARGS__)

#endif
