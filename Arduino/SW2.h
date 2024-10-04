// SW2 dimmer. Odd no-brand protocol. Swap-out Beken to ESP32-C3-super mini
#ifndef SW2_H
#define SW2_H

#include <Arduino.h>

#define STATUS_LED 5  // (low = on) Note: These are not default pins
#define LED2       6  // (low = on)
#define BUTTON1    7  // (low = press)

class Sw2
{
public:
  Sw2();
  bool listen(void);
  void init(uint8_t nUserRange);
  void setSwitch(uint8_t n, bool bOn);
  void setLevel(uint8_t n, uint8_t level);
  const char *getDevice(void);
  void  setLED(uint8_t no, bool bOn);
  uint8_t getPower(void);
  void IRAM_ATTR isr1(void);
  void IRAM_ATTR isr2(void);
  bool   m_bPower[2];      // power state
  float  m_fPower;
  float  m_fCurrent;
  float  m_fVolts = 0; // 0 = no monitor
  uint8_t m_nLightLevel[2] = {1, 0}; // current level (0=switch)
  bool   m_bOption;

private:
  void updateMCU(void);

  uint8_t m_nUserRange;
  uint8_t m_nNewLightLevel;
  bool   m_bNewPwr;
  bool   m_bLED[4];
  uint8_t m_nBlink;
  const uint8_t nLevelMin = 1;
  const uint8_t nLevelMax = 100; // 
  const uint8_t nWattMin = 60; // 1% = 60% power
};

#endif // SW2_H
