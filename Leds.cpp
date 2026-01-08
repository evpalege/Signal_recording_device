#include "Leds.h"

Leds::Leds(int pin1, int pin2)
{
  _pin1 = pin1;
  _pin2 = pin2;
  pinMode(_pin1, OUTPUT);
  pinMode(_pin2, OUTPUT);
  off();
}

void Leds::on()
{
  digitalWrite(_pin1, LOW);
  digitalWrite(_pin2, LOW);
}

void Leds::off()
{
  digitalWrite(_pin1, HIGH);
  digitalWrite(_pin2, HIGH);
}

void Leds::blink(int time)
{
  on();
  delay(time);
  off();
}