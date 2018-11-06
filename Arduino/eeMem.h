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

#define MAX_SCHED 50

struct eeSet // EEPROM backed data
{
  uint16_t size;          // if size changes, use defauls
  uint16_t sum;           // if sum is diiferent from memory struct, write
  char     szSSID[32];
  char     szSSIDPassword[64];
  char     ntpServer[32]; // ntp server URL
  uint16_t udpPort;       // udp port
  int8_t   tz;            // Timezone offset
  char     controlPassword[32];
  uint8_t  hostIP[4];
  uint16_t hostPort;
  char     szName[28];
  uint32_t autoTimer;
  uint16_t nMotionSecs;
  bool     bCall;        // use callHost
  uint8_t  reserved; // 212
  Sched    schedule[MAX_SCHED];  // 50*28
}; // 1612

extern eeSet ee;

class eeMem
{
public:
  eeMem();
  void update(void);
private:
  uint16_t Fletcher16( uint8_t* data, int count);
};

extern eeMem eemem;

#endif // EEMEM_H
