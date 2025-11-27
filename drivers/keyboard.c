#include "keyboard.h"

int rptr = 0, wptr = 0;
char screen_buff[256] = {0};

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
    
    // Get base character
    char c = shift ? sc_ascii_shift[scancode] : sc_ascii[scancode];
    
    // Handle caps lock for letters only
    if (caps_lock && c >= 'a' && c <= 'z') {
        c = shift ? c : (c - 32);  // Toggle case
    } else if (caps_lock && c >= 'A' && c <= 'Z') {
        c = shift ? (c + 32) : c;  // Toggle case
    }
    
    return c;
}


void keyboard_callback(registers_t *regs) {
    /* The PIC leaves us the scancode in port 0x60 */
    /* Todo: Add handling for extended key codes*/

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
    
    if (pressed)
    {
        char c = scancode_to_ascii(scancode, shift_pressed, caps_lock);
        screen_buff[wptr] = c;
        wptr++;
    }

    return;
}

int read_key()
{    
    int scancode = screen_buff[rptr];
    if(rptr == wptr)
    {
        return -1;
    }
    rptr++;
    return scancode;
}

void init_keyboard(int a) {
   register_interrupt_handler(IRQ1, keyboard_callback); 
}

