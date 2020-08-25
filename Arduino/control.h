#ifndef CONTROL_H
#define CONTROL_H

#include <Arduino.h>

class swControl
{
public:
  swControl();
  void listen(void);
  void init(uint8_t nRange);
  void setSwitch(bool bOn);
  void setLevel(void);
  void setLevel(uint8_t level);
  void test(uint8_t cmd, uint16_t v);
  void setLED(uint8_t no, bool bOn);
  uint8_t getPower(uint8_t nLevel);
  bool    m_bLightOn;      // state
  bool    m_bLED[2];
  uint8_t m_nLightLevel; // current level
  float   m_fVolts;
  float   m_fCurrent;
  float   m_fPower;
private:
  bool writeSerial(uint8_t cmd, uint8_t *p = NULL, uint16_t len = 0);
  void checkStatus(void);

  uint8_t m_nUserRange;
  uint8_t m_nNewLightLevel; // set in a callback
  uint8_t m_cs = 1;
  uint16_t nLevelMin = 25; // hardware range
  uint16_t nLevelMax = 255;
  const uint8_t nWattMin = 60; // 60% of full watts at lowest
};

#endif // CONTROL_H
