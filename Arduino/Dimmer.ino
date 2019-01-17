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

// Arduino IDE 1.9.0, esp8266 SDK 2.4.2

//uncomment to enable Arduino IDE Over The Air update code
#define OTA_ENABLE
//#define MOTION
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
#include <JsonClient.h>

#define ESP_LED    2  // open (ESP-07 low = blue LED on)
#define MOTION    16  // RCW-0516 (no internal pullup)

int serverPort = 80;                    // listen port

IPAddress lastIP;     // last IP that accessed the device
int nWrongPass;       // wrong password block counter

eeMem eemem;
UdpTime utime;
uint16_t nSecTimer; // off timer
uint16_t nDelayOn;  // delay on timer
uint8_t nSched;     // current schedule
bool    bOverride;    // automatic override of schedule
uint8_t nWsConnected;
bool    bEnMot = true;

WiFiManager wifi;  // AP page:  192.168.4.1
AsyncWebServer server( serverPort );
AsyncWebSocket ws("/ws"); // access at ws://[esp ip]/ws

void jsonCallback(int16_t iEvent, uint16_t iName, int iValue, char *psValue);
JsonParse jsonParse(jsonCallback);
JsonClient jsonPush(jsonCallback);

swControl cont;

enum reportReason
{
  Reason_Setup,
  Reason_Switch,
  Reason_Level,
  Reason_Motion,
};

String dataJson()
{
  String s = "state;{\"t\":";
  s += now() - ( (ee.tz + utime.getDST() ) * 3600);
  s += ",\"name\":\"";  s += ee.szName; s += "\"";
  s += ",\"on\":";  s += cont.m_bLightOn;
  s += ",\"l1\":";  s += digitalRead(WIFI_LED);
  s += ",\"lvl\":";  s += cont.m_nLightLevel;
  s += ",\"tr\":";  s += nSecTimer;
  s += ",\"sn\":";  s += nSched;
  s += ",\"mn\":";  s += cont.nLevelMin;
  s += ",\"mx\":";  s += cont.nLevelMax;
#ifdef MOTION
  s += ",\"mo\":";  s += digitalRead(MOTION);
#endif
  s += "}";
  return s;
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

    switch( p->name().charAt(0) )
    {
      case 'k': // key
        s.toCharArray(password, sizeof(password));
        break;
    }
  }

  IPAddress ip = request->client()->remoteIP();

  if(strcmp(ee.controlPassword, password))
  {
    if(nWrongPass == 0) // it takes at least 10 seconds to recognize a wrong password
      nWrongPass = 10;
    else if((nWrongPass & 0xFFFFF000) == 0 ) // time doubles for every high speed wrong password attempt.  Max 1 hour
      nWrongPass <<= 1;
    if(ip != lastIP)  // if different IP drop it down
       nWrongPass = 10;
    String data = "hack;{\"ip\":\"";
    data += request->client()->remoteIP().toString();
    data += "\",\"pass\":\"";
    data += password; // a String object here adds a NULL.  Possible bug in SDK
    data += "\"}";
    ws.textAll(data);
    data = String();
    lastIP = ip;
    return;
  }

  lastIP = ip;

  for ( uint8_t i = 0; i < request->params(); i++ ) {
    AsyncWebParameter* p = request->getParam(i);
    p->value().toCharArray(temp, 100);
    String s = wifi.urldecode(temp);
    bool bValue = (s == "true" || s == "1") ? true:false;

    switch( p->name().charAt(0)  )
    {
      case 's': // wifi SSID
        s.toCharArray(ee.szSSID, sizeof(ee.szSSID));
        break;
      case 'p': // wifi password
        wifi.setPass(s.c_str());
        break;
      case 'o': // light on/off
        if(nSched) bOverride = !bValue;
        cont.setSwitch(bValue);
        if(bValue && ee.autoTimer)
          nSecTimer = ee.autoTimer;
        break;
      case 'l': // level
        if(nSched) bOverride = (s.toInt() == 0);
        cont.setLevel( s.toInt() );
        if(s.toInt() && ee.autoTimer)
          nSecTimer = ee.autoTimer;
        break;
      case 'c':
        ee.bCall = bValue;
        break;
      case 'd': // delay off
        if(cont.m_bLightOn)
          nSecTimer = s.toInt();
        break;
      case 'n':
        if(s.length())
          s.toCharArray(ee.szName, sizeof(ee.szName));
        else
          strcpy(ee.szName, "Dimmer");
        eemem.update();
        ESP.reset();
        break;
      case 'a': // autoTimer
        ee.autoTimer = s.toInt();
        break;
      case 'h': // host IP / port  (call from host with ?h=80)
        ee.hostIP[0] = ip[0];
        ee.hostIP[1] = ip[1];
        ee.hostIP[2] = ip[2];
        ee.hostIP[3] = ip[3];
        ee.hostPort = s.toInt() ? s.toInt():80;
        CallHost(Reason_Setup); // test
        break;
    }
  }
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
        client->text("alert;restarted");
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
  "lvl",
  "TZ",
  "NTP",
  "NTPP", // 10
  "MOT",
  "ch",
  "I",
  "N",
  "S",
  "T",
  "W",
  "L", // 18
  NULL
};

void jsonCallback(int16_t iEvent, uint16_t iName, int iValue, char *psValue)
{
  if(!bKeyGood && iName) return; // only allow for key
  char *p, *p2;
  static int n;
  static int idx;

  switch(iEvent)
  {
    case 0: // cmd
      switch(iName)
      {
        case 0: // key
          if(!strcmp(psValue, ee.controlPassword)) // first item must be key
            bKeyGood = true;
          break;
        case 1: // on
          nSecTimer = 0;
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
          cont.setLED(iValue);
          break;
        case 7: // lvl
          cont.setLevel(iValue);
          break;
        case 8: // TZ
          ee.tz = iValue;
          utime.start();
          break;
        case 9: // NTP
          strncpy(ee.ntpServer, psValue, sizeof(ee.ntpServer) );
          utime.start();
          break;
        case 10: // NTPP
          ee.udpPort = iValue;
          utime.start();
          break;
        case 11: // MOT
          ee.nMotionSecs = iValue;
          break;
        case 12: // ch
          ee.bCall = iValue ? true:false;
          if(iValue) CallHost(Reason_Setup); // test
          break;
        case 13: //I
          n = constrain(iValue, 0, MAX_SCHED-1);
          break;
        case 14: // N
          strncpy(ee.schedule[n].sname, psValue, sizeof(ee.schedule[0].sname) );
          break;
        case 15: // S
          ee.schedule[n].timeSch = iValue;
          break;
        case 16: // T
          ee.schedule[n].seconds = iValue;
          break;
        case 17: // W
          ee.schedule[n].wday = iValue;
          break;
        case 18: // L
          ee.schedule[n].level = iValue;
          break;
      }
      break;
  }
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
  String url = ip.toString();
  jsonPush.begin(url.c_str(), sUri.c_str(), ee.hostPort, false, false, NULL, NULL);
  jsonPush.addList(jsonListCmd);
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
  wifi.autoConnect(ee.szName, ee.controlPassword); // Tries config AP, then starts softAP mode for config
  if(wifi.isCfg() == false)
  {
    if ( !MDNS.begin ( ee.szName, WiFi.localIP() ) );
    cont.setLED(true);
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
    s += "]\n";
    s += "data={\"tz\":";  s += ee.tz;
    s += ",\"mot\":";  s += ee.nMotionSecs;
    s += ",\"ch\":";   s += ee.bCall;
    s += ",\"auto\":";  s += ee.autoTimer;
    s += "}";

    request->send(200, "text/json", s);
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
    cont.setLED(false); // set it all to off
    cont.setSwitch(false);
  });
#endif
  if(wifi.isCfg() == false)
    utime.start();
  CallHost(Reason_Setup);
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

  if(!wifi.isCfg())
    utime.check(ee.tz);
  cont.listen();

  static uint8_t nOldLevel;
  static bool bOldOn;
  static uint32_t tm;
  if(cont.m_nLightLevel != nOldLevel && !Serial.available() && (millis() - tm > 120) ) // new level from MCU & no new serial data
  {
    sendState();
    CallHost(Reason_Level);
    tm = millis();
    bOldOn = cont.m_bLightOn; // reduce calls
    nOldLevel = cont.m_nLightLevel;
  }

  if(cont.m_bLightOn != bOldOn)
  {
    bOldOn = cont.m_bLightOn;
    nSecTimer = 0;
    CallHost(Reason_Switch);
    sendState();
    if(nSched && cont.m_bLightOn)
      bOverride = cont.m_bLightOn;
    if(cont.m_bLightOn && ee.autoTimer)
      nSecTimer = ee.autoTimer;
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
  }
#endif

  if(sec_save != second()) // only do stuff once per second
  {
    sec_save = second();

    if(wifi.isCfg())
    {
      wifi.seconds();
      digitalWrite(WIFI_LED, !digitalRead(WIFI_LED)); // blink for config
    }

    if(min_save != minute())    // only do stuff once per minute
    {
      min_save = minute();
      checkSched(false);        // check every minute for next schedule
      if (hour_save != hour())  // update our time daily (at 2AM for DST)
      {
        if( (hour_save = hour()) == 2)
          utime.start();
        eemem.update(); // update EEPROM if needed while we're at it (give user time to make many adjustments)
      }
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
        sendState(); // in case of monitoring
        CallHost(Reason_Switch);
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
          CallHost(Reason_Switch);
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
          CallHost(Reason_Switch);
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
  for(int i = 0; i < MAX_SCHED - 1; i++)
  {
     if(ee.schedule[i].sname[0] == 0)
     {
        ee.schedule[i] = ee.schedule[i+1];
        memset(&ee.schedule[i+1], 0, sizeof(Sched) );
     }
  }
}
