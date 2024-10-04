// SW7 dual dimmer. Odd no-brand protocol. Swap-out esp8285? to ESP32-C3-super mini
#ifndef SW7_H
#define SW7_H

#include <Arduino.h>

#define STATUS_LED 7  // bottom red (low = on) Note: These are not the default pins
#define LED2       8  // bottom blue
#define LED3       5  // top red 
#define LED4       1  // top blue (channel 2/left)
#define BUTTON1    0  // (low = press) (bottom)
#define BUTTON2    6  // (low = press) (channel 2/top)

#define RELAY2 // enableds 2 channel code

class Sw7
{
public:
  Sw7();
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
  uint8_t m_nLightLevel[2] = {1,1}; // current level (0=switch)
  bool   m_bOption;

private:
  void updateMCU(void);

  uint8_t m_nUserRange;
  uint8_t m_nNewLightLevel[2];
  bool   m_bNewPwr[2];
  bool   m_bLED[4];
  uint8_t m_nBlink;
  const uint8_t nLevelMin = 1;
  const uint8_t nLevelMax = 100; // 
  const uint8_t nWattMin = 60; // 1% = 60% power
};

#endif // SW7_H
