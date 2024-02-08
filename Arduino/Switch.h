#ifndef SWITCH_H
#define SWITCH_H

#include <Arduino.h>

// Uncomment one for S31 or other current sensor models
//#define S31
//#define NX_SP201 // (HLW8012 sensor)

#ifdef S31
#define BUTTON1     0  // side button
#define RELAY1     12  // Relay (high = on)
#define STATUS_LED 13  // green LED

#elif defined(NX_SP201)
#define HLWSel     2
#define HLWCF      4
#define HLWCF1     5
#define STATUS_LED 9  //blue LED
#define RELAY1    10
#define BUTTON2   12
#define RELAY2    13
#define BUTTON1   14
#define INT1_PIN   HLWCF
#define INT2_PIN   HLWCF1

#else // most basic switches
#define STATUS_LED 4  // top red LED (on low)
#define LED2       5  // Circle LED (low = on)
#define RELAY1    12  // Relay (high = on)
#define BUTTON1   13  // Touch switch input (low = press)
#endif

typedef enum {
    MODE_CURRENT,
    MODE_VOLTAGE
} hlw8012_mode_t;

class Switch
{
public:
  Switch();
  bool listen(void);
  void init(uint8_t nUserRange);
  void setSwitch(uint8_t n, bool bOn);
  void setLevel(uint8_t level);
  const char *getDevice(void);
  void  setLED(uint8_t no, bool bOn);
  uint8_t getPower(uint8_t nLevel);
  void ICACHE_RAM_ATTR isr1(void);
  void ICACHE_RAM_ATTR isr2(void);
  bool   m_bPower[2];      // power state
  float  m_fPower;
  float  m_fCurrent;
  float  m_fVolts = 0; // 0 = no monitor
  int8_t m_nLightLevel = 0; // current level (0=switch)
  bool   m_bOption;

private:

  bool   m_bLED[2];
  uint8_t m_nBlink;
  const uint8_t nLevelMin = 0;
  const uint8_t nLevelMax = 0; // no dimmer
  const uint8_t nWattMin = 100; // switch 100%

#ifdef NX_SP201 // All HLW8012 code Copyright (C) 2016-2023 by Xose Pérez <xose dot perez at gmail dot com>

#define PULSE_TIMEOUT       2000000
#define V_REF               1.28
#define R_CURRENT           0.001
#define R_VOLTAGE           2821
#define F_OSC               3579000

  volatile bool _mode = true;
  volatile unsigned long _voltage_pulse_width = 0; //Unit: us
  volatile unsigned long _current_pulse_width = 0; //Unit: us
  volatile unsigned long _power_pulse_width = 0;   //Unit: us
  volatile unsigned long _pulse_count = 0;
  volatile unsigned long _last_cf_interrupt = 0;
  volatile unsigned long _last_cf1_interrupt = 0;
  volatile unsigned long _first_cf1_interrupt = 0;

  double _current_resistor = R_CURRENT;
  double _voltage_resistor = R_VOLTAGE;

  double _current_multiplier; // Unit: us/A
  double _voltage_multiplier; // Unit: us/V
  double _power_multiplier;   // Unit: us/W

  void _calculateDefaultMultipliers(void);

#endif
};

#endif // SWITCH_H
