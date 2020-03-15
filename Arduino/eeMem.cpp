#include "eeMem.h"
#include <EEPROM.h>

eeSet ee = {              // Defaults for blank EEPROM
  sizeof(eeSet),
  0xAAAA,
  "",  // saved SSID
  "", // router password
  "0.us.pool.ntp.org", 2390, -5, // NTP server, udp port, TZ
  "password", // device password for control
  {192,168,0,100}, 80, // host IP and port
  "Basement",
  0, // autoTimer
  0, // motionSecs
  true, // report
  {0, 0}, // LED1
  19, // watts for device
  133, // 15 cents per KWH
  false,
  50,
  0, // total watts used
  0, // total seconds
  {
    {  6*60,  10*60, 254, 100, "Morning"},  // time, seconds, wday, level, name
    { 12*60,  60*60, 254, 60, "Lunch"},
    { 14*60, 120*60, 254, 20, "Something"},
  },
  {0}, // devices
  0, // motionPin  Basement=12, LivingRoom=16 RCW-0516, Back switch=14
  0
}; // 24xx

eeMem::eeMem()
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
