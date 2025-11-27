#include"print.h"
#include<stdarg.h>

static int current_level = 0;

static const u8 level_colors[] = {
    VGA_COLOR(VGA_COLOR_BLUE, VGA_COLOR_BLACK),    // DEBUG - Blue on Black
    VGA_COLOR(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK),   // INFO  - Light Grey on Black
    VGA_COLOR(VGA_COLOR_YELLOW, VGA_COLOR_BLACK),       // WARN  - Yellow on Black
    VGA_COLOR(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK)     // ERROR - Light Red on Black
};

void printk_init()
{
    current_level = KERN_INFO;
    pr_info("PRINTK", "Kernel print initialized..");
}

void set_log_level(int level)
{
    current_level = level;
}

void printk_core(int level, const char *module, const char *fmt, ...)
{
    if (level < current_level)
        return;

    if (*module != '\0')
    {
        put_char('[', level_colors[level]);
        while(*module)
        {
            put_char(*module, level_colors[level]);
            module++;
        }
        put_char(']', level_colors[level]);
        put_char(':', level_colors[level]);

    }

    va_list args;
    const char* p = fmt;

    va_start(args, fmt);

    while(*p)
    {
        if(*p  == '%')
        {
            p++;
            switch (*p)
            {
                case 'c': {
                    char value = (char)va_arg(args, int);
                    put_char(value, level_colors[level]);
                    break;
                }
                case 'd':
                case 'i': {
                    int value = va_arg(args, int);
                    put_char(value, level_colors[level]);
                    break;
                }
                // case 'u': {
                //     unsigned int value = va_arg(args, unsigned int);
                //     print_uint(value);
                //     break;
                // }
                // case 'x':
                // case 'X': {
                //     unsigned int value = va_arg(args, unsigned int);
                //     print_hex(value);
                //     break;
                // }
                case 's': {
                    char *str = va_arg(args, char*);
                    while (*str) {
                        put_char(*str++, level_colors[level]);
                    }
                    break;
                }
                case '%': {
                    put_char('%', level_colors[level]);
                    break;
                }
                default:
                    put_char('%', level_colors[level]);
                    put_char(*p, level_colors[level]);
                    break;
            }
        }
         else {
            put_char(*p, level_colors[level]);
        }
        p++;
    }
    va_end(args);
    put_char('\n');
}

void printk(const char * fmt, ...)
{
    va_list args;
    const char* p = fmt;

    va_start(args, fmt);

    while(*p)
    {
        if(*p  == '%')
        {
            p++;
            switch (*p)
            {
                case 'c': {
                    char value = (char)va_arg(args, int);
                    put_char(value, WHITE_ON_BLACK);
                    break;
                }
                case 'd':
                case 'i': {
                    int value = va_arg(args, int);
                    put_char(value, WHITE_ON_BLACK);
                    break;
                }
                // case 'u': {
                //     unsigned int value = va_arg(args, unsigned int);
                //     print_uint(value);
                //     break;
                // }
                // case 'x':
                // case 'X': {
                //     unsigned int value = va_arg(args, unsigned int);
                //     print_hex(value);
                //     break;
                // }
                case 's': {
                    char *str = va_arg(args, char*);
                    while (*str) {
                        put_char(*str++, WHITE_ON_BLACK);
                    }
                    break;
                }
                case '%': {
                    put_char('%', WHITE_ON_BLACK);
                    break;
                }
                default:
                    put_char('%', WHITE_ON_BLACK);
                    put_char(*p, WHITE_ON_BLACK);
                    break;
            }
        }
         else {
            put_char(*p, WHITE_ON_BLACK);
        }
        p++;
    }
    va_end(args);
}