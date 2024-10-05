#ifndef ADAPTER_H
#define ADAPTER_H

#include <Arduino.h>
#include "Target.h"
#include "Adaptee.h"
#include "Event.h"

class Adapter : public Target, public Adaptee {
public:
    void setSpecificRequest(Event (*specificRequestFunc)(Event timeEvent));
    Event request(Event timeEvent);

private:
    Event (*specificRequestFunc)(Event timeEvent);
};

#endif // ADAPTER_H
