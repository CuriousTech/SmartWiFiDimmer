// Class for controlling wired Tuya dimmer module QS-WIFI-D02-TRIAC

#include "control.h"
#include <TimeLib.h>
#include "eeMem.h"

#define SWITCH_IN 13

swControl::swControl()
{
  pinMode(SWITCH_IN, INPUT);
}

void swControl::init()
{
  Serial.begin(9600);
}

void swControl::listen()
{
  uint8_t s = second();
  static uint8_t ls;
  if(s != ls) // Fix for light turning on at 50% randomly
  {
    ls = s;
    uint8_t data = map(m_nLightLevel, 1, 100, nLevelMin, nLevelMax);
    writeSerial( m_bLightOn ? data : 0);
  }
}

void swControl::setSwitch(bool bOn)
{
  m_bLightOn = bOn;
}

bool swControl::writeSerial(uint8_t level)
{
  uint8_t buf[6];

  buf[0] = 0xFF;
  buf[1] = 0x55;
  buf[2] = level;
  buf[3] = 0x05; // 05DC = 1500?
  buf[4] = 0xDC;
  buf[5] = 0x0A;

  return Serial.write(buf, 6);
}

void swControl::setLevel(uint8_t n)
{
  m_nNewLightLevel = constrain(n, 1, 100);
}

void swControl::setLED(uint8_t no, bool bOn)
{
}
