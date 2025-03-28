#include <Arduino.h>

#include "alarmclock.h"
#include "setalarmts.h"
#include "fun.h"
#include "hwdefs.h"
#include "display.h"
#include "alarm.h"
#include "button.h"
#include "ldr.h"
#include "network.h"
#include "webserver.h"
#include "settings.h"
#include "mode.h"

static const char* ntpServer = "ntp.harry.thuis";
//static const char* ntpServer = "pool.ntp.org";
static const char* myTimezone = "CET-1CEST,M3.5.0/2,M10.5.0/3";

static int brightness = 5;

static AlarmClock alarmClock = AlarmClock();
static SetAlarmTs setAlarmTs = SetAlarmTs();
static Fun fun = Fun();
static Display *handlers[3] = {&alarmClock, &setAlarmTs, &fun};
static int mode = MODE_CLOCK;

Alarm alarmBuzzer = Alarm(BUZZER_PIN);

void ButtonHandler(Button::Id id, Button::Event event)
{
    int newMode = handlers[mode]->ButtonHandler(id, event);
    if (mode != newMode) {
        mode = newMode;
        handlers[mode]->Start();
    }
}

Button button1 = Button(BUTTON_LEFT_PIN, Button::Id::LEFT, ButtonHandler);
Button button2 = Button(BUTTON_SELECT_PIN, Button::Id::MID, ButtonHandler);
Button button3 = Button(BUTTON_RIGHT_PIN, Button::Id::RIGHT, ButtonHandler);

void setup()
{
    Serial.begin(115200);

    handlers[MODE_CLOCK] = &alarmClock;
    handlers[MODE_SET_ALARM_TS] = &setAlarmTs;

    LdrInit();

    alarmClock.init(brightness, CRGB::Red);

    NetworkInit();
    SettingsInit();
    WebserverInit(ButtonHandler);

    // Init and get the time
    configTime(0, 0, ntpServer);
    setenv("TZ", myTimezone, 1);
    tzset();

    // Wait for time sync
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        delay(100);
    }
}

#define PUBLISH_TIME 60000   // ms
#define SAMPLE_TIME  1000    // ms

void ReadLdr()
{
    static int count = 0;
    int val;

    if (LdrRead(&val, SAMPLE_TIME)) {
        brightness = val / 50;
        if (brightness > MAX_BRIGHTNESS) {
            brightness = MAX_BRIGHTNESS;
        } else if (brightness < 1) {
            brightness = 1;
        }

        handlers[mode]->SetBrightness(brightness);
        alarmClock.SetColor(brightness);    // Adpat the color for this brightness

        if (count++ >= PUBLISH_TIME / SAMPLE_TIME) {
            count = 0;
            NetworkPublishLdr(val);
            NetworkPublishBrightness(brightness);
        }
    }
}

static void Monitor()
{
    char c = Serial.read();
    if (c > 0x20 && c < 0x80) {
        if (c == 'a') {
            alarmBuzzer.trigger();
        }

        if (c == 'p') {
            ESP.restart();
        }
    }
}

void loop()
{
    static uint32_t uptime = 0;
    static int dispTime = -1;
    struct tm timeinfo;
    bool invert;

    NetworkTick();
    WebserverTick();

    if (alarmBuzzer.tick(invert)) {
        alarmClock.Invert(invert);
    }
    button1.Tick();
    button2.Tick();
    button3.Tick();
    ReadLdr();

    if (getLocalTime(&timeinfo)) {
       if (dispTime != ((timeinfo.tm_hour * 60) + timeinfo.tm_min)) {
            uptime++;
            dispTime = (timeinfo.tm_hour * 60) + timeinfo.tm_min;
            SettingsSetTimes(dispTime, uptime);

            if (SettingsGetAlarmActive() && (dispTime == SettingsGetWakeupTime())) {
                alarmBuzzer.trigger();
            }
            alarmClock.SetTime(timeinfo.tm_hour, timeinfo.tm_min);
        }
    }

    if (!handlers[mode]->Tick()) {
        mode = MODE_CLOCK;
        handlers[mode]->Start();
    }
    Monitor();
}
