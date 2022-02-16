#ifndef SWITCH_H
#define SWITCH_H

#include <Arduino.h>

// Uncomment for S31 current sensor model
//#define S31

#ifdef S31
#define TOUCH_IN  0  // side button
#define WIFI_LED  13  // green LED
#else // most basic switches
#define WIFI_LED   4  // top red LED (on low)
#define TOUCH_LED  5  // Circle LED (low = on)
#define TOUCH_IN  13  // Touch switch input (low = press)
#endif

#define RELAY     12  // Relay (high = on)

class Switch
{
public:
  Switch();
  bool listen(void);
  void init(uint8_t nUserRange);
  void setSwitch(bool bOn);
  void setLevel(uint8_t level);
  char *getDevice(void);
  void setLED(uint8_t no, bool bOn);
  uint8_t getPower(uint8_t nLevel);

  bool   m_bPower;      // power state
  float  m_fPower;
  float  m_fVolts = 0; // 0 = no monitor
  int8_t m_nLightLevel = 0; // current level (0=switch)
  float  m_fCurrent;
  bool   m_bOption;

private:
  bool   m_bLED[2];
#define ARR_CNT 20
  float  m_fCurrentArr[ARR_CNT];
  float  m_fPowerArr[ARR_CNT];
  uint8_t m_nBlink;
  const uint8_t nLevelMin = 0;
  const uint8_t nLevelMax = 0; // no dimmer
  const uint8_t nWattMin = 100; // switch 100%
};

#endif // SWITCH_H
