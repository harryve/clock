#include <Arduino.h>
#include "settings.h"
#include "network.h"
#include "alarmclock.h"
#include "hwdefs.h"
#include "mode.h"

#include "display.h"

AlarmClock::AlarmClock()
{
    drawRequest = false;
}

void AlarmClock::init(int b, CRGB c)
{
    InitLeds();
    SetBrightness(b);
    color = c;
    Start();
}

void AlarmClock::Start()
{
    SetMinimumBrightness(0);
    drawRequest = true;
}

void AlarmClock::SetColor(int brightness)
{
    int notRed = (brightness * 250) / MAX_BRIGHTNESS;
    if (notRed > 250) {
        notRed = 250;
    }
    color = CRGB(255, notRed, notRed);
    drawRequest = true;
}

boolean AlarmClock::Tick()
{
    if (!drawRequest) {
        return true;
    }
    drawRequest = false;

    if (invert) {
        Fill(color);
    }
    else {
        FastLED.clear();
    }

    CRGB c;
    if (SettingsGetAlarmActive()) {
        c = invert ? CRGB::Black : CRGB::Red;
        SetLed(0, 0, c);
    }

    if (invert) {
        c = CRGB::Black;
    }
    else {
        c = color;
    }
    // 5 1 5 3 5 1 5
    // 0   6   14  20
    int x = 3;
    if (hours / 10 != 0) {
       DrawDigit(x + 0, 0, hours / 10, c);
    }
    DrawDigit(x + 7, 0, hours % 10, c);
    SetLed(x + 13, 2, c);
    SetLed(x + 13, 5, c);
    DrawDigit(x + 15, 0, minutes / 10, c);
    DrawDigit(x + 22, 0, minutes % 10, c);

    int netwerkErrors = NetworkGetErrors();
    c = invert ? CRGB::Black : CRGB::Red;
    for (int i = 0; i < 8; i++) {
        if (netwerkErrors & (1 << i)) {
            SetLed(31, i, c);
        }
    }
    FastLED.show();
    return true;
}

int AlarmClock::ButtonHandler(Button::Id id, Button::Event event)
{
    if (event == Button::Event::SHORT_PRESS) {
        switch (id) {
            case Button::Id::MID:
                SettingsToggleAlarmActive();
                drawRequest = true;
                break;

            case Button::Id::RIGHT:
                return MODE_SET_ALARM_TS;

            case Button::Id::LEFT:
                return MODE_FUN;
        }
    }
    return MODE_CLOCK;
}

void AlarmClock::SetTime(int h, int m)
{
    hours = h;
    minutes = m;
    drawRequest = true;
}

void AlarmClock::Invert(bool i)
{
    invert = i;
    drawRequest = true;
}
