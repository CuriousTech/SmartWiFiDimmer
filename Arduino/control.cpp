// Class for controlling most serial dimmers (MOES, Geeni, etc.)

#include "control.h"
#include <TimeLib.h>
#include <UdpTime.h>
#include "eeMem.h"

// Only uncomment one

//#define GEENI // https://mygeeni.com/products/tap-dim-smart-wi-fi-dimmer-switch-white (Mine has no blue and green LEDs, maybe old model)
//#define MOES  // https://www.newegg.com/moes-ds01-1/p/0R7-00MY-00013
//#define MOES2 // v1.1 uses main serial and 104-1000 for level
#define WIRED // Wired module (FOXNSK): https://www.amazon.com/Dimmer-Switch-FOXNSK-Wireless-Compatible/dp/B07Q2XSYHS
//#define GLASS // Avatar maybe? https://www.sears.com/avatar-controls-smart-wifi-dimmer-switch-wall-light/p-A074841312

//#define DEBUG // prints data from MCU

#define BAUD 9600
#define DIM_CMD 2 // 2 for Geeni, glass, wired

#ifdef GEENI
#define BAUD 115200
#endif
#if defined(MOES) || defined(MOES2)
#define DIM_CMD 3
#endif
#if defined(WIRED) || defined(GLASS)
#define WIFI_LED  14  // Green LED (on high) for the wired one
#endif

swControl::swControl()
{
#ifdef MOES2 // 16 bit level on new version
  nLevelMin = 104;
  nLevelMax = 1000;
#endif
#ifdef GLASS
 nLevelMin = 2;
#endif
}

void swControl::init()
{
#ifdef WIFI_LED
  digitalWrite(WIFI_LED, LOW);
  pinMode(WIFI_LED, OUTPUT);
#endif
  Serial.begin(BAUD);
#ifdef MOES
  Serial.swap(); // MOES v1.0 uses alt serial
#endif
  checkStatus();
}

void swControl::checkStatus()
{
  writeSerial(0); // heartbeat
  m_cs = 15;
}

uint8_t swControl::getPower(uint8_t nLevel)
{
  return map(nLevel, 0, USER_RANGE, nWattMin, 100);  // 1% = about 60% power
}

extern void WsSend(String s);

void swControl::listen()
{
  static uint8_t inBuffer[52];
  static uint8_t idx;
  static uint8_t v;
  static uint8_t state;
  static uint8_t cmd;
  static uint16_t len;
  uint8_t n;
  uint16_t lvl;

  while(Serial.available())
  {
    uint8_t c = Serial.read();
    switch(state)
    {
      case 0:     // data packet: 55 AA vers cmd 00 len d0 d1 d2.... chk
        if(c == 0x55)
          state = 1;
        else if(c == 0xAA)
          state = 2;
        break;
      case 1:
        if(c == 0xAA)
          state = 2;
        break;
      case 2:
        // version 3
        v = c;
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
#ifdef DEBUG // send data to browser console.log
          String s="print;";
          s += String(v, HEX);
          s += " ";
          s += String(cmd, HEX);
          s += " ";
          s += String(len, HEX);
#endif
          uint8_t chk = 0xFF + len + v + cmd;
          for(int a = 0; a < len; a++)
          {
#ifdef DEBUG
            s += " ";
            s += String(inBuffer[a], HEX);
#endif
            chk += inBuffer[a];
          }
#ifdef DEBUG
          s += " = ";
          s += chk;
          WsSend(s);
#endif
          if( inBuffer[len] == chk) // good checksum
          {
            switch(cmd)
            {
              case 0: // heartbeat
                break;
              case 1: // product ID (51 or 21 bytes)
                break;
              case 2: // ack for MCU conf 0E 00
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
                  case 8: // 03 02 00 04 00 ?? lvlH lvlL
                    lvl = (inBuffer[6] << 8) | inBuffer[7];
                    m_nNewLightLevel = m_nLightLevel =
                      map(lvl, nLevelMin, nLevelMax, 1, USER_RANGE);
                    break;
                  default:
                    break;
                }
                break;
            }
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
  data[0] = DIM_CMD;
  data[1] = 2; // value type
  data[2] = 0;
  data[3] = 4; // probably ramp
  data[4] = 0;
  data[5] = 0; // 16 bit value
  uint16_t lvl = map(m_nLightLevel, 1, USER_RANGE, nLevelMin, nLevelMax);
  data[6] = lvl >> 8;
  data[7] = lvl & 0xFF;
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
  m_nNewLightLevel = constrain(n, 1, USER_RANGE);
}

void swControl::setLED(uint8_t no, bool bOn)
{
#if defined(MOES) || defined(MOES2)
  uint8_t data[1];
  m_bLED[no] = bOn;
  if(m_bLED[0] == 0 && m_bLED[1] == 0)
    data[0] = 3; // white glow
  else if(m_bLED[0])
    data[0] = 2; // red
  else if(m_bLED[1])
    data[0] = 1; // red blink
  writeSerial(3, data, 1);
#endif
#ifdef WIFI_LED
  m_bLED[no] = bOn;
  if(no == 0)
    digitalWrite(WIFI_LED, bOn);
#endif
}
