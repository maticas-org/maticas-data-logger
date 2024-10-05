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
#include "EventManager.h"
#include "Subscriber.h"
#include "secrets.h"

#define RESTART_INTERVAL_SECS 10800 // 3 hours

static DS3231 RTC;

class TimeEventManager : public EventManager {
private:
    int updateIntervalSecs;
    int startTimeUnix;

public:
    TimeEventManager(int defaultTimeUpdateIntervalSecs) {
        RTC.begin();
        RTC.setHourMode(CLOCK_H24);
        //RTC.setDay(19);
        //RTC.setMonth(4);
        //RTC.setYear(2024);
        //RTC.setHours(18);
        //RTC.setMinutes(8);
        //RTC.setSeconds(0);

        firstEvent = Event();
        lastEvent = Event();

        updateIntervalSecs = defaultTimeUpdateIntervalSecs;
        startTimeUnix = RTC.getEpoch();

        Serial.begin(9600);
        Serial.println("Initialized TimeEventManager.");
    }

    void notify() override {
        main();

        Serial.println("\nTimeEventManager notifying subscribers...");
        for (int i = 0; i < number_of_subs; i++){
            subscribers_[i]->update(lastEvent);
        }
    }

    void checkForRestart() {
        //if the distance in time between the current time and the first event's timestamp
        //is than the RESTART_INTERVAL_SECS, then restart the device
        int currentTimeUnix = RTC.getEpoch();
        int timeDiff = currentTimeUnix - startTimeUnix;

        if (timeDiff > RESTART_INTERVAL_SECS) {
            Serial.println("TimeEventManager: Restarting device due to time interval.");
            ESP.restart();
        }
    }

    void main() override {
        Serial.println("\nTimeEventManager running business logic...");
        checkForRestart();

        String data = "";

        try {
            
            int year = RTC.getYear();
            int month = RTC.getMonth();
            int day = RTC.getDay();
            int hour = RTC.getHours();
            int minute = RTC.getMinutes();
            int second = RTC.getSeconds();

            // Create timestamp add zero padding
            const String monthStr = (month < 10) ? "0" + String(month) : String(month);
            const String dayStr = (day < 10) ? "0" + String(day) : String(day);
            const String hourStr = (hour < 10) ? "0" + String(hour) : String(hour);
            const String minuteStr = (minute < 10) ? "0" + String(minute) : String(minute);
            const String secondStr = (second < 10) ? "0" + String(second) : String(second);

            String timestamp = String(year) + "-" + monthStr + "-" + dayStr + "T" +
                              hourStr + ":" + minuteStr + ":" + secondStr + " " + TZ;

            Event timeEvent(TIME_EVENT, OK_STATUS, timestamp, data);

            firstEvent = (firstEvent.getType() == UNKNOWN_EVENT) ? timeEvent : firstEvent;
            lastEvent = timeEvent;

            Serial.print("\tupdate: ");
            Serial.println(lastEvent.getTimestamp());
        } catch (const std::exception& e1) {
            Serial.print("Error on RTC read: ");
            Serial.println(e1.what());

            // Work around RTC failure
            try {
                Event timeEvent = workAroundRtcFailure();

            } catch (const std::exception& e2) {
                Serial.print("Error during work around RTC failure: ");
                Serial.println(e2.what());
            }
        }
    }

    long int getEpoch() {
        return RTC.getEpoch();
    }

    Event workAroundRtcFailure() {
        String data = "";
        String lastTimestamp = lastEvent.getTimestamp();

        String date = lastTimestamp.substring(0, 10);
        String time = lastTimestamp.substring(11);

        int year = date.substring(0, 4).toInt();
        int month = date.substring(5, 7).toInt();
        int day = date.substring(8, 10).toInt();

        int hour = time.substring(0, 2).toInt();
        int minute = time.substring(3, 5).toInt();
        int second = time.substring(6, 8).toInt();

        second += updateIntervalSecs;
        minute += second / 60;
        second %= 60;
        hour += minute / 60;
        minute %= 60;
        day += hour / 24;
        hour %= 24;
        month += day / 30;
        day %= 30;
        year += month / 12;
        month %= 12;

        String newTimestamp = String(year) + "-" + String(month) + "-" + String(day) + " " +
                             String(hour) + ":" + String(minute) + ":" + String(second);

        Event timeEvent(TIME_EVENT, BUG_RESILIENCE_STATUS, newTimestamp, data);
        return timeEvent;
    }
};