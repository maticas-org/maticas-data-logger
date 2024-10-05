#ifndef CONNECTION_EVENT_MANAGER_H
#define CONNECTION_EVENT_MANAGER_H


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


#include "CustomUtils.h"
#include "Event.h"
#include "Subscriber.h"
#include "EventManager.h"
#include "ApiClient.h"

#define MAX_MEASUREMENTS 3
#define MAX_EXCESS_EVENTS 3

class ConnectionEventManager : public EventManager, public Subscriber{
private:
    Event firstConnectionEvent;
    Event lastConnectionEvent;
    WiFiClient client;
    ApiClient apiClient = ApiClient(client);


public:
    int measurementEventsCount = 0;
    Event measurementEvents[MAX_MEASUREMENTS + MAX_EXCESS_EVENTS];

    ConnectionEventManager() {
        firstConnectionEvent = Event(CONNECTION_EVENT, SERVICE_UNAVAILABLE_STATUS, "", "{\"error\":\"No connection events yet\"}");
        lastConnectionEvent = firstConnectionEvent;
        for (int i = 0; i < MAX_MEASUREMENTS + MAX_EXCESS_EVENTS; i++){
            measurementEvents[i] = Event();
        }
    }

    Event getFirstEvent() {
        return firstConnectionEvent;
    }

    //------------------------ Event Manager Interface ------------------------
    void notify() override {
        // Run business logic
        main();
        Serial.println("ConnectionEventManager notifying subscribers...");

        // Notify subscribers
        for (int i = 0; i < number_of_subs; i++){
            subscribers_[i]->update(lastConnectionEvent);
        }
    }

    void main() override {
        Serial.println("\nConnectionEventManager running business logic...");

        if (lastConnectionEvent.getStatusCode() >= 500){
            Serial.println("Connection status code is 500 or greater ");
            connect();
        } else if (lastConnectionEvent.getStatusCode() == SERVICE_UNAVAILABLE_STATUS) {
            Serial.println("Connection status code is " + String(SERVICE_UNAVAILABLE_STATUS));
            connect(true);
        } else if (lastConnectionEvent.getStatusCode() == OK_STATUS) {
            Serial.println("Connection status code is " + String(OK_STATUS));
            lastConnectionEvent = check_connection();
        } else {
            String message = "Unhandled connection status code: " + String(lastConnectionEvent.getStatusCode());
            Serial.println(message);
            throw std::runtime_error(message.c_str());
        }
    }


    /*
    * Sends the pending data to the server
    */
    void sendMemAllocatedData(){
        Serial.println("There are " + String(measurementEventsCount) + " pending.");
        Serial.println("Sending pending [allocated in mem] data...");
        int* statusCodes = apiClient.sendEvents(measurementEvents, measurementEventsCount % (MAX_MEASUREMENTS + MAX_EXCESS_EVENTS));

        int currentCount = measurementEventsCount % (MAX_MEASUREMENTS + MAX_EXCESS_EVENTS);  // This ensures we loop correctly if the count exceeds the buffer size
        int readIndex = 0, writeIndex = 0;

        // Iterate through all events that have been attempted to send
        while (readIndex < currentCount) {
            if (statusCodes[readIndex] == OK_STATUS || statusCodes[readIndex] == CREATED_STATUS) {
                // Event was sent successfully, do not copy it to the write index
                measurementEvents[readIndex] = Event();  // Optional: Reset the event to clear data
            } else {
                // Event was not sent successfully, needs to be retained
                if (writeIndex != readIndex) {
                    // Only copy if readIndex has surpassed writeIndex
                    measurementEvents[writeIndex] = measurementEvents[readIndex];
                }
                writeIndex++;
            }
            readIndex++;
        }

        // Update the count of events in the buffer
        measurementEventsCount = writeIndex;

        // Clear any old events that are beyond the new count
        while (writeIndex < currentCount) {
            measurementEvents[writeIndex++] = Event();  // Reset the event to clear data
        }
    }

    /*
    * This function takes the passed events array, which should be an empty array,
    * and fills it with the excess events, this is usually used for avoiding filling the
    * excessEvents array. And is used later for the SD card to store the events.
    */
    bool returnExcessEvents(Event* events, int size) {
        if (measurementEventsCount < (MAX_MEASUREMENTS + MAX_EVENTS_PER_FILE - 1)) {
            Serial.println("Seeking excess events...");
            Serial.printf("Not enough events to return. Current count: %d\n", measurementEventsCount);
            return false;
        }

        // Calculate the actual number of events to return, should not exceed the available count or the size of the passed array
        int eventsToReturn = min(size, measurementEventsCount);

        // Calculate the start index for the oldest events in the circular buffer
        int startIdx = measurementEventsCount % (MAX_MEASUREMENTS + MAX_EXCESS_EVENTS);

        for (int i = 0; i < eventsToReturn; i++) {
            int index = (startIdx + i) % (MAX_MEASUREMENTS + MAX_EXCESS_EVENTS);
            events[i] = measurementEvents[index];
            measurementEvents[index] = Event(); // Reset the spot in the circular buffer
        }

        // Decrement the measurementEventsCount by the number of events returned
        measurementEventsCount -= eventsToReturn;

        // Compact the array to remove gaps if necessary
        int remainingEvents = measurementEventsCount;
        for (int i = 0; i < remainingEvents; i++) {
            int fromIndex = (startIdx + eventsToReturn + i) % (MAX_MEASUREMENTS + MAX_EXCESS_EVENTS);
            int toIndex = (startIdx + i) % (MAX_MEASUREMENTS + MAX_EXCESS_EVENTS);
            measurementEvents[toIndex] = measurementEvents[fromIndex];
            if (toIndex != fromIndex) {
                measurementEvents[fromIndex] = Event(); // Clear the vacated spot
            }
        }

        return true;
    }


    //------------------------ Subscriber Interface ------------------------
    void update(const Event* events, int size) override {

        // make sure size is not greater than MAX_MEASUREMENTS
        if (size > MAX_MEASUREMENTS){
            Serial.printf("WARNING: Size of events array is greater (%d) than MAX_MEASUREMENTS. Truncating to %d\n", size, MAX_MEASUREMENTS);
            size = MAX_MEASUREMENTS;
        }

        // if the first event is not a measurement event then 
        // can be assumed that the incoming array is an error
        if (events[0].getType() != MEASUREMENT_EVENT){
            Serial.println("First event is not a measurement event. Ignoring incoming events...");
            return;
        }

        Serial.printf("ConnectionEventManager received an array of %d events...\n", size);
        int* statusCodes = apiClient.sendEvents(events, size);

        
        // add the remainig events to the measurementEvents array (the ones that were not sent successfully)
        // if the measurementEvents array is full overwrite like in a circular buffer

        for (int i = 0; i < size; i++){
            
            // if the data was sent successfully, do not add it to the measurementEvents array
            if (statusCodes[i] == OK_STATUS || statusCodes[i] == CREATED_STATUS){
                continue;
            }
            measurementEvents[measurementEventsCount % (MAX_MEASUREMENTS + MAX_EXCESS_EVENTS)] = events[i];
            measurementEventsCount++;
        }

        logMemoryUsage();
    }

    /*
    *  This function is used to send loaded events from the SD card to the server
    *  It replaces the events that were sent successfully with empty events, so that the 
    *  events that were not sent can be stored back in the SD card, and the ones that were sent
    *  are ommited because they are EMPTY events.
    */
    bool updateFromLoadedEvents(Event events[], int size) {
        Serial.printf("ConnectionEventManager received an array from SD of %d events...\n", size);

        // if the first event is not a measurement event then
        // can be assumed that the incoming array is an error
        if (events[0].getType() != MEASUREMENT_EVENT){
            Serial.println("First event is not a measurement event. Ignoring incoming events...");
            return true;
        }

        // try to send the events
        int* statusCodes = apiClient.sendEvents(events, size);
        bool allSent = true;

        // replace the events that were sent successfully with empty events
        for (int i = 0; i < size; i++){
            if (statusCodes[i] == OK_STATUS || statusCodes[i] == CREATED_STATUS){
                events[i] = Event();
            }else{
                allSent = false;
            }
        }

        return allSent;
    }

    //------------------------ Business Logic ------------------------
    void connect(bool doReconnect = false) {

        // Disconnect if already connected
        WiFi.disconnect();

        // Connect to WiFi hotspot if not already connected
        if (!WiFi.isConnected() || lastConnectionEvent.getType() == -1) {
            Serial.println("Connecting to hotspot...");
            WiFi.begin(MY_SSID, MY_PASSWORD);

            // Wait up to 600 seconds (10 minutes) for connection to succeed
            for (int count = 0; count < 600; count++) {
                if (WiFi.status() == WL_CONNECTED) {
                    Serial.println("Connected to WiFi");
                    Serial.print("IP Address: ");
                    Serial.println(WiFi.localIP());
                    lastConnectionEvent = Event(CONNECTION_EVENT, OK_STATUS, "", "{\"error\":\"Succesfully connected to WiFi\"}"); 
                    return;
                }
                Serial.print(".");
                delay(1000);
            }

            // If connection fails, set the event accordingly
            lastConnectionEvent = Event(CONNECTION_EVENT, SERVICE_UNAVAILABLE_STATUS, "", "{\"error\":\"Connection failed\"}");
        } else {
            // Already connected
            Serial.println("Already connected");
            if (doReconnect) {
                Serial.println("Reconnecting...");
                reconnect();
            }
        }
    }

    void reconnect() {
        // Disconnect and reconnect to WiFi
        WiFi.disconnect();
        delay(1000);
        return connect(false);
    }

    Event check_connection(){
        // Check connection
        if (!client.connect("www.google.com", 80)) {
            Serial.println("Connection failed");
            return Event(CONNECTION_EVENT, SERVICE_UNAVAILABLE_STATUS, "", "{\"error\":\"Connection failed\"}");
        } else {
            Serial.println("Connection successful");
            return Event(CONNECTION_EVENT, OK_STATUS, "", "{\"error\":\"Connection successful\"}");
        }
    }

};



#endif // CONNECTION_EVENT_MANAGER_H
