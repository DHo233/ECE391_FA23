#include "signal.h"
#include "system_call.h"
#include "scheduler.h"
#include "lib.h"

void kill_the_task(void){
    clear();
    halt(0);
    return;
}

void ignore(void){
    /* to be finished */
    return;
}

void send_signal(uint8_t sig_num){
    // if(sig_num < 0 || sig_num >= NUM_SIGNAL) return;
    pcb_t* cur_pcb;
    if(sig_num == INTERRUPT){
        cur_pcb = (pcb_t*)(KERNEL_STACK_START - SIZE_8KB * (active_array[cur_terminal] + 1));
    }else{
        cur_pcb = (pcb_t*)(KERNEL_STACK_START - SIZE_8KB * (cur_process + 1));
    }
    cur_pcb->signal_array[sig_num] = 1;
    return;
}

void* dft_sig_handler[NUM_SIGNAL] = {&kill_the_task, &kill_the_task, &kill_the_task, &ignore, &ignore};

void do_signal(void){
    pcb_t* cur_pcb = (pcb_t*)(KERNEL_STACK_START - SIZE_8KB * (cur_process + 1));
    uint8_t sig_num;
    for(sig_num = 0; sig_num < NUM_SIGNAL; sig_num++){
        if(cur_pcb->signal_array[sig_num]){
            cur_pcb->signal_array[sig_num] = 0;
            uint8_t i;
            for(i = 0; i < NUM_SIGNAL; i++) cur_pcb->sig_mask[i] = 1;       // mask all other signals
            if(cur_pcb->sig_handler[sig_num] == dft_sig_handler[sig_num]){  // directly execute handler in kernel if it is default
                void (*handler)(void) = cur_pcb->sig_handler[sig_num];
                handler();
                return;
            }
            break;
        }
    }
    if(sig_num == NUM_SIGNAL) return;                                       // no pending signal
    // void (*handler)(void) = cur_pcb->sig_handler[sig_num];

    /* Set up the signal handler’s stack frame */
}
