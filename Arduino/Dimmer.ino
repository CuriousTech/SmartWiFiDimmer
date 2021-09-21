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
#include "control.h"
#include <JsonParse.h> // https://github.com/CuriousTech/ESP8266-HVAC/tree/master/Libraries/JsonParse
#include <JsonClient.h> // https://github.com/CuriousTech/ESP8266-HVAC/tree/master/Libraries/JsonClient
#include "jsonstring.h"

#define ESP_LED    2  // open (ESP-07 low = blue LED on)

int serverPort = 80;   // listen port

IPAddress lastIP;     // last IP that accessed the device
int nWrongPass;       // wrong password block counter

eeMem eemem;
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
JsonClient jsonPush(jsonPushCallback);

swControl cont;

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
  if(onCounter == 0)
    return;
  float w;
  if(cont.m_fPower) // real power monitor
    w = cont.m_fPower;
  else
    w = (float)ee.watts * cont.getPower(nLevel) / 100;
  ee.fTotalWatts += (float)onCounter * w  / 3600; // add current cycle
  ee.nTotalSeconds += onCounter;
  uint8_t hr = hour();
  eHours[hr].sec += nDaySec;
  eHours[hr].fwh += (float)nDaySec * w  / 3600;
  if(year() > 2019) // Ensure date is valid
  {
    ee.days[day()-1].sec = 0; // total the day from current hours
    ee.days[day()-1].fwh = 0;
    for(int i = 0; i <= hr; i++)
    {
      ee.days[day()-1].sec += eHours[i].sec;
      ee.days[day()-1].fwh += eHours[i].fwh;
    }
    ee.months[month()-1].sec = 0; // refresh the month with new values
    ee.months[month()-1].fwh = 0;
    for(int i = 0; i < day(); i++)
    {
      ee.months[month()-1].sec += ee.days[i].sec;
      ee.months[month()-1].fwh += ee.days[i].fwh;
    }
  }
  nDaySec = 0;
  onCounter = 0;
}

String dataJson()
{
  jsonString js("state");
  js.Var("name", ee.szName);
  js.Var("t", (long)now() - ( (ee.tz + utime.getDST() ) * 3600) );
  js.Var("on", cont.m_bLightOn);
  js.Var("l1", ee.flags1.led1);
  js.Var("l2", ee.flags1.led2);
  js.Var("lvl", cont.m_nLightLevel);
  js.Var("tr", nSecTimer);
  js.Var("sn", nSched);
  totalUpWatts(cont.m_nLightLevel);
  js.Var("ts", ee.nTotalSeconds);
  js.Var("st", ee.nTotalStart);
  if(ee.motionPin[0])
    js.Var("mo", digitalRead(ee.motionPin[0]));
  if(ee.motionPin[1])
    js.Var("mo2", digitalRead(ee.motionPin[1]));
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
  if(cont.m_fVolts)
  {
    js.Var("v", cont.m_fVolts);
    js.Var("c", cont.m_fCurrent);
    js.Var("p", cont.m_fPower);
  }
  else // fake it
  {
    float fw = (cont.m_bLightOn) ? ((float)ee.watts * cont.getPower(cont.m_nLightLevel) / 100) : 0;
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

void parseParams(AsyncWebServerRequest *request)
{
  IPAddress ip = request->client()->remoteIP();

  if(request->params() == 0)
  {
    lastIP = ip;
    return;
  }

  char temp[100];
  char password[64] = "";
  static uint8_t cmd;

  // get password first
  for( uint8_t i = 0; i < request->params(); i++)
  {
    AsyncWebParameter* p = request->getParam(i);

    p->value().toCharArray(temp, 100);
    String s = wifi.urldecode(temp);

    if( p->name().equals("key") )
    {
      s.toCharArray(password, sizeof(password));
      break;
    }
  }

  if(strcmp(ee.szControlPassword, password) || nWrongPass)
  {
    if(nWrongPass == 0) // it takes at least 10 seconds to recognize a wrong password
      nWrongPass = 10;
    else if((nWrongPass & 0xFFFFF000) == 0 ) // time doubles for every high speed wrong password attempt.  Max 1 hour
      nWrongPass <<= 1;
    if(ip != lastIP)  // if different IP drop it down
       nWrongPass = 10;
    jsonString js("hack");
    js.Var("ip", ip.toString() );
    js.Var("pass", password);
    ws.textAll(js.Close());
    lastIP = ip;
    return;
  }

  lastIP = ip;

  const char Names[][8]={
    "ssid", // 0
    "pass",
    "on",
    "level",
    "ch",
    "dly",
    "name",
    "auto",
    "hostip",
    "ntp",
    "port", // 10
    "motpin",
    "reset",
    "clear",
    "cmd",
    "motdly",
    "motpin2",
    "",
  };

  for ( uint8_t i = 0; i < request->params(); i++ ) {
    AsyncWebParameter* p = request->getParam(i);
    p->value().toCharArray(temp, 100);
    String s = wifi.urldecode(temp);
    bool bValue = (s == "true" || s == "1") ? true:false;

    uint8_t idx;
    for(idx = 0; Names[idx][0]; idx++)
      if( p->name().equals(Names[idx]) )
        break;
    switch( idx )
    {
      case 0: // wifi SSID
        s.toCharArray(ee.szSSID, sizeof(ee.szSSID));
        break;
      case 1: // wifi password
        wifi.setPass(s.c_str());
        break;
      case 2: // light on/off
        if(nSched) bOverride = !bValue;
        bOldOn = !bValue; // force report
        cont.setSwitch(bValue);
        if(bValue && ee.autoTimer)
          nSecTimer = ee.autoTimer;
        break;
      case 3: // level
        if(nSched) bOverride = (s.toInt() == 0);
        cont.setLevel( ee.nLightLevel = constrain(s.toInt(), 1, 200) );
        if(s.toInt() && ee.autoTimer)
          nSecTimer = ee.autoTimer;
        break;
      case 4: // callHost enable
        ee.flags1.call = bValue;
        break;
      case 5: // delay off
        if(cont.m_bLightOn)
          nSecTimer = s.toInt();
        break;
      case 6: // name
        if(s.length())
          s.toCharArray(ee.szName, sizeof(ee.szName));
        else
          strcpy(ee.szName, "Dimmer");
        eemem.update();
        ESP.reset();
        break;
      case 7: // autoTimer
        ee.autoTimer = s.toInt();
        break;
      case 8: // host IP / port  (call from host with ?h=80 or give full IP)
        if(s.length() > 9)
        {
          ee.hostPort = 80;
          ip.fromString(s.c_str());
        }
        else
          ee.hostPort = s.toInt() ? s.toInt():80;
        ee.hostIP[0] = ip[0];
        ee.hostIP[1] = ip[1];
        ee.hostIP[2] = ip[2];
        ee.hostIP[3] = ip[3];
        ee.flags1.call = 1;
        CallHost(Reason_Setup); // test
        break;
     case 9: // ntp
        s.toCharArray(ee.ntpServer, sizeof(ee.ntpServer));
        break;
      case 10: // host port
        ee.hostPort = s.toInt() ? s.toInt():80;
        break;
      case 11: // motpin
        ee.motionPin[0] = s.toInt();
        break;
      case 12: // reset
        eemem.update();
        ESP.reset();
        break;
      case 13: // clear device list
        memset(&ee.dev, 0, sizeof(ee.dev));
        eemem.update();
        break;
      case 14:
        cmd = s.toInt();
        break;
      case 15:
        motionSuppress = s.toInt();
        break;
      case 16: // motpin2
        ee.motionPin[1] = s.toInt();
        break;
    }
  }
}

void WsSend(String s)
{
  ws.textAll(s);
}

bool bKeyGood;

void onWsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len)
{  //Handle WebSocket event
  static bool bRestarted = true;

  switch(type)
  {
    case WS_EVT_CONNECT:      //client connected
      if(bRestarted)
      {
        bRestarted = false;
//        client->text("alert;Restarted");
      }
      client->text(dataJson());
//      client->ping();
      nWsConnected++;
      break;
    case WS_EVT_DISCONNECT:    //client disconnected
      compactSched();
      if(nWsConnected)
        nWsConnected--;
      break;
    case WS_EVT_ERROR:    //error was received from the other end
      break;
    case WS_EVT_PONG:    //pong message was received (in response to a ping request maybe)
      break;
    case WS_EVT_DATA:  //data packet
      AwsFrameInfo * info = (AwsFrameInfo*)arg;
      if(info->final && info->index == 0 && info->len == len){
        //the whole message is in a single frame and we got all of it's data
        if(info->opcode == WS_TEXT){
          data[len] = 0;
          char *pCmd = strtok((char *)data, ";"); // assume format is "name;{json:x}"
          char *pData = strtok(NULL, "");
          if(pCmd == NULL || pData == NULL) break;
          bKeyGood = false; // for callback (all commands need a key)
          jsonParse.process(pCmd, pData);
        }
      }
      break;
  }
}

const char *jsonListCmd[] = { "cmd",
  "key", // 0
  "on",
  "tmr",
  "dly",
  "auto",
  "sch",
  "led1",
  "led2",
  "lvl",
  "TZ",
  "NTP", // 10
  "NTPP",
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
  "name",
  "motpin", // 30
  "motpin2",
  "powerup",
  "hostip",
  NULL
};

void jsonCallback(int16_t iEvent, uint16_t iName, int iValue, char *psValue)
{
  static int n;
  static int idx;
  static int dev;

  switch(iEvent)
  {
    case 0: // cmd
      if(!bKeyGood && iName) return; // only allow for key
      switch(iName)
      {
        case 0: // key
          if(psValue) if(!strcmp(psValue, ee.szControlPassword)) // first item must be key
            bKeyGood = true;
          break;
        case 1: // on
          nSecTimer = 0;
          if(iValue == 2) iValue = cont.m_bLightOn^1;
          bOldOn = !iValue; // force report
          cont.setSwitch(iValue);
          if(nSched) bOverride = !iValue;
          if(iValue && ee.autoTimer)
            nSecTimer = ee.autoTimer;
          break;
        case 2: // tmr
          nSecTimer = iValue;
          break;
        case 3: // delay to on
          nDelayOn = iValue;
          break;
        case 4: // auto
          ee.autoTimer = iValue;
          break;
        case 5: // sch
          // schedule a time
          break;
        case 6: // led1
          cont.setLED(0, iValue ? true:false);
          ee.flags1.led1 = iValue;
          break;
        case 7: // led2
          cont.setLED(1, iValue ? true:false);
          ee.flags1.led2 = iValue;
          break;
        case 8: // lvl
          cont.setLevel(iValue);
          break;
        case 9: // TZ
          ee.tz = iValue;
          if(ee.ntpServer[0]) utime.start();
          break;
        case 10: // NTP
          if(psValue) strncpy(ee.ntpServer, psValue, sizeof(ee.ntpServer) );
          utime.start();
          break;
        case 11: // NTPP
          ee.udpPort = iValue;
          if(ee.ntpServer[0]) utime.start();
          break;
        case 12: // MOT
          ee.nMotionSecs = iValue;
          break;
        case 13: // reset
          eemem.update();
          ESP.reset();
          break;
        case 14: // ch
          ee.flags1.call = iValue;
          if(iValue) CallHost(Reason_Setup); // test
          break;
        case 15: // PPWH
          ee.ppkw = iValue;
          break;
        case 16: // watts
          ee.watts = iValue;
          break;
        case 17: // rt
          totalUpWatts(cont.m_nLightLevel);
          ee.fTotalWatts = 0;
          ee.nTotalSeconds = 0;
          ee.nTotalStart = now() - ( (ee.tz + utime.getDST() ) * 3600);
          break;
        case 18: //I
          n = constrain(iValue, 0, MAX_SCHED-1);
          break;
        case 19: // N
          if(psValue)
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
        case 27: // MOT (flag 2) // motion to other switch
          ee.dev[dev].flags &= ~2; // define this later
          ee.dev[dev].flags |= (iValue << 1);
          break;
        case 28: // DLY // delay off (when this one turns on)
          ee.dev[dev].delay = iValue;
          break;
        case 29: // name (host name)
          if(!strlen(psValue))
            break;
          strncpy(ee.szName, psValue, sizeof(ee.szName));
          eemem.update();
          ESP.reset();
          break;
        case 30: // motpin
          ee.motionPin[0] = iValue;
          break;
        case 31: // motpin2
          ee.motionPin[1] = iValue;
          break;
        case 32: // startmode
          ee.flags1.start = iValue;
          break;
        case 33: // hostip
          ee.hostPort = 80;
          ee.hostIP[0] = lastIP[0];
          ee.hostIP[1] = lastIP[1];
          ee.hostIP[2] = lastIP[2];
          ee.hostIP[3] = lastIP[3];
          ee.flags1.call = 1;
          break;
        default:
          break;
      }
      break;
  }
}

const char *jsonListPush[] = { "time",
  "time", // 0
  "ppkw",
  NULL
};

uint8_t failCnt;

void jsonPushCallback(int16_t iEvent, uint16_t iName, int iValue, char *psValue)
{
  switch(iEvent)
  {
    case -1: // status
      if(iName >= JC_TIMEOUT)
      {
        failCnt ++;
        if(failCnt > 5)
          ESP.restart();
      }
      else failCnt = 0;
      break;
    case 0: // time
      switch(iName)
      {
        case 0: // time
          setTime(iValue + (ee.tz * 3600));
          utime.DST();
          setTime(iValue + ((ee.tz + utime.getDST()) * 3600));
          break;
        case 1: // ppkw
          ee.ppkw = iValue;
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
#define CQ_CNT 8
cQ queue[CQ_CNT];
uint8_t qI;

void checkQueue()
{
  static uint32_t cqTime;
  static uint8_t cnt;
  static uint8_t qIdx;

  if(wifi.state() != ws_connected)
    return;
  if(jsonPush.status() != JC_IDLE) // These should be fast, so kill if not
  {
    if( (millis() - cqTime) > 800){
      jsonPush.end();
    }cnt = 50;
    return;
  }
  if(cnt) // delay to fix a bug (previous IP gets used)
    if(--cnt)
      return;

  if(qIdx == qI) // Nothing to do
    return;

  if( jsonPush.begin(queue[qIdx].ip, queue[qIdx].sUri.c_str(), queue[qIdx].port, false, false, NULL, NULL, 300) )
  {
    jsonPush.addList(jsonListPush);
    cqTime = millis();
    cnt = 50;
    queue[qIdx].port = 0;
    if(++qIdx >= CQ_CNT)
      qIdx = 0;
  }
}

bool callQueue(IPAddress ip, String sUri, uint16_t port)
{
  if(queue[qI].port)
  {
    return false; // full
  }
  queue[qI].ip = ip;
  queue[qI].sUri = sUri;
  queue[qI].port = port;

  if(++qI >= CQ_CNT)
    qI = 0;
  return true;
}

void CallHost(reportReason r)
{
  if(ee.hostIP[0] == 0 || ee.flags1.call == 0) // no host set
    return;

  String sUri = String("/wifi?name=\"");
  sUri += ee.szName;
  sUri += "\"&reason=";

  switch(r)
  {
    case Reason_Setup:
      sUri += "setup&port="; sUri += serverPort;
      sUri += "&on="; sUri += cont.m_bLightOn;
      sUri += "&level="; sUri += cont.m_nLightLevel;
      break;
    case Reason_Switch:
      sUri += "switch&on="; sUri += cont.m_bLightOn;
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

void checkDevs(int dt)
{
  for(int i = 0; i < MAX_DEV; i++)
  {
    if(ee.dev[i].IP[0] == 0)
      break;
    IPAddress ip(ee.dev[i].IP);
    String sUri = String("/?key=");
    sUri += ee.szControlPassword; // assumes all paswords are the same
    sUri += "&";
    if((dt & DC_LNK) && ee.dev[i].mode)
    {
      if(ee.dev[i].mode == DM_LNK) // turn other on and off
      {
        if(ee.dev[i].delay) // on/delayed off link
        {
          if(cont.m_bLightOn)
          {
            sUri += "on=";
            sUri += cont.m_bLightOn;
          }
          else
          {
            sUri += "dly=";
            sUri += ee.dev[i].delay;
          }
        }
        else // on/off link
        {
          sUri += "on=";
          sUri += cont.m_bLightOn;
        }
      }
      else if(ee.dev[i].mode == DM_REV) // turn other off if on
      {
        if(cont.m_bLightOn)
        {
          if(ee.dev[i].delay) // delayed off
          {
            sUri += "dly=";
            sUri += ee.dev[i].delay;
          }
          else // instant off
            sUri += "on=0";
        }
      }
      callQueue(ip, sUri, 80);
    }
    if((dt & DC_DIM) && (ee.dev[i].flags & DF_DIM) ) // link dimmer level
    {
      sUri += "level=";
      sUri += cont.m_nLightLevel;
      callQueue(ip, sUri, 80);
    }
    if((dt & DC_MOT) && (ee.dev[i].flags & DF_MOT) ) // test for motion turns on other light
    {
      sUri += "on=1";
      callQueue(ip, sUri, 80);
    }
  }
}

uint8_t ssCnt = 58;

void sendState()
{
  if(nWsConnected)
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

  for(int i = 0; i < n; ++i)
  {
    char szName[38];
    MDNS.hostname(i).toCharArray(szName, sizeof(szName));
    strtok(szName, "."); // remove .local

    for(d = 0; ee.dev[d].IP[0] && d < MAX_DEV; d++)
    {
      if(!strcmp(szName, ee.dev[d].szName))
      {
        ee.dev[d].IP[0] = MDNS.IP(i)[0]; // update IP
        ee.dev[d].IP[1] = MDNS.IP(i)[1];
        ee.dev[d].IP[2] = MDNS.IP(i)[2];
        ee.dev[d].IP[3] = MDNS.IP(i)[3];
        break;
      }
    }
    if(ee.dev[d].IP[0] == 0) // Not found (add it)
    {
      memset(&ee.dev[d], 0, sizeof(Device));
      strcpy(ee.dev[d].szName, szName);
      ee.dev[d].IP[0] = MDNS.IP(i)[0]; // copy IP
      ee.dev[d].IP[1] = MDNS.IP(i)[1];
      ee.dev[d].IP[2] = MDNS.IP(i)[2];
      ee.dev[d].IP[3] = MDNS.IP(i)[3];
      d++;
    }
    if(d) qsort(ee.dev, d, sizeof(struct Device), devComp);
  }
}

void ledSetStub(uint8_t no, bool bOn)
{
  cont.setLED(no, bOn);
}

void setup()
{
  eemem.init();
  cont.init(200);
  wifi.setLEDFunc(ledSetStub);
  wifi.autoConnect(ee.szName, ee.szControlPassword); // Tries config AP, then starts softAP mode for config

  // attach AsyncWebSocket
  ws.onEvent(onWsEvent);
  server.addHandler(&ws);
  
  server.on ( "/", HTTP_GET|HTTP_POST, [](AsyncWebServerRequest *request)
  {
    if(wifi.isCfg())
      request->send( 200, "text/html", wifi.page() ); // WIFI config page
    else
    {
      parseParams(request);
      request->send_P( 200, "text/html", page1);
    }
  });

  server.on( "/s", HTTP_GET|HTTP_POST, [](AsyncWebServerRequest *request)
  {
    parseParams(request);
    request->send(200, "text/plain", dataJson());
  });

  server.onNotFound([](AsyncWebServerRequest *request){
    request->send(404);
  });
  server.on("/heap", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", String(ESP.getFreeHeap()));
  });

  server.on("/data", HTTP_GET, [](AsyncWebServerRequest *request){
    String s = "item=[\n";

    for(int i = 0; i < MAX_SCHED; i++)
    {
      if(i) s += ",\n";
      s += "[\"";  s += ee.schedule[i].sname;
      s += "\","; s += ee.schedule[i].timeSch;
      s += ",";  s += ee.schedule[i].seconds;
      s += ","; s += ee.schedule[i].wday;
      s += ","; s += ee.schedule[i].level;
      s += "]";
      if(ee.schedule[i].sname[0]==0)
        break;
    }
    s += "]\ndev=[\n";

    for(int i = 0; i < MAX_DEV && ee.dev[i].IP[0]; i++)
    {
      if(i) s += ",\n";
      s += "[\"";  s += ee.dev[i].szName;
      s += "\",\""; s += ee.dev[i].IP[0]; s += "."; s += ee.dev[i].IP[1]; s += "."; s += ee.dev[i].IP[2]; s += "."; s += ee.dev[i].IP[3];
      s += "\",";  s += ee.dev[i].mode;
      s += ","; s += ee.dev[i].flags;
      s += ","; s += ee.dev[i].delay;
      s += "]";
    }

    s += "]\ndata=";
    jsonString js;
    js.Var("name", ee.szName);
    js.Var("tz", ee.tz);
    js.Var("mot", ee.nMotionSecs);
    js.Var("ch", ee.flags1.call);
    js.Var("auto", ee.autoTimer);
    js.Var("ppkw", ee.ppkw);
    js.Var("watt", (cont.m_fPower) ? cont.m_fPower : ee.watts);
    js.Var("pu", ee.flags1.start);
    js.Var("dev", cont.getDevice());
    s += js.Close();

    totalUpWatts(cont.m_nLightLevel);
    s += "\nhours=[";
    for(int i = 0; i < 24; i++)
    {
      if(i) s += ",\n";
      s += "[";  s += eHours[i].sec;
      s += ",";  s += eHours[i].fwh;
      s += "]";
    }
    s += "]";
    s += "\ndays=[";
    for(int i = 0; i < 31; i++)
    {
      if(i) s += ",\n";
      s += "[";  s += ee.days[i].sec;
      s += ",";  s += ee.days[i].fwh;
      s += "]";
    }
    s += "]";
    s += "\nmonths=[";
    for(int i = 0; i < 12; i++)
    {
      if(i) s += ",\n";
      s += "[";  s += ee.months[i].sec;
      s += ",";  s += ee.months[i].fwh;
      s += "]";
    }
    s += "]";

    request->send(200, "text/json", s);
  });
  server.on ( "/wifi", HTTP_GET|HTTP_POST, [](AsyncWebServerRequest *request)
  {
    jsonString js;
    js.Var("time", (long)now() - ( (ee.tz + utime.getDST() ) * 3600) );
    js.Var("ppkw", ee.ppkw );
    js.Var("on", cont.m_bLightOn );
    js.Var("level", cont.m_nLightLevel );
    request->send(200, "text/plain", js.Close());
  });
  server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request){
    AsyncWebServerResponse *response = request->beginResponse_P(200, "image/x-icon", favicon, sizeof(favicon));
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  });
  server.on ( "/mdns", HTTP_GET|HTTP_POST, [](AsyncWebServerRequest *request)
  {
    jsonString js;
    for(int i = 0; ee.dev[i].IP[0]; i++)
    {
      String s = ""; // Don't change. Compiler bug
      s += ee.dev[i].IP[0];
      s += ".";
      s += ee.dev[i].IP[1];
      s += ".";
      s += ee.dev[i].IP[2];
      s += ".";
      s += ee.dev[i].IP[3];
      js.Var(ee.dev[i].szName, s);
    }
    request->send(200, "text/plain",js.Close());
  });

  server.onFileUpload([](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final){
  });
  server.onRequestBody([](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
  });
  server.begin();

  jsonParse.addList(jsonListCmd);
#ifdef OTA_ENABLE
  ArduinoOTA.setHostname(ee.szName);
  ArduinoOTA.begin();
  ArduinoOTA.onStart([]() {
    eemem.update();
    cont.setLED(0, false); // set it all to off
    cont.setLED(1, false);
    cont.setSwitch(false);
  });
#endif
  cont.setLevel(ee.nLightLevel);
  switch(ee.flags1.start)
  {
    case 0: cont.setSwitch(ee.flags1.lightOn); break; // last saved
    case 1: cont.setSwitch(true); break; // start on
    case 2: cont.setSwitch(false); break; // start off
  }
  changeLEDs(ee.flags1.lightOn);
}

void changeLEDs(bool bOn)
{
  if(ee.flags1.led1 == 2)
    cont.setLED(0, bOn); // link
  else if(ee.flags1.led1 == 3)
    cont.setLED(0, bOn ? false:true); // reverse
  else
    cont.setLED(0, false); // off
  if(ee.flags1.led2 == 2)
    cont.setLED(1, bOn); // link
  else if(ee.flags1.led2 == 3)
    cont.setLED(1, bOn ? false:true); // reverse
  else
    cont.setLED(1, false); // off
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

  if(!wifi.isCfg() && ee.ntpServer[0])
    utime.check(ee.tz);
  cont.listen();
  wifi.service();
  if(wifi.connectNew())
  {
    MDNS.begin( ee.szName );
    if(ee.ntpServer[0])
      utime.start();
    MDNS.addService("esp", "tcp", serverPort);
    cont.setLED(0, ee.flags1.led1 ? true:false);
    MDNSscan();
  }

  static uint8_t nOldLevel;
  static uint32_t tm;
  if(cont.m_nLightLevel != nOldLevel && !Serial.available() && (millis() - tm > 400) ) // new level from MCU & no new serial data
  {
    totalUpWatts(nOldLevel);
    sendState();
    CallHost(Reason_Level);
    tm = millis();
    checkDevs(DC_DIM);
//    bOldOn = cont.m_bLightOn; // reduce calls
    nOldLevel = cont.m_nLightLevel;
  }

  if(cont.m_bLightOn != bOldOn)
  {
    bOldOn = cont.m_bLightOn;
    changeLEDs(bOldOn);
    CallHost(Reason_Switch);
    sendState();
    if(nSched && cont.m_bLightOn)
      bOverride = cont.m_bLightOn;
    if(cont.m_bLightOn && ee.autoTimer)
      nSecTimer = ee.autoTimer;
    if(cont.m_bLightOn == false)
      totalUpWatts(cont.m_nLightLevel);
    checkDevs(DC_LNK);
  }

  if(ee.motionPin[0])
  {
    bool bMot = digitalRead(ee.motionPin[0]);
    if(bMot != bMotion)
    {
      bMotion = bMot;
      if(motionSuppress == 0)
      {
        if(bMot) // got motion
        { // timing enabled   currently off
          if(ee.nMotionSecs && bEnMot)
          {
            if(nSecTimer < ee.nMotionSecs) // skip if schedule is active and higher
              nSecTimer = ee.nMotionSecs; // reset if on
            if(cont.m_bLightOn == false) // turn on
            {
              cont.setSwitch(true);
              if(ee.autoTimer)
                nSecTimer = ee.autoTimer;
            }
          }
          CallHost(Reason_Motion);
        }
        else
          sendState();
        checkDevs(DC_MOT | DC_LNK);
      }
    }
  }

  if(ee.motionPin[1])
  {
    bool bMot = digitalRead(ee.motionPin[1]);
    if(bMot != bMotion2)
    {
      bMotion2 = bMot;
      if(bMot) CallHost(Reason_Motion2);
    }
  }

  checkQueue();
  if(sec_save != second()) // only do stuff once per second
  {
    sec_save = second();

    if(cont.m_bOption)
    {
      cont.m_bOption = false;
      if(motionSuppress)
        motionSuppress = 0;
      else
        motionSuppress = 60*60; // long-press (basic switch) = suppress motion for 1 hour
    }

    if(sec_save == 59)
      totalUpWatts(cont.m_nLightLevel);

    if(min_save != minute())    // only do stuff once per minute
    {
      min_save = minute();
      WsSend(hourJson(hour_save));
      checkSched(false);        // check every minute for next schedule
      uint8_t hr = hour();
      if (hour_save != hr)  // update our time daily (at 2AM for DST)
      {
        if(hour_save==0 && last_day != 32)
          WsSend(dayJson(last_day));
        last_day = day()-1;
        if( (hour_save = hr) == 2)
        {
          if(ee.ntpServer[0]) utime.start();
        }
        eHours[hour_save].sec = 0;
        eHours[hour_save].fwh = 0;
        eemem.update(); // update EEPROM if needed while we're at it (give user time to make many adjustments)
      }

      static uint8_t setupCnt = 2;
      if(--setupCnt == 0)
      {
        setupCnt = 60;
        CallHost(Reason_Setup);
      }
      ee.nLightLevel = cont.m_nLightLevel; // update EEPROM with last settings
      ee.flags1.lightOn = cont.m_bLightOn;
    }

    if(cont.m_bLightOn)
    {
      ++nDaySec;
      if(++onCounter > (60*60*12))
        totalUpWatts(cont.m_nLightLevel);
    }

    if(nWrongPass)            // wrong password blocker
      nWrongPass--;

    if(motionSuppress)        // disable motion detect for x seconds
      motionSuppress--;

    if(--ssCnt == 0)
       sendState();

    if(nDelayOn)
    {
      if(--nDelayOn == 0)
      {
        cont.setSwitch(true);
        if(ee.autoTimer)
          nSecTimer = ee.autoTimer;
      }
    }

    if(nWsConnected)
      ws.textAll( dataEnergy() );

    if(nSecTimer) // scedule/motion shut off timer
    {
      if(--nSecTimer == 0)
      {
        if(checkSched(true) == false) // if no active scedule
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
  if(bCheck == false && cont.m_bLightOn) // skip if on, check if true
    return false;

  uint32_t timeNow = ((hour()*60) + minute()) * 60;

  for(int i = 0; i < MAX_SCHED && ee.schedule[i].sname[0]; i++)
  {
    uint32_t start = ee.schedule[i].timeSch * 60;
    if((ee.schedule[i].wday&1) && (ee.schedule[i].wday & (1 << weekday()) ) && // enabled, current day selected
        timeNow >= start && timeNow < start + ee.schedule[i].seconds ) // within time
    {
        if(ee.schedule[i].wday & E_M_On)
          bEnMot = true;
        if(ee.schedule[i].wday & E_M_Off)
          bEnMot = false;

        nSecTimer = ee.schedule[i].seconds - (timeNow - start); // delay for time left

        nSched = i + 1; // 1 = entry 0
        if(bOverride == false && ee.schedule[i].seconds && nSecTimer > 0)
        {
          if(ee.schedule[i].level)
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
  for(int i = 0; i < MAX_SCHED - 2; i++)
  {
     if(ee.schedule[i].sname[0] == 0)
     {
        ee.schedule[i] = ee.schedule[i+1];
        memset(&ee.schedule[i+1], 0, sizeof(Sched) );
     }
  }
}
