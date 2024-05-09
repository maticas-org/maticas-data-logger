#ifndef EVENT_H
#define EVENT_H

#include <Arduino.h>

// Define event types
#define TIME_EVENT 0
#define CONNECTION_EVENT 1
#define MEASUREMENT_EVENT 2
#define RAM_EVENT 3
#define UNKNOWN_EVENT -1

// Define event status codes
#define TO_BE_SENT_STATUS 0
#define OK_STATUS 200
#define CREATED_STATUS 201
#define BUG_RESILIENCE_STATUS 299
#define BAD_REQUEST_STATUS 400
#define UNAUTHORIZED_STATUS 401
#define FORBIDDEN_STATUS 403
#define NOT_FOUND_STATUS 404
#define INTERNAL_SERVER_ERROR 500
#define NOT_IMPLEMENTED_STATUS 501
#define SERVICE_UNAVAILABLE_STATUS 503

class Event {
public:
    int timesSent = 0;
    int lastSentStatus = TO_BE_SENT_STATUS;

    Event();
    Event(int type, int statusCode, const String& timestamp, const String& data);
    Event(String eventString);
    
    int getType() const;
    int getStatusCode() const;
    String getTimestamp() const;
    String getData() const;
    void setTimestamp(const String& timestamp);
    String toString() const;

    // Comparison operators
    bool operator==(const Event& other) const;
    bool operator!=(const Event& other) const;

private:
    int type_;
    int statusCode_;
    String timestamp_;
    String data_;
};

struct EventArray {
    Event* events;
    int size;
    
    EventArray(int numEvents) : size(numEvents) {
        events = new Event[numEvents];
        for (int i = 0; i < numEvents; i++) {
            events[i] = Event();
        }
    }
    
    ~EventArray() {
        delete[] events;
    }

    // Prevent copying to avoid double deletion
    EventArray(const EventArray&) = delete;
    EventArray& operator=(const EventArray&) = delete;

    // Optionally, support move semantics
    EventArray(EventArray&& other) noexcept : events(other.events), size(other.size) {
        other.events = nullptr;
        other.size = 0;
    }

    EventArray& operator=(EventArray&& other) noexcept {
        if (this != &other) {
            delete[] events;
            events = other.events;
            size = other.size;
            other.events = nullptr;
            other.size = 0;
        }
        return *this;
    }

    // Overload the [] operator
    Event& operator[](size_t index) {
        if (index >= size) {
            // Handle the error, for example, throw an exception
            // For Arduino, you might just halt or reset since exceptions aren't generally used
            Serial.println("Index out of bounds");
            // Implement a safe response, like halting or returning a static dummy Event
            static Event dummy;
            return dummy;
        }
        return events[index];
    }
};


#endif // EVENT_H