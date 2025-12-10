#include "syscall.h"

static command_t commands[] = {
    {"help",    "show all the commands",           cmd_help},
    {"clear",   "Clear screen",                    cmd_clear},
    {"echo",    "Echo text",                       cmd_echo},
    {"create",  "Create a new file",               cmd_create_file},
    {"ls",      "List files",               cmd_help},
    {"cat",     "Display file contents",    cmd_cat},
    {"meminfo", "Show memory info",         cmd_help},
    {NULL, NULL, NULL}  // Terminator
};


static void cmd_help(const char* args)
{
    kprint("\nAvailable Commands \n");

    for (int i = 0; commands[i].name != NULL; i++)
    {
        kprint(" Name ->  ");
        kprint(commands[i].name);
        kprint("        ");
        kprint("Description -> ");
        kprint(commands[i].description);
        kprint("\n");
    }
}

static void cmd_clear(const char* args)
{
    clear_screen();
}

static void cmd_echo(const char *args)
{
    kprint("\n");
    kprint(args);
    kprint("\n");
}

static void cmd_create_file(const char *args)
{
    kprint("\n");
    char name[12] = {0};
    int i = 0;
    FAT12_File* fp;

    while(*args != ' ')
    {
        name[i] = *args;
        i++;
        args++;
    }
    args++; /*Incrementing the space*/

    char buff[256] = {0};
    i = 0;
    while (*args != '\0')
    {
        buff[i] = *args;
        args++;
        i++;
    }

    fat12_create(name, 256); /*hardcoded for now*/
    fp = fat12_open(name);
    fat12_write(fp, buff, 256);

    kprint("\n File Created\n");
}

static void cmd_cat(const char *args)
{
    FAT12_File* fp;
    char name[12] = {0};
    char buff[256] = {0};
    int i = 0;

    while(*args != ' ')
    {
        name[i] = *args;
        i++;
        args++;
    }
    fp = fat12_open(name);
    fat12_read(fp, buff, 256);

    kprint("\n");
    kprint(buff);
    kprint("\nFile Ended\n");
}

static void cmd_ls(const char *args)
{
    kprint("\n");
    fat12_list_root();
    kprint("\n");
}


int process_cmd(char *buff)
{
    char cmd[20] = {0};
    int i = 0;

    while(*buff == ' ')
        buff++;
    
    while(*buff != ' ' && *buff != NULL) {
        cmd[i] = *buff;
        i++;
        buff++;
    }
    while(*buff == ' ')
        buff++;
    cmd[i] = '\0';
    if (memcmp(cmd, "help", 4) == 0)
    {
        cmd_help(buff);
    }
    else if(memcmp(cmd, "clear", 5) == 0)
    {
        cmd_clear(buff);
    }
    else if(memcmp(cmd, "echo", 4) == 0)
    {
        cmd_echo(buff);
    }
    else if(memcmp(cmd, "create", 6) == 0)
    {
        cmd_create_file(buff);
    }
    else if(memcmp(cmd, "cat", 3) == 0)
    {
        cmd_cat(buff);
    }
    else if(memcmp(cmd, "ls", 2) == 0)
    {
        cmd_ls(buff);
    }
    else {
        kprint("\nNot implemented cmd -> ");
        kprint(cmd);
        kprint("\n");
    }
    return 0;
}


// Main syscall dispatcher
u32 syscall_dispatcher(u32 syscall_num, u32 arg1, u32 arg2, u32 arg3, u32 arg4, u32 arg5) {
    // This function runs in RING 0 (kernel mode)
    // Even though it was called from ring 3!
    
    // Dispatch based on syscall number
    switch(syscall_num) {
        case SYS_EXIT:
            return sys_exit(arg1);
        
        case SYS_PRINT:
            return sys_print((const char*)arg1, 0);
        
        case SYS_READ:
            return sys_read(arg1, (char*)arg2, arg3);

        case SYS_EXEC_CMD:
            return process_cmd((const char*)arg1);

        case SYS_COLOR_PRINT:
            return sys_print((const char*)arg1, arg2);
        
        default:
            printk("[KERNEL] Unknown syscall: %d\n", syscall_num);
            return -1; 
    }
}

// Syscall 0: Exit
u32 sys_exit(u32 code) {
    printk("\n[KERNEL] Process exited with code: %d\n", code);
    printk("[KERNEL] System halted.\n");

    while(1) asm("hlt");
    
    return 0;
}

u32 sys_print(const char *str, int color) {
    // CRITICAL: Validate pointer!
    // User could pass a kernel address to read kernel memory!
    
    // Check if pointer is in user space
    // if((u32)str >= 0xC0000000) {  // Assuming kernel at 3GB+
    //     printk("[KERNEL] Invalid pointer from user space: 0x%x\n", (u32)str);
    //     return -1;  // Error: EFAULT (bad address)
    // }
    
    if(color == 0)
    {
        color = WHITE_ON_BLACK;
    }

    // Check if string is too long
    int len = 0;
    const char *p = str;
    while(*p && len < 4096) {
        p++;
        len++;
    }
    
    if(len >= 4096) {
        printk("[KERNEL] String too long\n");
        return -1;
    }
    
    printk_color(color, "%s", str);
    return len;
}

u32 sys_read(u32 fd, char *buf, u32 count) {
    pr_debug("KERNEL", "sys_read handler called\n");

    if (fd != 0) return -1;

    return keyboard_read(buf, count);
}
