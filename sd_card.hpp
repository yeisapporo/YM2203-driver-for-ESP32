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

#ifndef _HPP_SD_CARD_HPP_
#define _HPP_SD_CARD_HPP_

#include <SPI.h>
#include <Arduino.h>
#include <FreeRTOS.h>
#include <SD.h>

// SD I/F
#define SD_SCK  (18)
#define SD_MISO (19)
#define SD_MOSI (23)
#define SD_SS   ( 5)
SPIClass vSpi(VSPI);

// frep == 15800000  15000000
int sdSetup(unsigned long freq) {
  vSpi.end();
  vSpi.begin(SD_SCK, SD_MISO, SD_MOSI, SD_SS);
  vSpi.setFrequency(freq);
  vSpi.setDataMode(SPI_MODE1);

  if(!SD.begin(SD_SS, vSpi, freq)) {
    Serial.println("SD card mount failed!!");
    return -1;
  }
  return 0;
}

File gRoot;
int gFileCnt = 0;
#define PLAY_LIST_MAX (256)
char *playList[PLAY_LIST_MAX] = {0};

// store file paths into the play list.
void getFileNames(char **playList, File dir) {
  for(;;) {
    File item = dir.openNextFile();
    if(!item || gFileCnt >= PLAY_LIST_MAX) {
      break;
    } 

    if(item.isDirectory()) {
      // not to enter specific directories.
      if(strcmp(item.name(), "/System Volume Information") != 0 &&
         strcmp(item.name(), "/hidden") != 0) {
        getFileNames(playList, item);
      }
    } else {
      playList[gFileCnt] = (char *)malloc(256);
      strcpy(playList[gFileCnt], item.name());
      Serial.printf("%s\r\n",item.name()); 
      gFileCnt++;
    }
  }
}

// create a play list
void makePlayList(char **playList) {
  File root = SD.open("/");
  getFileNames(playList, root);
  root.close();
}

int fsSetup(void) {
  int ret = 0;

  ret = sdSetup(12800000);
  if(ret != 0) {
    return -1;
  }
  makePlayList(playList);
  return 0;
}

#endif /*  _HPP_SD_CARD_HPP_ */
