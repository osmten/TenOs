#include "shell.h"

u8 shell_buff[256] = {0};
int idx = 0;

const char sc_ascii[] = { '?', '?', '1', '2', '3', '4', '5', '6',     
    '7', '8', '9', '0', '-', '=', '?', '?', 'Q', 'W', 'E', 'R', 'T', 'Y', 
        'U', 'I', 'O', 'P', '[', ']', '?', '?', 'A', 'S', 'D', 'F', 'G', 
        'H', 'J', 'K', 'L', ';', '\'', '`', '?', '\\', 'Z', 'X', 'C', 'V', 
        'B', 'N', 'M', ',', '.', '/', '?', '?', '?', ' '};

int shell_init(void)
{
    kprint("\n");
    kprint("========================================\n");
    kprint("  Welcome to TenOS Shell v0.1\n");
    kprint("  Type 'help' for available commands\n");
    kprint("========================================\n");
    kprint("\n");
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
                    // process_cmd();
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