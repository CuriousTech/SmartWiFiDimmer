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

// Arduino IDE 1.9.0, esp8266 SDK 2.7.1

#define OTA_ENABLE //uncomment to enable Arduino IDE Over The Air update code

#include <Wire.h>
#include <EEPROM.h>
#include <ESP8266mDNS.h>
#include "WiFiManager.h"
#include <ESPAsyncWebServer.h> // https://github.com/me-no-dev/ESPAsyncWebServer
#include <TimeLib.h> // http://www.pjrc.com/teensy/td_libs_Time.html
#include "eeMem.h"
#include <UdpTime.h> // https://github.com/CuriousTech/ESP07_WiFiGarageDoor/tree/master/libraries/UdpTime
#ifdef OTA_ENABLE
#include <FS.h>
#include <ArduinoOTA.h>
#endif
#include "pages.h"
#include <JsonParse.h> // https://github.com/CuriousTech/ESP8266-HVAC/tree/master/Libraries/JsonParse
#include <JsonClient.h> // https://github.com/CuriousTech/ESP8266-HVAC/tree/master/Libraries/JsonClient
#include "jsonstring.h"

//#include "Tuya.h"
//Tuya cont;

//#include "Module.h"
//Module cont;

#include "Switch.h"
Switch cont;

//#include "Paddle.h"
//Paddle cont;

#define ESP_LED   2   // open (ESP-07 low = blue LED on)

int serverPort = 80;  // listen port

IPAddress lastIP;     // last IP that accessed the device
int nWrongPass;       // wrong password block counter

eeMem ee;
UdpTime utime;
uint16_t nSecTimer; // off timer
uint32_t onCounter; // usage timer
uint16_t nDelayOn;  // delay on timer
uint8_t nSched;     // current schedule
bool    bOverride;    // automatic override of schedule
uint8_t nWsConnected;
bool    bEnMot = true;
bool    bOldOn;
uint32_t nDaySec;
uint32_t nDayWh;
uint32_t motionSuppress;
Energy eHours[24];

WiFiManager wifi;  // AP page:  192.168.4.1
AsyncWebServer server( serverPort );
AsyncWebSocket ws("/ws"); // access at ws://[esp ip]/ws

void jsonCallback(int16_t iEvent, uint16_t iName, int iValue, char *psValue);
JsonParse jsonParse(jsonCallback);
void jsonPushCallback(int16_t iEvent, uint16_t iName, int iValue, char *psValue);
JsonClient jsonPush0(jsonPushCallback);
JsonClient jsonPush1(jsonPushCallback);

bool bKeyGood;

float wattArr[60 * 60];

DevState devst[MAX_DEV];

enum reportReason
{
  Reason_Setup,
  Reason_Switch,
  Reason_Level,
  Reason_Motion,
  Reason_Motion2,
  Reason_Test,
};

void totalUpWatts(uint8_t nLevel)
{
  if (onCounter == 0 || year() < 2020)
    return;
  float w;
  if (cont.m_fPower) // real power monitor
    w = cont.m_fPower;
  else
    w = (float)ee.watts * cont.getPower(nLevel) / 100;
  ee.fTotalWatts += (float)onCounter * w  / 3600; // add current cycle
  ee.nTotalSeconds += onCounter;
  uint8_t hr = hour();
  eHours[hr].sec += nDaySec;
  eHours[hr].fwh += (float)nDaySec * w  / 3600;
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
  js.Var("t", (long)now() - ( (ee.tz + utime.getDST() ) * 3600) );
  js.Var("name", ee.szName);
  js.Var("on", cont.m_bPower);
  js.Var("l1", ee.flags1.led1);
  js.Var("l2", ee.flags1.led2);
  js.Var("lvl", cont.m_nLightLevel);
  js.Var("tr", nSecTimer);
  js.Var("sn", nSched);
  totalUpWatts(cont.m_nLightLevel);
  js.Var("ts", ee.nTotalSeconds);
  js.Var("st", ee.nTotalStart);
  js.Var("mp1", ee.motionPin[0]);
  js.Var("mp2", ee.motionPin[1]);
  if (ee.motionPin[0])
    js.Var("mo", digitalRead(ee.motionPin[0]));
  if (ee.motionPin[1])
    js.Var("mo2", digitalRead(ee.motionPin[1]));
  return js.Close();
}

String setupJson()
{
  jsonString js("setup");
  js.Var("name", ee.szName);
  js.Var("tz", ee.tz);
  js.Var("mot", ee.nMotionSecs);
  js.Var("ch", ee.flags1.call);
  js.Var("auto", ee.autoTimer);
  js.Var("ppkw", ee.ppkw);
  js.Var("watt", (cont.m_fPower) ? cont.m_fPower : ee.watts);
  js.Var("pu", ee.flags1.start);
  js.Var("code", cont.getDevice());
  js.Array("sched",  ee.schedule, MAX_SCHED);
  js.Array("dev",  ee.dev, devst, MAX_DEV);
  return js.Close();
}

String dataEnergy()
{
  jsonString js("energy");
  js.Var("t", (long)now() - ( (ee.tz + utime.getDST() ) * 3600) );
  js.Var("rssi", WiFi.RSSI());
  js.Var("tr", nSecTimer);
  totalUpWatts(cont.m_nLightLevel);
  js.Var("wh", ee.fTotalWatts );
  if (cont.m_fVolts)
  {
    js.Var("v", cont.m_fVolts);
    js.Var("c", cont.m_fCurrent);
    js.Var("p", cont.m_fPower);
  }
  else // fake it
  {
    float fw = (cont.m_bPower) ? ((float)ee.watts * cont.getPower(cont.m_nLightLevel) / 100) : 0;
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

  totalUpWatts(cont.m_nLightLevel);

  js.Array("hours", eHours, 24);
  js.Array("days", ee.days, 31);
  js.Array("months", ee.months, 12);
  return js.Close();
}

const char *jsonListCmd[] = { "cmd",
  "key", // 0
  "ssid",
  "pass",
  "pwr",
  "tmr",
  "dlyon",
  "auto",
  "sch",
  "led1",
  "led2",
  "level", // 10
  "TZ",
  "NTP",
  "NTPP",
  "MOT",
  "reset",
  "ch",
  "ppkw",
  "watts",
  "rt",
  "I", // 20
  "N",
  "S",
  "T",
  "W",
  "L",
  "DEV",
  "MODE",
  "DIM",
  "OP",
  "DLY", // 30
  "DON",
  "devname",
  "motpin",
  "motpin2",
  "powerup",
  "hostip",
  "hostport",
  "motdly",
  "clear",
  "src", // 40
  "state",
  NULL
};

void parseParams(AsyncWebServerRequest *request)
{
  if (nWrongPass && request->client()->remoteIP() != lastIP) // if different IP drop it down
    nWrongPass = 10;
  lastIP = request->client()->remoteIP();

  for ( uint8_t i = 0; i < request->params(); i++ )
  {
    AsyncWebParameter* p = request->getParam(i);
    String s = request->urlDecode(p->value());

    uint8_t idx;
    for (idx = 1; jsonListCmd[idx]; idx++)
      if ( p->name().equals(jsonListCmd[idx]) )
        break;
    if (jsonListCmd[idx])
    {
      int iValue = s.toInt();
      if (s == "true") iValue = 1;
      if (i == 0 || bKeyGood)
        jsonCallback(0, idx - 1, iValue, (char *)s.c_str());
    }
  }
}

void WsSend(String s)
{
  ws.textAll(s);
}

int WsClientID;
int wattSend;

void onWsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len)
{ //Handle WebSocket event
  static bool bRestarted = true;

  switch (type)
  {
    case WS_EVT_CONNECT:      //client connected
      if (bRestarted)
        bRestarted = false;

      client->text(dataJson());
      client->text(setupJson());
      client->text( histJson() );
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
          char *pCmd = strtok((char *)data, ";"); // assume format is "name;{json:x}"
          char *pData = strtok(NULL, "");
          if (pCmd == NULL || pData == NULL) break;
          bKeyGood = false; // for callback (all commands need a key)
          jsonParse.process(pCmd, pData);
        }
      }
      break;
  }
}

void jsonCallback(int16_t iEvent, uint16_t iName, int iValue, char *psValue)
{
  static int n;
  static int idx;
  static int dev;
  static String src = "";

  switch (iEvent)
  {
    case 0: // cmd
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
        case 1: // wifi SSID from config page
          strncpy((char *)&ee.szSSID, psValue, sizeof(ee.szSSID));
          break;
        case 2: // wifi password from config page
          wifi.setPass(psValue);
          break;
        case 3: // pwr (was on)
          nSecTimer = 0;
          if (iValue == 2) iValue = cont.m_bPower ^ 1; // toggle
          bOldOn = !iValue; // force report
          cont.setSwitch(iValue);
          if (nSched) bOverride = !iValue;
          if (iValue && ee.autoTimer)
            nSecTimer = ee.autoTimer;
          break;
        case 4: // tmr
          nSecTimer = iValue;
          break;
        case 5: // delay to on
          nDelayOn = iValue;
          break;
        case 6: // auto
          ee.autoTimer = iValue;
          break;
        case 7: // sch
          // schedule a time
          break;
        case 8: // led1
          cont.setLED(0, iValue ? true : false);
          ee.flags1.led1 = iValue;
          break;
        case 9: // led2
          cont.setLED(1, iValue ? true : false);
          ee.flags1.led2 = iValue;
          break;
        case 10: // level
          if (nSched) bOverride = (iValue == 0);
          cont.setLevel( ee.nLightLevel = constrain(iValue, 1, 200) );
          if (iValue && ee.autoTimer)
            nSecTimer = ee.autoTimer;
          break;
        case 11: // TZ
          ee.tz = iValue;
          if (ee.bUseNtp)
            utime.start();
          break;
        case 12: // NTP
          ee.bUseNtp = iValue ? true:false;
          if(ee.bUseNtp)
            utime.start();
          break;
        case 13:
          break;
        case 14: // MOT
          ee.nMotionSecs = iValue;
          break;
        case 15: // reset
          ee.update();
          ESP.reset();
          break;
        case 16: // ch
          ee.flags1.call = iValue;
          if (iValue) CallHost(Reason_Setup); // test
          break;
        case 17: // PPWH
          ee.ppkw = iValue;
          break;
        case 18: // watts
          ee.watts = iValue;
          break;
        case 19: // rt
          totalUpWatts(cont.m_nLightLevel);
          ee.fTotalWatts = 0;
          ee.nTotalSeconds = 0;
          ee.nTotalStart = now() - ( (ee.tz + utime.getDST() ) * 3600);
          break;
        case 20: //I
          n = constrain(iValue, 0, MAX_SCHED - 1);
          break;
        case 21: // N
          if (psValue)
            strncpy(ee.schedule[n].sname, psValue, sizeof(ee.schedule[0].sname) );
          break;
        case 22: // S
          ee.schedule[n].timeSch = iValue;
          break;
        case 23: // T
          ee.schedule[n].seconds = iValue;
          break;
        case 24: // W
          ee.schedule[n].wday = iValue;
          break;
        case 25: // L
          ee.schedule[n].level = iValue;
          break;
        case 26: // DEV index of device to set
          dev = iValue;
          break;
        case 27: // MODE disable, link, reverse
          ee.dev[dev].mode = iValue;
          break;
        case 28: // DIM // link dimming
          ee.dev[dev].flags &= ~1; // define this later
          ee.dev[dev].flags |= (iValue & 1);
          break;
        case 29: // OP (flag 2) // motion to other switch
          ee.dev[dev].flags &= ~2; // define this later
          ee.dev[dev].flags |= (iValue << 1);
          break;
        case 30: // DLY // delay off (when this one turns on)
          ee.dev[dev].delay = iValue;
          break;
        case 31: // DON  device power (see checkDevChanges)
          devst[dev].cmd = iValue + 1;
          devst[dev].bPwr = iValue ? true : false;
          break;
        case 32: // devname (host name)
          if (!strlen(psValue))
            break;
          strncpy(ee.szName, psValue, sizeof(ee.szName));
          ee.update();
          ESP.reset();
          break;
        case 33: // motpin
          ee.motionPin[0] = iValue;
          break;
        case 34: // motpin2
          ee.motionPin[1] = iValue;
          break;
        case 35: // startmode
          ee.flags1.start = iValue;
          break;
        case 36: // hostip
          ee.hostPort = 80;
          ee.hostIP[0] = lastIP[0];
          ee.hostIP[1] = lastIP[1];
          ee.hostIP[2] = lastIP[2];
          ee.hostIP[3] = lastIP[3];
          ee.flags1.call = 1;
          CallHost(Reason_Setup); // test
          break;
        case 37: // host port
          ee.hostPort = iValue;
          break;
        case 38:
          motionSuppress = iValue;
          break;
        case 39: // clear device list
          memset(&ee.dev, 0, sizeof(ee.dev));
          ee.update();
          break;
        case 40: // src : name of querying device
          src = psValue;
          break;
        case 41: // state : also current state of querying device
          if (src.length())
            setDevState(src, iValue ? true : false);
          break;
        default:
          break;
      }
      break;
  }
}

void setDevState(String sDev, bool bPwr)
{
  for (int d = 0; ee.dev[d].IP[0] && d < MAX_DEV; d++)
  {
    if (sDev.equals(ee.dev[d].szName))
    {
      if (devst[d].bPwr != bPwr)
        devst[d].bChanged = true;
      devst[d].bPwr = bPwr;
      devst[d].bPwrS = cont.m_bPower; // Response will update client, so set here
      break;
    }
  }
}

const char *jsonListPush[] = { "time",
 "name", // 0
 "time",
 "ppkw",
 "tz",
 "on",
 "level", // 5
 NULL
};

void jsonPushCallback(int16_t iEvent, uint16_t iName, int iValue, char *psValue)
{
  static int8_t nDev;
  static uint8_t failCnt;

  switch (iEvent)
  {
    case -1: // status
      if (iName >= JC_TIMEOUT)
      {
        failCnt ++;
        if (failCnt > 5)
          ESP.restart();
      }
      else failCnt = 0;
      break;
    case 0: // time
      switch (iName)
      {
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
        case 1: // time
          if (iValue < now() + ((ee.tz + utime.getDST()) * 3600) )
            break;
          setTime(iValue + (ee.tz * 3600));
          utime.DST();
          setTime(iValue + ((ee.tz + utime.getDST()) * 3600));
          if (nDev < 0) break;
          devst[nDev].tm = iValue;
          break;
        case 2: // ppkw
          ee.ppkw = iValue;
        case 3: // tz
          ee.tz = iValue;
          break;
        case 4: // on
          if (nDev < 0) break;
          if (devst[nDev].bPwr != iValue)
            devst[nDev].bChanged = true;
          devst[nDev].bPwr = iValue;
          break;
        case 5: // level
          if (nDev < 0) break;
          if (devst[nDev].nLevel != iValue)
            devst[nDev].bChanged = true;
          devst[nDev].nLevel = iValue;
          break;
      }
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
  if(wifi.state() != ws_connected)
    return;

  int idx;
  for(idx = 0; idx < CQ_CNT; idx++)
  {
    if(queue[idx].port)
      break;
  }
  if(idx == CQ_CNT) // nothing to do
    return;

  if (jsonPush0.status() == JC_IDLE)
  {
    if ( jsonPush0.begin(queue[idx].ip, queue[idx].sUri.c_str(), queue[idx].port, false, false, NULL, NULL, 500) )
    {
      jsonPush0.addList(jsonListPush);
      queue[idx].port = 0;
    }
  }
  else if (jsonPush1.status() == JC_IDLE) // Using alternating requests now for faster response
  {
    if ( jsonPush1.begin(queue[idx].ip, queue[idx].sUri.c_str(), queue[idx].port, false, false, NULL, NULL, 500) )
    {
      jsonPush1.addList(jsonListPush);
      queue[idx].port = 0;
    }
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

  String sUri = String("/wifi?name=\"");
  sUri += ee.szName;
  sUri += "\"&reason=";

  switch (r)
  {
    case Reason_Setup:
      sUri += "setup&port="; sUri += serverPort;
      sUri += "&on="; sUri += cont.m_bPower;
      sUri += "&level="; sUri += cont.m_nLightLevel;
      break;
    case Reason_Switch:
      sUri += "switch&on="; sUri += cont.m_bPower;
      break;
    case Reason_Level:
      sUri += "level&value="; sUri += cont.m_nLightLevel;
      break;
    case Reason_Motion:
      sUri += "motion";
      break;
    case Reason_Motion2:
      sUri += "motion2";
      break;
  }

  IPAddress ip(ee.hostIP);
  callQueue(ip, sUri, ee.hostPort);
}

#define DC_LNK 1
#define DC_DIM 2
#define DC_MOT 4

void manageDevs(int dt)
{
  String sUri;

  for (int i = 0; ee.dev[i].IP[0] && i < MAX_DEV; i++)
  {
    if (ee.dev[i].mode == 0 && ee.dev[i].flags == 0)
      continue;

    IPAddress ip(ee.dev[i].IP);
    sUri = String("/wifi?key=");
    sUri += ee.szControlPassword; // assumes all paswords are the same
    sUri += "&src=";
    sUri += ee.szName;
    sUri += "&";

    if ((dt & DC_LNK) && ee.dev[i].mode)
    {
      if (ee.dev[i].mode == DM_LNK) // turn other on and off
      {
        if (ee.dev[i].delay) // on/delayed off link
        {
          if (cont.m_bPower)
          {
            sUri += "pwr=";
            sUri += cont.m_bPower;
          }
          else
          {
            sUri += "tmr=";
            sUri += ee.dev[i].delay;
          }
        }
        else // on/off link
        {
          sUri += "pwr=";
          sUri += cont.m_bPower;
        }
      }
      else if (ee.dev[i].mode == DM_REV) // turn other off if on
      {
        if (cont.m_bPower)
        {
          if (ee.dev[i].delay) // delayed off
          {
            sUri += "tmr=";
            sUri += ee.dev[i].delay;
          }
          else // instant off
          {
            sUri += "pwr=0";
          }
        }
        else
          continue;
      }
      else
        continue;
      sUri += "&state=";
      sUri += cont.m_bPower;
      devst[i].bPwrS = cont.m_bPower;
      callQueue(ip, sUri, 80);
//      String s = "print;Link ";
//      s += ip.toString();
//      s += sUri;
//      WsSend(s);
    }
    if ((dt & DC_DIM) && (ee.dev[i].flags & DF_DIM) ) // link dimmer level
    {
      sUri += "level=";
      sUri += cont.m_nLightLevel;
      sUri += "&state=";
      sUri += cont.m_bPower;
      devst[i].bPwrS = cont.m_bPower;
      callQueue(ip, sUri, 80);
    }
    if ((dt & DC_MOT) && (ee.dev[i].flags & DF_MOT) ) // test for motion turns on other light
    {
      sUri += "pwr=1";
      sUri += "&state=";
      sUri += cont.m_bPower;
      devst[i].bPwrS = cont.m_bPower;
      callQueue(ip, sUri, 80);
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

  if (devst[nDev].bPwrS == cont.m_bPower) // && devst[nDev].nLevelS == cont.m_nLightLevel)
  {
    nDev++;
    return;
  }

  String sUri = String("/wifi?key=");
  sUri += ee.szControlPassword;
  sUri += "&src=";
  sUri += ee.szName;
  sUri += "&state=";
  sUri += cont.m_bPower;
  devst[nDev].bPwrS = cont.m_bPower;

  IPAddress ip(ee.dev[nDev].IP);
  if ( callQueue(ip, sUri, 80) )
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
      js.Var("on", devst[i].bPwr);
      js.Var("level", devst[i].nLevel);
      ws.textAll( js.Close() );
    }
    if (devst[i].cmd) // send commands to other device
    {
      String sUri = "/wifi?key=";
      sUri += ee.szControlPassword;
      sUri += "&src=";
      sUri += ee.szName;
      sUri += "&pwr=";
      sUri += devst[i].cmd - 1;
      sUri += "&state=";
      sUri += cont.m_bPower;
      devst[i].bPwrS = cont.m_bPower;
      IPAddress ip(ee.dev[i].IP);
      callQueue(ip, sUri, 80);
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

void MDNSscan()
{
  int n = MDNS.queryService("esp", "tcp");
  int d;

  for (int i = 0; i < n; ++i)
  {
    char szName[38];
    MDNS.hostname(i).toCharArray(szName, sizeof(szName));
    strtok(szName, "."); // remove .local

    for (d = 0; ee.dev[d].IP[0] && d < MAX_DEV; d++)
    {
      if (!strcmp(szName, ee.dev[d].szName))
      {
        ee.dev[d].IP[0] = MDNS.IP(i)[0]; // update IP
        ee.dev[d].IP[1] = MDNS.IP(i)[1];
        ee.dev[d].IP[2] = MDNS.IP(i)[2];
        ee.dev[d].IP[3] = MDNS.IP(i)[3];
        break;
      }
    }
    if (ee.dev[d].IP[0] == 0) // Not found (add it)
    {
      memset(&ee.dev[d], 0, sizeof(Device));
      strcpy(ee.dev[d].szName, szName);
      ee.dev[d].IP[0] = MDNS.IP(i)[0]; // copy IP
      ee.dev[d].IP[1] = MDNS.IP(i)[1];
      ee.dev[d].IP[2] = MDNS.IP(i)[2];
      ee.dev[d].IP[3] = MDNS.IP(i)[3];
      d++;
    }
    if (d) qsort(ee.dev, d, sizeof(struct Device), devComp);
  }
}

void setup()
{
  cont.init(200);
  wifi.autoConnect(ee.szName, ee.szControlPassword); // Tries config AP, then starts softAP mode for config

  // attach AsyncWebSocket
  ws.onEvent(onWsEvent);
  server.addHandler(&ws);

  server.on ( "/", HTTP_GET | HTTP_POST, [](AsyncWebServerRequest * request)
  {
    if (wifi.state() == ws_config)
      request->send( 200, "text/html", wifi.page() ); // WIFI config page
    else
    {
      parseParams(request);
      request->send_P( 200, "text/html", page1);
    }
  });

  server.on( "/s", HTTP_GET | HTTP_POST, [](AsyncWebServerRequest * request)
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
    if (year() > 2020)
      js.Var("time", (long)now() - ( (ee.tz + utime.getDST() ) * 3600) );
    js.Var("ppkw", ee.ppkw );
    js.Var("tz", ee.tz );
    js.Var("on", cont.m_bPower );
    js.Var("level", cont.m_nLightLevel );
    request->send(200, "text/plain", js.Close());
  });
  server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest * request) {
    AsyncWebServerResponse *response = request->beginResponse_P(200, "image/x-icon", favicon, sizeof(favicon));
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  });
  server.on ( "/mdns", HTTP_GET | HTTP_POST, [](AsyncWebServerRequest * request)
  {
    jsonString js;
    for (int i = 0; ee.dev[i].IP[0]; i++)
    {
      IPAddress ip(ee.dev[i].IP);
      js.Var(ee.dev[i].szName, ip.toString());
    }
    request->send(200, "text/plain", js.Close());
  });

  server.onFileUpload([](AsyncWebServerRequest * request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
  });
  server.onRequestBody([](AsyncWebServerRequest * request, uint8_t *data, size_t len, size_t index, size_t total) {
  });
  server.begin();

  jsonParse.addList(jsonListCmd);
#ifdef OTA_ENABLE
  ArduinoOTA.setHostname(ee.szName);
  ArduinoOTA.begin();
  ArduinoOTA.onStart([]() {
    cont.setLED(0, false); // set it all to off
    cont.setLED(1, false);
    cont.setSwitch(false);
    ee.update();
  });
#endif
  cont.setLevel(ee.nLightLevel);
  switch (ee.flags1.start)
  {
    case 0: cont.setSwitch(ee.flags1.lightOn); break; // last saved
    case 1: cont.setSwitch(false); break; // start off
    case 2: cont.setSwitch(true); break; // start on
  }
}

void changeLEDs(bool bOn)
{
  switch (ee.flags1.led1)
  {
    case 0: // off
      cont.setLED(0, false);
      break;
    case 1: // on
      cont.setLED(0, true);
      break;
    case 2:
      cont.setLED(0, bOn); // link
      break;
    case 3:
      cont.setLED(0, !bOn); // reverse
      break;
  }
  switch (ee.flags1.led2)
  {
    case 0: // off
      cont.setLED(1, false);
      break;
    case 1: // on
      cont.setLED(1, true);
      break;
    case 2:
      cont.setLED(1, bOn); // link
      break;
    case 3:
      cont.setLED(1, !bOn); // reverse
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
  static bool bMotion, bMotion2;

  MDNS.update();
#ifdef OTA_ENABLE
  ArduinoOTA.handle();
#endif

  if (wifi.state() == ws_connected)
    utime.check(ee.tz);
  if (wifi.service() == ws_connectSuccess)
  {
    MDNS.begin( ee.szName );
    if (ee.bUseNtp)
      utime.start();
    MDNS.addService("esp", "tcp", serverPort);
    changeLEDs(cont.m_bPower);
    MDNSscan();
  }

  bool bChange = cont.listen();

  static uint8_t nOldLevel;
  static uint32_t tm;
  if (cont.m_nLightLevel != nOldLevel && !Serial.available() && (millis() - tm > 400) ) // new level from MCU & no new serial data
  {
    totalUpWatts(nOldLevel);
    sendState();
    CallHost(Reason_Level);
    tm = millis();
    manageDevs(DC_DIM);
    //    bOldOn = cont.m_bPower; // reduce calls
    nOldLevel = cont.m_nLightLevel;
  }

  if (cont.m_bPower != bOldOn)
  {
    changeLEDs(cont.m_bPower);
    CallHost(Reason_Switch);
    sendState();
    if (bChange && cont.m_bPower)
      nSecTimer = 0; // cancel deley-off
    if (nSched && cont.m_bPower)
      bOverride = cont.m_bPower;
    if (cont.m_bPower && ee.autoTimer)
      nSecTimer = ee.autoTimer;
    if (cont.m_bPower == false)
      totalUpWatts(cont.m_nLightLevel);
    manageDevs(DC_LNK);
    bOldOn = cont.m_bPower;
  }

  if (ee.motionPin[0])
  {
    bool bMot = digitalRead(ee.motionPin[0]);
    if (bMot != bMotion)
    {
      bMotion = bMot;
      sendState();
      if (motionSuppress == 0 && bMot)
      { // timing enabled   currently off
        if (ee.nMotionSecs && bEnMot)
        {
          if (nSecTimer < ee.nMotionSecs) // skip if schedule is active and higher
            nSecTimer = ee.nMotionSecs; // reset if on
          if (cont.m_bPower == false) // turn on
          {
            cont.setSwitch(true);
            if (ee.autoTimer)
              nSecTimer = ee.autoTimer;
          }
        }
        CallHost(Reason_Motion);
        manageDevs(DC_MOT | DC_LNK);
      }
    }
  }

  if (ee.motionPin[1])
  {
    bool bMot = digitalRead(ee.motionPin[1]);
    if (bMot != bMotion2)
    {
      bMotion2 = bMot;
      if (bMot) CallHost(Reason_Motion2);
    }
  }

  checkQueue();

  checkDevChanges();

  if (sec_save != second()) // only do stuff once per second
  {
    sec_save = second();

    if (wifi.state() == ws_config)
    {
      static bool bLED;
      cont.setLED(0, bLED);
      bLED = !bLED;
    }
    else if (wifi.state() == ws_connecting)
    {
      cont.setLED(0, true);
      delay(20);
      cont.setLED(0, false);
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
      totalUpWatts(cont.m_nLightLevel);

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
          if (ee.bUseNtp) utime.start();
        }
        eHours[hour_save].sec = 0;
        eHours[hour_save].fwh = 0;
        ee.update(); // update EEPROM if needed while we're at it (give user time to make many adjustments)
      }

      static uint8_t setupCnt = 2;
      if (--setupCnt == 0)
      {
        setupCnt = 60;
        CallHost(Reason_Setup);
      }
      ee.nLightLevel = cont.m_nLightLevel; // update EEPROM with last settings
      ee.flags1.lightOn = cont.m_bPower;

    }

    if (cont.m_bPower)
    {
      ++nDaySec;
      if (++onCounter > (60 * 60 * 12))
        totalUpWatts(cont.m_nLightLevel);
    }

    if (nWrongPass)     // wrong password blocker
      nWrongPass--;

    if (motionSuppress) // disable motion detect for x seconds
      motionSuppress--;

    if (--ssCnt == 0)
      sendState();

    queryDevs();        // only sends changes

    if (nDelayOn)
    {
      if (--nDelayOn == 0)
      {
        cont.setSwitch(true);
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
      wattArr[idx] = (cont.m_bPower) ? ((float)ee.watts * cont.getPower(cont.m_nLightLevel) / 100) : 0;
    }
    sendWatts();

    if (nWsConnected)
      ws.textAll( dataEnergy() );

    if (nSecTimer) // scedule/motion shut off timer
    {
      if (--nSecTimer == 0)
      {
        if (checkSched(true) == false) // if no active scedule
        {
          cont.setSwitch(false);
          nSched = 0;
          sendState(); // in case of monitoring
        }
      }
    }
  }
}

bool checkSched(bool bCheck) // Checks full schedule at the beginning of every minute
{
  if (bCheck == false && cont.m_bPower) // skip if on, check if true
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
          cont.setLevel(ee.schedule[i].level);
        else
          cont.setSwitch(true);
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
