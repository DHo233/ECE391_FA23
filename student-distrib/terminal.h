#ifndef TERMINAL_H
#define TERMINAL_H

#include "types.h"

#define NUM_TERMINAL 3
#define BUFFER_SIZE 128                    /* keyboard buffer size */       

/* define a structure to terminal. */
typedef struct terminal_t
{
	char  	line_buffer[BUFFER_SIZE];     		    /* buffer */
	int		count;                    		    /* number of elements in buffer  */
	uint8_t read_open;							/* to tell the keyboard it's read. */
	uint8_t enter_flag;							/* synchorize the terminal and keyboard interrupt. */ 
	int		x;									/* current x coordinate of video mem */
	int		y;									/* current y coordinate of video mem */
}terminal_t;

extern volatile uint8_t cur_terminal;

extern terminal_t multi_terms[NUM_TERMINAL];

extern uint32_t back_video_buf_addr[NUM_TERMINAL];

/* open the terminal. */
extern int32_t terminal_open(const uint8_t* filename);

/* close the terminal. */
extern int32_t terminal_close(int32_t fd);

/* read nbytes from user input to buf. */
extern int32_t terminal_read(int32_t fd, void* buf, int32_t nbytes);

/* write nbytes from buf to screen directly.*/
extern int32_t terminal_write(int32_t fd, const void* buf, int32_t nbytes);

/* put a char into buffer */
extern int32_t fill_line_buffer(uint8_t c);

extern void terminal_switch(uint8_t term_id);

#endif
