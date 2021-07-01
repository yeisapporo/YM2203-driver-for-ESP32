#ifndef _HPP_BUFFER_HPP_
#define _HPP_BUFFER_HPP_

#include <SPI.h>
#include <Arduino.h>
#include <FreeRTOS.h>
#include <SD.h>
#include "sd_card.hpp"
#include "vgm.hpp"

FILE_BUFF fbuff = {0};
File vgmfile;

// initialize all r/w buffers.
void init_buff(void) {
	int i;

	fbuff.locked = true;
	// fill all buffers with data from the storage device.
	for (i = 0; i < FBUFF_NUM; i++) {
        vgmfile.read(&fbuff.dat[i][0], FBUFF_SZ);
		fbuff.state[i] = FBUFF_FULL;
	}
	// ↓ testing now.
	fbuff.current_read_num = 0;
	fbuff.current_write_num = 0;
	fbuff.read_ptr = 0;
	fbuff.locked = false;
}

// [thread]
// write to the empty buffer. [debug done]
void write_buff(void *param) {
	for (;;) {
		if(fbuff.state[fbuff.current_write_num] == FBUFF_EMPTY && fbuff.locked == false) {
			fbuff.locked = true;
			fbuff.state[fbuff.current_write_num] = FBUFF_WRITING;
			vgmfile.read(&fbuff.dat[fbuff.current_write_num][0], FBUFF_SZ);
			
			fbuff.state[fbuff.current_write_num] = FBUFF_FULL;
			//Serial.printf(">>>> write:.dat[%d] %d\r\n", fbuff.current_write_num, ret);
			if (++fbuff.current_write_num >= FBUFF_NUM) {
				while (fbuff.state[0] != FBUFF_EMPTY) {
					delay(1);
				}
				fbuff.current_write_num = 0;
				fbuff.state[fbuff.current_write_num] = FBUFF_EMPTY;
			}
			fbuff.locked = false;
		}
		delay(1);
	}
}

// get 1byte data. [debug done]
unsigned char bfgetc(void) {
	unsigned char dat;

	// wait for a readable buffer available.
	while ((fbuff.state[fbuff.current_read_num] & ~(FBUFF_FULL | FBUFF_READING)) && fbuff.locked == true) {
		delayMicroseconds(1);
	}
	fbuff.locked = true;
	// read 1byte data from current read buffer.
	fbuff.state[fbuff.current_read_num] = FBUFF_READING;
	dat = fbuff.dat[fbuff.current_read_num][fbuff.read_ptr];
	//Serial.printf("<<<< read:.dat[%d] (%d)\r\n", fbuff.current_read_num, fbuff.read_ptr);
	if (++fbuff.read_ptr >= FBUFF_SZ) {
		fbuff.state[fbuff.current_read_num] = FBUFF_EMPTY;
		if(++fbuff.current_read_num >= FBUFF_NUM) {
			fbuff.current_read_num = 0;
		}
		fbuff.read_ptr = 0;
	}
	fbuff.locked = false;

	return dat;
}

#endif /* _HPP_BUFFER_HPP_ */
