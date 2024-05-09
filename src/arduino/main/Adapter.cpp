#include "Adapter.h"
#include "Event.h"

void Adapter::setSpecificRequest(Event (*specificRequestFunc)(Event timeEvent)) {
    this->specificRequestFunc = specificRequestFunc;
}

Event Adapter::request(Event timeEvent) {
    return specificRequestFunc(timeEvent);
}
