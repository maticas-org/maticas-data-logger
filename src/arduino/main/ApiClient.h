#ifndef API_CLIENT_H
#define API_CLIENT_H

#include <WiFi.h> // Include the appropriate WiFi library for your board
#include <Arduino.h>
#include <ArduinoHttpClient.h>
#include "secrets.h"
#include "Event.h"


#define MAX_MEASUREMENTS 30
class ApiClient {

    private:
        HttpClient http_;
        int last_results[MAX_MEASUREMENTS];

    public:

        ApiClient(WiFiClient &client) : http_(client, API_URL, API_PORT) {
            reset_last_results();
            http_.connectionKeepAlive();
            //http_.setHttpResponseTimeout(HTTP_TIMEOUT);
        }

        int sendEvent(const Event& event) {
            
            //if event is the empty event, return
            if (event.getType() == UNKNOWN_EVENT) {
                return OK_STATUS;
            }

            String contentType = "application/json";
            String postData = event.getData();
            int dataLength = postData.length() + 1;

            // Send event to server
            Serial.println("Sending event to server...");
            Serial.println("Length: " + String(dataLength));

            int connStatus = http_.connect(API_URL.c_str(), API_PORT);
            Serial.println("Connect result: " + String(connStatus));
            http_.connectionKeepAlive();

            http_.beginRequest();
            http_.post(API_ENDPOINT);
            http_.sendHeader("Authorization", API_TOKEN);
            http_.sendHeader("Content-Type", contentType);
            http_.sendHeader("Content-Length", dataLength);
            http_.beginBody();
            http_.print(postData);
            http_.endRequest();

            int statusCode = http_.responseStatusCode();
            //String response = http_.responseBody();

            if (statusCode == HTTP_SUCCESS){
                Serial.println("Success Sending event");
                statusCode = http_.responseStatusCode();
                
                if (statusCode >= 0) {
                    String response = http_.responseBody();
                    Serial.println("Status code: " + String(statusCode));
                    Serial.println("Body length: " + http_.contentLength());
                }
            }

            Serial.println("Status code: " + String(statusCode));
            //Serial.println("Response: " + response);

            if (statusCode == OK_STATUS || statusCode == CREATED_STATUS) {
                Serial.println("Event sent successfully");
            } else if (statusCode == HTTP_ERROR_INVALID_RESPONSE) {
                Serial.println("Seems like server got the event, but it didn't respond properly");
            }else{
                Serial.println("Failed to send event");
            }

            http_.stop(); 
            return statusCode;
        }


        int* sendEvents(const Event* events, int n) {
            // Send events to server
            Serial.println("Sending events to server...");
            reset_last_results();

            for (int i = 0; i < n; i++){
                //if event is different from measurement, omit it
                if (events[i].getType() != MEASUREMENT_EVENT) {
                    last_results[i] = OK_STATUS;
                    continue;
                }
                last_results[i] = sendEvent(events[i]);

                //if the result was HTTP_ERROR_INVALID_RESPONSE,
                //it means that the server got the event, but it didn't respond properly
                if (last_results[i] == HTTP_ERROR_INVALID_RESPONSE) {
                    last_results[i] = OK_STATUS;
                }
            }

            return last_results;
        }

        int* getLastResults() {
            return last_results;
        }

        void reset_last_results() {
            for (int i = 0; i < MAX_MEASUREMENTS; i++) {
                last_results[i] = -1;
            }
        }
};

#endif // API_CLIENT_H