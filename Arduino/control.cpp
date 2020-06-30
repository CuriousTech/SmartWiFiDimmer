// Class for controlling capacitive touch (+*-) dimmer

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
  digitalWrite(WIFI_LED, LOW);
  pinMode(WIFI_LED, OUTPUT);
  Serial.begin(9600);
  checkStatus();
}

void swControl::checkStatus()
{
  writeSerial(0, NULL, 0);
  m_cs = 15;
}

uint8_t swControl::getPower()
{
  return map(m_nLightLevel, 0, 200, nWattMin, 100);  // 1% = about 60% power
}

void swControl::listen()
{
  static uint8_t inBuffer[32];
  static uint8_t idx;
  static uint8_t state;
  static uint16_t cmd;
  static uint16_t len;

  while(Serial.available())
  {
    uint8_t c = Serial.read();
    switch(state)
    {
      case 0:     // data packet: 55 AA 00 cmd 00 len d0 d1 d2.... chk
        if(c == 0x55)
          state = 1;
        break;
      case 1:
        if(c == 0xAA)
          state = 2;
        break;
      case 2:
//        cmd = (uint16_t)c<<8;
        state = 3;
        break;
      case 3:
        cmd = (uint16_t)c;
        state = 4;
        break;
      case 4:
        len = (uint16_t)c<<8;
        state = 5;
        break;
      case 5:
        len |= (uint16_t)c;
        state = 6;
        idx = 0;
        break;
      case 6:
        inBuffer[idx++] = c; // get length + checksum
        if(idx > len || idx >= sizeof(inBuffer) )
        {
          switch(cmd)
          {
            case 0: // main data (1 byte) 0 or 1
              break;
            case 1: // key (21 bytes)
              break;
            case 2: //  (2 bytes) 0E 00
              break;
            case 7: // light status
              switch(len)
              {
                case 5: // 01 00 01 00 01 01
                  m_bLightOn = inBuffer[4];
                  break;
                case 8: // 02 02 00 04 00 ?? 00 93
                  m_nLightLevel = map(inBuffer[7], nLevelMin, nLevelMax, 1, 200);
                  if(m_nLightLevel == 0) m_nLightLevel = 1;
                  m_nNewLightLevel = m_nLightLevel;
                  break;
              }
              break;
          }
          
          state = 0;
          idx = 0;
          len = 0;
        }
        break;
    }
  }

  if(m_nNewLightLevel != m_nLightLevel) // new requested level
  {
    m_nLightLevel = m_nNewLightLevel;
    setLevel();
  }

  static uint8_t sec;
  if(sec != second())
  {
    sec = second();
    if(--m_cs == 0) // keepalive
      checkStatus();
  }
}

// 0 = req status (len=0)
// 1 = req key (len=0)
// 2 = req ? (len=0)
// 6 = set dimmer (len:5=on/off or 8=level)
// 8 = req full (len=0) returns both 7/5 and 7/8

void swControl::setSwitch(bool bOn)
{
  uint8_t data[5];// = {1,1,0,1,bOn};
  data[0] = 1;
  data[1] = 1;
  data[2] = 0;
  data[3] = 1;
  data[4] = bOn;
  writeSerial(6, data, 5);
}

void swControl::setLevel()
{
  uint8_t data[8];// = {2,2,0,4,0,0,0,nLightLevel};
  data[0] = 2;
  data[1] = 2;
  data[2] = 0;
  data[3] = 4; // probably ramp
  data[4] = 0;
  data[5] = 0; // not sure
  data[6] = 0;
  data[7] = map(m_nLightLevel, 1, 200, nLevelMin, nLevelMax);
  writeSerial(6, data, 8);
}

bool swControl::writeSerial(uint8_t cmd, uint8_t *p, uint8_t len)
{
  uint8_t buf[16] = {0};

  buf[0] = 0x55;
  buf[1] = 0xAA;
  buf[2] = 0; // version
  buf[3] = cmd; 
  buf[4] = 0;
  buf[5] = len;  // big endien

  int i;
  if(p) for(i = 0; i < len; i++)
    buf[6 + i] = p[i];

  uint16_t chk = 0;
  for(i = 0; i < len + 6; i++)
    chk += buf[i];
  buf[6 + len] = (uint8_t)chk;
  return Serial.write(buf, 7 + len);
}

void swControl::setLevel(uint8_t n)
{
  m_nNewLightLevel = constrain(n, 0, 200);
}

void swControl::setLED(uint8_t no, bool bOn)
{
  m_bLED[no] = bOn;
  if(no == 0)
    digitalWrite(WIFI_LED, bOn);
}
