#include "shell.h"

u8 shell_buff[256] = {0};
int idx = 0;

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

int shell_init(void)
{
    kprint("\n");
    kprint("========================================\n");
    kprint("  Welcome to TenOS Shell v0.1\n");
    kprint("  Type 'help' for available commands\n");
    kprint("========================================\n");
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
    if (memcmp(cmd, "HELP", 4) == 0)
    {
        cmd_help(buff);
    }
    else if(memcmp(cmd, "CLEAR", 5) == 0)
    {
        cmd_clear(buff);
    }
    else if(memcmp(cmd, "ECHO", 4) == 0)
    {
        cmd_echo(buff);
    }
    else if(memcmp(cmd, "CREATE", 6) == 0)
    {
        cmd_create_file(buff);
    }
    else if(memcmp(cmd, "CAT", 3) == 0)
    {
        cmd_cat(buff);
    }
    else if(memcmp(cmd, "LS", 2) == 0)
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

int shell_process()
{
    while(1)
    {
        char c = read_key();
        if (c != -1)
        {
            switch (c)
            {
                case ENTER_KEY:
                    process_cmd(shell_buff);
                    shell_buff[0] = '\0';
                    idx = 0;
                    return;
                    break;

                case BACK_SPACE:
                    shell_buff[idx-1] = '\0';
                    idx--;
                    kprint("\b");
                    break;
                default:
                    shell_buff[idx] = c;
                    shell_buff[idx+1] = '\0';
                    kprint(shell_buff + idx);
                    idx++;
                    break;
            }
        }
    }
}