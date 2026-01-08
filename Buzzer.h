#ifndef BUZZER_H
#define BUZZER_H

#include <Arduino.h>

class Buzzer
{

public:
  Buzzer(int pin);
  void setVolume(uint16_t volume);
  void sound();
  void silence();
  void chirp();

private:
  int _pin;
  int _volume;

};

#endif