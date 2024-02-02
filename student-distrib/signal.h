#ifndef SIGNAL_H
#define SIGNAL_H

#include "types.h"

#define NUM_SIGNAL      5
#define DIV_ZERO        0
#define SEGFAULT        1
#define INTERRUPT       2
#define ALARM           3
#define USER1           4

extern void* dft_sig_handler[NUM_SIGNAL];

extern void send_signal(uint8_t sig_num);

extern void do_signal(void);

#endif
