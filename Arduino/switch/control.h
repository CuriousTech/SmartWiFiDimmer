#ifndef CONTROL_H
#define CONTROL_H

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

class swControl
{
public:
  swControl();
  bool listen(void);
  void init(uint8_t nUserRange);
  void setSwitch(bool bOn);
  void setLevel(uint8_t level);
  char *getDevice(void);
  void setLED(uint8_t no, bool bOn);
  uint8_t getPower(uint8_t nLevel);

  bool   m_bLightOn;      // power state
  bool   m_bLED[2];
  int8_t m_nLightLevel = 0; // current level (0=switch)
  float  m_fVolts = 0; // 0 = no monitor
  float  m_fCurrent;
  float  m_fPower;
  float  m_fCurrentArr[10];
  float  m_fPowerArr[10];
  bool  m_bOption;
  uint8_t m_nBlink;
  const uint8_t nLevelMin = 0;
  const uint8_t nLevelMax = 0; // no dimmer
  const uint8_t nWattMin = 100; // switch 100%
};

#endif // CONTROL_H
