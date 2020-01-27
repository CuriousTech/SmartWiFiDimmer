// Class for controlling toggle dimmer (esp8285 / PS-16-DZ / eWeLink)

#include "control.h"
#include <TimeLib.h>
#include <UdpTime.h>
#include "eeMem.h"

extern UdpTime utime;

swControl::swControl()
{
}

void swControl::init()
{
  digitalWrite(WIFI_LED, HIGH);
  pinMode(WIFI_LED, OUTPUT);
  Serial.begin(19200);
}

void swControl::listen()
{
  static char inBuffer[64];
  static uint8_t idx;

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
        m_bLightOn = b;
      }
      if((p = strstr(inBuffer, "bright")) != NULL) // "bright":99
      {
        m_nNewLightLevel = atoi(p + 8); // 10-99
        m_nLightLevel = m_nNewLightLevel;
      }
      String s;
      s = "AT+SEND=ok\x1B"; // respond with ok
      Serial.write((uint8_t*)s.c_str(), s.length());
    }
    else
    {
      inBuffer[idx++] = c;
      if(idx > sizeof(inBuffer)-2) idx--;
    }
  }

  if(m_nNewLightLevel != m_nLightLevel) // new requested level
  {
    m_nLightLevel = m_nNewLightLevel;
    setLevel();
  }
}

void swControl::setSwitch(bool bOn)
{
  String s;
  s = "AT+UPDATE=\"sequence\":\"";
  s += (now() - ( (ee.tz + utime.getDST() ) * 3600)*1000) + millis();
  s += "\",\"switch\":\"";
  s += (bOn)? "on" : "off";
  s += "\"\x1B";
  Serial.write((uint8_t*)s.c_str(), s.length());
  m_bLightOn = bOn;
}

void swControl::setLevel()
{
  String s;
  s = "AT+UPDATE=\"sequence\":\"";
  s += (now() - ( (ee.tz + utime.getDST() ) * 3600)*1000) + millis();
  s += "\",\"bright\":";
  s += constrain(m_nLightLevel, nLevelMin, nLevelMax);
  s += "\x1B";
  Serial.write((uint8_t*)s.c_str(), s.length());
}

void swControl::setLevel(uint8_t n)
{
  m_nNewLightLevel = constrain(n, nLevelMin, nLevelMax);
}

void swControl::setLED(uint8_t no, bool bOn)
{
  m_bLED[no] = bOn;
  if(no == 0)
    digitalWrite(WIFI_LED, bOn ? false:true); // invert for this one
}
