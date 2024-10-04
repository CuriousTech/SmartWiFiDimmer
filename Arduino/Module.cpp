// Class for controlling wired Tuya dimmer module QS-WIFI-D02-TRIAC

#include "Module.h"
#include <TimeLib.h>
#include "eeMem.h"

#define SWITCH_IN 13

extern void WsSend(String s);

Module::Module()
{
  pinMode(SWITCH_IN, INPUT);
}

void Module::init(uint8_t nUserRange)
{
  m_nUserRange = nUserRange;
  Serial.begin(9600);
}

const char *Module::getDevice()
{
  return "DIMMER_CUBE";
}

uint8_t Module::getPower()
{
  return map(m_nLightLevel[0], 0, m_nUserRange, nWattMin, 100);  // 1% = about 60% power
}

void Module::isr()
{
}

bool Module::listen()
{
  // handle 60Hz AC switch input (low pulse while pressed)
  static int cnt;
  static uint32_t tm;
  static int8_t nDirection;
  bool bChange = false;

  if(digitalRead(SWITCH_IN) == LOW) // catches at around 20-400ms
  {
    cnt++;
    tm = millis();
    if(cnt > 4) // holding
    {
      if(nDirection == 0) // set direction
      {
        if(m_bPower[0] == false)
        {
          m_bPower[0] = true;
          bChange = true;
        }
        if(m_nLightLevel[0] < m_nUserRange)
            nDirection = 1; // up
        else
            nDirection = 2; // down
      }
      if(nDirection == 1) // up
      {
        if(m_nLightLevel[0] < m_nUserRange)
          m_nLightLevel[0]++;
      }
      else // down
      {
        if(m_nLightLevel[0] > 1)
          m_nLightLevel[0]--;
      }
    }
  }
  else if((millis() - tm) > 500 && cnt) // >500ms = release
  {
    if(cnt <= 4) // short tap (~100ms)
    {
      m_bPower[0] = !m_bPower[0];
      bChange = true;
    }
    cnt = 0;
//    nDirection = 0; // reset will be up next time
  }

  uint8_t s = second();
  static uint8_t ls;
  if(s != ls) // Set new light value every second
  {
    ls = s;
    uint8_t level = map(m_nLightLevel[0], 1, m_nUserRange, nLevelMin, nLevelMax);
    writeSerial( m_bPower[0] ? level : 0);
  }
  return bChange;
}

void Module::setSwitch(uint8_t idx, bool bOn)
{
  m_bPower[0] = bOn;
}

bool Module::writeSerial(uint8_t level)
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

void Module::setLevel(uint8_t idx, uint8_t n)
{
  m_nLightLevel[0] = constrain(n, 1, m_nUserRange);
}

void Module::setLED(uint8_t no, bool bOn)
{
}
