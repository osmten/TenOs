#include "shell.h"

u8 shell_buff[256] = {0};
int idx = 0;

const char sc_ascii[] = { '?', '?', '1', '2', '3', '4', '5', '6',     
    '7', '8', '9', '0', '-', '=', '?', '?', 'Q', 'W', 'E', 'R', 'T', 'Y', 
        'U', 'I', 'O', 'P', '[', ']', '?', '?', 'A', 'S', 'D', 'F', 'G', 
        'H', 'J', 'K', 'L', ';', '\'', '`', '?', '\\', 'Z', 'X', 'C', 'V', 
        'B', 'N', 'M', ',', '.', '/', '?', '?', '?', ' '};


static command_t commands[] = {
    {"help",    "show all the commands",           cmd_help},
    {"clear",   "Clear screen",                    cmd_clear},
    {"echo",    "Echo text",                       cmd_echo},
    {"ls",      "List files",               cmd_help},
    {"cat",     "Display file contents",    cmd_help},
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
        int scancode = read_key();
        if (scancode != -1)
        {
            switch (scancode)
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
                    shell_buff[idx] = sc_ascii[scancode];
                    shell_buff[idx+1] = '\0';
                    kprint(shell_buff + idx);
                    idx++;
                    break;
            }
        }
    }
}