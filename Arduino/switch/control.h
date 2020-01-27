#ifndef CONTROL_H
#define CONTROL_H

#include <Arduino.h>

#define WIFI_LED   4  // top red LED (on low)
#define TOUCH_LED  5  // Circle LED (low = on)
#define RELAY     12  // Relay (high = on)
#define TOUCH_IN  13  // Touch switch input (low = press)

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
  int8_t m_nLightLevel = 0; // current level
  const uint8_t nLevelMin = 0;
  const uint8_t nLevelMax = 0; // no dimmer
private:
  uint8_t m_nNewLightLevel = 0; // set in a callback
};

#endif // CONTROL_H
