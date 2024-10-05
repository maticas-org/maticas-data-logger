#ifndef TARGET_H
#define TARGET_H

#include <Arduino.h>
#include "Event.h"

class Target {
public:
    virtual Event request(Event timeEvent) = 0;
};

#endif // TARGET_H