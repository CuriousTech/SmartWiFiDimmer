// Class for controlling paddle dimmer (esp8285 / PS-16-DZ / eWeLink)
// https://usa.testzon.com/product/48861

#include "Paddle.h"
#include <TimeLib.h>
//#include <UdpTime.h>
#include "eeMem.h"

//extern UdpTime utime;

void Paddle::init(uint8_t nRange)
{
  m_nUserRange = nRange;
  digitalWrite(WIFI_LED, HIGH);
  pinMode(WIFI_LED, OUTPUT);
  Serial.begin(19200);
}

const char *Paddle::getDevice()
{
  return "PADDLE";
}

uint8_t Paddle::getPower(void)
{
  return map(m_nLightLevel[0], 0, m_nUserRange, nWattMin, 100);  // 1% = about 60% power
}

bool Paddle::listen()
{
  static char inBuffer[64];
  static uint8_t idx;
  bool bChange = false;

  while(Serial.available())
  {
    uint8_t c = Serial.read();
    if(c == 0x1B)
    {
      inBuffer[idx] = 0;
      idx = 0;
      char *p;
      // parse
      if((p = strstr(inBuffer, "switch")) != NULL) // "switch":"on"
      {
        bool b = (p[10] == 'n') ? true:false; // on or off
        if(m_bPower[0] != b)
          bChange = true;
        m_bPower[0] = b;
      }
      if((p = strstr(inBuffer, "bright")) != NULL) // "bright":99
      {
        m_nNewLightLevel = map(atoi(p + 8), nLevelMin, nLevelMax, 0, m_nUserRange); // 10-99
        m_nLightLevel[0] = m_nNewLightLevel;
      }
      String s = "AT+SEND=ok\x1B"; // respond with ok
      Serial.write((uint8_t*)s.c_str(), s.length());
    }
    else
    {
      inBuffer[idx++] = c;
      if(idx > sizeof(inBuffer)-2) idx--;
    }
  }

  if(m_nNewLightLevel != m_nLightLevel[0]) // new requested level
  {
    m_nLightLevel[0] = m_nNewLightLevel;
    setLevel();
  }

  if(m_nBlink)
  {
    static uint32_t mil;
    if(millis() - mil > m_nBlink * 16)
    {
      mil = millis();
      setLED(0, !m_bLED[0]);
    }
  }
  return bChange;
}

void Paddle::setSwitch(uint8_t idx, bool bOn)
{
  String s;
  s = "AT+UPDATE=\"sequence\":\"";
  s += now()*1000 + millis();
  s += "\",\"switch\":\"";
  s += (bOn)? "on" : "off";
  s += "\"\x1B";
  Serial.write((uint8_t*)s.c_str(), s.length());
  m_bPower[0] = bOn;
}

void Paddle::setLevel()
{
  String s;
  s = "AT+UPDATE=\"sequence\":\"";
  s += now()*1000 + millis();
  s += "\",\"bright\":";
  s += constrain(m_nLightLevel[0], nLevelMin, nLevelMax);
  s += "\x1B";
  Serial.write((uint8_t*)s.c_str(), s.length());
}

void Paddle::setLevel(uint8_t idx, uint8_t level)
{
  m_nNewLightLevel = constrain(level, 0, m_nUserRange);
}

void Paddle::setLED(uint8_t no, bool bOn)
{
  m_bLED[no] = bOn;
  if(no == 0)
    digitalWrite(WIFI_LED, bOn ? false:true); // invert for this one
}
