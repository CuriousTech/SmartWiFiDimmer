// Class for controlling capacitive touch (center LED bar) dimmer (MOES, Geeni) and other Tuya dimmers

#include "Tuya.h"
#include <TimeLib.h>
//#include <UdpTime.h>
#include "eeMem.h"

// Only uncomment one

//#define GEENI // https://mygeeni.com/products/tap-dim-smart-wi-fi-dimmer-switch-white (Mine has no blue and green LEDs, maybe old model)
//#define MOES1  // https://www.amazon.com/MOES-Replaces-Multi-Control-Required-Compatible/dp/B08NJKSKRJ/ref=sr_1_12?m=AM2ATWLFGFUBV&qid=1639721132&s=merchant-items&sr=1-12
//#define MOES2 // v1.1 uses main serial and 104-1000 for level
#define MOES3 // EDM-1WAA-US KER_V1.0 Uses main serial, 115200 baud and 10-1000 for level
//#define WIRED // Wired module (FOXNSK): https://www.amazon.com/Dimmer-Switch-FOXNSK-Wireless-Compatible/dp/B07Q2XSYHS
//#define GLASS // Avatar maybe? https://www.aliexpress.us/item/3256805580530574.html?src=google&gatewayAdapt=glo2usa

#if defined(GEENI) || defined (MOES3)
#define BAUD 115200
#define DIM_CMD 2 // 2 for Geeni, glass, wired, and MOES3
#elif defined(MOES1) || defined(MOES2)
#define BAUD 9600
#define DIM_CMD 3
#elif defined(WIRED) || defined(GLASS)
#define BAUD 9600
#define DIM_CMD 2 // 2 for Geeni, glass, wired, and MOES3
#define WIFI_LED  14  // Green LED (on high) for the wired one
#else
#define BAUD 9600
#define DIM_CMD 2 // 2 for Geeni, glass, wired, and MOES3
#endif

extern void WsSend(String s);

Tuya::Tuya()
{
#if defined (MOES2) // 16 bit level on new version
  nLevelMin = 104;
  nLevelMax = 1000;
#elif defined(MOES3)
  nLevelMin = 10;
  nLevelMax = 1000;
#elif defined (GLASS)
  nLevelMin = 2;
#endif
}

void Tuya::init(uint8_t nUserRange)
{
 m_nUserRange = nUserRange;
 m_nLightLevel[0] = m_nNewLightLevel = m_nUserRange / 2; // set in a callback
#ifdef WIFI_LED
  digitalWrite(WIFI_LED, LOW);
  pinMode(WIFI_LED, OUTPUT);
#endif
  Serial.begin(BAUD);
#ifdef MOES1
//  Serial.swap(); // MOES v1.0 uses alt serial
#endif
  checkStatus();
}

const char *Tuya::getDevice()
{
#if defined(MOES1)
  return "MOES1";
#elif defined(MOES2)
  return "MOES2";
#elif defined(MOES3)
  return "MOES3";
#elif defined(GEENI)
  return "GEENI";
#elif defined(WIRED)
  return "WIRED";
#elif defined(GLASS)
  return "GLASS";
#else
  return "UNKNOWN";
#endif
}

void Tuya::checkStatus()
{
  writeSerial(0); // heartbeat
  m_cs = 15;
}

uint8_t Tuya::getPower()
{
  return map(m_nLightLevel[0], 0, m_nUserRange, nWattMin, 100);  // 1% = about 60% power
}

bool Tuya::listen()
{
  static uint8_t inBuffer[52];
  static uint8_t idx;
  static uint8_t v;
  static uint8_t state;
  static uint8_t cmd;
  static uint16_t len;
  uint8_t n;
  uint16_t lvl;
  bool bChange = false;
  
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
          uint8_t chk = 0xFF + len + v + cmd;
          for(int a = 0; a < len; a++)
            chk += inBuffer[a];
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
                    m_bPower[0] = inBuffer[4];
                    bChange = false;
                    break;
                  case 8: // 03 02 00 04 00 ?? lvlH lvlL
#ifndef WIRED // the wired one sends back levels as it fades up/down. Annoying!
                    lvl = (inBuffer[6] << 8) | inBuffer[7];
                    if(lvl < nLevelMin) lvl = nLevelMin;
                    m_nNewLightLevel = m_nLightLevel[0] =
                      map(lvl, nLevelMin, nLevelMax, 1, m_nUserRange);
#endif
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

  if(state == 0 && m_nNewLightLevel != m_nLightLevel[0]) // new requested level (only send when idle!)
  {
    m_nLightLevel[0] = m_nNewLightLevel;
    setLevel();
  }

  static uint8_t sec;
  if(sec != second())
  {
    sec = second();
    if(--m_cs == 0) // keepalive
      checkStatus();
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

void Tuya::setSwitch(uint8_t idx, bool bOn)
{
  uint8_t data[5];// = {1,1,0,1,bOn};
  data[0] = 1;
  data[1] = 1; //  bool type
  data[2] = 0;
  data[3] = 1; // 1 byte value
  data[4] = bOn;
  writeSerial(6, data, 5);
}

void Tuya::setLevel()
{
  uint8_t data[8];// = {2,2,0,4,0,0,0,nLightLevel};
  data[0] = DIM_CMD;
  data[1] = 2; // value type
  data[2] = 0;
  data[3] = 4; // 4 byte value
  data[4] = 0;
  uint16_t lvl = map(m_nLightLevel[0], 1, m_nUserRange, nLevelMin, nLevelMax);
  data[5] = 0;
  data[6] = lvl >> 8;
  data[7] = lvl & 0xFF;
  writeSerial(6, data, 8);
}

bool Tuya::writeSerial(uint8_t cmd, uint8_t *p, uint16_t len)
{
  uint8_t buf[16];

  buf[0] = 0x55;
  buf[1] = 0xAA;
  buf[2] = 0; // version
  buf[3] = cmd;
  buf[4] = len >> 8; // 16 bit len big endien
  buf[5] = len & 0xFF;

  int i;
  if(p) for(i = 0; i < len; i++)
    buf[6 + i] = p[i];

  uint16_t chk = 0;
  for(i = 0; i < len + 6; i++)
    chk += buf[i];
  buf[6 + len] = (uint8_t)chk;
  return Serial.write(buf, 7 + len);
}

void Tuya::setLevel(uint8_t idx, uint8_t level)
{
  m_nNewLightLevel = constrain(level, 1, m_nUserRange);
}

// MOES3 0=fast blink, 1=slow blink, 2=off, 3 or 4=white on?

void Tuya::setLED(uint8_t no, bool bOn)
{
  m_bLED[no] = bOn;

#if defined(MOES1) || defined(MOES2) || defined(MOES3)
  uint8_t data[1]; // 0 = no WiFi
  
  if(m_bLED[0] == 0 && m_bLED[1] == 0)
    data[0] = 3; // white glow
  else if(m_bLED[0])
    data[0] = 2; // red
  else if(m_bLED[1])
    data[0] = 1; // red blink

  writeSerial(3, data, 1);
#endif
#ifdef WIFI_LED
  if(no == 0)
    digitalWrite(WIFI_LED, bOn);
#endif
}
