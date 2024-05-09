#include "CustomUtils.h"

void logMemoryUsage() {
  long int free_hmem = ESP.getFreeHeap();
  long int total_hmem = ESP.getHeapSize();
  long int free_mem = ESP.getFreePsram();
  long int total_mem = ESP.getPsramSize();

  float usedPercentage = ((float)(total_hmem - free_hmem) * 100.0f) / (float)total_hmem;

  Serial.printf("\tFree H. memory: %d B, Total H. memory: %d B, used H. percentage: %.2f\n", free_hmem, total_hmem, usedPercentage);
  Serial.printf("\tFree memory: %d B, Total memory: %d B\n", free_mem, total_mem);
}

void listDir(fs::FS &fs, const char * dirname, uint8_t levels){
    Serial.printf("Listing directory: %s\n", dirname);

    File root = fs.open(dirname);
    if(!root){
        Serial.println("Failed to open directory");
        return;
    }
    if(!root.isDirectory()){
        Serial.println("Not a directory");
        return;
    }

    File file = root.openNextFile();
    while(file){
        if(file.isDirectory()){
            Serial.print("  DIR : ");
            Serial.println(file.name());
            if(levels){
                listDir(fs, file.path(), levels -1);
            }
        } else {
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("  SIZE: ");
            Serial.println(file.size());
        }
        file = root.openNextFile();
    }
}

void createDir(fs::FS &fs, const char * path){
    Serial.printf("Creating Dir: %s\n", path);
    if(fs.mkdir(path)){
        Serial.println("Dir created");
    } else {
        Serial.println("mkdir failed");
    }
}

void removeDir(fs::FS &fs, const char * path){
    Serial.printf("Removing Dir: %s\n", path);
    if(fs.rmdir(path)){
        Serial.println("Dir removed");
    } else {
        Serial.println("rmdir failed");
    }
}

void readFile(fs::FS &fs, const char * path){
    Serial.printf("Reading file: %s\n", path);

    File file = fs.open(path);
    if(!file){
        Serial.println("Failed to open file for reading");
        return;
    }

    Serial.print("Read from file: ");
    while(file.available()){
        Serial.write(file.read());
    }
    file.close();
}

void writeFile(fs::FS &fs, const char * path, const char * message){
    Serial.printf("Writing file: %s\n", path);

    File file = fs.open(path, FILE_WRITE);
    if(!file){
        Serial.println("Failed to open file for writing");
        return;
    }
    if(file.print(message)){
        Serial.println("File written");
    } else {
        Serial.println("Write failed");
    }
    file.close();
}

void appendFile(fs::FS &fs, const char * path, const char * message){
    Serial.printf("Appending to file: %s\n", path);

    File file = fs.open(path, FILE_APPEND);
    if(!file){
        Serial.println("Failed to open file for appending");
        return;
    }
    if(file.print(message)){
        Serial.println("Message appended");
    } else {
        Serial.println("Append failed");
    }
    file.close();
}

void renameFile(fs::FS &fs, const char * path1, const char * path2){
    Serial.printf("Renaming file %s to %s\n", path1, path2);
    if (fs.rename(path1, path2)) {
        Serial.println("File renamed");
    } else {
        Serial.println("Rename failed");
    }
}

void deleteFile(fs::FS &fs, const char * path){
    Serial.printf("Deleting file: %s\n", path);
    if(fs.remove(path)){
        Serial.println("File deleted");
    } else {
        Serial.println("Delete failed");
    }
}

void showAditionalSDCardInfo(fs::SDFS &fs) {
  uint8_t cardType = fs.cardType();

  if(cardType == CARD_NONE){
      Serial.println("No SD card attached");
      return;
  }

    Serial.print("SD Card Type: ");
    if(cardType == CARD_MMC){
        Serial.println("MMC");
    } else if(cardType == CARD_SD){
        Serial.println("SDSC");
    } else if(cardType == CARD_SDHC){
        Serial.println("SDHC");
    } else {
        Serial.println("UNKNOWN");
    }

    uint64_t cardSize = fs.cardSize() / (1024 * 1024);
    Serial.printf("SD Card Size: %lluMB\n", cardSize);
}


/*
* store events in a file, only stores events that are not empty.
* @param fs: file system object
* @param events: array of events
* @param numevents: number of events to store
* @param path: path to the file
* @return true if the events were stored successfully, false otherwise
*/
bool storeEvents(fs::FS &fs, Event* events, int numEvents, const char * path){
    Serial.printf("Storing %d events in file: %s\n", numEvents, path);
    File file = fs.open(path, FILE_WRITE, true);

    if(!file){
        Serial.println("Failed to open file for writing");
        return false;
    }

    const Event emptyEvent = Event();

    for (int i = 0; i < numEvents; i++) {
        if (events[i] == emptyEvent) {
            continue;
        }
        file.print(events[i].toString());
        file.print("\n");
    }

    file.close();
    return true;
}

/*
* load events from a file
* @param fs: file system object
* @param events: array of events
* @param numevents: number of events to load
* @param path: path to the file
* @return true if the events were loaded successfully, false otherwise
*/
bool loadEvents(fs::FS &fs, Event* events, int numEvents, const char * path){
    Serial.printf("Loading %d events from file: %s\n", numEvents, path);
    File file = fs.open(path, FILE_READ);

    if(!file){
        Serial.println("Failed to open file for reading");
        return false;
    }

    for (int i = 0; i < numEvents; i++) {
        String line = file.readStringUntil('\n');
        events[i] = Event(line);
    }

    file.close();
    return true;
}


// Helper function to extract the timestamp from a filename
unsigned long getTimestampFromFilename(const char* filename) {
    String fname(filename);
    int dotIndex = fname.lastIndexOf('.');
    if(dotIndex != -1) {
        return fname.substring(0, dotIndex).toInt();
    }
    return 0;
}

// Function to find the most recent or oldest file
String findFileByDate(fs::FS &fs, const char *dirname, bool findNewest) {
    File root = fs.open(dirname);
    if (!root || !root.isDirectory()) {
        Serial.println("Failed to open directory or not a directory");
        return String(""); // Return an empty string if there are issues
    }

    Serial.println("Looking files in SD card ...");
    File file = root.openNextFile();
    unsigned long oldestTimestamp = ULONG_MAX;
    unsigned long newestTimestamp = 0;
    String oldestFile;
    String newestFile;

    while (file) {
        if (!file.isDirectory()) {
            String filename = file.name();
            if (filename.endsWith(".txt")) {
                unsigned long timestamp = getTimestampFromFilename(filename.c_str());
                if (timestamp < oldestTimestamp) {
                    oldestTimestamp = timestamp;
                    oldestFile = filename; 
                    Serial.println("Oldest file:" + oldestFile);
                }
                if (timestamp > newestTimestamp) {
                    newestTimestamp = timestamp;
                    newestFile = filename;
                    Serial.println("Newest file:" + newestFile);
                }
            }
        }
        file = root.openNextFile();
    }

    root.close();

    // Return the file name based on whether the newest or oldest is requested
    return findNewest ? newestFile : oldestFile;
}


/*
* format of the string is: 
    'YYYY-MM-DDThh:mm:ss +/-xx:xx'
*/
DateTime fromTimestampStringToDatetime(const String &dtString){
    int year = dtString.substring(0, 4).toInt();
    int month = dtString.substring(5, 7).toInt();
    int day = dtString.substring(8, 10).toInt();
    int hour = dtString.substring(11, 13).toInt();
    int minute = dtString.substring(14, 16).toInt();
    int second = dtString.substring(17, 19).toInt();
    return DateTime(year, month, day, hour, minute, second);
}

// Function to check if a given year is a leap year
bool isLeapYear(int year) {
    return (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0));
}


// Get the Unix timestamp for this DateTime object
long long fromDatetimeToUnix(const DateTime &dt) {
    // Months days count from January to December, ignoring leap years
    static int monthDays[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    long long days = 0;

    // Count all days from years between 1970 and the given year exclusive
    for (int y = 1970; y < dt.year; y++) {
        days += 365 + isLeapYear(y);
    }

    // Count all days from months in the given year
    for (int m = 0; m < dt.month - 1; m++) {
        days += monthDays[m];
        if (m == 1 && isLeapYear(dt.year)) // February in leap year
            days += 1;
    }

    // Add days in the current month
    days += dt.day - 1;

    // Convert all days to seconds
    long long totalSeconds = days * 86400LL;
    totalSeconds += (dt.hours * 3600 + dt.minutes * 60 + dt.seconds);

    return totalSeconds;
}