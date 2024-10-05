#ifndef ADAPTEE_H
#define ADAPTEE_H

#include <Arduino.h>
#include "Event.h"

class Adaptee {
public:
    virtual Event specificRequest(Event timeEvent) = 0;
};

#endif // ADAPTEE_H