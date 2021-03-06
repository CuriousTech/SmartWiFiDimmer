// Class for controlling non-dimmer switches and smart sockets

#include "control.h"
#include <TimeLib.h>
#include "eeMem.h"

extern void WsSend(String s);

swControl::swControl()
{
}

void swControl::init(uint8_t nUserRange)
{
  digitalWrite(WIFI_LED, HIGH);
  pinMode(WIFI_LED, OUTPUT);
#ifdef TOUCH_LED
  pinMode(TOUCH_LED, OUTPUT);
#endif
  digitalWrite(RELAY, LOW);
  pinMode(RELAY, OUTPUT);
#ifdef S31
  Serial.begin(4800, SERIAL_8E1);
#endif
}

uint8_t swControl::getPower(uint8_t nLevel)
{
  return 100;  // no reduction
}

bool swControl::listen()
{
  static bool bBtnState = true;
  bool bRet = false;
  bool bBtn = digitalRead(TOUCH_IN);
  if(bBtn != bBtnState)
  {
    bBtnState = bBtn;
    if(bBtn) // release
    {
    }
    else // press
    {
      setSwitch( !digitalRead(RELAY) ); // toggle
    }
  }

#ifdef S31
  if(m_fVolts == 0) // set a default before valid packets
    m_fVolts = 120;

  static uint8_t inBuffer[24];
  static uint8_t idx;
  static uint8_t state;
  static uint8_t nCnt;
  static uint8_t nArr;
  static uint16_t lastCF;
  static uint32_t timeout;

  while(Serial.available())
  {
    uint8_t c = Serial.read();

    switch(state)
    {
      case 0:
        if(c == 0x55)
          state = 1;
        break;
      case 1:
        if(c == 0x5A) // ignore anything invalid
          state = 2;
        else if(c != 0x55)
          state = 0;
        break;
      case 2:
        inBuffer[idx++] = c; // get data + checksum
        if(idx > 21 || idx >= sizeof(inBuffer) )
        {
          timeout = millis();

          uint8_t chk = 0;
          int i;
          for(i = 0; i < 21; i++)
            chk += inBuffer[i];
          if(chk == inBuffer[i])
          {
            uint32_t dwVoltCoef = inBuffer[0] << 16 | inBuffer[1] << 8 | inBuffer[2];
            uint32_t dwVoltCycle = inBuffer[3] << 16 | inBuffer[4] << 8 | inBuffer[5];
            uint32_t dwCurrentCoef = inBuffer[6] << 16 | inBuffer[7] << 8 | inBuffer[8];
            uint32_t dwCurrentCycle = inBuffer[9] << 16 | inBuffer[10] << 8 | inBuffer[11];
            uint32_t dwPowerCoef = inBuffer[12] << 16 | inBuffer[13] << 8 | inBuffer[14];
            uint32_t dwPowerCycle = inBuffer[15] << 16 | inBuffer[16] << 8 | inBuffer[17];
            uint8_t Adj = inBuffer[18];
            uint16_t wCF = inBuffer[19] << 8 | inBuffer[20];

            if(Adj & 0x40)
               m_fVolts =  (float)dwVoltCoef / (float)dwVoltCycle;

            static uint8_t nIdx;

            if(wCF != lastCF) // Usually every 10 (10Hz), but not always
            {
              lastCF = wCF;
              nIdx = 0;
              nCnt = 0;
            }

            if( Adj & 0x20 )
            {
              if(nIdx < 10)
              {
                m_fCurrentArr[nIdx] = (float)dwCurrentCoef / (float)dwCurrentCycle;
                m_fPowerArr[nIdx] =  (float)dwPowerCoef / (float)dwPowerCycle;
              }

              float fCurr, fPow;

              if(nArr < 10) nArr++; // valid entries in array
              for(int i = 0; i < nArr; i++) // average
              {
                fCurr += m_fCurrentArr[i];
                fPow += m_fPowerArr[i];
              }
              m_fCurrent = fCurr / nArr;
              m_fPower = fPow / nArr;
              if(++nIdx >= 10)// wrap at 10 samples even if no new CF
                nIdx = 0;
            }
          }
          bRet = true;
          state = 0;
          idx = 0;
        }
        break;
    }
  }
  if((!m_bLightOn) || (millis() - timeout) > 120) // Off or no serial data/invalid header (not 555A)
  {
    m_fPower = 0;
    m_fCurrent = 0;
    nArr = 0;
    bRet = false;
  }
#endif
  return bRet;
}

void swControl::setSwitch(bool bOn)
{
  digitalWrite(RELAY, bOn);
  m_bLightOn = bOn;
}

void swControl::setLevel(uint8_t n)
{
}

void swControl::setLED(uint8_t no, bool bOn)
{
  m_bLED[no] = bOn;
  switch(no)
  {
    case 0:
      digitalWrite(WIFI_LED, bOn ? false:true); // invert for this one
      break;
    case 1:
#ifdef TOUCH_LED
      digitalWrite(TOUCH_LED, bOn ? false:true); // invert for this one
#endif
      break;
  }
}
