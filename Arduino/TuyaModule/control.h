#ifndef CONTROL_H
#define CONTROL_H

#include <Arduino.h>

class swControl
{
public:
  swControl();
  void listen(void);
  void init(uint8_t nUserRange);
  void setSwitch(bool bOn);
  void setLevel(uint8_t level);
  void setLED(uint8_t no, bool bOn);
  uint8_t getPower(uint8_t nLevel);
  void test(uint8_t cmd, uint16_t v);
  bool    m_bLightOn;      // state
  bool    m_bLED[2];
  uint8_t m_nLightLevel = 100; // current level
  float   m_fVolts;
  float   m_fCurrent;
  float   m_fPower;
  const uint8_t nLevelMin = 4;
  const uint8_t nLevelMax = 255;
  const uint8_t nWattMin = 60; // 60% of full watts at lowest
private:
  bool writeSerial(uint8_t level);

  uint8_t m_nNewLightLevel = 100; // set in a callback
  uint8_t m_nUserRange = 200;
};

#endif // CONTROL_H
