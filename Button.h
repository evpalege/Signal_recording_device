#ifndef BUTTON_H
#define BUTTON_H

#include <Arduino.h>
#include "Buzzer.h"

class ButtonHandler
{

public:

  enum Event
  {
    NONE = 0,
    SHORT_PRESS = 1,
    LONG_PRESS = 2
  };

  ButtonHandler(int pin, Buzzer* buzzerPtr);                                      

  Event getEvent();                                                               // Button polling method (0 - nothong, 1 - click, 2 - hold)

  uint32_t tick();                                                                // Method for returning the button press time

  bool isClick();                                                                 // Method returns a short button press and release as true

  bool isHold();                                                                  // Method returns a long press and release of the button as true

  bool isStep();                                                                  // Method returns true at intervals while the button is pressed

  bool state();                                                                   // Method returns the current state of the button

private:

  int _pin;
  Buzzer* _bz;                                                                    // Link to the buzzer object
  uint32_t _timer;
  uint32_t _stepTimer;                                                            // Step Interval Timer
  bool _isPress;
  bool _holdFlag;

  const uint16_t STEP_TIME = 200;                                                 // Scroll speed (200 ms)  
  const uint16_t DEBOUNCE = 50;                                                   // Debounce threshold
  const uint16_t HOLD_TIME = 600;                                                 // Long press threshold
};

#endif