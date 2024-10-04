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
  digitalWrite(STATUS_LED, HIGH);
  pinMode(STATUS_LED, OUTPUT);
#ifdef LED2
  pinMode(LED2, OUTPUT);
#endif
  digitalWrite(RELAY1, LOW);
  pinMode(RELAY1, OUTPUT);
#ifdef RELAY2
  digitalWrite(RELAY2, LOW);
  pinMode(RELAY2, OUTPUT);
#endif
#ifdef S31
  Serial.begin(4800, SERIAL_8E1);
  ee.watts = 0; // 0W when off
#endif

#ifdef NX_SP201
  ee.watts = 0; // 0W when off
  digitalWrite(HLWSel, _mode);
  pinMode(HLWSel, OUTPUT);
  pinMode(HLWCF, INPUT_PULLUP);
  pinMode(HLWCF1, INPUT_PULLUP);
  _calculateDefaultMultipliers();
#endif
  pinMode(BUTTON1, INPUT_PULLUP);
#ifdef BUTTON2
  pinMode(BUTTON2, INPUT_PULLUP);
#endif
}

const char *Switch::getDevice()
{
#ifdef S31
  return "S31";
#elif defined(NX_SP201)
  return "NX_SP201";
#elif defined(THREEWAY)
  return "3WAY";
#else
  return "SWITCH";
#endif
}

uint8_t Switch::getPower()
{
  return 100;  // no reduction
}

#ifdef INT1_PIN
void ICACHE_RAM_ATTR Switch::isr1(void)
{
    unsigned long now1 = micros();
    _power_pulse_width = now1 - _last_cf_interrupt;
    _last_cf_interrupt = now1;
    _pulse_count++;
}
#endif

#ifdef INT2_PIN
void ICACHE_RAM_ATTR Switch::isr2(void)
{
  unsigned long now1 = micros();

  if ((now1 - _first_cf1_interrupt) > PULSE_TIMEOUT)
  {
    unsigned long pulse_width;
    
    if (_last_cf1_interrupt == _first_cf1_interrupt)
        pulse_width = 0;
    else
        pulse_width = now1 - _last_cf1_interrupt;
    
    if (_mode)
        _voltage_pulse_width = pulse_width;
    else
        _current_pulse_width = pulse_width;
    
    _mode = !_mode;
    digitalWrite(HLWSel, _mode);
    _first_cf1_interrupt = now1;
  }
  _last_cf1_interrupt = now1;
}
#endif

bool Switch::listen()
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

#ifdef BUTTON2
#define BUTTON_CNT 2
uint8_t inpin[2] = {BUTTON1, BUTTON2};
#else
#define BUTTON_CNT 1
uint8_t inpin[2] = {BUTTON1};
#endif

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
      m_bPower[i] = !m_bPower[i];
      bChange[i] = true;
      setSwitch( i, m_bPower[i] ); // toggle
    }
    lbState[i] = bNewState[i];
  }

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

            if(wCF != lastCF) // Usually every 10 (10Hz), but not always
            {
              lastCF = wCF;
              dwVoltCoef = inBuffer[0] << 16 | inBuffer[1] << 8 | inBuffer[2];
              dwCurrentCoef = inBuffer[6] << 16 | inBuffer[7] << 8 | inBuffer[8];
              dwPowerCoef = inBuffer[12] << 16 | inBuffer[13] << 8 | inBuffer[14];
            }

            if(Adj & 0x40)
               m_fVolts =  (float)dwVoltCoef / (float)dwVoltCycle;
            if(Adj & 0x10)
              m_fPower =  (float)dwPowerCoef / (float)dwPowerCycle;

            if( Adj & 0x20 )
            {
              m_fCurrent = (float)dwCurrentCoef / (float)dwCurrentCycle;

              if(m_fCurrent > 14.5) // 15A limit for S31
              {
                setSwitch( 0, false );
                bChange[0] = true;
                WsSend("{\"cmd\":\"alert\",\"text\":\"Current Too High\"");
              }
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

#ifdef NX_SP201

  if(m_bPower[0] || m_bPower[1])
  {
    if ((micros() - _last_cf_interrupt) > PULSE_TIMEOUT)
      m_fPower = 0;
    else
      m_fPower = (_power_pulse_width > 0) ? _power_multiplier / _power_pulse_width / 2 : 0;

    m_fCurrent = (_current_pulse_width > 0) ? _current_multiplier / _current_pulse_width / 2 : 0;
  }
  else
  {
    m_fCurrent = 0;
    m_fPower = 0;
  }

  if ((micros() - _last_cf1_interrupt) > PULSE_TIMEOUT)
  {
      if (_mode)
          _voltage_pulse_width = 0;
      else
          _current_pulse_width = 0;

      _mode = !_mode;
      digitalWrite(HLWSel, _mode);
  }

  if( _voltage_pulse_width > 0)
    m_fVolts = _voltage_multiplier / _voltage_pulse_width / 2;

#endif

#ifdef THREEWAY
  if(m_bPower[0] == digitalRead(THREEWAY) ) // Aida is low for on
  {
    m_bPower[0] = !m_bPower[0];
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

void Switch::setSwitch(uint8_t n, bool bOn)
{
  if(n)
  {
#ifdef RELAY2
    digitalWrite(RELAY2, bOn);
#endif
  }
  else
#ifdef THREEWAY
    digitalWrite(RELAY1, !digitalRead(RELAY1)); // 3-way uses a SPDT relay
#else
    digitalWrite(RELAY1, bOn);
    m_bPower[n] = bOn;
#endif
}

void Switch::setLevel(uint8_t n, uint8_t level)
{
}

void Switch::setLED(uint8_t no, bool bOn)
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

#ifdef NX_SP201
void Switch::_calculateDefaultMultipliers()
{
    _current_multiplier = ( 1000000.0 * 512 * V_REF / _current_resistor / 24.0 / F_OSC );
    _voltage_multiplier = ( 1000000.0 * 512 * V_REF * _voltage_resistor / 2.0 / F_OSC );
    _power_multiplier = ( 1000000.0 * 128 * V_REF * V_REF * _voltage_resistor / _current_resistor / 48.0 / F_OSC );
}
#endif
