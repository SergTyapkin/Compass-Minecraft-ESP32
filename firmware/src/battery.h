#pragma once
#include <Arduino.h>


class Battery {
private:
    const int CHARGING_PIN;
    const int CHARGE_FULL_PIN;
    
public:
    enum State {
        STATE_CHARGING,
        STATE_FULL,
        STATE_NOT_CHARGING,
        STATE_ERROR
    };
    
    // Конструктор с параметрами
    Battery(int chargingPin, int fullPin)
        : CHARGING_PIN(chargingPin), CHARGE_FULL_PIN(fullPin) {
    }
    
    void init() {
        pinMode(CHARGING_PIN, INPUT);
        pinMode(CHARGE_FULL_PIN, INPUT);
    }
    
    State getState() {
        bool charging = digitalRead(CHARGING_PIN) == LOW;
        bool full = digitalRead(CHARGE_FULL_PIN) == LOW;
        
        if (charging && full) {
            return STATE_ERROR;
        } else if (charging) {
            return STATE_CHARGING;
        } else if (full) {
            return STATE_FULL;
        } else {
            return STATE_NOT_CHARGING;
        }
    }
    
    bool isCharging() { return getState() == STATE_CHARGING; }
    bool isFull() { return getState() == STATE_FULL; }
    bool isNotCharging() { return getState() == STATE_NOT_CHARGING; }
    bool isError() { return getState() == STATE_ERROR; }
    
    bool isChargerConnected() {
        State state = getState();
        return (state != STATE_NOT_CHARGING);
    }
};
