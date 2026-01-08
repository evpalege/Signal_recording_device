#ifndef LEDS_H
#define LEDS_H

#include <Arduino.h>

class Leds {
    
public:
    Leds(int pin1, int pin2);
    void on();
    void off();
    void blink(int time);

private:
    int _pin1;
    int _pin2;
};

#endif