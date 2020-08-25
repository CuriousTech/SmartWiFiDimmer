// Class for controlling wired Tuya dimmer module QS-WIFI-D02-TRIAC

#include "control.h"
#include <TimeLib.h>
#include "eeMem.h"

#define SWITCH_IN 13

swControl::swControl()
{
  pinMode(SWITCH_IN, INPUT);
}

void swControl::init(uint8_t nUserRange)
{
  m_nUserRange = nUserRange;
  Serial.begin(9600);
}

uint8_t swControl::getPower(uint8_t nLevel)
{
  return map(nLevel, 0, m_nUserRange, nWattMin, 100);  // 1% = about 60% power
}

void swControl::listen()
{
  // handle 60Hz AC switch input (low pulse while pressed)
  static int cnt;
  static uint32_t tm;
  static int8_t nDirection;
  if(digitalRead(SWITCH_IN) == LOW) // catches at around 20-400ms
  {
    cnt++;
    tm = millis();
    if(cnt > 4) // holding
    {
      if(nDirection == 0) // set direction
      {
        if(m_bLightOn == false)
          m_bLightOn = true;
        if(m_nLightLevel < m_nUserRange)
            nDirection = 1; // up
        else
            nDirection = 2; // down
      }
      if(nDirection == 1)
      {
        if(m_nLightLevel < m_nUserRange)
          m_nLightLevel++;
      }
      else
      {
        if(m_nLightLevel > 1)
          m_nLightLevel--;
      }
    }
  }
  else if((millis() - tm) > 500 && cnt) // >500ms = release
  {
    if(cnt <= 4) // short tap (~100ms)
      m_bLightOn = !m_bLightOn;
    cnt = 0;
//    nDirection = 0; // reset will be up next time
  }

  uint8_t s = second();
  static uint8_t ls;
  if(s != ls) // Set new light value every second
  {
    ls = s;
    uint8_t data = map(m_nLightLevel, 1, m_nUserRange, nLevelMin, nLevelMax);
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

void swControl::test(uint8_t cmd, uint16_t v)
{
  uint8_t buf[6];

  buf[0] = 0xFF;
  buf[1] = 0x55;
  buf[2] = cmd;
  buf[3] = v>>8;
  buf[4] = v&0xFF;
  buf[5] = 0x0A;

  Serial.write(buf, 6);
}

void swControl::setLevel(uint8_t n)
{
  m_nNewLightLevel = constrain(n, 1, m_nUserRange);
}

void swControl::setLED(uint8_t no, bool bOn)
{
}
