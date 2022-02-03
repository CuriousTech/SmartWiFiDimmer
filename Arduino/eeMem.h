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
}; // 40

struct DevState
{
  bool bPwr;
  uint8_t nLevel;
  uint32_t tm;
  bool bChanged;
  uint8_t cmd;
  bool bPwrS;
  uint8_t nLevelS;
};

struct Energy
{
  float fwh;     // WH used
  uint32_t sec;  // seconds on
}; // 8

struct flags_t
{
  uint16_t call:1;
  uint16_t lightOn:1;
  uint16_t start:2;
  uint16_t led1:2;
  uint16_t led2:2;
  uint16_t res:8;
};

struct eeSet // EEPROM backed data
{
  uint16_t size;          // if size changes, use defaults
  uint16_t sum;           // if sum is diiferent from memory struct, write
  char     szSSID[32];
  char     szSSIDPassword[64];
  char     ntpServer[32]; // ntp server URL
  uint16_t udpPort;       // udp port
  int8_t   tz;
  flags_t  flags1;
  char     szControlPassword[32];
  uint8_t  hostIP[4];
  uint16_t hostPort;
  char     szName[28];
  uint32_t autoTimer;
  uint16_t nMotionSecs;
  uint16_t watts;
  uint16_t ppkw;
  uint8_t  nLightLevel;
  float    fTotalWatts;
  uint32_t nTotalSeconds;
  uint32_t nTotalStart;
  uint8_t motionPin[2];
  Sched    schedule[MAX_SCHED];  // 50*28
  Device   dev[MAX_DEV];
  Energy days[31]; // 248 the esp-07 EEPROM seems to be smaller than ESP-12?
  Energy months[12]; // 96
}; // 2272 + Energy = 2653

extern eeSet ee;

class eeMem
{
public:
  eeMem(){};
  void init(void);
  void update(void);
private:
  uint16_t Fletcher16( uint8_t* data, int count);
};

extern eeMem eemem;

#endif // EEMEM_H
