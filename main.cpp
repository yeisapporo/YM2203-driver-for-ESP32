/*

 YM2203C driver for ESP22

Copyright (c) 2025 Kazuteru Yamada(yeisapporo).  All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

- Redistributions of source code must retain the above copyright notice,
  this list of conditions and the following disclaimer.
- Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
 */

#include <SPI.h>
#include <SD.h>
#include <Arduino.h>
#include <FreeRTOS.h>
#include "buffer.hpp"

// direct gpio
#define GPIO_0TO31SET_REG   *((volatile unsigned long *)GPIO_OUT_W1TS_REG)
#define GPIO_0TO31CLR_REG   *((volatile unsigned long *)GPIO_OUT_W1TC_REG)
#define GPIO_0TO31RD_REG    *((volatile unsigned long *)GPIO_IN_REG)

hw_timer_t *htimer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;
portMUX_TYPE cntMux = portMUX_INITIALIZER_UNLOCKED;
volatile unsigned long timercnt = 0;
extern File vgmfile;
extern void write_buff(void *param);

// synthesizer i/f with ESP32
// data
#define YM2203_D0	  (25)  // LSB
#define YM2203_D1 	(26)
#define YM2203_D2 	(14)
#define YM2203_D3 	( 4)
#define YM2203_D4 	(16)
#define YM2203_D5 	(17)
#define YM2203_D6 	(21)
#define YM2203_D7 	(22)  // MSB
// address
#define YM2203_A0 	(27)  // 0:reg addr 1:reg data
// write enable
#define YM2203_XWE	(13)
#define WRITE_ENABLE	(0)
#define WRITE_DISABLE	(1)
// reset YM2203
#define YM2203_XIC  (2)
// wait
#define ADDR_WAIT (5)
#define DATA_WAIT (22)

inline void writeYM2203(unsigned char addr, unsigned char data) {
  unsigned long setd = 0;
  unsigned long clrd = 0;

  // put D0 to D7 together into one to make data to write.
  if(data & 0b00000001) { setd |= 1 << YM2203_D0; }
  if(data & 0b00000010) { setd |= 1 << YM2203_D1; }
  if(data & 0b00000100) { setd |= 1 << YM2203_D2; }
  if(data & 0b00001000) { setd |= 1 << YM2203_D3; }
  if(data & 0b00010000) { setd |= 1 << YM2203_D4; }
  if(data & 0b00100000) { setd |= 1 << YM2203_D5; }
  if(data & 0b01000000) { setd |= 1 << YM2203_D6; }
  if(data & 0b10000000) { setd |= 1 << YM2203_D7; }
  // clear GPIO directly.
  clrd |= 1 << YM2203_D0 | 1 << YM2203_D1 | 1 << YM2203_D2 | 1 << YM2203_D3
        | 1 << YM2203_D4 | 1 << YM2203_D5 | 1 << YM2203_D6 | 1 << YM2203_D7;
  GPIO_0TO31CLR_REG = clrd;
  // specify whether addr or data.
  digitalWrite(YM2203_A0, addr);
  delayMicroseconds(DATA_WAIT);
  // enable to write.
  digitalWrite(YM2203_XWE, WRITE_ENABLE);
  // wriite GPIO directly.
  GPIO_0TO31SET_REG = setd;
  if(addr == 0) {
    delayMicroseconds(ADDR_WAIT);
  } else {
    delayMicroseconds(DATA_WAIT);
  }
  // disable to write.
  digitalWrite(YM2203_XWE, WRITE_DISABLE);
}

void IRAM_ATTR samplenumcounter() {
  portENTER_CRITICAL_ISR(&timerMux);
  timercnt++;
  portEXIT_CRITICAL_ISR(&timerMux);
}

// not used.
void IRAM_ATTR dacclock() {
  static int clock = 1;
  digitalWrite(12, clock);
  clock ^= 1;
}

void resetYM2203(void) {
  digitalWrite(YM2203_XIC, LOW);
  delay(3);
  digitalWrite(YM2203_XIC, HIGH);
}

void setup(void) {
  // pin settings.
  pinMode(YM2203_A0, OUTPUT);
  pinMode(YM2203_D0, OUTPUT);
  pinMode(YM2203_D1, OUTPUT);
  pinMode(YM2203_D2, OUTPUT);
  pinMode(YM2203_D3, OUTPUT);
  pinMode(YM2203_D4, OUTPUT);
  pinMode(YM2203_D5, OUTPUT);
  pinMode(YM2203_D6, OUTPUT);
  pinMode(YM2203_D7, OUTPUT);
  pinMode(      13, OUTPUT);  // XWE
  pinMode(      12, OUTPUT);  // test clock(unused).
  pinMode(       2, OUTPUT);  // XRST
  
  Serial.begin(115200);
  // prepare to use sample # wait function. 
  htimer = timerBegin(1, 2, true);
  timerAttachInterrupt(htimer, samplenumcounter, true);
  // 907 / 40 = 22.675usec
  timerAlarmWrite(htimer, 907 - 1, true);

  fsSetup();
  resetYM2203();
  delay(5000);
}

// sample # wait func.
inline void samplenumwait(unsigned long n) {
  portENTER_CRITICAL_ISR(&cntMux);
  timerAlarmDisable(htimer);
  timercnt = 0;
  timerAlarmEnable(htimer);
  portEXIT_CRITICAL_ISR(&cntMux);
  for(;;) {
    if(timercnt >= n) {
      break;
    }
  }
}

void loop() 
{
  static VGM_HEADER vgm_header;
  static int vgmIdx = 0;

  int ret = 0;
  int data_len;
  int data_cnt;
  unsigned char cmd;
  unsigned char aa;
  unsigned char dd;
  unsigned short nnnn;

  vgmfile = SD.open(playList[vgmIdx], FILE_READ);
	if (vgmfile == 0) {
		Serial.printf("vgm file open error.\r\n");
		exit(-1);
	}

	//ret = fread(&vgm_header, sizeof(unsigned char), sizeof(vgm_header), fp);
  ret = vgmfile.readBytes((char*)&vgm_header, sizeof(vgm_header));
	if (ret != sizeof(vgm_header)) {
		Serial.printf("vgm header read error.\r\n");
		exit(-1);
	};
	// test vgm identifier.
	if (vgm_header.vgm_ident[0] != 0x56 || vgm_header.vgm_ident[1] != 0x67 ||
      vgm_header.vgm_ident[2] != 0x6d || vgm_header.vgm_ident[3] != 0x20) {
		Serial.printf("unknown format. quit.\r\n");
		exit(-1);
	}
	// now only YM2203 supported.
	if (vgm_header.ym2203_clock == 0) {
		Serial.printf("not supported other than YM2203. quit.\r\n");
		exit(-1);
	}

	// fill all buffers
	init_buff();
	// create file buffering thread.
	xTaskCreatePinnedToCore(write_buff, "write_buff", 4096 * 2, NULL, 1, NULL, 0);
	// vgm body parser.
	data_len = vgm_header.gd3_offset + 0x14 - sizeof(vgm_header);
	data_cnt = 0;

	//set pre-scalers.
  writeYM2203(0, 0x2d);
  writeYM2203(1, 0xff);
  // test
  Serial.printf("Now playing:%s\r\n", playList[vgmIdx]);
	do {
		cmd = bfgetc();

		switch (cmd) {
		case 0x55:
			aa = bfgetc();
			dd = bfgetc();
			writeYM2203(0, aa);
      writeYM2203(1, dd);
      data_cnt++;
			break;
		case 0x61:
			nnnn = bfgetc() + bfgetc() * 256;
			//Serial.printf("61:wait %d samples.\r\n",nnnn);
			samplenumwait(nnnn);
			data_cnt++;
			break;
		case 0x62:
			//Serial.printf("wait 735 samples.\r\n");
			samplenumwait(735);
			break;
		case 0x63:
			//Serial.printf("wait 882 samples.\r\n");
			samplenumwait(882);
			break;
		case 0x66:
			Serial.printf("----- end of data. -----\r\n");
			//data_cnt = data_len - 1;
			data_cnt = data_len;
			break;
		case 0x70: case 0x71: case 0x72: case 0x73:
		case 0x74: case 0x75: case 0x76: case 0x77:
		case 0x78: case 0x79: case 0x7a: case 0x7b:
		case 0x7c: case 0x7d: case 0x7e: case 0x7f:
			//Serial.printf("wait 1 sample.\r\n");
			samplenumwait(cmd - 0x6f);
			break;
		default:
			Serial.printf("***** encountered unknown command for YM2203. quit. *****\r\n");
			Serial.printf("value:%02x cnt:%d\r\n", cmd, data_cnt);
			exit(-1);
			break;
		}

		data_cnt++;
	} while (data_cnt < data_len);

  delay(100);

	if(++vgmIdx % gFileCnt == 0) {
		vgmIdx = 0;
	}
  delay(1);
}
