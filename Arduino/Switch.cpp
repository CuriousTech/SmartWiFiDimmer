// Class for controlling non-dimmer switches and smart sockets

#include "Switch.h"
#include <TimeLib.h>
#include "eeMem.h"

extern void WsSend(String s);

Switch::Switch()
{
}

void Switch::init(uint8_t nUserRange)
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
  ee.watts = 0; // 0W when off
#endif
}

char *Switch::getDevice()
{
#ifdef S31
  return "S31";
#else
  return "SWITCH";
#endif
}

uint8_t Switch::getPower(uint8_t nLevel)
{
  return 100;  // no reduction
}

bool Switch::listen()
{
  static bool bNewState;
  static bool lbState;
  static bool bState;
  static long debounce;
  static long lRepeatMillis;
  static bool bRepeat;
  static uint8_t nRepCnt = 60; // bad fix for start issue

#define REPEAT_DELAY 300 // increase for slower repeat

  bool bChange = false;

  bNewState = digitalRead(TOUCH_IN);
  if(bNewState != lbState)
    debounce = millis(); // reset on state change

  bool bInvoke = false;

  if((millis() - debounce) > 30)
  {
    if(bNewState != bState) // press or release
    {
      bState = bNewState;
      if (bState == LOW) // pressed
      {
        lRepeatMillis = millis(); // initial increment
        nRepCnt = 0;
      }
      else // release
      {
        if(nRepCnt)
        {
          if(nRepCnt > 50); // first is bad
          else if(nRepCnt > 1)
            m_bOption = true;
        }
        else
        {
          bInvoke = true;
          bRepeat = false;
        }
      }
    }
    else if(bState == LOW) // holding down
    {
      if( (millis() - lRepeatMillis) > REPEAT_DELAY * (bRepeat?1:2) )
      {
        lRepeatMillis = millis();
        nRepCnt++;
        bRepeat = true;
#ifdef TOUCH_LED
        bool bLed = digitalRead(TOUCH_LED);
        digitalWrite(TOUCH_LED, !bLed);
        delay(20);
        digitalWrite(TOUCH_LED, bLed);
#endif
      }
    }
  }

  if(bInvoke)
  {
    m_bPower = !m_bPower;
    bChange = true;
    setSwitch( m_bPower ); // toggle
  }
  lbState = bNewState;

#ifdef S31
  if(m_fVolts == 0) // set a default before valid packets
    m_fVolts = 120;

  static uint8_t inBuffer[24];
  static uint8_t idx;
  static uint8_t state;
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
          for(i = 0; i < 21; i++) // checksum
            chk += inBuffer[i];
          if(chk == inBuffer[i])
          {
            static uint32_t dwVoltCoef;
            static uint32_t dwCurrentCoef;
            static uint32_t dwPowerCoef;

            uint32_t dwVoltCycle = inBuffer[3] << 16 | inBuffer[4] << 8 | inBuffer[5];
            uint32_t dwCurrentCycle = inBuffer[9] << 16 | inBuffer[10] << 8 | inBuffer[11];
            uint32_t dwPowerCycle = inBuffer[15] << 16 | inBuffer[16] << 8 | inBuffer[17];
            uint8_t Adj = inBuffer[18];
            uint16_t wCF = inBuffer[19] << 8 | inBuffer[20];
 
            static uint8_t nIdx;

            if(wCF != lastCF) // Usually every 10 (10Hz), but not always
            {
              lastCF = wCF;
              nIdx = 0;
              dwVoltCoef = inBuffer[0] << 16 | inBuffer[1] << 8 | inBuffer[2];
              dwCurrentCoef = inBuffer[6] << 16 | inBuffer[7] << 8 | inBuffer[8];
              dwPowerCoef = inBuffer[12] << 16 | inBuffer[13] << 8 | inBuffer[14];
            }

           if(Adj & 0x40)
               m_fVolts =  (float)dwVoltCoef / (float)dwVoltCycle;

            if( Adj & 0x20 )
            {
              if(nIdx < ARR_CNT)
              {
                m_fCurrentArr[nIdx] = (float)dwCurrentCoef / (float)dwCurrentCycle;
                m_fPowerArr[nIdx] =  (float)dwPowerCoef / (float)dwPowerCycle;
              }

              if(m_fCurrentArr[nIdx] > 14.5) // 15A limit for S31
              {
                m_bPower = false;
                bChange = true;
                setSwitch( m_bPower );
                WsSend("alert;Current Too High");
              }

              float fCurr, fPow;

              if(nArr < ARR_CNT) nArr++; // valid entries in array
              for(int i = 0; i < nArr; i++) // average
              {
                fCurr += m_fCurrentArr[i];
                fPow += m_fPowerArr[i];
              }
              m_fCurrent = fCurr / nArr;
              m_fPower = fPow / nArr;
              if(++nIdx >= ARR_CNT)// wrap at ARR_CNT samples even if no new CF
                nIdx = 0;
            }
          }
          state = 0;
          idx = 0;
        }
        break;
    }
  }
  if((!m_bPower) || (millis() - timeout) > 120) // Off or no serial data/invalid header (not 555A)
  {
    m_fPower = 0;
    m_fCurrent = 0;
    nArr = 0;
  }
#endif

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

void Switch::setSwitch(bool bOn)
{
  digitalWrite(RELAY, bOn);
  m_bPower = bOn;
}

void Switch::setLevel(uint8_t n)
{
}

void Switch::setLED(uint8_t no, bool bOn)
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
