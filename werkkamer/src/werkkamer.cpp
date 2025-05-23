#include <Arduino.h>

#include "webserver.h"
#include "network.h"
#include "sensor.h"

void setup()
{
    Serial.begin(115200);
    Serial.println("\n\nStart werkkamer sensor");

    NetworkInit();
    WebServerInit();
    SensorInit();

    delay(1000);
    Serial.println("Setup complete");
}


void loop()
{
    static unsigned long previousMillis;
    unsigned long currentMillis = millis();

    NetworkLoop();
    WebServerLoop();

    // Update values every 5 minutes
    if (currentMillis - previousMillis >= 1000 * 60 * 5) {
        previousMillis = currentMillis;

        NetworkPublish(SensorTemperature(), SensorHumidity(), SensorPressure(), NetworkSignalStrength());
    }
}
