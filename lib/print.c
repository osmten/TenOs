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

static void vformat_print(const char *fmt, va_list args, u8 color) {
    const char *p = fmt;
    
    while (*p) {
        if (*p == '%') {
            p++;
            
            switch (*p) {
                case 'c': {
                    char value = (char)va_arg(args, int);
                    put_char(value, color);
                    break;
                }
                case 'x': {
                    unsigned int value = va_arg(args, unsigned int);
                    print_hex(value, color);
                    break;
                }
                case 'd':
                case 'i': {
                    int value = va_arg(args, int);
                    print_int(value, color);
                    break;
                }
                case 's': {
                    char *str = va_arg(args, char*);
                    while (*str) {
                        put_char(*str++, color);
                    }
                    break;
                }
                case 'u': {
                    unsigned int value = va_arg(args, unsigned int);
                    print_uint(value, color);
                    break;
                }
                case '%': {
                    put_char('%', color);
                    break;
                }
                default:
                    put_char('%', color);
                    put_char(*p, color);
                    break;
            }
        } else {
            put_char(*p, color);
        }
        p++;
    }
}

void printk_core(int level, const char *module, const char *fmt, ...) {
    if (level < current_level)
        return;
    
    u8 color = level_colors[level];
    
    if (*module != '\0') {
        put_char('[', color);
        while (*module) {
            put_char(*module++, color);
        }
        put_char(']', color);
        put_char(':', color);
    }
    
    va_list args;
    va_start(args, fmt);
    vformat_print(fmt, args, color);
    va_end(args);
    
    put_char('\n', color);
}

void printk(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vformat_print(fmt, args, WHITE_ON_BLACK);
    va_end(args);
}

void printk_color(int attr, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vformat_print(fmt, args, attr);
    va_end(args);
}


void print_int(int value, int color)
{
    if (value == 0) {
        put_char('0', color);
        return;
    }
    
    if (value < 0) {
        put_char('-', color);
        value = -value;
    }
    
    char num[12] = {'\0'};
    int i = 0;
    while(value > 0 && i < 11)
    {
        num[i] = (value % 10) + '0';
        i++;
        value /= 10;
    }
    reverse(num);
    
    char *p = num;
    while (*p) {
        put_char(*p++, color);
    }
}

void print_hex(unsigned int value, int color)
{
    char *hex_map = "0123456789ABCDEF";
    char hex[9] = {'\0'};
    int i = 0;

    put_char('0', color);
    put_char('x', color);
    
    if (value == 0) {
        put_char('0', color);
        return;
    }
    
    while(value && i < 8)
    {
        hex[i++] = hex_map[value & 0xF];
        value >>= 4;
    }
    reverse(hex);
    
    char *p = hex;
    while (*p) put_char(*p++, color);
}

void print_uint(unsigned int value, int color)
{
    if (value == 0) {
        put_char('0', color);
        return;
    }
    
    char num[11] = {'\0'};
    int i = 0;
    while(value > 0 && i < 10)
    {
        num[i++] = (value % 10) + '0';
        value /= 10;
    }
    reverse(num);
    
    char *p = num;
    while (*p) put_char(*p++, color);
}