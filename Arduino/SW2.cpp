// Class for controlling SW2 (or SW2_K?) dimmer

#include "Sw2.h"
#include <TimeLib.h>
#include "eeMem.h"

#ifdef ESP32
#define UART Serial1
#else
#define UART Serial
#endif

extern void WsSend(String s);

Sw2::Sw2()
{
}

void Sw2::init(uint8_t nUserRange)
{
  m_nUserRange = nUserRange;
  digitalWrite(STATUS_LED, HIGH);
  pinMode(STATUS_LED, OUTPUT);
#ifdef LED2
  pinMode(LED2, OUTPUT);
#endif

#ifdef ESP32
  UART.begin(115200, SERIAL_8N1, 8, 9); // UART for MCU comms (not USB port)
#else
  UART.begin(115200); // UART for MCU comms
#endif

  pinMode(BUTTON1, INPUT_PULLUP);
  m_nNewLightLevel = m_nLightLevel[0];
}

const char *Sw2::getDevice()
{
  return "SW2";
}

uint8_t Sw2::getPower()
{
  return map(m_nLightLevel[0], 0, m_nUserRange, nWattMin, 100);  // 1% = about 60% power
}

bool Sw2::listen()
{
  static bool bNewState[2];
  static bool lbState[2];
  static bool bState[2];
  static long debounce[2];
  static long lRepeatMillis[2];
  static bool bRepeat[2];
  static uint8_t nRepCnt[2] = {60, 60}; // bad fix for start issue
  bool bInvoke[2] = {false, false};
  bool bChange[2] = {false, false};

#define BUTTON_CNT 1
uint8_t inpin[2] = {BUTTON1};

#define REPEAT_DELAY 300 // increase for slower repeat

  uint8_t i;

  for(i = 0; i < BUTTON_CNT; i++)
  {
    bNewState[i] = digitalRead( inpin[i] );
  
    if(bNewState[i] != lbState[i])
      debounce[i] = millis(); // reset on state change
  
    if((millis() - debounce[i]) > 30)
    {
      if(bNewState[i] != bState[i]) // press or release
      {
        bState[i] = bNewState[i];
        if (bState[i] == LOW) // pressed
        {
          lRepeatMillis[i] = millis(); // initial increment
          nRepCnt[i] = 0;
        }
        else // release
        {
          if(nRepCnt[i])
          {
            if(nRepCnt[i] > 50); // first is bad
            else if(nRepCnt[i] > 1)
              m_bOption = true;
          }
          else
          {
            bInvoke[i] = true;
            bRepeat[i] = false;
          }
        }
      }
      else if(bState[i] == LOW) // holding down
      {
        if( (millis() - lRepeatMillis[i]) > REPEAT_DELAY * (bRepeat[i]?1:2) )
        {
          lRepeatMillis[i] = millis();
          nRepCnt[i]++;
          bRepeat[i] = true;
  #ifdef LED2
          bool bLed = digitalRead(LED2);
          digitalWrite(LED2, !bLed);
          delay(20);
          digitalWrite(LED2, bLed);
  #endif
        }
      }
    }
    if(bInvoke[i])
    {
      bChange[i] = true;
      m_bNewPwr = !m_bPower[i]; // toggle
    }
    lbState[i] = bNewState[i];
  }

  static uint8_t state;

  while(UART.available())
  {
    uint8_t c = UART.read();
  
    switch(state)
    {
      case 0:
        if(c == 0x24)
          state = 1;
        break;
      case 1:
        m_nNewLightLevel = (c * 200 / 150); // 1 - 150
        if(m_nNewLightLevel == 0)
          m_nNewLightLevel = 1;
        state = 2;
        break;
      case 2:
        if(c == 0x23)
          state = 0;
        break;
    }
  }

  if(m_bNewPwr != m_bPower[0] || m_nNewLightLevel != m_nLightLevel[0])
  {
    m_bPower[0] = m_bNewPwr;
    m_nLightLevel[0] = m_nNewLightLevel;
    updateMCU();
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

void Sw2::updateMCU()
{
  uint8_t nData[2];

  nData[0] = (m_nLightLevel[0] + 30) / 30; // LED bar: 1-200 -> 1-7 (0 is fine)
 
  if(m_bPower[0]) // power on bit
    nData[0] |= 0x40;

  nData[1] = 0x80 | (m_nLightLevel[0] >> 1); // 1-200 -> 1-99
  if(nData[1] == 0x80) // make sure it's 1, not 0
    nData[1] = 0x81;
  UART.write(nData[0]);
  delay(14);
  UART.write(nData[1]);
}

void Sw2::setSwitch(uint8_t n, bool bOn)
{
  m_bNewPwr = bOn;
}

void Sw2::setLevel(uint8_t n, uint8_t level)
{
  m_nNewLightLevel = level;
}

void Sw2::setLED(uint8_t no, bool bOn)
{
  m_bLED[no] = bOn;
  switch(no)
  {
    case 0:
      digitalWrite(STATUS_LED, bOn ? false:true); // invert for this one
      break;
    case 1:
#ifdef LED2
      digitalWrite(LED2, bOn ? false:true); // invert for this one
#endif
      break;
  }
}
