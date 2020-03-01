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

// Arduino IDE 1.9.0, esp8266 SDK 2.6.3

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
//#define MOTION    14  // Basement=12, LivingRoom=16 RCW-0516, Back switch 14

int serverPort = 80;   // listen port

IPAddress lastIP;     // last IP that accessed the device
int nWrongPass;       // wrong password block counter

eeMem eemem;
UdpTime utime;
uint16_t nSecTimer; // off timer
uint16_t onCounter; // usage timer
uint16_t nDelayOn;  // delay on timer
uint8_t nSched;     // current schedule
bool    bOverride;    // automatic override of schedule
uint8_t nWsConnected;
bool    bEnMot = true;
bool    bOldOn;

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
  Reason_Test,
};

void totalUpWatts()
{
  if(onCounter == 0)
    return; // level 0 = switch
  uint8_t m = (cont.m_nLightLevel ? map(cont.m_nLightLevel, 0, 100, 60, 100) : 100);  // 1% = about 60% power
  float w = (float)ee.watts * m / 100;
  ee.fTotalWatts += (float)onCounter * w  / 3600; // add current cycle
  ee.nTotalSeconds += onCounter;
  onCounter = 0;
}

String dataJson()
{
  jsonString js("state");
  js.Var("t", now() - ( (ee.tz + utime.getDST() ) * 3600) );
  js.Var("on", cont.m_bLightOn);
  js.Var("l1", ee.bLED[0]);
  js.Var("l2", ee.bLED[1]);
  js.Var("lvl", constrain(cont.m_nLightLevel, 1, 100));
  js.Var("tr", nSecTimer);
  js.Var("sn", nSched);
  js.Var("rssi", WiFi.RSSI());
  totalUpWatts();
  js.Var("wh", ee.fTotalWatts );
  js.Var("ts", ee.nTotalSeconds);
#ifdef MOTION
  js.Var("mo", digitalRead(MOTION));
#endif
  return js.Close();
}

void parseParams(AsyncWebServerRequest *request)
{
  if(request->params() == 0)
    return;

  char temp[100];
  char password[64] = "";

  // get password first
  for ( uint8_t i = 0; i < request->params(); i++ ) {
    AsyncWebParameter* p = request->getParam(i);

    p->value().toCharArray(temp, 100);
    String s = wifi.urldecode(temp);

    if( p->name().equals("key") )
    {
      s.toCharArray(password, sizeof(password));
      break;
    }
  }

  IPAddress ip = request->client()->remoteIP();

  if(strcmp(ee.szControlPassword, password))
  {
    if(nWrongPass == 0) // it takes at least 10 seconds to recognize a wrong password
      nWrongPass = 10;
    else if((nWrongPass & 0xFFFFF000) == 0 ) // time doubles for every high speed wrong password attempt.  Max 1 hour
      nWrongPass <<= 1;
    if(ip != lastIP)  // if different IP drop it down
       nWrongPass = 10;
    jsonString js("hack");
    js.Var("ip", request->client()->remoteIP().toString() );
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
        cont.setLevel( constrain(s.toInt(), 1, 100) );
        if(s.toInt() && ee.autoTimer)
          nSecTimer = ee.autoTimer;
        break;
      case 4: // callHost enable
        ee.bCall = bValue;
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
        CallHost(Reason_Setup); // test
        break;
     case 9: // ntp
        s.toCharArray(ee.ntpServer, sizeof(ee.ntpServer));
        break;
      case 10: // host port
        ee.hostPort = s.toInt() ? s.toInt():80;
        break;
    }
  }
}

void WsSend(String s)
{
  ws.textAll(s);
}

String removeQuotes(String s)
{
  if(s.charAt(0) == '"')
  {
    s.remove(0,1);
    s.remove(s.length()-1, 1);
  }
  return s;
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
        client->text("alert;Restarted");
      }
      client->text(dataJson());
      client->ping();
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
  "DEL",
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
  "NM",
  "IP",
  "MODE",
  "DIM",
  "OP",
  "DLY", // 30
  NULL
};

void jsonCallback(int16_t iEvent, uint16_t iName, int iValue, char *psValue)
{
  char *p, *p2;
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
          ee.bLED[0] = iValue;
          break;
        case 7: // led2
          cont.setLED(1, iValue ? true:false);
          ee.bLED[1] = iValue;
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
        case 13: // del
          ee.dev[dev].IP[0] = 0;
          for(int i = 0; i < MAX_DEV; i++)
          {
            if(ee.dev[dev].IP[0] == 0)
            {
              memset(&ee.dev[i], 0, sizeof(Device));
              if( i < MAX_DEV - 1)
                memmove(&ee.dev[i], &ee.dev[i+1], sizeof(Device) * (MAX_DEV - i));
              break;
            }
          }
          break;
        case 14: // ch
          ee.bCall = iValue ? true:false;
          if(iValue) CallHost(Reason_Setup); // test
          break;
        case 15: // PPWH
          ee.ppkw = iValue;
          break;
        case 16: // watts
          ee.watts = iValue;
          break;
        case 17: // rt
          ee.fTotalWatts = 0;
          ee.nTotalSeconds = 0;
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
        case 24: // DEV index of device it set
          dev = iValue;
          break;
        case 25: // NM name of device
          strncpy(ee.dev[dev].szName, psValue, sizeof(ee.dev[dev].szName) );
          break;
        case 26: // IP of devie
          {
            IPAddress ip;
            if (ip.fromString(psValue))
            {
              ee.dev[dev].IP[0] = ip[0];
              ee.dev[dev].IP[1] = ip[1];
              ee.dev[dev].IP[2] = ip[2];
              ee.dev[dev].IP[3] = ip[3];
            }
          }
          break;
        case 27: // MODE disable, link, reverse
          ee.dev[dev].mode = iValue;
          break;
        case 28: // DIM // link dimming
          ee.dev[dev].flags &= ~1; // define this later
          ee.dev[dev].flags |= iValue;
          break;
        case 29: // OP (flaf 2) // next option?
          ee.dev[dev].flags &= ~2; // define this later
          ee.dev[dev].flags |= (iValue << 1);
          break;
        case 30: // DLY // delay off (when this one turns on)
          ee.dev[dev].delay = iValue;
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
          setTime(iValue + ( (ee.tz + utime.getDST() ) * 3600));
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

cQ queue[32];

void checkQueue()
{
  static int cnt;
  if(jsonPush.status() != JC_IDLE) // timeout in asyncClient fails
  {
    if(++cnt > 50)
    {
      jsonPush.end();
      cnt = 0;
    }
    return;
  }
  cnt = 0;

  int i;
  for(i = 0; i < 32; i++)
  {
    if(queue[i].ip[0])
      break;
  }
  if(i == 32) return;
  String url = queue[i].ip.toString();
  String s = "send ";
  s += url;
  s += " ";
  s += queue[i].sUri;
  ws.textAll(s);
  jsonPush.begin(url.c_str(), queue[i].sUri.c_str(), queue[i].port, false, false, NULL, NULL);
  jsonPush.addList(jsonListPush);
  queue[i].ip[0] = 0;
}

void callQueue(IPAddress ip, String sUri, uint16_t port)
{
  int i;
  for(i = 0; i < 32; i++)
  {
    if(queue[i].ip[0] == 0)
      break;
  }
  if(i == 32) return; // full
  String s ="set ";
  s += ip.toString();
  s += " ";
  s += sUri;
  ws.textAll(s);
  queue[i].ip = ip;
  queue[i].sUri = sUri;
  queue[i].port = port;
}

void CallHost(reportReason r)
{
  if(ee.hostIP[0] == 0 || ee.bCall == false) // no host set
    return;

  String sUri = String("/wifi?name=\"");
  sUri += ee.szName;
  sUri += "\"&reason=";

  switch(r)
  {
    case Reason_Setup:
      sUri += "setup&port="; sUri += serverPort;
      sUri += "&on="; sUri += cont.m_bLightOn;
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
  }

  IPAddress ip(ee.hostIP);
  callQueue(ip, sUri, ee.hostPort);
}

enum dev_mng_type{
  DC_LNK,
  DC_DIM,
  DC_OP,
};

void checkDevs(int dt)
{
  for(int i = 0; i < MAX_DEV; i++)
  {
    if(ee.dev[i].IP[0])
    {
      IPAddress ip(ee.dev[i].IP);
      String sUri = String("/?key=");
      sUri += ee.szControlPassword; // assumes all paswords are the same
      sUri += "&";
      if(dt == DC_LNK && ee.dev[i].mode)
      {
        if(ee.dev[i].mode == 1) // turn other on or off
        {
          sUri += "on=";
          sUri += cont.m_bLightOn;
        }
        else // reverse mode
        {
          if(ee.dev[i].delay) // delayed off
          {
            sUri += "dly=";
            sUri += ee.dev[i].delay;
          }
          else if(cont.m_bLightOn) // instant off
          {
            sUri += "on=0";
          }
        }
        callQueue(ip, sUri, 80);
      }
      if(dt == DC_DIM && (ee.dev[i].flags&1) ) // link dimmer level
      {
        sUri += "level=";
        sUri += cont.m_nLightLevel;
        callQueue(ip, sUri, 80);
      }
      if(dt == DC_OP && (ee.dev[i].flags&2) ) // test for motion turns on other light
      {
        sUri += "on=1";
        callQueue(ip, sUri, 80);
      }
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

void setup()
{
  cont.init();
  WiFi.hostname(ee.szName);
  wifi.autoConnect(ee.szName, ee.szControlPassword); // Tries config AP, then starts softAP mode for config
  if(wifi.isCfg() == false)
  {
    if ( !MDNS.begin ( ee.szName, WiFi.localIP() ) );
    cont.setLED(0, ee.bLED[0] ? true:false);
  }
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

  server.on ( "/s", HTTP_GET|HTTP_POST, [](AsyncWebServerRequest *request)
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
    js.Var("ch", ee.bCall);
    js.Var("auto", ee.autoTimer);
    js.Var("ppkw", ee.ppkw);
    js.Var("watt", ee.watts);
    s += js.Close();

    request->send(200, "text/json", s);
  });
  server.on ( "/wifi", HTTP_GET|HTTP_POST, [](AsyncWebServerRequest *request)
  {
    jsonString js;
    js.Var("time", now() - ( (ee.tz + utime.getDST() ) * 3600) );
    js.Var("ppkw", ee.ppkw );
    request->send(200, "text/plain", js.Close());
  });
  server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request){
    AsyncWebServerResponse *response = request->beginResponse_P(200, "image/x-icon", favicon, sizeof(favicon));
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  });

  server.onFileUpload([](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final){
  });
  server.onRequestBody([](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
  });
  server.begin();

  MDNS.addService("http", "tcp", serverPort);

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
  if(wifi.isCfg() == false && ee.ntpServer[0])
    utime.start();
  cont.m_nLightLevel = ee.nLightLevel;
  cont.setSwitch(ee.bLightOn);
}

void loop()
{
  static uint8_t hour_save, min_save, sec_save;
  static bool bBtnState;
  static uint32_t btn_time;
  static bool bMotion;

  MDNS.update();
#ifdef OTA_ENABLE
  ArduinoOTA.handle();
#endif

  if(!wifi.isCfg() && ee.ntpServer[0])
    utime.check(ee.tz);
  cont.listen();

  static uint8_t nOldLevel;
  static uint32_t tm;
  if(cont.m_nLightLevel != nOldLevel && !Serial.available() && (millis() - tm > 120) ) // new level from MCU & no new serial data
  {
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
    if(ee.bLED[0] == 2)
      cont.setLED(0, bOldOn); // link
    else if(ee.bLED[0] == 3)
      cont.setLED(0, bOldOn ? false:true); // reverse
    if(ee.bLED[1] == 2)
      cont.setLED(1, bOldOn); // link
    else if(ee.bLED[1] == 3)
      cont.setLED(1, bOldOn ? false:true); // reverse
    CallHost(Reason_Switch);
    sendState();
    if(nSched && cont.m_bLightOn)
      bOverride = cont.m_bLightOn;
    if(cont.m_bLightOn && ee.autoTimer)
      nSecTimer = ee.autoTimer;

    if(cont.m_bLightOn == false)
      totalUpWatts();
    checkDevs(DC_LNK);
  }

#ifdef MOTION
  bool bMot = digitalRead(MOTION);
  if(bMot != bMotion)
  {
    bMotion = bMot;
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
    }
    if(bMot == false)
      sendState();
    else
      CallHost(Reason_Motion);
    checkDevs(DC_OP);
  }
#endif

  checkQueue();
  if(sec_save != second()) // only do stuff once per second
  {
    sec_save = second();

    if(wifi.isCfg())
    {
      wifi.seconds();
      cont.setLED(0, !cont.m_bLED ); // blink for config
    }

    if(min_save != minute())    // only do stuff once per minute
    {
      min_save = minute();
      checkSched(false);        // check every minute for next schedule
      if (hour_save != hour())  // update our time daily (at 2AM for DST)
      {
        if( (hour_save = hour()) == 2)
        {
          if(ee.ntpServer[0]) utime.start();
        }
        eemem.update(); // update EEPROM if needed while we're at it (give user time to make many adjustments)
      }

      static uint8_t setupCnt = 2;
      if(--setupCnt == 0)
      {
        setupCnt = 60;
        CallHost(Reason_Setup);
      }
      ee.nLightLevel = cont.m_nLightLevel; // update EEPROM with last settings
      ee.bLightOn = cont.m_bLightOn;
    }

    if(cont.m_bLightOn)
    {
      if(++onCounter > (60*60*12))
        totalUpWatts();
    }

    if(nWrongPass)            // wrong password blocker
      nWrongPass--;

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
  delay(10);
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
          sendState();
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
