#ifndef EEMEM_H
#define EEMEM_H

#include <Arduino.h>

enum Enable_bits
{
  E_Enable = 1,
  E_Sun = 2,
  E_Mon = 4,
  E_Tue = 8,
  E_Wed = 0x10,
  E_Thu = 0x20,
  E_Fri = 0x40,
  E_Sat = 0x80,
  E_M_On = 0x100,
  E_M_Off = 0x200,
};

struct Sched          // User set schedule item
{
  uint16_t timeSch;   // time of day in minutes
  uint16_t seconds;   // seconds on
  uint16_t wday;      // weekday bits (0 = enabled, 1 = Sunday, 7 = Saturday)
  uint8_t level;      // brightness
  char    sname[20];  // entry name
}; // 28 bytes aligned

#define MAX_SCHED 30
#define MAX_DEV 30

enum DEV_FLAGS{
  DF_DIM = 1,
  DF_MOT = (1<<1),
};

enum DEV_MODE{
  DM_OFF,
  DM_LNK,
  DM_REV,
};

struct Device
{
  char szName[32];
  uint8_t IP[4];
  uint16_t delay;
  uint8_t mode; // DEV_MODE
  uint8_t flags; // DEV_FLAGS
  uint8_t chns; // channels
}; // 42

struct DevState
{
  uint8_t nLevel;
  uint8_t nLevelS;
  uint32_t tm;
  bool bChanged;
  uint8_t cmd;
  bool bPwr[4];
  bool bPwrS[4];
};

struct Energy
{
  float fwh;     // WH used
  uint32_t sec;  // seconds on
}; // 8

struct flags_t
{
  uint16_t call:1;
  uint16_t lightOn0:1;
  uint16_t lightOn1:1;
  uint16_t start0:3;
  uint16_t start1:3;
  uint16_t led0:2;
  uint16_t led1:2;
  uint16_t res:3;
};

#define EESIZE (offsetof(eeMem, end) - offsetof(eeMem, size) )

class eeMem
{
public:
  eeMem();
  void update(void);
private:
  uint16_t Fletcher16( uint8_t* data, int count);
public:
  uint16_t size = EESIZE;          // if size changes, use defaults
  uint16_t sum = 0xAAAA;           // if sum is diiferent from memory struct, write
  char     szSSID[32] = ""; // Enter your SSID here
  char     szSSIDPassword[64] = ""; // and SSID password
  char     szName[28] = "BackLight"; // Device/OTA name
  uint8_t  hostIP[4] = {192,168,31,100}; // Control/status hub
  uint16_t hostPort = 80;
  uint8_t  controlIP[4] = {192,168,31,191}; // A secondary controller
  uint16_t controlPort = 80;
  int8_t   tz = -5;
  bool     bUseNtp;
  uint16_t watts = 7;  // Fixed watts of devices without metering
  uint16_t ppkw = 150;  // 15.0 cents/KWH
  uint8_t  motionPin[2] = {0, 0};   // Basement=14, Back switch=14
  uint8_t  nLightLevel = 100; // 50%
  flags_t  flags1 = {0};
  char     szControlPassword[32] = "password"; // password for WebSocket and HTTP params
  uint32_t autoTimer;
  uint16_t nMotionSecs;
  float    fTotalWatts;
  uint32_t nTotalSeconds;
  uint32_t nTotalStart;
  Sched    schedule[MAX_SCHED] =  // 30*28
  {
    {  6*60,  10*60, 254, 100, "Morning"},  // time, seconds, wday, level, name
    { 12*60,  60*60, 254, 200, "Lunch"},
    { 14*60, 120*60, 254, 20, "Something"},
  };
  Device  dev[MAX_DEV];
  Energy  days[31]; // 248 the esp-07 EEPROM seems to be smaller than ESP-12?
  Energy  months[12]; // 96
  uint8_t res[32];
  uint8_t end; // marker for EEPROM data size and end
}; // 2272 + Energy = 2653

extern eeMem ee;

#endif // EEMEM_H
