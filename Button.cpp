#include "Button.h"

  ButtonHandler::ButtonHandler(int pin, Buzzer* buzzerPtr)   
  {
    _pin = pin;
    _bz = buzzerPtr;                                                                            // Saving the buzzer address
    pinMode(_pin, INPUT_PULLUP);
  }
  
  ButtonHandler::Event ButtonHandler::getEvent()
  {
    ButtonHandler::Event event = NONE;
    bool currentState = !digitalRead(_pin);                                                     // Reversing the logic so that a pressed button = true

    if (currentState != _isPress && millis() - _timer > DEBOUNCE)
    {
      _isPress = currentState;                                                                  // Remember the new stable state
      _timer = millis();                                                                        // Resetting the timer to measure duration
    
      if (_isPress)
      {
        _bz->sound();
        _holdFlag = false;                                                                      // Button has just been pressed, it hasn't been held down yet
      }
      else
      {
        _bz->silence();
        if (!_holdFlag)
        {
          event = SHORT_PRESS;                                                                  // Click! (released earlier than the hold activated)
        }
      }
    } 

    if (_isPress && !_holdFlag && (millis() - _timer > HOLD_TIME))
    {
      _holdFlag = true;
      event = LONG_PRESS;                                                                       // Long press detected
    }

    return event;
  }                                                              

  uint32_t ButtonHandler::tick()
  {
    if (_isPress)
    {
      return millis() - _timer;
    } 
    else
    {
      return 0;
    }
  }                                                           

  bool ButtonHandler::isClick()
  {
    if (getEvent() == SHORT_PRESS)
    {
      return true;
    }
    return false;
  }                                                                

  bool ButtonHandler::isHold()
  {
    if (getEvent() == LONG_PRESS)
    {
      return true;
    }
    return false;
  }                                                              

  bool ButtonHandler::isStep()
  {
    if (!_isPress)
    {
      return false;                                                                       // If the button is not pressed
    }

    if (millis() - _timer > HOLD_TIME)                                                    // If the button is pressed, check if the HOLD_TIME (600ms) has passed
    {
        
      if (millis() - _stepTimer > STEP_TIME)                                              // Checking if the step interval (200ms) has passed since the last 'tick'
      {
        _stepTimer = millis();                                                            // Resetting the step timer
        _bz->chirp();
        return true;                                                                      // The scroll "step" is triggered
      }
    }
    return false;
  }                                                              

  bool ButtonHandler::state()
  {
    return _isPress;
  }                                                                  
