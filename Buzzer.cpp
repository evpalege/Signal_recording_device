#include "Buzzer.h"

Buzzer::Buzzer(int pin)
{
  _pin = pin;
  pinMode(_pin, OUTPUT);
}

void Buzzer::setVolume(uint16_t volume)
{
  _volume = volume;
}

void Buzzer::sound()
{
  tone(_pin, _volume);
}

void Buzzer::silence()
{
  noTone(_pin);
}

void Buzzer::chirp()
{
  tone(_pin, _volume);
  delay(100);
  silence();
}