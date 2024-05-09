#include "EventManager.h"

/*
* The EventManager class is responsible for managing events and notifying subscribers.
*/
EventManager::EventManager() {
    firstEvent = Event();
    lastEvent = Event();
    number_of_subs = 0;
}

Event EventManager::getFirstEvent() {
    return firstEvent;
}

void EventManager::subscribe(Subscriber* subscriber) {
    Serial.println("Adding subscriber...");
    // Add the subscriber to the list
    if (number_of_subs < MAX_NUMBER_OF_SUBSCRIBERS) {
        subscribers_[number_of_subs] = subscriber;
        number_of_subs++;
    }
    else {
        Serial.println("Error: Maximum number of subscribers reached");
    }
}

void EventManager::unsubscribe(Subscriber* subscriber) {
    Serial.println("Removing subscriber...");
    // Remove the subscriber from the list
    for (int i = 0; i < number_of_subs; i++) {
        if (subscribers_[i] == subscriber) {
            for (int j = i; j < number_of_subs - 1; j++) {
                subscribers_[j] = subscribers_[j + 1];
            }
            number_of_subs--;
            break;
        }
    }
}

void EventManager::notify() {
    for (int i = 0; i < number_of_subs; i++){
        subscribers_[i]->update(lastEvent);
    }
}

void EventManager::main() {
    Serial.println("Executing business logic...");
    // Some previous business logic...
    // ...
    Serial.println("Done! Notifying subscribers...");
    notify();
}
