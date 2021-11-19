#include "eeMem.h"
#include <EEPROM.h>

eeSet ee = {              // Defaults for blank EEPROM
  sizeof(eeSet),
  0xAAAA, // checksum
  "",  // saved SSID
  "", // router password
  "", 2390, -5, // NTP server, udp port, TZ
  {0}, // flags1
  "password", // device password for control
  {192,168,31,100}, 80, // host IP and port
  "DiningRoom",
  0, // autoTimer
  0, // motionSecs
  23, // watts for device
  143, // 14 cents per KWH
  100, // level
  0, // total watts used
  0, // total seconds
  0, // total start time
  {0,0}, // motionPin  Basement=14, LivingRoom=16 RCW-0516, Back switch=14
  {
    {  6*60,  10*60, 254, 100, "Morning"},  // time, seconds, wday, level, name
    { 12*60,  60*60, 254, 60, "Lunch"},
    { 14*60, 120*60, 254, 20, "Something"},
  },
  {0}, // devices
  {0}, // Energy mon for days
  {0} // Energy months
}; // 2616

void eeMem::init()
{
  EEPROM.begin(sizeof(eeSet));

  uint8_t data[sizeof(eeSet)];
  uint16_t *pwTemp = (uint16_t *)data;

  int addr = 0;
  for(int i = 0; i < sizeof(eeSet); i++, addr++)
    data[i] = EEPROM.read( addr );

  if(pwTemp[0] != sizeof(eeSet)) return; // revert to defaults if struct size changes
  uint16_t sum = pwTemp[1];
  pwTemp[1] = 0;
  pwTemp[1] = Fletcher16(data, sizeof(eeSet) );
  if(pwTemp[1] != sum) return; // revert to defaults if sum fails
  memcpy(&ee, data, sizeof(eeSet) );
}

void eeMem::update() // write the settings if changed
{
  uint16_t old_sum = ee.sum;
  ee.sum = 0;
  ee.sum = Fletcher16((uint8_t*)&ee, sizeof(eeSet));

  if(old_sum == ee.sum)
    return; // Nothing has changed?

  uint16_t addr = 0;
  uint8_t *pData = (uint8_t *)&ee;
  for(int i = 0; i < sizeof(eeSet); i++, addr++)
    EEPROM.write(addr, pData[i] );
  EEPROM.commit();
}

uint16_t eeMem::Fletcher16( uint8_t* data, int count)
{
   uint16_t sum1 = 0;
   uint16_t sum2 = 0;

   for( int index = 0; index < count; ++index )
   {
      sum1 = (sum1 + data[index]) % 255;
      sum2 = (sum2 + sum1) % 255;
   }

   return (sum2 << 8) | sum1;
}
