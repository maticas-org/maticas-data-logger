#ifndef UTILS_H
#define UTILS_H

#ifndef EXT_IMPORTS
#define EXT_IMPORTS
  #include "FS.h"
  #include "SD.h"
  #include "SPI.h"
  #include "WiFi.h"
  #include "I2C_RTC.h"
  #include <Arduino.h>
  #include <ArduinoHttpClient.h>
#endif

#include "Event.h"



// Date and time related functions
DateTime fromTimestampStringToDatetime(const String &dtString);
long long fromDatetimeToUnix(const DateTime &dt);

// Memory related functions
void logMemoryUsage();

// File system related functions
void createDir(fs::FS &fs, const char * path);
void removeDir(fs::FS &fs, const char * path);
void readFile(fs::FS &fs, const char * path);
void writeFile(fs::FS &fs, const char * path, const char * message);
void appendFile(fs::FS &fs, const char * path, const char * message);
void renameFile(fs::FS &fs, const char * path1, const char * path2);
void deleteFile(fs::FS &fs, const char * path);

void showAditionalSDCardInfo(fs::SDFS &fs);
bool storeEvents(fs::FS &fs, Event* events, int numEvents, const char * path);
bool loadEvents(fs::FS &fs, Event* events, int numEvents, const char * path);
String findFileByDate(fs::FS &fs, const char *dirname, bool findNewest = true);

#endif // UTILS_H