#include "shell.h"

static int strcmp_user(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++; s2++;
    }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

void shell_main() {
    char line[256];
    
    print("\n========================================\n");
    print("  TenOS Shell v0.2 (User Mode)\n");
    print("========================================\n\n");
    
    while(1) {
        print("TenOS> ");
        
        int len = read(0, line, sizeof(line) - 1);
        print("after read \n");
        
        if (len > 0) {
            line[len] = '\0';
            
            // Remove trailing newline
            if (len > 0 && line[len-1] == '\n') {
                line[len-1] = '\0';
            }
            
            // Skip empty lines
            char *p = line;
            while (*p == ' ') p++;
            if (*p == '\0') continue;
            
            // Special commands handled in user space
            if (strcmp_user(p, "exit") == 0) {
                print("\nGoodbye!\n");
                exit(0);
            }
            
            // Pass everything else to kernel via syscall
            int result = sys_exec_command(line);
            
            if (result < 0) {
                print("\nCommand failed or unknown command\n");
                print("Type 'help' for available commands\n");
            }
        }
    }
}