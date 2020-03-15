// Class for controlling capacitive touch (center LED bar) dimmer (MOES)
// https://www.newegg.com/moes-ds01-1/p/0R7-00MY-00013

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
  Serial.begin(9600);
  Serial.swap();
  checkStatus();
}

void swControl::checkStatus()
{
  writeSerial(0); // heartbeat
  m_cs = 15;
}

void swControl::listen()
{
  static uint8_t inBuffer[52];
  static uint8_t idx;
  static uint8_t state;
  static uint8_t cmd;
  static uint16_t len;
  uint8_t n;
  
  while(Serial.available())
  {
    uint8_t c = Serial.read();
    switch(state)
    {
      case 0:     // data packet: 55 AA cmd cmd 00 len d0 d1 d2.... chk
        if(c == 0x55)
          state = 1;
        break;
      case 1:
        if(c == 0xAA)
          state = 2;
        break;
      case 2:
        // version
        state = 3;
        break;
      case 3:
        cmd = c;
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
            case 0: // heartbeat
              break;
            case 1: // product ID (51 bytes)
              break;
            case 2: // ack for MCU conf
              break;
            case 3: // ack for WiFi state
              break;
            case 5: // respond to WiFi select (ACK)
              writeSerial(5);
              break;
            case 7: // light state
              switch(len)
              {
                case 5: // 01 01 00 01 01 on
                  m_bLightOn = inBuffer[4];
                  break;
                case 8: // 03 02 00 04 00 ?? 00 lvl
                  m_nNewLightLevel = m_nLightLevel =
                    map(inBuffer[7], nLevelMin, nLevelMax, 0, 100);

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
  data[1] = 1; //  bool type
  data[2] = 0;
  data[3] = 1;
  data[4] = bOn;
  writeSerial(6, data, 5);
}

void swControl::setLevel()
{
  uint8_t data[8];// = {2,2,0,4,0,0,0,nLightLevel};
  data[0] = 3;
  data[1] = 2; // value type
  data[2] = 0;
  data[3] = 4;
  data[4] = 0;
  data[5] = 0; // 32 bit value
  data[6] = 0;
  data[7] = map(m_nLightLevel, 0, 100, nLevelMin, nLevelMax);
  writeSerial(6, data, 8);
}

bool swControl::writeSerial(uint8_t cmd, uint8_t *p, uint8_t len)
{
  uint8_t buf[16] = {0};

  buf[0] = 0x55;
  buf[1] = 0xAA;
  buf[2] = 0; // version
  buf[3] = cmd;
  buf[4] = 0; // 16 bit len big endien
  buf[5] = len;

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
  m_nNewLightLevel = constrain(n, 0, 100);
}

void swControl::setLED(uint8_t no, bool bOn)
{
  uint8_t data[1];
  m_bLED[no] = bOn;
  if(m_bLED[0] == 0 || m_bLED[1] == 0)
    data[0] = 3; // white glow
  else if(m_bLED[0])
    data[0] = 2; // red
  else if(m_bLED[1])
    data[0] = 1; // red blink
  writeSerial(3, data, 1);
}
