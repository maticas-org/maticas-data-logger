#ifndef SUBSCRIBER_H
#define SUBSCRIBER_H

#include "Event.h"

class Subscriber {
public:
    Subscriber();
    virtual void update(const Event& event);
    virtual void update(const Event* events, int size);
    //virtual void update(const Event events[], int size);
};

#endif // SUBSCRIBER