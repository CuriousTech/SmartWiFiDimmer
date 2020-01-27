#ifndef CONTROL_H
#define CONTROL_H

#include <Arduino.h>

#define WIFI_LED  14  // top green LED (on high)

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

  bool    m_bLightOn;      // state
  bool    m_bLED[2];
  uint8_t m_nLightLevel = 50; // current level
  const uint8_t nLevelMin = 0;
  const uint8_t nLevelMax = 255;
private:
  bool writeSerial(uint8_t cmd, uint8_t *p, uint8_t len);
  void checkStatus(void);

  uint8_t m_nNewLightLevel = 50; // set in a callback
  uint8_t m_cs = 1;
};

#endif // CONTROL_H
