// Class for controlling non-dimmer switches and smart sockets

#include "control.h"
#include <TimeLib.h>
#include <UdpTime.h>
#include "eeMem.h"

extern UdpTime utime;

swControl::swControl()
{
}

void swControl::init()
{
  digitalWrite(WIFI_LED, HIGH);
  pinMode(WIFI_LED, OUTPUT);
  pinMode(TOUCH_LED, OUTPUT);
  digitalWrite(RELAY, LOW);
  pinMode(RELAY, OUTPUT);
}

uint8_t swControl::getPower()
{
  return 100;  // no reduction
}

void swControl::listen()
{
  static bool bBtnState = true;

  bool bBtn = digitalRead(TOUCH_IN);
  if(bBtn != bBtnState)
  {
    bBtnState = bBtn;
    if(bBtn) // release
    {
    }
    else // press
    {
      if(digitalRead(RELAY)) // is on, turn off
      {
        setSwitch(false);
      }
      else // turn on
      {
        setSwitch(true);
      }
    }
  }
}

void swControl::setSwitch(bool bOn)
{
  digitalWrite(RELAY, bOn);
  m_bLightOn = bOn;
}

void swControl::setLevel(uint8_t n)
{
}

void swControl::setLED(uint8_t no, bool bOn)
{
  m_bLED[no] = bOn;
  switch(no)
  {
    case 0:
      digitalWrite(WIFI_LED, bOn ? false:true); // invert for this one
      break;
    case 1:
      digitalWrite(TOUCH_LED, bOn ? false:true); // invert for this one
      break;
  }
}
