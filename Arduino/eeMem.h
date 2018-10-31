#ifndef EEMEM_H
#define EEMEM_H

#include <Arduino.h>

struct Sched          // User set schedule item
{
  uint16_t timeSch;   // time of day in minutes
  uint16_t seconds;   // seconds on
  uint8_t wday;       // weekday bits (0 = enabled, 1 = Sunday, 7 = Saturday)
  uint8_t level;      // brightness
  char    sname[20];  // entry name
}; // 26 bytes aligned

#define MAX_SCHED 46

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
  uint8_t  reserved;
  Sched    schedule[MAX_SCHED];  // 46*28
};

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
