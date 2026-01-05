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
    printk_color(VGA_COLOR_MAGENTA,"\n==============Available Commands=============\n");

    printk_color(VGA_COLOR_GREEN, "---------------------------------------------------\n");
    for (int i = 0; commands[i].name != NULL; i++)
    {
        printk_color(VGA_COLOR_CYAN, "Name of the cmd -----> %s\n", commands[i].name);
        printk_color(VGA_COLOR_CYAN, "Description of the cmd ------> %s\n", commands[i].description);
        printk_color(VGA_COLOR_GREEN, "---------------------------------------------------\n");
    }
}

static void cmd_clear(const char* args)
{
    clear_screen();
}

static void cmd_echo(const char *args)
{
    printk("\n");
    printk("%s\n", args);
}

static void cmd_create_file(const char *args)
{
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

    fat12_create(name, 256);
    fp = fat12_open(name);
    fat12_write(fp, buff, 256);

    printk_color(VGA_COLOR_YELLOW, "\nFile %s Created\n", name);
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

    printk("\nFile %s Contents :- \n", name);
    printk("%s", buff);
    printk("\nFile Ended\n");
}

static void cmd_ls(const char *args)
{
    printk("\n");
    fat12_list_root();
    printk("\n");
}


int process_cmd(const char *buff)
{
    char cmd[20] = {0};
    int i = 0;

    while(*buff == ' ')
        buff++;
    
    while(*buff != ' ' && *buff != '\0') {
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
        printk_color(VGA_COLOR_RED,"\n %s cmd not implemented\n", cmd);
    }
    return 0;
}


// Main syscall dispatcher
u32 syscall_dispatcher(u32 syscall_num, u32 arg1, u32 arg2, u32 arg3, u32 arg4, u32 arg5) {
    // This function runs in RING 0 (kernel mode)
     u32 ebp;
    asm volatile("mov %%ebp, %0" : "=r"(ebp));
    
    u32 *frame = (u32*)ebp;
    
    printk("\n========== SYSCALL DEBUG (using EBP) ==========\n");
    printk("EBP = 0x%x\n", ebp);
    
    // printk("\nFunction call frame:\n");
    // printk("[EBP+0]  = %x (Saved EBP)\n", frame[0]);
    // printk("[EBP+4]  = %x (Return address)\n", frame[1]);
    // printk("[EBP+8]  = %x (syscall_num = %u)\n", frame[2], frame[2]);
    // printk("[EBP+12] = %x (arg1 = %u)\n", frame[3], frame[3]);
    // printk("[EBP+16] = %x (arg2 = %u)\n", frame[4], frame[4]);
    // printk("[EBP+20] = %x (arg3 = %u)\n", frame[5], frame[5]);
    // printk("[EBP+24] = %x (arg4 = %u)\n", frame[6], frame[6]);
    // printk("[EBP+28] = %x (arg5 = %u)\n", frame[7], frame[7]);
    
    // printk("\nSaved registers:\n");
    // printk("[EBP+32] = 0x%08x (saved EAX)\n", frame[8]);
    // printk("[EBP+36] = 0x%08x (saved EBX)\n", frame[9]);
    // printk("[EBP+40] = 0x%08x (saved ECX)\n", frame[10]);
    // printk("[EBP+44] = 0x%08x (saved EDX)\n", frame[11]);
    // printk("[EBP+48] = 0x%08x (saved ESI)\n", frame[12]);
    // printk("[EBP+52] = 0x%08x (saved EDI)\n", frame[13]);
    
    printk("\nCPU-pushed values:\n");
    printk("[EBP+56] = %x (User EIP)\n", frame[14]);
    printk("[EBP+60] = %x (User CS)\n", frame[15]);
    printk("[EBP+64] = %x (User EFLAGS)\n", frame[16]);
    printk("[EBP+68] = %x (User ESP)\n", frame[17]);
    printk("[EBP+72] = %x (User SS)\n", frame[18]);
    
    // printk("\nValidation:\n");
    // u32 user_eip = frame[14];
    // u32 user_cs = frame[15];
    // u32 user_esp = frame[17];
    // u32 user_ss = frame[18];
    
    // if (user_cs == 0x1B) {
    //     printk("  ✓ CS = 0x1B (user code segment)\n");
    // } else {
    //     printk("  ✗ CS = 0x%x (WRONG! Should be 0x1B)\n", user_cs);
    // }
    
    // if (user_ss == 0x23) {
    //     printk("  ✓ SS = 0x23 (user data segment)\n");
    // } else {
    //     printk("  ✗ SS = 0x%x (WRONG! Should be 0x23)\n", user_ss);
    // }
    
    // if (user_eip >= 0x00001000 && user_eip < 0x00100000) {
    //     printk("  ✓ EIP = 0x%x (in user code range)\n", user_eip);
    // } else {
    //     printk("  ✗ EIP = 0x%x (SUSPICIOUS! Check if valid)\n", user_eip);
    // }
    
    // if (user_esp >= 0x7FFFF000 && user_esp <= 0x80000000) {
    //     printk("  ✓ ESP = 0x%x (in user stack range)\n", user_esp);
    // } else {
    //     printk("  ✗ ESP = 0x%x (WRONG! Not in user stack range)\n", user_esp);
    // }
    
    // printk("===============================================\n\n");
    switch(syscall_num) {
        case SYS_EXIT:
            return sys_exit(arg1);
        
        case SYS_PRINT:
            return sys_print((const char*)arg1, 0);
        
        case SYS_READ:
            sys_read(arg1, (char*)arg2, arg3);
            asm volatile("mov %%ebp, %0" : "=r"(ebp));
    
            frame = (u32*)ebp;
            printk("\nCPU-pushed values:\n");
            printk("[EBP+56] = %x (User EIP)\n", frame[14]);
            printk("[EBP+60] = %x (User CS)\n", frame[15]);
            printk("[EBP+64] = %x (User EFLAGS)\n", frame[16]);
            printk("[EBP+68] = %x (User ESP)\n", frame[17]);
            printk("[EBP+72] = %x (User SS)\n", frame[18]);

            break;

        case SYS_EXEC_CMD:
            return process_cmd((const char*)arg1);

        case SYS_COLOR_PRINT:
            return sys_print((const char*)arg1, arg2);
        
        default:
            printk("[KERNEL] Unknown syscall: %d\n", syscall_num);
            return -1; 
    }



    return 0;
}

// Syscall 0: Exit
u32 sys_exit(u32 code) {

    printk("\n[KERNEL] Process exited with code: %d\n", code);
    printk("[KERNEL] System halted.\n");

    while(1)
        asm("hlt");
    
    return 0;
}

u32 sys_print(const char *str, int color) {
    // CRITICAL: Validate pointer!
    // TODO: Linux like copy from user     
    
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
    if (fd != 0)
        return -1;

    return keyboard_read(buf, count);
}
