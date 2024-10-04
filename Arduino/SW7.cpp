// Class for controlling SW7 (or SM02?) dual dimmer

#include "Sw7.h"
#include <TimeLib.h>
#include "eeMem.h"

#ifdef ESP32
#define UART Serial1
#else
#define UART Serial
#endif

extern void WsSend(String s);

Sw7::Sw7()
{
}

void Sw7::init(uint8_t nUserRange)
{
  m_nUserRange = nUserRange;
  digitalWrite(STATUS_LED, HIGH);
  digitalWrite(LED2, HIGH);
  digitalWrite(LED3, HIGH);
  digitalWrite(LED4, HIGH);
  pinMode(STATUS_LED, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(LED3, OUTPUT);
  pinMode(LED4, OUTPUT);

#ifdef ESP32
  UART.begin(115200, SERIAL_8N1, 3, 4); // UART for MCU comms (not USB port)
#else
  UART.begin(115200); // UART for MCU comms
#endif

  pinMode(BUTTON1, INPUT_PULLUP);
  pinMode(BUTTON2, INPUT_PULLUP);
  m_nNewLightLevel[0] = m_nLightLevel[0];
  m_nNewLightLevel[1] = m_nLightLevel[1];
}

const char *Sw7::getDevice()
{
  return "SW7";
}

uint8_t Sw7::getPower()
{
  uint8_t pw = map(m_nLightLevel[0], 0, m_nUserRange, nWattMin, 100);  // 1% = about 60% power
  pw += map(m_nLightLevel[1], 0, m_nUserRange, nWattMin, 100);
  return pw;
}

bool Sw7::listen()
{
  static bool bNewState[2];
  static bool lbState[2];
  static bool bState[2];
  static long debounce[2];
  static long lRepeatMillis[2];
  static bool bRepeat[2];
  static uint8_t nRepCnt[2] = {60, 60}; // bad fix for start issue
  bool bInvoke[2] = {false, false};
  bool bChange = false;

#define BUTTON_CNT 2
uint8_t inpin[2] = {BUTTON1, BUTTON2};

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
          bool bLed = digitalRead(LED2);
          digitalWrite(LED2, !bLed);
          delay(20);
          digitalWrite(LED2, bLed);
        }
      }
    }
    if(bInvoke[i])
    {
      bChange = true;
      m_bNewPwr[i] = !m_bPower[i]; // toggle
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
        m_nNewLightLevel[0] = (c << 1); // 1 - 100 -> 200
        state = 2;
        break;
      case 2:
        m_nNewLightLevel[1] = (c << 1); // 1 - 100 -> 200
        state = 3;
        break;
      case 3:
        state = 0;
        break;
    }
  }

  if(m_bNewPwr[0] != m_bPower[0])
  {
    m_bPower[0] = m_bNewPwr[0];
    updateMCU();
  }

  if(m_nNewLightLevel[0] != m_nLightLevel[0])
  {
    m_nLightLevel[0] = m_nNewLightLevel[0];
    updateMCU();
  }

  if(m_bNewPwr[1] != m_bPower[1])
  {
    m_bPower[1] = m_bNewPwr[1];
    updateMCU();
  }

  if(m_nNewLightLevel[1] != m_nLightLevel[1])
  {
    m_nLightLevel[1] = m_nNewLightLevel[1];
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

// 24 64 64 07 07 23 // off 100%
// 24 E4 E4 07 07 23 // on 100%
// 24 81 E4 07 07 23 // on 1% on 100%

void Sw7::updateMCU()
{
  uint8_t nData[6];

  nData[0] = 0x24;

  nData[1] = (m_nLightLevel[0] >> 1); // 1-200 -> 1-99
  if(m_bPower[0]) // power on bit
    nData[1] |= 0x80;
  nData[2] = (m_nLightLevel[1] >> 1); // 1-200 -> 1-99
  if(m_bPower[1]) // power on bit
    nData[2] |= 0x80;

  nData[3] = (m_nLightLevel[0] + 30) / 30; // LED bar: 1-200 -> 1-7 (0 is fine)
  nData[4] = (m_nLightLevel[1] + 30) / 30; // LED bar: 1-200 -> 1-7 (0 is fine)
  nData[5] = 0x23;

  UART.write(nData, 6);
}

void Sw7::setSwitch(uint8_t n, bool bOn)
{
  m_bNewPwr[n] = bOn;
}

void Sw7::setLevel(uint8_t n, uint8_t lvl)
{
  m_nNewLightLevel[n] = lvl;
}

void Sw7::setLED(uint8_t no, bool bOn)
{
  m_bLED[no] = bOn;
  switch(no)
  {
    case 0:
      digitalWrite(STATUS_LED, bOn ? false:true); // invert for this one
      break;
    case 1:
      digitalWrite(LED2, bOn ? false:true); // invert for this one
      break;
    case 2:
      digitalWrite(LED3, bOn ? false:true); // invert for this one
      break;
    case 3:
      digitalWrite(LED4, bOn ? false:true); // invert for this one
      break;
  }
}
