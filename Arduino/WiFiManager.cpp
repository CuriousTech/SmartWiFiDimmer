/**************************************************************
 * WiFiManager is a library for the ESP8266/Arduino platform
 * (https://github.com/esp8266/Arduino) to enable easy
 * configuration and reconfiguration of WiFi credentials and
 * store them in EEPROM.
 * inspired by http://www.esp8266.com/viewtopic.php?f=29&t=2520
 * https://github.com/chriscook8/esp-arduino-apboot
 * Built by AlexT https://github.com/tzapu
 * Licensed under MIT license
 **************************************************************/

#include "WiFiManager.h"
#include "eeMem.h"

void WiFiManager::autoConnect(char const *apName, const char *pPass)
{
  _apName = apName;
  _pPass = pPass;

  WiFi.hostname(ee.szName);
  if ( ee.szSSID[0] ) {
    DEBUG_PRINT("Waiting for Wifi to connect");

    WiFi.mode(WIFI_STA);
    WiFi.begin(ee.szSSID, ee.szSSIDPassword);
//    WiFi.setHostname(apName);
    _state = ws_connecting;
    _timer = 50;
  }
  else
  {
    startAP();
  }
}

// Start AP mode
void WiFiManager::startAP()
{
  //setup AP
  WiFi.mode(WIFI_AP);
  WiFi.softAP(_apName);
  DEBUG_PRINT("Started Soft Access Point");

  DEBUG_PRINT(WiFi.softAPIP());

  if (!MDNS.begin(_apName))
    DEBUG_PRINT("Error setting up MDNS responder!");
  WiFi.scanNetworks();

  _state = ws_config;
}

// return current connection sate
int WiFiManager::state()
{
  return _state;
}

void WiFiManager::setPass(const char *p){
  strncpy(ee.szSSIDPassword, p, sizeof(ee.szSSIDPassword) );
  ee.update();
  DEBUG_PRINT("Updated EEPROM.  Restaring.");
  autoConnect(_apName, _pPass);
}

// Called at any frequency
int WiFiManager::service()
{
  static int s = 1; // do first list soon
  static uint32_t m;
  static uint16_t ticks;
  static bool bBlink;

  if((millis() - m) > 200)
  {
    m = millis();
    ticks++;
    if(_state == ws_connecting)
    {
#ifdef DEBUG
      Serial.print(".");
#endif
      if(_timer)
      {
        if (WiFi.status() == WL_CONNECTED)
        {
          DEBUG_PRINT("Connected");
          _state = ws_connected;
          return ws_connectSuccess;
        }
        else if(--_timer == 0)
        {
          DEBUG_PRINT("");
          DEBUG_PRINT("Could not connect to WiFi");
          startAP();
        }
      }
      return _state;
    }
  }

  if(ticks < 5)
    return _state;
  ticks = 0;

  if(_state != ws_config)
    return _state;
  if(--s)
    return _state;
  s = 60;
  int n = WiFi.scanNetworks(); // scan for stored SSID each minute
  if(n == 0 )
    return _state;

  for (int i = 0; i < n; i++)
  {
    if(WiFi.SSID(i) == ee.szSSID) // found cfg SSID
    {
      DEBUG_PRINT("SSID found.  Restarting.");
      autoConnect(_apName, _pPass);
      s = 5; // set to 5 seconds in case it fails again
    }
  }
  return _state;
}

String WiFiManager::page()
{
  String s = HTTP_HEAD;
  s += HTTP_SCRIPT;
  s += HTTP_STYLE;
  s += HTTP_HEAD_END;

  for (int i = 0;  WiFi.SSID(i).length(); ++i)
  {
    DEBUG_PRINT(WiFi.SSID(i));
    DEBUG_PRINT(WiFi.RSSI(i));
    String item = HTTP_ITEM;
    item.replace("{v}", WiFi.SSID(i) );
    s += item;
  }
  WiFi.scanDelete();
  String form = HTTP_FORM;
  form.replace("$key", _pPass );
  s += form;
  s += HTTP_END;

  return s;
}
