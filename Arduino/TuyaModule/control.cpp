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
  static uint8_t inBuffer[52];
  static uint8_t idx;
  
  while(Serial.available())
  {
    uint8_t c = Serial.read();
    inBuffer[idx++] = c;
    inBuffer[idx] = 0;
    if(c == 0x0A || idx > 20)
    {
      idx = 0;
    }
  }

  if (m_nNewLightLevel != m_nLightLevel) // new requested level
  {
    m_nLightLevel = m_nNewLightLevel;
    setLevel();
  }

  uint8_t s = second();
  static uint8_t ls;
  if(s != ls) // Fix for light turning on at 50% randomly
  {
    ls = s;
    setSwitch(m_bLightOn);
  }
}

void swControl::setSwitch(bool bOn)
{
  writeSerial( bOn ? m_nLightLevel : 0);
  m_bLightOn = bOn;
}

void swControl::setLevel()
{
  uint8_t data = map(m_nLightLevel, 1, 100, nLevelMin, nLevelMax);
  writeSerial(data);
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
