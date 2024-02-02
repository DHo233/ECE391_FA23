#include "keyboard.h"
#include "lib.h"
#include "i8259.h"
#include "terminal.h"
#include "system_call.h"
#include "scheduler.h"
#include "signal.h"

// define modifier keys flag.
static flag_t  modifier_flag;

/* define a table. from scancode to character. from 00-3A */
char keys_table[TABLE_SIZE] = {                                     // The value of scancode refer to the website https://wiki.osdev.org/PS/2_Keyboard#Scan_Code_Sets. 
    0x0, 0x0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0',     // errorcode, esc
    '-', '=', '\b', 0x0, 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i',   // backspace, tab    
    'o', 'p', '[', ']', '\n', 0x0, 'a', 's', 'd', 'f','g', 'h',     // enter, Lctrl
    'j', 'k', 'l' ,';', '\'', '`', 0x0, '\\',                       // Lshift
	'z', 'x', 'c', 'v', 'b', 'n', 'm',                                   
	',', '.', '/', 0x0, 0x0, 0x0, ' ', 0x0,                         // Rshift, Keypad, Lalt, Spacebar, CapsLock
    // fn and keypad buttons are not implentmented.
};

/* define a shifted mapping table. from scancode to shifted character. */
char shifted_table[TABLE_SIZE] = {                                  // The value of scancode refer to the website https://wiki.osdev.org/PS/2_Keyboard#Scan_Code_Sets. 
    0x0, 0x0, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')',     // errorcode, esc
    '_', '+', '\b', 0x0, 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I',   // backspace, tab    
    'O', 'P', '{', '}', '\n', 0x0, 'A', 'S', 'D', 'F','G', 'H',     // enter, Lctrl
    'J', 'K', 'L' ,':', '"', '~', 0x0, '|',                         // Lshift
	'Z', 'X', 'C', 'V', 'B', 'N', 'M',                                   
	'<', '>', '?', 0x0, 0x0, 0x0, ' ', 0x0,                         // Rshift, Keypad, Lalt, Spacebar, CapsLock
    // fn and keypad buttons are not implentmented.
};

/* define a Caps mapping table. from scancode to Caps character. */
char capslk_table[TABLE_SIZE] = {                                   // The value of scancode refer to the website https://wiki.osdev.org/PS/2_Keyboard#Scan_Code_Sets. 
    0x0, 0x0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0',     // errorcode, esc
    '-', '=', '\b', 0x0, 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I',   // backspace, tab    
    'O', 'P', '[', ']', '\n', 0x0, 'A', 'S', 'D', 'F','G', 'H',     // enter, Lctrl
    'J', 'K', 'L' ,';', '\'', '`', 0x0, '\\',                       // Lshift
	'Z', 'X', 'C', 'V', 'B', 'N', 'M',                                   
	',', '.', '/', 0x0, 0x0, 0x0, ' ', 0x0,                         // Rshift, Keypad, Lalt, Spacebar, CapsLock
    // fn and keypad buttons are not implentmented.
};

/* define a Caps_shifted mapping table. from scancode to Caps_shift character. */
char capslk_shifted_table[TABLE_SIZE] = {                           // The value of scancode refer to the website https://wiki.osdev.org/PS/2_Keyboard#Scan_Code_Sets. 
    0x0, 0x0, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')',     // errorcode, esc
    '_', '+', '\b', 0x0, 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i',   // backspace, tab    
    'o', 'p', '[', ']', '\n', 0x0, 'a', 's', 'd', 'f','g', 'h',     // enter, Lctrl
    'j', 'k', 'l' ,':', '"', '~', 0x0, '|',                         // Lshift
	'z', 'x', 'c', 'v', 'b', 'n', 'm',                                   
	'<', '>', '?', 0x0, 0x0, 0x0, ' ', 0x0,                         // Rshift, Keypad, Lalt, Spacebar, CapsLock
    // fn and keypad buttons are not implentmented.
};

/**
 * keyboard_init
 *  DESCRIPTION : enable the interrupt request of the keyboard.
 *  INPUTS : none
 *  OUTPUTS : none
 *  RETURN VALUE : none
 *  SIDE EFFECTS : enable the pin#1 interrupt request of master_PIC.
 */
void keyboard_init(void){
    /* initialize the modifier flags. */
    modifier_flag.shift_flag = 0;
    modifier_flag.capsLk_flag = 0;
    modifier_flag.ctrl_flag = 0;
    modifier_flag.alt_flag = 0;
    /* enable the interrupt request. */
    enable_irq(KEYBOARD_IRQ_NUM);
}

/**
 * keyboard_handler
 *  DESCRIPTION : When an interrupt of keyboard occurs, read inputs from keyboard port. 
 *                Put the input into the video memory for display.
 *  INPUTS : none
 *  OUTPUTS : none
 *  RETURN VALUE : none
 *  SIDE EFFECTS : put one byte into video memory by putc function.
 */
void keyboard_handler(void){
    
    /* read the input from keyboard port */
    uint32_t scancode = inb(KEYBOARD_PORT);                 // one byte, zero-extended 32-bit
    char ascii;

    /* handle the modifier keys to set modifier_flags */
    if (is_modifier(scancode)){
        send_eoi(KEYBOARD_IRQ_NUM);
        return;
    }

    /* handle Alt+F# to switch terminal */
    if (modifier_flag.alt_flag){
        if (scancode == F1_PRESSED){
            terminal_switch(0);
        }
        if (scancode == F2_PRESSED){
            terminal_switch(1);
            // if(active_array[1] == -1){
            //     if(cur_process != (MAX_PROCESS-1)){
            //         send_eoi(KEYBOARD_IRQ_NUM);
            //         sche_term = 1;
            //         cur_process = -1;
            //         update_video_mem_paging(sche_term);
            //         clear();
            //         execute((uint8_t*)"shell");
            //     }
            // }
        }
        if (scancode == F3_PRESSED){
            terminal_switch(2);
            // if(active_array[2] == -1){
            //     if(cur_process != (MAX_PROCESS-1)){
            //         send_eoi(KEYBOARD_IRQ_NUM);
            //         sche_term = 2;
            //         cur_process = -1;
            //         update_video_mem_paging(sche_term);
            //         clear();
            //         execute((uint8_t*)"shell");
            //     }
            // }
        }
        send_eoi(KEYBOARD_IRQ_NUM);
        return;
    }

    /* handle the ctrl+l to clear */
    if (modifier_flag.ctrl_flag && (keys_table[scancode]=='l')){
        // clear the screen and put the cursor to upper left corner.
        clear_intr();
        send_eoi(KEYBOARD_IRQ_NUM);
        return;
    }
    /* handle the ctrl+c to kill the task */
    if (modifier_flag.ctrl_flag && (keys_table[scancode]=='c')){
        send_signal(INTERRUPT);
        send_eoi(KEYBOARD_IRQ_NUM);
        return;
    }

    /* based on the modifier press state, chose the corresponding character. */
    if (scancode < TABLE_SIZE){
        if (modifier_flag.capsLk_flag){   
            /* capslk open and shift pressed */                     
            if (modifier_flag.shift_flag){  
                ascii = capslk_shifted_table[scancode];
            }
            /* capslk open and shift released */
            else{                            
                ascii = capslk_table[scancode];                        
            }
        }
        /* caps close and shift open */
        else if(modifier_flag.shift_flag){ 
            ascii = shifted_table[scancode];                 
        }
        /* normal case */
        else{
            ascii = keys_table[scancode];
        }
        /* print it on the screen.*/
        if(fill_line_buffer(ascii)){             // condition: 1. not overflow under read mode, or 2. the read mode close,
            putc_intr(ascii);
        }
    }
    
    /* interrupt handler finished */
    send_eoi(KEYBOARD_IRQ_NUM);                      
}

/**
 * is_modifier
 *  DESCRIPTION : Check whether the scancode is modifier key.
 *                If the scancode is modifier key, update the corresponding flag state and return 1.
 *                otherwise, do nothing, return 0.
 *  INPUTS : scancode
 *  OUTPUTS : none
 *  RETURN VALUE : if the scancode is modifier key and pressed, return 1. otherwise, return 0. 
 *  SIDE EFFECTS : if the scancode is modifier key, update the modifier_flag.
 */
uint8_t is_modifier(uint32_t scancode){
    switch(scancode){
        case CAPS_LOCK_PRESS:                               // capslk
            if (modifier_flag.capsLk_flag == 0){            // switch mode
                modifier_flag.capsLk_flag = 1;
            }
            else{
                modifier_flag.capsLk_flag = 0;
            }
            return 1;
        case LEFT_CTRL_PRESS:                               // ctrl
            modifier_flag.ctrl_flag = 1;
            return 1;
        case LEFT_CTRL_RELEASE:
            modifier_flag.ctrl_flag = 0;
            return 0;
        case LEFT_ALT_PRESS:                                // alt
            modifier_flag.alt_flag = 1;
            return 1;
        case LEFT_ALT_RELEASE:
            modifier_flag.alt_flag = 0;
            return 0;
        case LEFT_SHIFT_PRESS:                              // shift
            modifier_flag.shift_flag = 1;
            return 1;
        case LEFT_SHIFT_RELEASE:
            modifier_flag.shift_flag = 0;
            return 0;
        case RIGHT_SHIFT_PRESS:
            modifier_flag.shift_flag = 1;
            return 1;
        case RIGHT_SHIFT_RELEASE:
            modifier_flag.shift_flag = 0;
            return 0;
        default:                                            // dafault case
            return 0;
    }
}
