#ifndef CONTROL_H
#define CONTROL_H

#include <Arduino.h>

class swControl
{
public:
  swControl();
  void listen(void);
  void init(void);
  void setSwitch(bool bOn);
  void setLevel(uint8_t level);
  void setLED(uint8_t no, bool bOn);

  bool    m_bLightOn;      // state
  bool    m_bLED[2];
  uint8_t m_nLightLevel = 50; // current level
  const uint8_t nLevelMin = 16; // flickers below about 15
  const uint8_t nLevelMax = 255;
private:
  bool writeSerial(uint8_t level);

  uint8_t m_nNewLightLevel = 50; // set in a callback
};

#endif // CONTROL_H
