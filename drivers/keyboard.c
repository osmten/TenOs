#include "keyboard.h"

#define BUFFER_SIZE 256
static char input_buffer[BUFFER_SIZE];
static int read_pos = 0;
static int write_pos = 0;

// Current line being stored (before Enter is pressed)
static char line_buffer[BUFFER_SIZE];
static int line_pos = 0;


const char sc_ascii[] = {
    '?',  '?',  '1',  '2',  '3',  '4',  '5',  '6',     // 0x00-0x07
    '7',  '8',  '9',  '0',  '-',  '=',  '\b', '\t',    // 0x08-0x0F (backspace, tab)
    'q',  'w',  'e',  'r',  't',  'y',  'u',  'i',     // 0x10-0x17
    'o',  'p',  '[',  ']',  '\n', '?',  'a',  's',     // 0x18-0x1F (enter, ctrl)
    'd',  'f',  'g',  'h',  'j',  'k',  'l',  ';',     // 0x20-0x27
    '\'', '`',  '?',  '\\', 'z',  'x',  'c',  'v',     // 0x28-0x2F (left shift)
    'b',  'n',  'm',  ',',  '.',  '/',  '?',  '*',     // 0x30-0x37 (right shift, keypad *)
    '?',  ' ',  '?',  '?',  '?',  '?',  '?',  '?',     // 0x38-0x3F (alt, space, caps, F1-F4)
    '?',  '?',  '?',  '?',  '?',  '?',  '?',  '7',     // 0x40-0x47 (F5-F10, num lock, scroll, keypad 7/Home)
    '8',  '9',  '-',  '4',  '5',  '6',  '+',  '1',     // 0x48-0x4F (keypad)
    '2',  '3',  '0',  '.',  '?',  '?',  '?',  '?',     // 0x50-0x57 (keypad, F11, F12)
    '?'                                                 // 0x58
};

// Shift pressed ASCII characters
const char sc_ascii_shift[] = {
    '?',  '?',  '!',  '@',  '#',  '$',  '%',  '^',     // 0x00-0x07
    '&',  '*',  '(',  ')',  '_',  '+',  '\b', '\t',    // 0x08-0x0F
    'Q',  'W',  'E',  'R',  'T',  'Y',  'U',  'I',     // 0x10-0x17
    'O',  'P',  '{',  '}',  '\n', '?',  'A',  'S',     // 0x18-0x1F
    'D',  'F',  'G',  'H',  'J',  'K',  'L',  ':',     // 0x20-0x27
    '"',  '~',  '?',  '|',  'Z',  'X',  'C',  'V',     // 0x28-0x2F
    'B',  'N',  'M',  '<',  '>',  '?',  '?',  '*',     // 0x30-0x37
    '?',  ' ',  '?',  '?',  '?',  '?',  '?',  '?',     // 0x38-0x3F
    '?',  '?',  '?',  '?',  '?',  '?',  '?',  '7',     // 0x40-0x47
    '8',  '9',  '-',  '4',  '5',  '6',  '+',  '1',     // 0x48-0x4F
    '2',  '3',  '0',  '.',  '?',  '?',  '?',  '?',     // 0x50-0x57
    '?'                                                 // 0x58
};


#define SC_MAX 57
static int shift_pressed = 0;
static int ctrl_pressed = 0;
static int alt_pressed = 0;
static int caps_lock = 0;

char scancode_to_ascii(u8 scancode, int shift, int caps_lock) {
    if (scancode >= 0x59) return '?';  // Out of range
    
    char c = shift ? sc_ascii_shift[scancode] : sc_ascii[scancode];
    
    if (caps_lock && c >= 'a' && c <= 'z') {
        c = shift ? c : (c - 32);
    } else if (caps_lock && c >= 'A' && c <= 'Z') {
        c = shift ? (c + 32) : c;
    }
    
    return c;
}

// Commit current line to the input buffer
static void commit_line() {

    for (int i = 0; i < line_pos; i++) {
        input_buffer[write_pos] = line_buffer[i];
        write_pos = (write_pos + 1) % BUFFER_SIZE;
    }
    
    input_buffer[write_pos] = '\n';
    write_pos = (write_pos + 1) % BUFFER_SIZE;

    line_pos = 0;
}

void keyboard_callback(registers_t *regs) {

    u8 scancode = port_byte_in(0x60);
    int pressed = !(scancode & 0x80);
    scancode = scancode & 0x7F;

    if (scancode == 0x2A || scancode == 0x36) {  // Left/Right Shift
        shift_pressed = pressed;
        return;
    } else if (scancode == 0x1D) {  // Left Ctrl
        ctrl_pressed = pressed;
        return;
    } else if (scancode == 0x38) {  // Left Alt
        alt_pressed = pressed;
        return;

    } else if (scancode == 0x3A && pressed) {  // Caps Lock (toggle on press only)
        caps_lock = !caps_lock;
        return;
        // TODO: Update keyboard LED
    }
    
    if (!pressed) return;  // Only handle key press, not release
    
    char c = scancode_to_ascii(scancode, shift_pressed, caps_lock);
    
    if (c == '\n') {
        printk("\n");
        commit_line();
    } 
    else if (c == '\b') {
        if (line_pos > 0) {
            line_pos--;
            printk("\b");
        }
    }
    else if (c != '?') {
        if (line_pos < BUFFER_SIZE - 1) {
            line_buffer[line_pos++] = c;
            
            char str[2] = {c, '\0'};
            printk("%s", str);
        }
    }
    return;
}

// Check if there's a complete line available
int has_complete_line() {
    int pos = read_pos;
    
    while (pos != write_pos) {
        if (input_buffer[pos] == '\n') {
            return 1;
        }
        pos = (pos + 1) % BUFFER_SIZE;
    }
    
    return 0;
}

// Read from keyboard buffer (called by syscall)
// This is BLOCKING - waits until a complete line is available
int keyboard_read(char *user_buf, unsigned int count) {
    int bytes_read = 0;
    
    // Wait for a complete line (with newline)
    while (!has_complete_line()) {
        // Enable interrupts and halt until next interrupt
        asm volatile("sti");
        asm volatile("hlt");
    }
    
    while (bytes_read < count && read_pos != write_pos) {
        user_buf[bytes_read] = input_buffer[read_pos];
        read_pos = (read_pos + 1) % BUFFER_SIZE;
        bytes_read++;
        
        if (user_buf[bytes_read - 1] == '\n') {
            user_buf[bytes_read - 1] = '\0';
            break;
        }
    }

    return bytes_read;
}


void init_keyboard(int a) {
   register_interrupt_handler(IRQ1, keyboard_callback); 
}

