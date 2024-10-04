#ifndef TUYA_H
#define TUYA_H

#include <Arduino.h>

class Tuya
{
public:
  Tuya();
  bool listen(void);
  void init(uint8_t nUserRange);
  const char *getDevice(void);
  void setSwitch(uint8_t idx, bool bOn);
  void setLevel(uint8_t idx, uint8_t level);
  void setLED(uint8_t no, bool bOn);
  uint8_t getPower(void);
  bool    m_bPower[2];      // state
  bool    m_bLED[4];
  uint8_t m_nBlink;
  uint8_t m_nLightLevel[2]; // current level
  float   m_fVolts;
  float   m_fCurrent;
  float   m_fPower;
  bool    m_bOption;
private:
  void setLevel(void);
  bool writeSerial(uint8_t cmd, uint8_t *p = NULL, uint16_t len = 0);
  void checkStatus(void);

  uint8_t m_nUserRange;
  uint8_t m_nNewLightLevel; // set in a callback
  uint8_t m_cs = 1;
  uint16_t nLevelMin = 25; // hardware range
  uint16_t nLevelMax = 255;
  const uint8_t nWattMin = 60; // 60% of full watts at lowest
};

#endif // TUYA_H
