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

// プレイリストにファイルパスを格納
void getFileNames(char **playList, File dir) {
  for(;;) {
    File item = dir.openNextFile();
    if(!item || gFileCnt >= PLAY_LIST_MAX) {
      break;
    } 

    if(item.isDirectory()) {
      // 特定ディレクトリは中に入らないよう除外
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

// プレイリスト作成
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
