/**The MIT License (MIT)

  Copyright (c) 2018 by Greg Cunningham, CuriousTech

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/

// Arduino IDE 2.3.2, esp8266 SDK 3.1.1

#define OTA_ENABLE //uncomment to enable Arduino IDE Over The Air update code

#include <Wire.h>
#include <EEPROM.h>
#ifdef ESP32
#include <ESPmDNS.h>
#else
#include <ESP8266mDNS.h>
#endif
#include <ESPAsyncWebServer.h> // https://github.com/me-no-dev/ESPAsyncWebServer
#include <TimeLib.h> // http://www.pjrc.com/teensy/td_libs_Time.html
#include "WorldTime.h"
#include "eeMem.h"
#ifdef OTA_ENABLE
#include <FS.h>
#include <ArduinoOTA.h>
 #ifdef ESP32
 #include <SPIFFS.h>
 #endif
#endif
#include "pages.h"
#include <JsonParse.h> // https://github.com/CuriousTech/ESP8266-HVAC/tree/master/Libraries/JsonParse
#include <JsonClient.h> // https://github.com/CuriousTech/ESP8266-HVAC/tree/master/Libraries/JsonClient
#include "jsonString.h"
#include "uriString.h"

// Uncomment only one

//#include "Tuya.h" // Different models in Tuya.cpp
//Tuya cont;

#include "Switch.h"  // #define S31 is in Switch.h
Switch cont;

//#include "Sw2.h"  // custom for SW2 using ESP32-C3-super mini
//Sw2 cont;

//#include "Sw7.h"  // custom for SW7 using ESP32-C3-super mini
//Sw7 cont;

//#include "Module.h"
//Module cont;

//#include "Paddle.h"
//Paddle cont;

int serverPort = 80;  // listen port

IPAddress lastIP;     // last IP that accessed the device
int nWrongPass;       // wrong password block counter

WorldTime wTime;

eeMem ee;
uint16_t nSecTimer; // off timer
uint32_t onCounter; // usage timer
uint16_t nDelayOn;  // delay on timer
uint8_t nSched;     // current schedule
bool    bOverride;    // automatic override of schedule
uint8_t nWsConnected;
bool    bEnMot = true;
bool    bOldOn[2];
uint32_t nDaySec;
uint32_t nDayWh;
uint32_t motionSuppress;
uint8_t nPresenceCnt;
Energy eHours[24];

AsyncWebServer server( serverPort );
AsyncWebSocket ws("/ws"); // access at ws://[esp ip]/ws

void jsonCallback(int16_t iName, int iValue, char *psValue);
JsonParse jsonParse(jsonCallback);
void jsonPushCallback(int16_t iName, int iValue, char *psValue);
JsonClient jsonPush0(jsonPushCallback);
JsonClient jsonPush1(jsonPushCallback);

bool bConfigDone = false;
bool bStarted = false;
uint32_t connectTimer;

bool bKeyGood;

float wattArr[60 * 60];

DevState devst[MAX_DEV];

enum reportReason
{
  Reason_Setup,
  Reason_Switch0,
  Reason_Switch1,
  Reason_Level,
  Reason_Motion0,
  Reason_Motion1,
  Reason_Test,
};

int WsClientID;
int wattSend;
bool bParamCall;

void totalUpWatts()
{
  if (onCounter == 0 || year() < 2020)
	    return;
  float w;
  if (cont.m_fVolts) // real power monitor
    w = cont.m_fPower;
  else
    w = (float)ee.watts * cont.getPower() / 100;
  ee.fTotalWatts += (float)onCounter * w / 3600; // add current cycle
  ee.nTotalSeconds += onCounter;
  uint8_t hr = hour();
  eHours[hr].sec += nDaySec;
  eHours[hr].fwh += (float)nDaySec * w / 3600;
  if (year() > 2019) // Ensure date is valid
  {
    ee.days[day() - 1].sec = 0; // total the day from current hours
    ee.days[day() - 1].fwh = 0;
    for (int i = 0; i <= hr; i++)
    {
      ee.days[day() - 1].sec += eHours[i].sec;
      ee.days[day() - 1].fwh += eHours[i].fwh;
    }
    ee.months[month() - 1].sec = 0; // refresh the month with new values
    ee.months[month() - 1].fwh = 0;
    for (int i = 0; i < day(); i++)
    {
      ee.months[month() - 1].sec += ee.days[i].sec;
      ee.months[month() - 1].fwh += ee.days[i].fwh;
    }
  }
  nDaySec = 0;
  onCounter = 0;
}

String dataJson()
{
  jsonString js("state");
  js.Var("t", wTime.getUTCtime() );
  js.Var("name", ee.szName);
  js.Var("on0", cont.m_bPower[0]);
#ifdef RELAY2
  js.Var("on1", cont.m_bPower[1]);
#endif
  js.Var("l0", ee.flags1.led0);
  js.Var("l1", ee.flags1.led1);
  js.Var("lvl0", cont.m_nLightLevel[0]);
#ifdef RELAY2
  if(cont.m_nLightLevel[1]) // set to 0 in eemem.h to skip
    js.Var("lvl1", cont.m_nLightLevel[1]);
#ifdef LED3
  js.Var("l2", ee.flags1.led2);
#endif
#ifdef LED4
  js.Var("l3", ee.flags1.led3);
#endif
#endif
  js.Var("tr", nSecTimer);
  js.Var("sn", nSched);
  totalUpWatts();
  js.Var("ts", ee.nTotalSeconds);
  js.Var("st", ee.nTotalStart);
  if (ee.motionPin[0])
    js.Var("mo0", nPresenceCnt);
  if (ee.motionPin[1])
    js.Var("mo1", digitalRead(ee.motionPin[1]));
  return js.Close();
}

String setupJson()
{
  jsonString js("setup");
  js.Var("name", ee.szName);
  js.Var("code", cont.getDevice());
  js.Var("on0", cont.m_bPower[0]);
#ifdef RELAY2
  js.Var("on1", cont.m_bPower[1]);
#endif
  js.Var("ntp", ee.flags1.useNtp);
  js.Var("mot", ee.nMotionSecs);
  js.Var("ch", ee.flags1.call);
  js.Var("auto", ee.autoTimer);
  js.Var("ppkw", ee.ppkw);
  js.Var("watt", (cont.m_fPower) ? cont.m_fPower : ee.watts);
  js.Var("pu0", ee.flags1.start0);
  js.Var("pu1", ee.flags1.start1);
  js.Var("mp0", ee.motionPin[0]);
  js.Var("mp1", ee.motionPin[1]);
  js.Var("pd", ee.nPresenceDelay);
  js.Array("sched",  ee.schedule, MAX_SCHED);
  js.Array("dev",  ee.dev, devst, MAX_DEV);
  return js.Close();
}

String dataEnergy()
{
  jsonString js("energy");
  js.Var("t", wTime.getUTCtime() );
  js.Var("rssi", WiFi.RSSI());
  js.Var("tr", nSecTimer);
  totalUpWatts();
  js.Var("wh", ee.fTotalWatts );
  if (cont.m_fVolts)
  {
    js.Var("v", cont.m_fVolts);
    js.Var("c", cont.m_fCurrent);
    js.Var("p", cont.m_fPower);
  }
  else // fake it
  {
    float fw = (cont.m_bPower[0] || cont.m_bPower[1]) ? ((float)ee.watts * cont.getPower() / 200) : 0;
    js.Var("v", 0);
    js.Var("c", fw / 120);
    js.Var("p", fw);
  }
  return js.Close();
}

String hourJson(uint8_t hour_save)
{
  jsonString js("update");
  js.Var("type", "hour");
  js.Var("e", hour_save);
  js.Var("sec", eHours[hour_save].sec);
  js.Var("p", eHours[hour_save].fwh);
  return js.Close();
}

String dayJson(uint8_t day)
{
  jsonString js("update");
  js.Var("type", "day");
  js.Var("e", day);
  js.Var("sec", ee.days[day].sec);
  js.Var("p", ee.days[day].fwh);
  return js.Close();
}

String histJson()
{
  jsonString js("hist");

  totalUpWatts();

  js.Array("hours", eHours, 24);
  js.Array("days", ee.days, 31);
  js.Array("months", ee.months, 12);
  return js.Close();
}

const char *jsonListCmd[] = {
  "key", // 0
  "contip",
  "contport",
  "hostip",
  "hostport",
  "tmr",
  "dlyon",
  "auto",
  "sch",
  "level0",
  "TZ", // 10
  "NTP",
  "MOT",
  "reset",
  "ch",
  "ppkw",
  "watts",
  "rt",
  "I",
  "N",
  "S", // 20
  "T",
  "W",
  "L",
  "DEV",
  "MODE",
  "DIM",
  "OP",
  "DLY",
  "devname",
  "motdly", // 30
  "clear",
  "src",
  "motpin0",
  "motpin1",
  "pwr",
  "pwr0",
  "pwr1",
  "pwr2",
  "pwr3",
  "led0", //40
  "led1",
  "led2",
  "led3",
  "DON0",
  "DON1",
  "DON2",
  "DON3",
  "powerup0",
  "powerup1",
  "state0", // 50
  "state1",
  "pd",
  "level1",
};

void parseParams(AsyncWebServerRequest *request)
{
  if (nWrongPass && request->client()->remoteIP() != lastIP) // if different IP drop it down
    nWrongPass = 10;
  lastIP = request->client()->remoteIP();

  for ( uint8_t i = 0; i < request->params(); i++ )
  {
    const AsyncWebParameter* p = request->getParam(i);
    String s = request->urlDecode(p->value());

    uint8_t idx;
    for (idx = 0; jsonListCmd[idx]; idx++)
      if ( p->name().equals("key") )
       if( s.equals( ee.szControlPassword) )
        bKeyGood = true;
  }

  for ( uint8_t i = 0; i < request->params(); i++ )
  {
    const AsyncWebParameter* p = request->getParam(i);
    String s = request->urlDecode(p->value());

    uint8_t idx;
    for (idx = 0; jsonListCmd[idx]; idx++)
      if ( p->name().equals(jsonListCmd[idx]) )
        break;
    if (jsonListCmd[idx])
    {
      int iValue = s.toInt();
      if (s == "true") iValue = 1;
      bParamCall = true;
      jsonCallback(idx, iValue, (char *)s.c_str());
      bParamCall = false;
    }
  }
}

void WsSend(String s)
{
  ws.textAll(s);
}

void onWsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len)
{ //Handle WebSocket event
  static bool bRestarted = true;

  switch (type)
  {
    case WS_EVT_CONNECT:      //client connected
      client->text(setupJson());
      client->text(dataJson());
      client->text( histJson() );
      if (bRestarted)
      {
        bRestarted = false;
        WsSend("Restarted");
      }
      nWsConnected++;
      WsClientID = client->id();
      wattSend = 0;
      break;
    case WS_EVT_DISCONNECT:    //client disconnected
      compactSched();
      if (nWsConnected)
        nWsConnected--;
      WsClientID = 0;
      break;
    case WS_EVT_ERROR:    //error was received from the other end
      break;
    case WS_EVT_PONG:    //pong message was received (in response to a ping request maybe)
      break;
    case WS_EVT_DATA:  //data packet
      AwsFrameInfo * info = (AwsFrameInfo*)arg;
      if (info->final && info->index == 0 && info->len == len) {
        //the whole message is in a single frame and we got all of it's data
        if (info->opcode == WS_TEXT) {
          data[len] = 0;
          bKeyGood = false; // for callback (all commands need a key)
          jsonParse.process((char *)data);
        }
      }
      break;
  }
}

void jsonCallback(int16_t iName, int iValue, char *psValue)
{
  static int n;
  static int idx;
  static int dev;
  static String src = "";

  if (!bKeyGood && iName)
  {
    if (nWrongPass == 0)
      nWrongPass = 10;
    else if ((nWrongPass & 0xFFFFF000) == 0 ) // time doubles for every high speed wrong password attempt.  Max 1 hour
      nWrongPass <<= 1;
    return;
  }

  switch (iName)
  {
    case 0: // key
      if (psValue) if (!strcmp(psValue, ee.szControlPassword)) // first item must be key
          bKeyGood = true;
      src = ""; // reset source ID
      break;
    case 1: // contip
      {
        IPAddress IP;
        IP.fromString(psValue);
        ee.controlPort = 80;
        ee.controlIP[0] = IP[0];
        ee.controlIP[1] = IP[1];
        ee.controlIP[2] = IP[2];
        ee.controlIP[3] = IP[3];
        CallHost(Reason_Setup); // test
      }
      break;
    case 2: // contport
      ee.controlPort = iValue;
      break;
    case 3: // hostip
      ee.hostPort = 80;
      ee.hostIP[0] = lastIP[0];
      ee.hostIP[1] = lastIP[1];
      ee.hostIP[2] = lastIP[2];
      ee.hostIP[3] = lastIP[3];
      ee.flags1.call = 1;
      CallHost(Reason_Setup); // test
      break;
    case 4: // host port
      ee.hostPort = iValue;
      break;
    case 5: // tmr
      if(cont.m_bPower[0])
        nSecTimer = iValue;
      break;
    case 6: // delay to on
      nDelayOn = iValue;
      break;
    case 7: // auto
      ee.autoTimer = iValue;
      break;
    case 8: // sch
      // schedule a time
      break;
    case 9: // level
      if (nSched) bOverride = (iValue == 0);
      cont.setLevel( 0, ee.nLightLevel[0] = constrain(iValue, 1, 200) );
      if (iValue && ee.autoTimer)
        nSecTimer = ee.autoTimer;
      break;
    case 10: // TZ
      break;
    case 11: // NTP
      ee.flags1.useNtp = iValue ? 1:0;
      if(ee.flags1.useNtp)
        wTime.update();
      break;
    case 12: // MOT
      ee.nMotionSecs = iValue;
      break;
    case 13: // reset
      ee.update();
      delay(200);
#ifdef ESP32
      ESP.restart();
#else
      ESP.reset();
#endif
      break;
    case 14: // ch
      ee.flags1.call = iValue;
      if (iValue) CallHost(Reason_Setup); // test
      break;
    case 15: // PPWH
      ee.ppkw = iValue;
      break;
    case 16: // watts
      ee.watts = iValue;
      break;
    case 17: // rt
      totalUpWatts();
      ee.fTotalWatts = 0;
      ee.nTotalSeconds = 0;
      ee.nTotalStart = wTime.getUTCtime();
      break;
    case 18: //I
      n = constrain(iValue, 0, MAX_SCHED - 1);
      break;
    case 19: // N
      if (psValue)
        strncpy(ee.schedule[n].sname, psValue, sizeof(ee.schedule[0].sname) );
      break;
    case 20: // S
      ee.schedule[n].timeSch = iValue;
      break;
    case 21: // T
      ee.schedule[n].seconds = iValue;
      break;
    case 22: // W
      ee.schedule[n].wday = iValue;
      break;
    case 23: // L
      ee.schedule[n].level = iValue;
      break;
    case 24: // DEV index of device to set
      dev = iValue;
      break;
    case 25: // MODE disable, link, reverse
      ee.dev[dev].mode = iValue;
      break;
    case 26: // DIM // link dimming
      ee.dev[dev].flags &= ~1; // define this later
      ee.dev[dev].flags |= (iValue & 1);
      break;
    case 27: // OP (flag 2) // motion to other switch
      ee.dev[dev].flags &= ~2; // define this later
      ee.dev[dev].flags |= (iValue << 1);
      break;
    case 28: // DLY // delay off (when this one turns on)
      ee.dev[dev].delay = iValue;
      break;
    case 29: // devname (host name)
      if (!strlen(psValue))
        break;
      strncpy(ee.szName, psValue, sizeof(ee.szName));
      ee.update();
#ifdef ESP32
      ESP.restart();
#else
      ESP.reset();
#endif
      break;
    case 30: // motdly
      motionSuppress = iValue;
      break;
    case 31: // clear device list
      memset(&ee.dev, 0, sizeof(ee.dev));
      ee.update();
      break;
    case 32: // src : name of querying device
      src = psValue;
      addDev(src, lastIP);
      break;
    case 33: // motpin0
      ee.motionPin[0] = iValue;
      break;
    case 34: // motpin1
      ee.motionPin[1] = iValue;
      break;
    case 35: // pwr (all)
      if(bParamCall) // probably from another switch
      {
        if(nPresenceCnt && iValue == 0) // occupied, don't turn off
          break;
      }
      nSecTimer = 0;
      if(bOldOn[0] == iValue && bOldOn[1] == iValue)
        break;
      if (iValue == 2) iValue = cont.m_bPower[0] ^ 1; // toggle
      bOldOn[0] = !iValue; // force report
      cont.setSwitch(0, iValue);
      cont.setSwitch(1, iValue);
      if (nSched) bOverride = !iValue;
      if (iValue && ee.autoTimer)
        nSecTimer = ee.autoTimer;
      break;
    case 36: // pwr0
      if(bOldOn[0] == iValue)
        break;
      if(bParamCall) // probably from another switch
      {
        if(nPresenceCnt && iValue == 0) // occupied, don't turn off
          break;
      }
      nSecTimer = 0;
      if (iValue == 2) iValue = cont.m_bPower[0] ^ 1; // toggle
      bOldOn[0] = !iValue; // force report
      cont.setSwitch(0, iValue);
      if (nSched) bOverride = !iValue;
      if (iValue && ee.autoTimer)
        nSecTimer = ee.autoTimer;
      break;
    case 37: // pwr1
      if(bOldOn[1] == iValue)
        break;
      if (iValue == 2) iValue = cont.m_bPower[1] ^ 1; // toggle
      cont.setSwitch(1, iValue);
      break;
    case 38: // pwr2
      break;
    case 39: // pwr3
      break;
    case 40: // led0
      cont.setLED(0, iValue ? true : false);
      ee.flags1.led0 = iValue;
      break;
    case 41: // led1
      cont.setLED(1, iValue ? true : false);
      ee.flags1.led1 = iValue;
      break;
    case 42: // led2
      cont.setLED(2, iValue ? true : false);
      break;
    case 43: // led3
      cont.setLED(3, iValue ? true : false);
      ee.flags1.led3 = iValue;
      break;
    case 44: // DON0 device power (see checkDevChanges)
      devst[dev].cmd = iValue + 1;
      devst[dev].bPwr[0] = iValue ? true : false;
      break;
    case 45: // DON1 device power (see checkDevChanges)
      devst[dev].cmd = iValue + 3;
      devst[dev].bPwr[1] = iValue ? true : false;
      break;
    case 46: // DON2 device power (see checkDevChanges)
      break;
    case 47: // DON3 device power (see checkDevChanges)
      break;
    case 48: // powerup0
      ee.flags1.start0 = iValue;
      break;
    case 49: // powerup1
      ee.flags1.start1 = iValue;
      break;
    case 50: // state0 : also current state of querying device
      if (src.length())
        setDevState(src, 0, iValue ? true : false);
      break;
    case 51: // state1 : also current state of querying device
      if (src.length())
        setDevState(src, 1, iValue ? true : false);
      break;
    case 52: // pd
      ee.nPresenceDelay = iValue;
      break;
    case 53: // level2
      if (nSched) bOverride = (iValue == 0);
      cont.setLevel( 1, ee.nLightLevel[1] = constrain(iValue, 1, 200) );
      if (iValue && ee.autoTimer)
        nSecTimer = ee.autoTimer;
      break;
  }
}

void setDevState(String sDev, uint8_t idx, bool bPwr)
{
  for (int d = 0; ee.dev[d].IP[0] && d < MAX_DEV; d++)
  {
    if (sDev.equals(ee.dev[d].szName))
    {
      if (devst[d].bPwr[idx] != bPwr)
        devst[d].bChanged = true;
      devst[d].bPwr[idx] = bPwr;
      devst[d].bPwrS[idx] = cont.m_bPower[idx]; // Response will update client, so set here
      break;
    }
  }
}

const char *jsonListPush[] = {
  "name", // 0
  "tzoffset",
  "time",
  "ppkw",
  "level",
  "level0",
  "level1",
  "ch",  // 7
  "on0",
  "on1",
  NULL
};

void jsonPushCallback(int16_t iName, int iValue, char *psValue)
{
  static int8_t nDev;
  static uint8_t failCnt;

  switch (iName)
  {
    case -1: // status
      if (iValue >= JC_TIMEOUT)
      {
        failCnt ++;
        if (failCnt > 5)
          ESP.restart();
      }
      else failCnt = 0;
      break;
    case 0: // name
      {
        nDev = -1;
        for (int i = 0; i < MAX_DEV; i++)
          if (!strcmp(psValue, ee.dev[i].szName) )
          {
            nDev = i;
            break;
          }
      }
      break;
    case 1: // tzoffset
      wTime.setTZOffset(iValue);
      break;
    case 2: // time
      if (!ee.flags1.useNtp)
      {
        if (iValue > now() + wTime.getTZOffset() )
          setTime(iValue + wTime.getTZOffset() );
      }
      if (nDev >= 0)
        devst[nDev].tm = iValue;
      break;
    case 3: // ppkw
      ee.ppkw = iValue;
      break;
    case 4: // level
      if (nDev < 0) break;
      if (devst[nDev].nLevel[0] != iValue)
        devst[nDev].bChanged = true;
      devst[nDev].nLevel[0] = iValue;
      devst[nDev].nLevel[1] = iValue;
      break;
    case 5: // level0
      if (nDev < 0) break;
      if (devst[nDev].nLevel[0] != iValue)
        devst[nDev].bChanged = true;
      devst[nDev].nLevel[0] = iValue;
      break;
    case 6: // level1
      if (nDev < 0) break;
      if (devst[nDev].nLevel[1] != iValue)
        devst[nDev].bChanged = true;
      devst[nDev].nLevel[1] = iValue;
      break;
    case 7: // ch (channels)
      if (nDev < 0) break;
      ee.dev[nDev].chns = iValue;
      break;
    case 8: // on0
      if (nDev < 0) break;
      if (devst[nDev].bPwr[0] != iValue)
        devst[nDev].bChanged = true;
      devst[nDev].bPwr[0] = iValue;
      break;
    case 9: // on1
      if (nDev < 0) break;
      if (devst[nDev].bPwr[1] != iValue)
        devst[nDev].bChanged = true;
      devst[nDev].bPwr[1] = iValue;
      break;
  }
}

struct cQ
{
  IPAddress ip;
  String sUri;
  uint16_t port;
};
#define CQ_CNT 16 // Should be larger than iot domain
cQ queue[CQ_CNT];

void checkQueue()
{
  if(WiFi.status() != WL_CONNECTED)
    return;

  int idx;
  for(idx = 0; idx < CQ_CNT; idx++)
  {
    if(queue[idx].port)
      break;
  }
  if(idx == CQ_CNT || queue[idx].port == 0) // nothing to do
    return;

  if ( jsonPush0.begin(queue[idx].ip, queue[idx].sUri.c_str(), queue[idx].port, false, false, NULL, NULL, 1) )
  {
    jsonPush0.setList(jsonListPush);
    queue[idx].port = 0;
  }
  else if ( jsonPush1.begin(queue[idx].ip, queue[idx].sUri.c_str(), queue[idx].port, false, false, NULL, NULL, 1) )
  {
    jsonPush1.setList(jsonListPush);
    queue[idx].port = 0;
  }
}

bool callQueue(IPAddress ip, String sUri, uint16_t port)
{
  int idx;
  for(idx = 0; idx < CQ_CNT; idx++)
  {
    if(queue[idx].port == 0)
      break;
  }
  if(idx == CQ_CNT) // nothing to do
  {
    WsSend("print; Q full");
    return false;
  }

  queue[idx].ip = ip;
  queue[idx].sUri = sUri;
  queue[idx].port = port;

  return true;
}

void CallHost(reportReason r)
{
  if (ee.hostIP[0] == 0 || ee.flags1.call == 0) // no host set
    return;

  uriString uri("/wifi");

  uri.Param("name", ee.szName);
  
  switch (r)
  {
    case Reason_Setup:
      uri.Param("reason", "setup");
      uri.Param("port", serverPort);
      uri.Param("ch", 1);
      uri.Param("on0", cont.m_bPower[0]);
      uri.Param("level0", cont.m_nLightLevel[0]);
#ifdef RELAY2
      uri.Param("ch", 2);
      uri.Param("on1", cont.m_bPower[1]);
      uri.Param("level1", cont.m_nLightLevel[1]);
#else
      uri.Param("ch", 1);
#endif
      break;
    case Reason_Switch0:
      uri.Param("reason", "switch");
      uri.Param("on0", cont.m_bPower[0]);
      break;
    case Reason_Switch1:
      uri.Param("reason", "switch");
      uri.Param("on1", cont.m_bPower[1]);
      break;
    case Reason_Level:
      uri.Param("reason", "level0");
      uri.Param("value", cont.m_nLightLevel[0]);
      break;
    case Reason_Motion0:
      uri.Param("reason", "motion0");
      uri.Param("value", (nPresenceCnt) ? 1 : 0);
      break;
    case Reason_Motion1:
      uri.Param("reason", "motion1");
      break;
  }

  IPAddress ip(ee.hostIP);

  callQueue(ip, uri.string(), ee.hostPort);

  if(ee.controlIP[3])
  {
    IPAddress ip2(ee.controlIP);
    callQueue(ip2, uri.string(), ee.controlPort);
  }
}

void manageDevs(int dt, bool bOn)
{
  String sUri;

  for (int i = 0; ee.dev[i].IP[0] && i < MAX_DEV; i++)
  {
    if (ee.dev[i].mode == 0 && ee.dev[i].flags == 0)
      continue;

    IPAddress ip(ee.dev[i].IP);

    uriString uri("/wifi");
    uri.Param("key", ee.szControlPassword);
    uri.Param("src", ee.szName);
    uri.Param("state0", cont.m_bPower[0]);

    if ((dt & DC_LNK) && ee.dev[i].mode)
    {
      if (ee.dev[i].mode == DM_LNK) // turn other on and off
      {
        if (ee.dev[i].delay) // on/delayed off link
        {
          if (cont.m_bPower[0])
          {
            uri.Param("pwr0", cont.m_bPower[0]);
          }
          else
          {
            uri.Param("tmr", ee.dev[i].delay);
          }
        }
        else // on/off link
        {
          uri.Param("pwr0", cont.m_bPower[0]);
        }
      }
      else if (ee.dev[i].mode == DM_REV) // turn other off if on
      {
        if (cont.m_bPower[0])
        {
          if (ee.dev[i].delay) // delayed off
          {
            uri.Param("tmr", ee.dev[i].delay);
          }
          else // instant off
          {
            uri.Param("pwr0", 0);
          }
        }
        else
          continue;
      }
      else
        continue;
      devst[i].bPwrS[0] = cont.m_bPower[0];
      callQueue(ip, uri.string(), 80);
    }
    if ((dt & DC_DIM) && (ee.dev[i].flags & DF_DIM) ) // link dimmer level
    {
      uri.Param("level0", cont.m_nLightLevel[0]);
      uri.Param("level1", cont.m_nLightLevel[1]);
      devst[i].bPwrS[0] = cont.m_bPower[0];
      callQueue(ip, uri.string(), 80);
    }
    if ((dt & DC_MOT) && (ee.dev[i].flags & DF_MOT) ) // motion turns on/off other light
    {
      uri.Param("pwr", bOn);
      devst[i].bPwrS[0] = bOn;//cont.m_bPower[0];
      callQueue(ip, uri.string(), 80);
    }
  }
}

void queryDevs() // checks one device per minute
{
  static uint8_t nDev;

  if (nDev >= MAX_DEV || ee.dev[nDev].IP[0] == 0)
  {
    if (nDev == 0)
      return;

    nDev = 0; // restart at dev 0
  }

  uriString uri("/wifi");
  uri.Param("key", ee.szControlPassword);
  uri.Param("src", ee.szName);

  if (devst[nDev].bPwrS[0] == cont.m_bPower[0] && devst[nDev].bPwrS[1] == cont.m_bPower[1])
  {
    nDev++;
    return;
  }

  if (devst[nDev].bPwrS[0] != cont.m_bPower[0])
  {
    uri.Param("state0", cont.m_bPower[0]);
    devst[nDev].bPwrS[0] = cont.m_bPower[0];
  }

  if (devst[nDev].bPwrS[1] != cont.m_bPower[1])
  {
    uri.Param("state1", cont.m_bPower[1]);
    devst[nDev].bPwrS[1] = cont.m_bPower[1];
  }

  IPAddress ip(ee.dev[nDev].IP);
  if ( callQueue(ip, uri.string(), 80) )
    nDev++;
}

void checkDevChanges() // realtime update of device changes
{
  for (int i = 0; i < MAX_DEV && ee.dev[i].IP[0]; i++)
  {
    if (devst[i].bChanged) // update web page with device states
    {
      devst[i].bChanged = false;
      jsonString js("dev");
      js.Var("dev", i);
      js.Var("on0", devst[i].bPwr[0]);
      js.Var("level0", devst[i].nLevel[0]);
#ifdef RELAY2
      js.Var("on1", devst[i].bPwr[1]);
      js.Var("level1", devst[i].nLevel[1]);
#endif
      ws.textAll( js.Close() );
    }
    if (devst[i].cmd) // send commands to other device
    {
      uriString uri("/wifi");
      uri.Param("key", ee.szControlPassword);
      uri.Param("src", ee.szName);

      switch(devst[i].cmd)
      {
        case 1:
          uri.Param("pwr0", 0);
          break;
        case 2:
          uri.Param("pwr0", 1);
          break;
        case 3:
          uri.Param("pwr1", 0);
          break;
        case 4:
          uri.Param("pwr1", 1);
          break;
      }
      uri.Param("state0", cont.m_bPower[0]);
#ifdef RELAY2
      uri.Param("state1", cont.m_bPower[1]);
#endif
      devst[i].bPwrS[0] = (devst[i].cmd - 1);
      IPAddress ip(ee.dev[i].IP);
      callQueue(ip, uri.string(), 80);
      devst[i].cmd = 0;
    }
  }
}

uint8_t ssCnt = 58;

void sendState()
{
  if (nWsConnected)
    ws.textAll( dataJson() );
  ssCnt = 58;
}

int devComp(const void *a, const void*b)
{
  Device *a1 = (Device *)a;
  Device *b1 = (Device *)b;
  return strcmp(a1->szName, b1->szName);
}

void addDev(String sName, IPAddress ip)
{
  char szName[38];
  sName.toCharArray(szName, sizeof(szName));
  strtok(szName, "."); // remove .local

  int d;

  for (d = 0; ee.dev[d].IP[0] && d < MAX_DEV; d++)
  {
      if (!strcmp(szName, ee.dev[d].szName))
      {
        ee.dev[d].IP[0] = ip[0]; // update IP
        ee.dev[d].IP[1] = ip[1];
        ee.dev[d].IP[2] = ip[2];
        ee.dev[d].IP[3] = ip[3];
        break;
      }
      if(ip[0] == ee.dev[d].IP[0] && ip[1] == ee.dev[d].IP[1] && ip[2] == ee.dev[d].IP[2] && ip[3] == ee.dev[d].IP[3])
      {
        if(strcmp(szName, ee.dev[d].szName)) // name was changed
          strcpy(ee.dev[d].szName, szName);
      }
  }
  if (ee.dev[d].IP[0] == 0) // Not found (add it)
  {
    memset(&ee.dev[d], 0, sizeof(Device));
    strcpy(ee.dev[d].szName, szName);
    ee.dev[d].IP[0] = ip[0]; // copy IP
    ee.dev[d].IP[1] = ip[1];
    ee.dev[d].IP[2] = ip[2];
    ee.dev[d].IP[3] = ip[3];
    d++;
  }

  if (d) qsort(ee.dev, d, sizeof(struct Device), devComp);
}

void MDNSscan()
{
  int n = MDNS.queryService("esp", "tcp");

  for (int i = 0; i < n; ++i)
  {
    addDev(MDNS.hostname(i), MDNS.IP(i) );
  }
}

#ifdef INT1_PIN
void ICACHE_RAM_ATTR isr1Stub()
{
  cont.isr1();
}
#endif

#ifdef INT2_PIN
void ICACHE_RAM_ATTR isr2Stub()
{
  cont.isr2();
}
#endif

void setup()
{
#ifdef ESP32
  SPIFFS.begin(true); // format on fail since it's not really used for anything but settings (eemem)
#endif
  ee.init();
  cont.init(200);

  WiFi.hostname(ee.szName);
  WiFi.mode(WIFI_STA);

  if ( ee.szSSID[0] )
  {
    WiFi.begin(ee.szSSID, ee.szSSIDPassword);
    bConfigDone = true;
  }
  else
  {
//    Serial.println("No SSID. Waiting for EspTouch.");
    WiFi.beginSmartConfig();
  }
  connectTimer = now();

  // attach AsyncWebSocket
  ws.onEvent(onWsEvent);
  server.addHandler(&ws);

  server.on ( "/", HTTP_GET | HTTP_POST, [](AsyncWebServerRequest * request)
  {
    parseParams(request);
    request->send_P( 200, "text/html", page1);
  });

  server.on( "/s", HTTP_GET | HTTP_POST, [](AsyncWebServerRequest * request) // for quick commands
  {
    parseParams(request);
    request->send(200, "text/plain", dataJson());
  });

  server.onNotFound([](AsyncWebServerRequest * request) {
    request->send(404);
  });
  server.on("/heap", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(200, "text/plain", String(ESP.getFreeHeap()));
  });

  server.on ( "/wifi", HTTP_GET | HTTP_POST, [](AsyncWebServerRequest * request)
  {
    parseParams(request);
    jsonString js;
    js.Var("name", ee.szName );
    if (ee.flags1.useNtp) // only propogate accurate time
    {
      if (year() > 2020)
      {
        js.Var("tzoffset", wTime.getTZOffset() );
        js.Var("time", wTime.getUTCtime() );
      }
    }
//    js.Var("ppkw", ee.ppkw );

#ifdef RELAY2
    js.Var("ch", 2 );
#else
    js.Var("ch", 1 );
#endif
    js.Var("on0", cont.m_bPower[0] );
    js.Var("on1", cont.m_bPower[1] );
    js.Var("level0", cont.m_nLightLevel[0] );
    js.Var("level1", cont.m_nLightLevel[1] );
    request->send(200, "text/plain", js.Close());
  });
  server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest * request) {
    AsyncWebServerResponse *response = request->beginResponse_P(200, "image/x-icon", favicon, sizeof(favicon));
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  });
  server.on ( "/mdns", HTTP_GET | HTTP_POST, [](AsyncWebServerRequest * request) // used by hosts to quickly list all switches
  {
    String s = "{";
    for (int i = 0; ee.dev[i].IP[0]; i++)
    {
      if(i) s += ",";
      s += "\""; s+= ee.dev[i].szName; s += "\":[\"";
      IPAddress ip( ee.dev[i].IP );
      s += ip.toString();
      s += "\",";
      s += ee.dev[i].chns; s += ",";
      s += devst[i].bPwr[0]; s += ",";
      s += devst[i].bPwr[1];
      s += "]";
    }
    s += "}";
    request->send(200, "text/plain", s);
  });

  server.onFileUpload([](AsyncWebServerRequest * request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
  });
  server.onRequestBody([](AsyncWebServerRequest * request, uint8_t *data, size_t len, size_t index, size_t total) {
  });
  server.begin();

  jsonParse.setList(jsonListCmd);
#ifdef OTA_ENABLE
  ArduinoOTA.setHostname(ee.szName);
  ArduinoOTA.begin();
  ArduinoOTA.onStart([]() {
    ee.update();
    cont.setLED(0, false); // set it all to off
    cont.setLED(1, false);
    cont.setSwitch(0, false);
    cont.setSwitch(1, false);
    SPIFFS.end();
    jsonString js("alert");
    js.Var("text", "OTA Update Started");
    ws.textAll(js.Close());
    ws.closeAll();
  });
#endif

  cont.setLevel(0, ee.nLightLevel[0]);
  cont.setLevel(1, ee.nLightLevel[1]);
  switch (ee.flags1.start0)
  {
    case 0: cont.setSwitch(0, ee.flags1.lightOn0); // last saved
            cont.setSwitch(1, ee.flags1.lightOn1); break;
    case 1: break; // start off
    case 2: cont.setSwitch(0, true);
            cont.setSwitch(1, true); break;
  }

#ifdef RELAY2
  switch (ee.flags1.start1)
  {
    case 0: cont.setSwitch(1, ee.flags1.lightOn1); break; // last saved
    case 1: cont.setSwitch(1, false); break; // start off
    case 2: cont.setSwitch(1, true); break; // start on
  }
#endif

#ifdef INT1_PIN
  attachInterrupt(digitalPinToInterrupt(INT1_PIN), isr1Stub, RISING);
#endif
#ifdef INT2_PIN
  attachInterrupt(digitalPinToInterrupt(INT2_PIN), isr2Stub, RISING);
#endif
}

void changeLEDs(bool bOn, bool bOn2)
{
  switch (ee.flags1.led0)
  {
    case LM_OFF: // off
      cont.setLED(0, false);
      break;
    case LM_ON: // on
      cont.setLED(0, true);
      break;
    case LM_LNK:
      cont.setLED(0, bOn); // link
      break;
    case LM_REV:
      cont.setLED(0, !bOn); // reverse
      break;
  }
  switch (ee.flags1.led1)
  {
    case LM_OFF: // off
      cont.setLED(1, false);
      break;
    case LM_ON: // on
      cont.setLED(1, true);
      break;
    case LM_LNK:
      cont.setLED(1, bOn); // link
      break;
    case LM_REV:
      cont.setLED(1, !bOn); // reverse
      break;
  }
  switch (ee.flags1.led2)
  {
    case LM_OFF: // off
      cont.setLED(2, false);
      break;
    case LM_ON: // on
      cont.setLED(2, true);
      break;
    case LM_LNK:
      cont.setLED(2, bOn2); // link
      break;
    case LM_REV:
      cont.setLED(2, !bOn2); // reverse
      break;
  }
  switch (ee.flags1.led3)
  {
    case LM_OFF: // off
      cont.setLED(3, false);
      break;
    case LM_ON: // on
      cont.setLED(3, true);
      break;
    case LM_LNK:
      cont.setLED(3, bOn2); // link
      break;
    case LM_REV:
      cont.setLED(3, !bOn2); // reverse
      break;
  }
}

void sendWatts()
{
  if (WsClientID == 0 || wattSend >= 60*60)
    return;

  jsonString js("watts");
  wattSend = js.RLE("watts", wattArr, wattSend, 60*60);
  ws.text(WsClientID, js.Close());
}

void loop()
{
  static uint8_t hour_save, min_save, sec_save, last_day = 32;
  static bool bBtnState;
  static uint32_t btn_time;
  static bool bMotion[2];
#ifdef ESP8266
  MDNS.update();
#endif
#ifdef OTA_ENABLE
  ArduinoOTA.handle();
#endif

  bool bChange = cont.listen();

  static uint8_t nOldLevel[2];
  static uint32_t tm;
  if ((cont.m_nLightLevel[0] != nOldLevel[0] || cont.m_nLightLevel[1] != nOldLevel[1]) && !Serial.available() && (millis() - tm > 400) ) // new level from MCU & no new serial data
  {
    totalUpWatts();
    sendState();
    CallHost(Reason_Level);
    tm = millis();
    manageDevs(DC_DIM, 0);
    nOldLevel[0] = cont.m_nLightLevel[0];
    nOldLevel[1] = cont.m_nLightLevel[1];
  }

  if (cont.m_bPower[0] != bOldOn[0])
  {
    changeLEDs(cont.m_bPower[0], cont.m_bPower[1]);
    CallHost(Reason_Switch0);
    sendState();
    if (bChange && cont.m_bPower[0])
      nSecTimer = 0; // cancel deley-off
    if (nSched && cont.m_bPower[0])
      bOverride = cont.m_bPower[0];
    if (cont.m_bPower[0] && ee.autoTimer)
      nSecTimer = ee.autoTimer;
    if (cont.m_bPower[0] == false)
      totalUpWatts();
    manageDevs(DC_LNK, 0);
    bOldOn[0] = cont.m_bPower[0];
  }

#ifdef RELAY2
  if (cont.m_bPower[1] != bOldOn[1])
  {
    CallHost(Reason_Switch1);
    sendState();
    manageDevs(DC_LNK, 0);
    bOldOn[1] = cont.m_bPower[1];
  }
#endif

  if (ee.motionPin[0])
  {
    bool bMot = digitalRead(ee.motionPin[0]);
    if (bMot != bMotion[0])
    {
      bMotion[0] = bMot;

      if(ee.flags1.led0 == LM_MOT)
        cont.setLED(0, bMot);
      if(ee.flags1.led1 == LM_MOT)
        cont.setLED(1, bMot);

      if(nPresenceCnt == 0)
      {
        nPresenceCnt = ee.nPresenceDelay;
        CallHost(Reason_Motion0);
        sendState();
      }

      nPresenceCnt = ee.nPresenceDelay; // delay before reporting non-presence and turning off
      if(motionSuppress == 0 && bMot)
      { // timing enabled   currently off
        manageDevs(DC_MOT, true); // linked but not power-linked devs: "Mt" checkmark
        if (ee.nMotionSecs && bEnMot)
        {
          nSecTimer = 0; // no turning off while presence high
          if (cont.m_bPower[0] == false) // turn on
          {
            cont.setSwitch(0, true);
            manageDevs(DC_MOT | DC_LNK, true);
          }
        }
      }
    }
  }

  if (ee.motionPin[1])
  {
    bool bMot = digitalRead(ee.motionPin[1]);
    if (bMot != bMotion[1])
    {
      bMotion[1] = bMot;
      if (bMot) CallHost(Reason_Motion1);
    }
  }

  checkQueue();

  checkDevChanges();

  if (sec_save != second()) // only do stuff once per second
  {
    sec_save = second();

    if(!bConfigDone)
    {
      if(WiFi.status() == WL_CONNECTED)
      {
        WiFi.mode(WIFI_STA);
        bConfigDone = true;
        connectTimer = now();
      }
      else if( WiFi.smartConfigDone())
      {
        WiFi.mode(WIFI_STA);
//        Serial.println("SmartConfig set");
        bConfigDone = true;
        connectTimer = now();
      }
    }
    if(bConfigDone)
    {
      if(WiFi.status() == WL_CONNECTED)
      {
        if(!bStarted)
        {
//          Serial.println("WiFi Connected");
          WiFi.mode(WIFI_STA);
          MDNS.begin( ee.szName );
          bStarted = true;
          if (ee.flags1.useNtp)
            wTime.update();
          MDNS.addService("esp", "tcp", serverPort);
          WiFi.SSID().toCharArray(ee.szSSID, sizeof(ee.szSSID)); // Get the SSID from SmartConfig or last used
          WiFi.psk().toCharArray(ee.szSSIDPassword, sizeof(ee.szSSIDPassword) );
          changeLEDs(cont.m_bPower[0], cont.m_bPower[1]);
          MDNSscan();
          ee.update();
        }
        if(wTime.hasUpdated())
        {
          checkSched(true);  // initialize
        }

      }
      else if(now() - connectTimer > 10) // failed to connect for some reason
      {
//        Serial.println("Connect failed. Starting SmartConfig");
        connectTimer = now();
        ee.szSSID[0] = 0;
        WiFi.mode(WIFI_AP_STA);
        WiFi.beginSmartConfig();
        bConfigDone = false;
        bStarted = false;
      }

      if(nPresenceCnt && bMotion[0] == false) // delay on break for mmWave
      {
        if(--nPresenceCnt == 0)
        {
          sendState();
          CallHost(Reason_Motion0);
          manageDevs(DC_MOT, false); // turn motion linked devs off
          if (nSecTimer < ee.nMotionSecs) // skip if schedule is active and higher
            nSecTimer = ee.nMotionSecs; // reset if on
          if (ee.autoTimer)
            nSecTimer = ee.autoTimer;
        }
      }

    } // seconds

    int8_t st = WiFi.status();

    if (st == WL_NO_SSID_AVAIL || st == WL_IDLE_STATUS || st == WL_DISCONNECTED || st == WL_CONNECTION_LOST )
    {
      cont.setLED(0, true); // flicker LED if not connected/connecting
      delay(20);
      cont.setLED(0, false);
    }
    else if (st != WL_CONNECTED)
    {
      static bool bLED;
      cont.setLED(0, bLED);
      bLED = !bLED;
    }

    if (cont.m_bOption)
    {
      cont.m_bOption = false;
      if (motionSuppress)
        motionSuppress = 0;
      else
        motionSuppress = 60 * 60; // long-press (basic switch) = suppress motion for 1 hour
    }

    if (sec_save == 59)
      totalUpWatts();

    if (min_save != minute() && year() > 2020)   // only do stuff once per minute
    {
      min_save = minute();
      WsSend(hourJson(hour_save));
      checkSched(false);        // check every minute for next schedule
      uint8_t hr = hour();
      if (hour_save != hr)  // update our time daily (at 2AM for DST)
      {
        if (hour_save == 0 && last_day != 32)
          WsSend(dayJson(last_day));
        last_day = day() - 1;
        if ( (hour_save = hr) == 2)
        {
          if (ee.flags1.useNtp) wTime.update();
        }
        eHours[hour_save].sec = 0;
        eHours[hour_save].fwh = 0;
        ee.update(); // update EEPROM if needed while we're at it (give user time to make many adjustments)
      }

      ee.nLightLevel[0] = cont.m_nLightLevel[0]; // update EEPROM with last settings
      ee.nLightLevel[1] = cont.m_nLightLevel[1];
      ee.flags1.lightOn0 = cont.m_bPower[0];
      ee.flags1.lightOn1 = cont.m_bPower[1];
    }

    static uint32_t setupCnt = 2;
    if (--setupCnt == 0)
    {
      setupCnt = 15*60;
      CallHost(Reason_Setup);
    }

    if (cont.m_bPower[0])
    {
      ++nDaySec;
      if (++onCounter > (60 * 60 * 12))
        totalUpWatts();
    }

    if (nWrongPass)     // wrong password blocker
      nWrongPass--;

    if (motionSuppress) // disable motion detect for x seconds
      motionSuppress--;

    if (--ssCnt == 0)
      sendState();

    queryDevs();        // only sends changes

    if (nDelayOn) // delay on, controls all channels
    {
      if (--nDelayOn == 0)
      {
        cont.setSwitch(0, true);
#ifdef RELAY2
        cont.setSwitch(1, true);
#endif
        if (ee.autoTimer)
          nSecTimer = ee.autoTimer;
      }
    }

    int idx = minute() * 60 + second();
    if (cont.m_fVolts)
    {
      wattArr[idx] = cont.m_fPower;
    }
    else // fake it
    {
      wattArr[idx] = (cont.m_bPower[0] || cont.m_bPower[1]) ? (((float)ee.watts * cont.getPower() ) / 100) : 0;
    }
    sendWatts();

    if (nWsConnected)
      ws.textAll( dataEnergy() );

    if (nSecTimer) // scedule/motion shut off timer
    {
      if (--nSecTimer == 0)
      {
        if (checkSched(true) == false) // if no active schedule
        {
          cont.setSwitch(0, false);
#ifdef RELAY2
          cont.setSwitch(1, false);
#endif
          nSched = 0;
          sendState(); // in case of monitoring
        }
      }
    }
  }
}

bool checkSched(bool bCheck) // Checks full schedule at the beginning of every minute for turning on
{
  if (bCheck == false && cont.m_bPower[0]) // skip if on, check if true
    return false;

  uint32_t timeNow = ((hour() * 60) + minute()) * 60;

  for (int i = 0; i < MAX_SCHED && ee.schedule[i].sname[0]; i++)
  {
    uint32_t start = ee.schedule[i].timeSch * 60;
    if ((ee.schedule[i].wday & 1) && (ee.schedule[i].wday & (1 << weekday()) ) && // enabled, current day selected
        timeNow >= start && timeNow < start + ee.schedule[i].seconds ) // within time
    {
      if (ee.schedule[i].wday & E_M_On)
        bEnMot = true;
      if (ee.schedule[i].wday & E_M_Off)
        bEnMot = false;

      nSecTimer = ee.schedule[i].seconds - (timeNow - start); // delay for time left

      nSched = i + 1; // 1 = entry 0
      if (bOverride == false && ee.schedule[i].seconds && nSecTimer > 0)
      {
        if (ee.schedule[i].level)
        {
          cont.setLevel(0, ee.schedule[i].level);
          cont.setLevel(1, ee.schedule[i].level);
        }
        else
        {
          cont.setSwitch(0, true);
#ifdef RELAY2
          cont.setSwitch(1, true);
#endif
        }
      }
      return true;
    }
  }
  bOverride = false; // end the override
  return false;
}

void compactSched()   // remove blanks in the middle of a list by shifting up
{
  for (int i = 0; i < MAX_SCHED - 2; i++)
  {
    if (ee.schedule[i].sname[0] == 0)
    {
      ee.schedule[i] = ee.schedule[i + 1];
      memset(&ee.schedule[i + 1], 0, sizeof(Sched) );
    }
  }
}
