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


#include "EventManager.h"
#include "TimeEventManager.h"
#include "ConnectionEventManager.h"

#include "CustomUtils.h"
#include "secrets.h"
#include "SensorAdapters.h"
#include "SensorsMicroService.h"

//SD card
bool sdCardInitialized = false;

//time in seconds
#define timeEventManagerFrequency 5              //5 seconds
#define sensorsMicroServiceFrequency 10*1        //2 minutes
#define connectionEventManagerFrequency 41*1    //10 minutes
#define sdStoreFrequency 60*2                    //11 minutes
#define sdLoadFrequency 60*1                     //12 minutes
#define LED 2

void updateEventManager(EventManager &eventManager, unsigned long &previousEventMillis, unsigned long &currentMillis, unsigned long eventFrequency);
void sendPendingAndStoreExcessEvents(ConnectionEventManager &connectioneventmanager, TimeEventManager &timeeventmanager);
void loadAndSendEvents(ConnectionEventManager &connectionEventManager, bool fromNewestToOldest = true);

void setup() {

  //initialize the serial port, the i2c bus, the spi bus
  Serial.begin(9600);
  delay(250);
  Wire.begin();
  delay(250);
  SPI.begin(SCK, MISO, MOSI, CS);
  delay(250);

  //initialize the LED
  pinMode(LED, OUTPUT);
  digitalWrite(LED, HIGH);
  delay(1000);
  digitalWrite(LED, LOW);

  // start the SD card
  if(!SD.begin()){
      Serial.println("Card Mount Failed");
  }else{
      Serial.println("Card Mount Success");
      sdCardInitialized = true;
  }

  showAditionalSDCardInfo(SD);
}

void loop() {

  delay(100);
  TimeEventManager timeEventManager = TimeEventManager(timeEventManagerFrequency);
  ConnectionEventManager connectionEventManager;
  SensorsMicroService sensorsMicroService = SensorsMicroService();
  DHTAdapter dhtAdapter = DHTAdapter();
  LuxAndDLIAdapter luxAndDLIAdapter = LuxAndDLIAdapter(connectionEventManagerFrequency);

  sensorsMicroService.AddSensor(&dhtAdapter);
  sensorsMicroService.AddSensor(&luxAndDLIAdapter);

  timeEventManager.subscribe(&sensorsMicroService);
  sensorsMicroService.subscribe(&connectionEventManager);
  delay(100);
  logMemoryUsage();
  connectionEventManager.main();
  logMemoryUsage();

  unsigned long previousTimeEventMillis = 0;
  unsigned long previousConnectionEventMillis = 0;
  unsigned long previousSensorsMicroServiceMillis = 0;
  unsigned long sdStoreMillis = 0;
  unsigned long sdLoadMillis = 0;
  unsigned long currentMillis = millis();

  while (true) {
    currentMillis = millis();
    
    //update the time
    updateEventManager(timeEventManager, previousTimeEventMillis, currentMillis, timeEventManagerFrequency);
    
    //update the connection - this tries to connect to the wifi and check if the connection
    //is still alive
    updateEventManager(connectionEventManager, previousConnectionEventMillis, currentMillis, connectionEventManagerFrequency);

    //update the time
    updateEventManager(timeEventManager, previousTimeEventMillis, currentMillis, timeEventManagerFrequency);

    //update the measurements from sensors - this tries to get the measurements from
    //the sensors and notify the subscribers
    updateEventManager(sensorsMicroService, previousSensorsMicroServiceMillis, currentMillis, sensorsMicroServiceFrequency);

    //update the time
    updateEventManager(timeEventManager, previousTimeEventMillis, currentMillis, timeEventManagerFrequency);

    //send pending events and store excess events
    sendPendingAndStoreExcessEvents(connectionEventManager, timeEventManager);

    //load and send events from the SD card
    loadAndSendEvents(connectionEventManager);

    //show memory usage
    logMemoryUsage();
    delay(2500);
  }

}



/*
* Update the event manager
*/
void updateEventManager(EventManager &eventManager,
                        unsigned long &previousEventMillis,
                        unsigned long &currentMillis,
                        unsigned long eventFrequency) {
  if (currentMillis - previousEventMillis >= eventFrequency * 1000) {
    eventManager.notify();
    previousEventMillis = currentMillis;
  }
  currentMillis = millis();
}

void sendPendingAndStoreExcessEvents(ConnectionEventManager &connectionEventManager, 
                                     TimeEventManager &timeEventManager){
  
  String epoch = String(abs(timeEventManager.getEpoch()));
  String fileName = "/" + epoch + ".txt";

  // try to send any pending events
  connectionEventManager.sendMemAllocatedData();

  // store excess pending events in the SD card
  Event excessEvents[MAX_EVENTS_PER_FILE];
  bool availableData = connectionEventManager.returnExcessEvents(excessEvents, MAX_EVENTS_PER_FILE);
  if (availableData) {
    storeEvents(SD, excessEvents, MAX_EVENTS_PER_FILE, fileName.c_str());
  }
}

void loadAndSendEvents(ConnectionEventManager &connectionEventManager, 
                       bool fromNewestToOldest) {
  Event loadedEvents[MAX_EVENTS_PER_FILE];
  String filename = "/" + findFileByDate(SD, "/", fromNewestToOldest);
  bool loadedData = loadEvents(SD, loadedEvents, MAX_EVENTS_PER_FILE, filename.c_str());

  if (loadedData) {
    bool allSent = connectionEventManager.updateFromLoadedEvents(loadedEvents, MAX_EVENTS_PER_FILE);
    //if all events were sent, delete the file, otherwise store the remaining events
    if (allSent) {
      deleteFile(SD, filename.c_str());
    }else{
      storeEvents(SD, loadedEvents, MAX_EVENTS_PER_FILE, filename.c_str());
    }
  }
}