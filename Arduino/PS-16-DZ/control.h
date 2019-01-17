#ifndef CONTROL_H
#define CONTROL_H

#include <Arduino.h>

#define WIFI_LED  13  // top green LED (on low)

class swControl
{
public:
  swControl();
  void listen(void);
  void init(void);
  void setSwitch(bool bOn);
  void setLevel(void);
  void setLevel(uint8_t level);
  void setLED(bool bOn);

  bool    m_bLightOn;      // state
  uint8_t m_nLightLevel = 50; // current level
  const uint8_t nLevelMin = 10;
  const uint8_t nLevelMax = 99;
private:
  uint8_t m_nNewLightLevel = 50; // set in a callback
};

#endif // CONTROL_H
