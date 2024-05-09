#ifndef EVENT_MANAGER_H
#define EVENT_MANAGER_H

#include <Arduino.h>
#include "Event.h"
#include "Subscriber.h"

#define MAX_NUMBER_OF_SUBSCRIBERS 3

/*
* The EventManager class is responsible for managing events and notifying subscribers.
*/
class EventManager {
public:
    EventManager();
    Event getFirstEvent();
    void subscribe(Subscriber* subscriber);
    void unsubscribe(Subscriber* subscriber);
    virtual void notify();
    virtual void main();

protected:
    Event firstEvent;
    Event lastEvent;
    Subscriber* subscribers_[MAX_NUMBER_OF_SUBSCRIBERS];
    int number_of_subs;
};

#endif // EVENT_MANAGER_H
