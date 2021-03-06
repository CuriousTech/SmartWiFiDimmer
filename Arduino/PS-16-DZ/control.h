#ifndef CONTROL_H
#define CONTROL_H

#include <Arduino.h>

#define WIFI_LED  13  // top green LED (on low)

class swControl
{
public:
  swControl();
  void listen(void);
  void init(uint8_t nUserRange);
  void setSwitch(bool bOn);
  void setLevel(void);
  void setLevel(uint8_t level);
  void setLED(uint8_t no, bool bOn);
  uint8_t getPower(uint8_t nLevel);

  bool    m_bLightOn;      // state
  bool    m_bLED[2];
  uint8_t m_nLightLevel = 50; // current level
  const uint8_t nLevelMin = 10;
  const uint8_t nLevelMax = 99;
  const uint8_t nWattMin = 60; // 60% of full watts at lowest
  float   m_fVolts;
  float   m_fCurrent;
  float   m_fPower;
private:
  uint8_t m_nUserRange;
  uint8_t m_nNewLightLevel = 100; // set in a callback
};

#endif // CONTROL_H
