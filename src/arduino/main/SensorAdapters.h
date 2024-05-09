#include "DHT.h"
#include <BH1750.h>

#include "secrets.h"
#include "Event.h"
#include "Adapter.h"
#include "CustomUtils.h"

#define DHTPIN 33
#define DHTTYPE DHT11
#define MAX_RETRIES 5

DHT dht = DHT(DHTPIN, DHTTYPE);
BH1750 lightMeter;
const double lux2parConversionFactor = 0.0185; // Conversion factor from lux to PAR

//--------------------HELPER FUNCTIONS--------------------

float average(float values[], int size)
{
    if (size == 0)
    {
        return 0;
    }

    float sum = 0;
    for (int i = 0; i < size; i++)
    {
        sum += values[i];
    }
    return sum / size;
}

bool isvalid(float value)
{
    return !isnan(value) && !isinf(value) && value >= 0;
}

/***************** Vapour Pressur Deficit in kPa ********************/
float calculateSaturationVaporPressure(float temperature)
{
    return 0.6108 * exp((17.27 * temperature) / (temperature + 237.3));
}

float calculateVPD(float temperature, float relativeHumidity)
{
    float saturationVaporPressure = calculateSaturationVaporPressure(temperature);
    return (1.0 - (relativeHumidity / 100.0)) * saturationVaporPressure; // kPa
}

/***************** Dew point in °C ********************/
// Magnus coefficients
const float a = 17.625;
const float b = 243.04;

// Function to calculate the Magnus-Tetens intermediate value alpha
float calculateAlpha(float temperature, float relativeHumidity)
{
    return log(relativeHumidity / 100.0) + (a * temperature) / (b + temperature);
}

// Function to calculate Dew Point (Ts) using the Magnus-Tetens formula
float calculateDewPoint(float temperature, float relativeHumidity)
{
    float alpha = calculateAlpha(temperature, relativeHumidity);
    return (b * alpha) / (a - alpha);
}

//--------------------ADAPTERS--------------------
class DHTAdapter : public Adapter
{
private:
    Event (*specificRequestFunc)() = nullptr;
    int maxRetries;
    int retryDelay;
    long int lastRequestTimestamp = -1;

public:
    DHTAdapter(int maxRetries = 5, int retryDelay = 2100) : Adapter()
    {
        dht.begin();
        this->maxRetries = maxRetries;
        this->retryDelay = retryDelay;
    }

    Event request(Event timeEvent)
    {
        Serial.println("DHTAdapter handling request...");
        return specificRequest(timeEvent);
    }

    Event specificRequest(Event timeEvent)
    {
        // Run specific request function if it is set
        // Otherwise, run the default request function
        if (specificRequestFunc != nullptr)
        {
            return specificRequestFunc();
        }
        else
        {
            return default_request(timeEvent);
        }
    }

    Event default_request(Event timeEvent)
    {
        float temperatureArray[maxRetries];
        float humidityArray[maxRetries];

        try
        {
            dht.begin();
            int validReadings = 0;

            for (int i = 0; i < maxRetries; i++)
            {
                float humidity = dht.readHumidity();
                float temperature = dht.readTemperature();
                Serial.println("Humidity: " + String(humidity) + " %\t Temperature: " + String(temperature) + " *C ");

                if (isvalid(humidity) && isvalid(temperature))
                {
                    temperatureArray[i] = temperature;
                    humidityArray[i] = humidity;
                    validReadings++;
                }

                delay(retryDelay);
            }

            if (validReadings == 0)
            {
                Serial.println("No valid data.");
                return Event(MEASUREMENT_EVENT, INTERNAL_SERVER_ERROR, "", "No valid data");
            }

            float temperature = average(temperatureArray, validReadings);
            float humidity = average(humidityArray, validReadings);
            float vpd = calculateVPD(temperature, humidity);
            float dewPoint = calculateDewPoint(temperature, humidity);

            // Format data in JSON-like string as a list of measurements
            const String timestamp = String(timeEvent.getTimestamp());
            const String data1 = "{\"variable\": " + TEMPERATURE_UIID + ", \"value\": " + String(temperature) + ", \"crop\": " + CROP_UIID + ", \"datetime\": \"" + timestamp + "\"}";
            const String data2 = "{\"variable\": " + HUMIDITY_UIID + ", \"value\": " + String(humidity) + ", \"crop\": " + CROP_UIID + ", \"datetime\": \"" + timestamp + "\"}";
            const String data3 = "{\"variable\": " + VPD_UIID + ", \"value\": " + String(vpd) + ", \"crop\": " + CROP_UIID + ", \"datetime\": \"" + timestamp + "\"}";
            const String data4 = "{\"variable\": " + DEWPOINT_UIID + ", \"value\": " + String(dewPoint) + ", \"crop\": " + CROP_UIID + ", \"datetime\": \"" + timestamp + "\"}";
            const String data = "[" + data1 + ", " + data2 + ", " + data3 + ", " + data4 + "]";
            return Event(MEASUREMENT_EVENT, OK_STATUS, timestamp, data);
        }
        catch (std::exception e)
        {
            // Print the exception and the message
            Serial.println(e.what());
            return Event(MEASUREMENT_EVENT, INTERNAL_SERVER_ERROR, "", "DHT sensor not found");
        }
    }
};

/*
 * Adapter for the BH1750 light sensor
 */

class LuxAndDLIAdapter : public Adapter
{

private:
    Event (*specificRequestFunc)() = nullptr;
    int maxRetries;
    int retryDelay;
    int measurement_interval; // in seconds

    // used for DLI calculation
    DateTime prevDateTime = DateTime(0, 0, 0, 0, 0, 0);
    double lastLux = 0;
    double dailyLightSum = 0;
    long int lastRequestTimestamp = -1;

public:
    LuxAndDLIAdapter(int measurement_interval,
                     int maxRetries = 5,
                     int retryDelay = 250) : Adapter()
    {
        // Initialize the I2C bus (BH1750 library doesn't do this automatically)
        Wire.begin();
        // Initialize BH1750
        if (!lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE))
        {
            Serial.println(F("Error initializing BH1750 sensor!"));
        }

        // wait a bit and retry
        delay(1000);
        if (!lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE))
        {
            Serial.println(F("Error initializing BH1750 sensor!"));
        }

        this->maxRetries = maxRetries;
        this->retryDelay = retryDelay;
        this->measurement_interval = measurement_interval;
    }

    Event request(Event timeEvent)
    {
        Serial.println("DHTAdapter handling request...");
        return specificRequest(timeEvent);
    }

    Event specificRequest(Event timeEvent)
    {
        // Run specific request function if it is set
        // Otherwise, run the default request function
        if (specificRequestFunc != nullptr)
        {
            return specificRequestFunc();
        }
        else
        {
            return default_request(timeEvent);
        }
    }

    Event default_request(Event timeEvent)
    {

        DateTime currentDateTime = fromTimestampStringToDatetime(timeEvent.getTimestamp());
        float luxArray[maxRetries];

        try
        {
            int validReadings = 0;

            for (int i = 0; i < maxRetries; i++)
            {
                float lux = lightMeter.readLightLevel();
                Serial.println("Lux: " + String(lux) + " lx");

                if (isvalid(lux))
                {
                    luxArray[i] = lux;
                    validReadings++;
                }

                delay(retryDelay);
            }

            if (validReadings == 0)
            {
                Serial.println("No valid data.");
                return Event(MEASUREMENT_EVENT, INTERNAL_SERVER_ERROR, "", "No valid data");
            }

            double lux = average(luxArray, validReadings);
            double par = lux * lux2parConversionFactor; // Convert lux to PAR (need definition of lux2parConversionFactor)

            // Update DLI using the trapezoidal rule
            if (lastLux >= 0)
            { // Check if a previous measurement exists
                // Calculate the average of the last and current lux readings
                double averageLux = (lastLux + lux) / 2;
                double timeDifference = 0;

                // If the previous timestamp is was the default value use as time difference the measurement interval
                // Otherwise calculate the time difference between the current and previous timestamp
                if (prevDateTime.year == 0){
                    timeDifference = measurement_interval;
                }else{
                    timeDifference = fromDatetimeToUnix(currentDateTime) - fromDatetimeToUnix(prevDateTime);
                }

                // Check if the DLI should be reset, if not, update the DLI
                if (!resetDLI(currentDateTime)){
                    dailyLightSum += (averageLux * timeDifference * lux2parConversionFactor) / 3600.0; // Convert seconds to hours for DLI calculation
                }
            }

            // Update the lastLux for the next measurement
            lastLux = lux;

            // Update the previous date time for the next measurement
            prevDateTime = currentDateTime;

            // Format data in JSON-like string as a list of measurements
            const String timestamp = String(timeEvent.getTimestamp());
            const String data1 = "{\"variable\": " + LUX_UIID + ", \"value\": " + String(lux) + ", \"crop\": " + CROP_UIID + ", \"datetime\": \"" + timestamp + "\"}";
            const String data2 = "{\"variable\": " + DLI_UIID + ", \"value\": " + String(dailyLightSum) + ", \"crop\": " + CROP_UIID + ", \"datetime\": \"" + timestamp + "\"}";
            const String data = "[" + data1 + ", " + data2 + "]";
            return Event(MEASUREMENT_EVENT, OK_STATUS, timestamp, data);
        }
        catch (std::exception e)
        {
            // Print the exception and the message
            Serial.println(e.what());
            return Event(MEASUREMENT_EVENT, INTERNAL_SERVER_ERROR, "", "BH1750 sensor not found");
        }
    }

    //checks whether the DLI should be reset
    bool resetDLI(const DateTime &currentDateTime){

        if (prevDateTime.year != currentDateTime.year || prevDateTime.month != currentDateTime.month || prevDateTime.day != currentDateTime.day)
        {
            prevDateTime = currentDateTime;
            dailyLightSum = 0;
            return true;
        }
        return false;
    }
};