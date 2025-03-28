#pragma once
#include "display.h"
#include "button.h"

class SetAlarmTs : public Display {
public:
    SetAlarmTs();
    virtual void Start();
    virtual boolean Tick();
    virtual int ButtonHandler(Button::Id id, Button::Event event);

private:
    uint32_t startTime;
    int wakeupTime;
};
