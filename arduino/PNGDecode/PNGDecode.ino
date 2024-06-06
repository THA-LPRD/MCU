
#include <Arduino.h>
#include "FS.h"
#include <LittleFS.h>
#include "./DEV_Config.h"
#include <time.h>
#include <PNGdec.h>
#include "EPD.h"
#include "GUI_Paint.h"
// #include <stdlib.h>

PNG png;

// fs::FS &FileSystem;

File myfile;

// uint16_t rawimage[800][480];
// uint16_t *rawimage;
unsigned char gImage_7in3g[96000];
// unsigned char* gImage_7in3g[];

void *myOpen(const char *filename, int32_t *size) {
  Serial.printf("Attempting to open %s\n", filename);
  //myfile = SD.open(filename);
  myfile = LittleFS.open(filename);
  *size = myfile.size();
  return &myfile;
}
void myClose(void *handle) {
  if (myfile) myfile.close();
}
int32_t myRead(PNGFILE *handle, uint8_t *buffer, int32_t length) {
  if (!myfile) return 0;
  return myfile.read(buffer, length);
}
int32_t mySeek(PNGFILE *handle, int32_t position) {
  if (!myfile) return 0;
  return myfile.seek(position);
}

uint8_t mapTo2Bits(uint16_t pixel) {
  if (pixel == 0x0000) {
    return 0b00;
  } else if (pixel == 0xF800 || pixel == 0xD9AB)  { // 1111100000000000
  // #DA345C
    return 0b11;
  } else if (pixel == 0xFFFF) { // 1111111111111111
    return 0b01;
  } else if (pixel == 0xFFE0) { // 1111111111100000
    return 0b10;
  } else {
    return 0b01; // Standardfall
  }
}

// Function to draw pixels to the display
void PNGDraw(PNGDRAW *pDraw) {
  uint16_t usPixels[800];
  png.getLineAsRGB565(pDraw, usPixels, PNG_RGB565_LITTLE_ENDIAN, 0xffffffff);

  for (int i = 0; i < 800; i += 4) {
    uint8_t value = 0;
    value |= mapTo2Bits(usPixels[i]) << 6;
    value |= mapTo2Bits(usPixels[i + 1]) << 4;
    value |= mapTo2Bits(usPixels[i + 2]) << 2;
    value |= mapTo2Bits(usPixels[i + 3]);
    
    gImage_7in3g[pDraw->y * 200 + i / 4] = value;
  }
}
/*
  Serial.print(pDraw->y);
  Serial.print(": ");
  Serial.print(usPixels[0]);
  Serial.print(" ");
  Serial.print(usPixels[479]);
  Serial.print(" ");
  Serial.print(usPixels[480]);
  Serial.print(" ");
  Serial.println(usPixels[799]);

  delay(10);
  */
  /* WORKING!
  for (int x = 0; x < 200; x++) {
    //if (usPixels[x*4 + 800*pDraw->y] != 0) {
    if (usPixels[x*4] != 0) {
      gImage_7in3g[x+200*pDraw->y] = 0x55;
    }
    else {
    //if (usPixels[x*4 + 800*pDraw->y] == 0) {
      gImage_7in3g[x+200*pDraw->y] = 0x00;
    }
  }
  */
/*
  for (int x = 0; x < 800; x++) {
    char pxgroup;
    
    if (usPixels[x + 800*pDraw->y] == 0) {
      // Black 00
      bitWrite(pxgroup, x%4, 0);
      bitWrite(pxgroup, x%4+1, 0);
    }
    if (usPixels[x + 800*pDraw->y] != 0) {
      // White 01
      bitWrite(pxgroup, x%4, 0);
      bitWrite(pxgroup, x%4+1, 1);
    }
    if (x%4 == 0) {
      // Neue Gruppe
      gImage_7in3g[(x+800*pDraw->y)/4] = pxgroup;
    }
    
    /*
    if (usPixels[x + 800*pDraw->y] != 0) {
      Serial.print("F");
      delay(1);
    }
    //Serial.print(usPixels[x + 800*pDraw->y]);
    */

  //}
  // Serial.println(pDraw->y);
  /*
  for (int x = 0; x < 800/4; x++) {
      char byte;
      for (int i = 0; i < 4; i++) {
        if (usPixels[x*4+i] == 0000000000000000) {
        // Black 00
          bitWrite(byte, i, 0);
          bitWrite(byte, i+1, 0);
          Serial.print(usPixels[x*4+i]);
          // Serial.println(" ");
        } else if (usPixels[x*4+i] == 1111111111111111) {
          // White 01
          bitWrite(byte, i, 0);
          bitWrite(byte, i+1, 1);
          Serial.print(usPixels[x*4+i]);
          // Serial.println("Weiß");
        }
      }
      // Serial.println(byte, HEX);


      // gImage_7in3g[pDraw->y*480/4 + x] = byte;
      // rawimage[pDraw->y][i] = usPixels[x];
  }*/
  
// delay(15);
  // tft.writeRect(0, pDraw->y + 24, pDraw->iWidth, 1, usPixels);



#define FORMAT_LITTLEFS_IF_FAILED true

void listDir(fs::FS &fs, const char *dirname, uint8_t levels) {
  Serial.printf("Listing directory: %s\r\n", dirname);

  File root = fs.open(dirname);
  if (!root) {
    Serial.println("- failed to open directory");
    return;
  }
  if (!root.isDirectory()) {
    Serial.println(" - not a directory");
    return;
  }

  File file = root.openNextFile();
  while (file) {
    if (file.isDirectory()) {
      Serial.print("  DIR : ");

      Serial.print(file.name());
      time_t t = file.getLastWrite();
      struct tm *tmstruct = localtime(&t);
      Serial.printf(
        "  LAST WRITE: %d-%02d-%02d %02d:%02d:%02d\n", (tmstruct->tm_year) + 1900, (tmstruct->tm_mon) + 1, tmstruct->tm_mday, tmstruct->tm_hour,
        tmstruct->tm_min, tmstruct->tm_sec);

      if (levels) {
        listDir(fs, file.name(), levels - 1);
      }
    } else {
      Serial.print("  FILE: ");
      Serial.print(file.name());
      Serial.print("  SIZE: ");

      Serial.print(file.size());
      time_t t = file.getLastWrite();
      struct tm *tmstruct = localtime(&t);
      Serial.printf(
        "  LAST WRITE: %d-%02d-%02d %02d:%02d:%02d\n", (tmstruct->tm_year) + 1900, (tmstruct->tm_mon) + 1, tmstruct->tm_mday, tmstruct->tm_hour,
        tmstruct->tm_min, tmstruct->tm_sec);
    }
    file = root.openNextFile();
  }
}

void readFile(fs::FS &fs, const char *path) {
  Serial.printf("Reading file: %s\r\n", path);

  File file = fs.open(path);
  if (!file || file.isDirectory()) {
    Serial.println("- failed to open file for reading");
    return;
  }

  Serial.println("- read from file:");
  while (file.available()) {
    Serial.write(file.read());
  }
  file.close();
}

void writeFile(fs::FS &fs, const char *path, const char *message) {
  Serial.printf("Writing file: %s\r\n", path);

  File file = fs.open(path, FILE_WRITE);
  if (!file) {
    Serial.println("- failed to open file for writing");
    return;
  }
  if (file.print(message)) {
    Serial.println("- file written");
  } else {
    Serial.println("- write failed");
  }
  file.close();
}

void appendFile(fs::FS &fs, const char *path, const char *message) {
  Serial.printf("Appending to file: %s\r\n", path);

  File file = fs.open(path, FILE_APPEND);
  if (!file) {
    Serial.println("- failed to open file for appending");
    return;
  }
  if (file.print(message)) {
    Serial.println("- message appended");
  } else {
    Serial.println("- append failed");
  }
  file.close();
}

void setup() {
  Serial.begin(115200);
  if (!LittleFS.begin(FORMAT_LITTLEFS_IF_FAILED)) {
    Serial.println("LittleFS Mount Failed");
    return;
  }

  listDir(LittleFS, "/", 0);
  fs::FS &FileSystem = LittleFS;
  /*
  createDir(LittleFS, "/mydir");
  writeFile(LittleFS, "/mydir/hello2.txt", "Hello2");
  //writeFile(LittleFS, "/mydir/newdir2/newdir3/hello3.txt", "Hello3");
  writeFile2(LittleFS, "/mydir/newdir2/newdir3/hello3.txt", "Hello3");
  listDir(LittleFS, "/", 3);
  deleteFile(LittleFS, "/mydir/hello2.txt");
  //deleteFile(LittleFS, "/mydir/newdir2/newdir3/hello3.txt");
  deleteFile2(LittleFS, "/mydir/newdir2/newdir3/hello3.txt");
  removeDir(LittleFS, "/mydir");
  listDir(LittleFS, "/", 3);
  writeFile(LittleFS, "/hello.txt", "Hello ");
  appendFile(LittleFS, "/hello.txt", "World!\r\n");
  readFile(LittleFS, "/hello.txt");
  renameFile(LittleFS, "/hello.txt", "/foo.txt");
  readFile(LittleFS, "/foo.txt");
  deleteFile(LittleFS, "/foo.txt");
  testFileIO(LittleFS, "/test.txt");
  deleteFile(LittleFS, "/test.txt");
  */





  int rc, filecount = 0;


  //gImage_7in3g = (unsigned char*)ps_malloc(96000);
  //Serial.println(&gImage_7in3g);



  // rawimage = (uint16_t *)ps_malloc(800 * 480 * sizeof(uint16_t));
  /*
  rawimage = (uint16_t*) heap_caps_malloc(800 * 480 * sizeof(uint16_t), MALLOC_CAP_SPIRAM);
  if (rawimage == NULL) {
Serial.println("Fehler: Konnte keinen Speicher im PSRAM zuweisen.");
    while (1);
  } else {
    Serial.println("Speicher im PSRAM erfolgreich zugewiesen.");
    // Serial.println(&rawimage);
  }
  */

  const char *name = "/st3.png";
  const int len = strlen(name);
  delay(200);
  rc = png.open((const char *)name, myOpen, myClose, myRead, mySeek, PNGDraw);
  Serial.println(png.getWidth());
  Serial.println(png.getHeight());
  delay(200);

  rc = png.decode(NULL, 0);

  png.close();

  delay(1000);
/*
  byte pxgroup;

  bitWrite(pxgroup, 0, 1);
  bitWrite(pxgroup, 1, 1);
  bitWrite(pxgroup, 2, 1);
  bitWrite(pxgroup, 3, 1);
  bitWrite(pxgroup, 4, 1);
  bitWrite(pxgroup, 5, 1);
  bitWrite(pxgroup, 6, 1);
  bitWrite(pxgroup, 7, 1);
  
  Serial.println(pxgroup, HEX);

  bitWrite(pxgroup, 0, 0);
  bitWrite(pxgroup, 1, 1);
  bitWrite(pxgroup, 2, 0);
  bitWrite(pxgroup, 3, 1);
  bitWrite(pxgroup, 4, 0);
  bitWrite(pxgroup, 5, 1);
  bitWrite(pxgroup, 6, 0);
  bitWrite(pxgroup, 7, 1);
  
  Serial.println(pxgroup, HEX);
*/

/*
  for (int y = 0; y < 480; y++) {
    for (int x = 0; x < 199; x++) {
      Serial.print(gImage_7in3g[x + 200*y], HEX);
    }
    Serial.println(gImage_7in3g[200 + 200*y], HEX);
    delay(10);
  }
  */

/*
  for (int y = 0; y < 10; y++) {
    for (int x = 0; x < 9; x++) {
      Serial.print(rawimage[y*10 +x], BIN);
      Serial.print(" ");
    }
    Serial.println(rawimage[y*10 + 9], BIN);
  }*/
/*
  for (int i = 0; i < 96000; i++) {
    gImage_7in3g[i] = 0xFF;
  }
  */

  Serial.println(gImage_7in3g[0], HEX);
  Serial.println(gImage_7in3g[4000], HEX);
  Serial.println(gImage_7in3g[48000], HEX);

    printf("EPD_7IN3G_test Demo\r\n");
    DEV_Module_Init();

    printf("e-Paper Init and Clear...\r\n");
    EPD_7IN3G_Init();

    EPD_7IN3G_Clear(EPD_7IN3G_WHITE);
    DEV_Delay_ms(2000);

    //Create a new image cache
    UBYTE *BlackImage;
    UDOUBLE Imagesize = ((EPD_7IN3G_WIDTH % 4 == 0)? (EPD_7IN3G_WIDTH / 4 ): (EPD_7IN3G_WIDTH / 4 + 1)) * EPD_7IN3G_HEIGHT;
    if((BlackImage = (UBYTE *)malloc(Imagesize)) == NULL) {
        printf("Failed to apply for black memory...\r\n");
        while (1);
    }
    printf("Paint_NewImage\r\n");
    Paint_NewImage(BlackImage, EPD_7IN3G_WIDTH, EPD_7IN3G_HEIGHT, 0, EPD_7IN3G_WHITE);
    Paint_SetScale(4);

#if 1   // show bmp
    printf("show BMP-----------------\r\n");
    Paint_SelectImage(BlackImage);
    Paint_DrawBitMap(gImage_7in3g);
    EPD_7IN3G_Display(BlackImage);
    DEV_Delay_ms(8000);
#endif  

    printf("Clear...\r\n");
    EPD_7IN3G_Clear(EPD_7IN3G_WHITE);

    printf("Goto Sleep...\r\n");
    EPD_7IN3G_Sleep();
    free(BlackImage);
    BlackImage = NULL;
    DEV_Delay_ms(2000);//important, at least 2s
    // close 5V
    printf("close 5V, Module enters 0 power consumption ...\r\n");

  Serial.println("Test complete");
  delay(200);
  Serial.println("Test complete");
  delay(20000);
}

void loop() {}