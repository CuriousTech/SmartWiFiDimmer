#ifndef MODULE_H
#define MODULE_H

#include <Arduino.h>

class Module
{
public:
  Module();
  void listen(void);
  void init(uint8_t nUserRange);
  char *getDevice(void);
  void setSwitch(bool bOn);
  void setLevel(uint8_t level);
  void setLED(uint8_t no, bool bOn);
  uint8_t getPower(uint8_t nLevel);
  bool    m_bLightOn;      // state
  bool    m_bLED[2];
  uint8_t m_nLightLevel = 100; // current level
  float   m_fVolts;
  float   m_fCurrent;
  float   m_fPower;
  uint8_t m_nBlink;
  bool    m_bOption;
  const uint8_t nLevelMin = 4;
  const uint8_t nLevelMax = 255;
  const uint8_t nWattMin = 60; // 60% of full watts at lowest
private:
  bool writeSerial(uint8_t level);

  uint8_t m_nNewLightLevel = 100; // set in a callback
  uint8_t m_nUserRange = 200;
};

#endif // MODULE_H
