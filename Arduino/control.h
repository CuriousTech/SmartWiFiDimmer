#ifndef CONTROL_H
#define CONTROL_H

#include <Arduino.h>

#define USER_RANGE 200

class swControl
{
public:
  swControl();
  void listen(void);
  void init(void);
  void setSwitch(bool bOn);
  void setLevel(void);
  void setLevel(uint8_t level);
  void setLED(uint8_t no, bool bOn);
  uint8_t getPower(uint8_t nLevel);
  bool    m_bLightOn;      // state
  bool    m_bLED[2];
  uint8_t m_nLightLevel = USER_RANGE / 2; // current level
private:
  bool writeSerial(uint8_t cmd, uint8_t *p = NULL, uint8_t len = 0);
  void checkStatus(void);

  uint8_t m_nNewLightLevel = USER_RANGE / 2; // set in a callback
  uint8_t m_cs = 1;
  uint16_t nLevelMin = 25; // hardware range
  uint16_t nLevelMax = 255;
  const uint8_t nWattMin = 60; // 60% of full watts at lowest
};

#endif // CONTROL_H
