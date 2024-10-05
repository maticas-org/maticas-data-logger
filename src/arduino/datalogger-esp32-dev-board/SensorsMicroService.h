#include "Event.h"
#include "EventManager.h"
#include "Subscriber.h"
#include "Adapter.h"

#define MAX_STORED_EVENTS 30 //maximum number of events to store in the microservice
#define MAX_SENSORS 10 //maximum number of sensors to store in the microservice

class SensorsMicroService : public EventManager, public Subscriber {

    private:
        Event last_time_event_;
        Event last_measurement_events_[MAX_STORED_EVENTS];
        Adapter* sensors_[MAX_SENSORS];
        
        int nmeasurement_events_;
        int sensors_count;

        Subscriber* subscribers_[MAX_NUMBER_OF_SUBSCRIBERS];
        int number_of_subs;

    public:
        SensorsMicroService(){
            sensors_count = 0;
            nmeasurement_events_ = 0;
            number_of_subs = 0;

            // Initialize last_measurement_events_ to empty events
            for (int i = 0; i < MAX_STORED_EVENTS; i++) {
                last_measurement_events_[i] = Event();
            }
        }

        //-------------------------------------------------------------
        //-------------------Event Manager Interface-------------------
        //-------------------------------------------------------------
        void main() override {
            Serial.println("\nSensorsMicroService running business logic...");

            // Iterate over the sensors and get the data
            for (int i = 0; i < MAX_SENSORS; i++) {
                if (i < sensors_count && sensors_[i] != nullptr) {
                    Event event = sensors_[i]->request(last_time_event_);

                    if (event.getStatusCode() == INTERNAL_SERVER_ERROR) {
                        Serial.println("SensorsMicroService got an error event: ");
                        Serial.print(event.toString());
                        continue;
                    }

                    // store the event
                    last_measurement_events_[i] = event; 

                    Serial.println("SensorsMicroService got event: ");
                    Serial.print(last_measurement_events_[i].toString());
                    nmeasurement_events_++;
                }
            }
        }


        void notify() override{
            main();
            Serial.println("\nSensorsMicroService notifying subscribers...");

            //iterate over the subscribers and notify them
            for (int i = 0; i < number_of_subs; i++){
                subscribers_[i]->update(last_measurement_events_, nmeasurement_events_); 
            }

            // Reset the last_measurement_events_ to empty events
            for (int i = 0; i < MAX_STORED_EVENTS; i++) {
                last_measurement_events_[i] = Event();
            }

            // Reset the number of measurement events
            nmeasurement_events_ = 0;
        }

        void subscribe(Subscriber* subscriber){
            Serial.println("\nSensorsMicroService subscribing...");
            if (number_of_subs < MAX_NUMBER_OF_SUBSCRIBERS){
                subscribers_[number_of_subs] = subscriber;
                number_of_subs++;
            }else{
                Serial.println("Max number of subscribers reached.");
            }
        }

        void unsubscribe(Subscriber* subscriber){
            Serial.println("\nSensorsMicroService unsubscribing...");   
            for (int i = 0; i < number_of_subs; i++){
                if (subscribers_[i] == subscriber){
                    for (int j = i; j < number_of_subs - 1; j++){
                        subscribers_[j] = subscribers_[j + 1];
                    }
                    number_of_subs--;
                    break;
                }
            }
        }

        //----------------------------------------------------------
        //-------------------Subscriber Interface-------------------
        //----------------------------------------------------------
        void update(const Event& event) override{
            Serial.println("\nSensorsMicroService got event: ");
            Serial.print(event.toString());

            if (event.getType() == TIME_EVENT){
                last_time_event_ = event;
            }else{
                Serial.println("\nSensorsMicroService got an event of type: ");
                Serial.print(event.getType());
                Serial.println("Which is unsuporrted by this microservice");
            }
        }
        
        
        //----------------------------------------------------
        //-------------------Business Logic-------------------
        //----------------------------------------------------
        const Event* getLastMeasurementEvent() const{
            return last_measurement_events_;
        }
        void AddSensor(Adapter* sensor) {
            if (sensors_count < MAX_SENSORS) {
                sensors_[sensors_count] = sensor; // Add sensor pointer to the array
                sensors_count++; // Increment the count of added sensors
            } else {
                Serial.println("Max number of sensors reached.");
            }
        }

        void RemoveSensor(Adapter* sensor) {
            // Find and remove the sensor
            for (int i = 0; i < sensors_count; i++) {
                if (sensors_[i] == sensor) {
                    for (int j = i; j < sensors_count - 1; j++) {
                        sensors_[j] = sensors_[j + 1]; // Shift sensors to fill the gap
                    }
                    sensors_count--; // Decrement the count of added sensors
                    break;
                }
            }
        }

};
    