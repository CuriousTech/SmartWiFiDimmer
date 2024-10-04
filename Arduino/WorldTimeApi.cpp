// Code for worldtimeapi
// Note: This can only be called every 2 minutes or longer. Only enable NTP on one device. It will send updates
// to the rest of the devices.
//

#include "WorldTime.h"
#include <TimeLib.h> // http://www.pjrc.com/teensy/td_libs_Time.html

const char host[] = "worldtimeapi.org";

// Initialize instance
WorldTime::WorldTime()
{
  m_ac.onConnect([](void* obj, AsyncClient* c) { (static_cast<WorldTime*>(obj))->_onConnect(c); }, this);
  m_ac.onDisconnect([](void* obj, AsyncClient* c) { (static_cast<WorldTime*>(obj))->_onDisconnect(c); }, this);
  m_ac.onError([](void* obj, AsyncClient* c, int8_t error) { (static_cast<WorldTime*>(obj))->_onError(c, error); }, this);
  m_ac.onTimeout([](void* obj, AsyncClient* c, uint32_t time) { (static_cast<WorldTime*>(obj))->_onTimeout(c, time); }, this);
  m_ac.onData([](void* obj, AsyncClient* c, void* data, size_t len) { (static_cast<WorldTime*>(obj))->_onData(c, static_cast<char*>(data), len); }, this);

  m_ac.setRxTimeout(10000);
}

bool WorldTime::update()
{
  m_bufIdx = 0;
  return m_ac.connect( host, 80 );
}

bool WorldTime::hasUpdated()
{
  if( m_bUpdated )
  {
    m_bUpdated = false;
    return true;
  }
  return false;
}

uint32_t WorldTime::getUTCtime(void)
{
  return now() - m_rawOffet - m_dstOffset;
}

int32_t WorldTime::getTZOffset(void)
{
  return m_rawOffet + m_dstOffset;
}

void WorldTime::setTZOffset(int32_t offset)
{
  m_rawOffet = offset;
}

bool WorldTime::getDST(void)
{
  return m_bDST;
}

void WorldTime::_onTimeout(AsyncClient* client, uint32_t time)
{
  (void)client;
}

void WorldTime::_onError(AsyncClient* client, int8_t error)
{
  (void)client;
}

void WorldTime::_onConnect(AsyncClient* client)
{
  (void)client;

  String s = String("GET /api/ip HTTP/1.1\r\n");
  s += "Host: ";
  s += host;
  s += "\r\n"
       "Content-Type: application/json\r\n"
       "User-Agent: Arduino\r\n"
       "Connection: close\r\n"
       "Accept: */*\r\n"
       "\r\n";

  m_ac.add(s.c_str(), s.length());
}

void WorldTime::_onData(AsyncClient* client, char* data, size_t len)
{
  (void)client;

  if(m_bufIdx == 0) // first chunk
  {
    while(*data != '{' && len) // skip headers
    {
      data++;
      len--;
    }
    if(len == 0)
      return;
  }

  if(m_bufIdx + len >= WT_BUFSIZE)
  {
    len = WT_BUFSIZE - m_bufIdx - 1; // truncate
    if(len <= 0) return;
  }

  memcpy(m_buffer + m_bufIdx, data, len);
  m_bufIdx += len;
  m_buffer[m_bufIdx] = 0;
}

//"utc_offset":"-04:00",
//"timezone":"America/New_York",
//"day_of_week":5,
//"day_of_year":222,
//"datetime":"2024-08-09T05:44:58.595466-04:00",
//"utc_datetime":"2024-08-09T09:44:58.595466+00:00",
//"unixtime":1723196698,
//"raw_offset":-18000,
//"week_number":32,
//"dst":true,
//"abbreviation":"EDT",
//"dst_offset":3600,
//"dst_from":"2024-03-10T07:00:00+00:00",
//"dst_until":"2024-11-03T06:00:00+00:00",
//"client_ip":"xxxxxxxxxxx"}

void WorldTime::_onDisconnect(AsyncClient* client)
{
  (void)client;

  const char *jsonList[] = { // root keys
    "utc_offset",  // 0 -04:00
    "timezone",    // 1 "America\New York"
    "day_of_week", // 2 1-7
    "day_of_year", // 3 0-364
    "datetime",    // 4 "2024-08-09T05:44:58.595466-04:00"
    "utc_datetime",// 5 "2024-08-09T09:44:58.595466+00:00"
    "unixtime",
    "raw_offset", // -18000
    "week_number", // 1-52?
    "dst", // true
    "abbreviation", // 10 "EDT"
    "dst_offset", // 3600
    "dst_from",
    "dst_until", 
    "client_ip", // 14
    NULL
  };

  processJson(m_buffer, jsonList);
}

void WorldTime::callback(uint8_t iName, uint32_t iValue, char *psValue)
{
  switch(iName)
  {
    case 0: // "utc_offset",  // 0 -04:00
      break;
    case 1: //    "timezone",    // 1 "America\New York"
      break;
    case 2: //    "day_of_week", // 2 0-6
      break;
    case 3: //    "day_of_year", // 3 0-364
      break;
    case 4: //    "datetime",    // 4 "2024-08-09T05:44:58.595466-04:00"
      break;
    case 5: //    "utc_datetime",// 5 "2024-08-09T09:44:58.595466+00:00"
      break;
    case 6: //    "unixtime",
      m_unixTime = iValue;
      break;
    case 7: //    "raw_offset", // -18000
      m_rawOffet = iValue;
      break;
    case 8: //    "week_number", // 1-52?
      break;
    case 9: //    "dst", // true
      m_bDST = iValue;
      break;
    case 10: //    "abbreviation", // 10 "EDT"
      break;
    case 11: //    "dst_offset", // 3600
      m_dstOffset = iValue;
      setTime( m_unixTime + m_rawOffet + m_dstOffset);
      m_bUpdated = true;
      break;
    case 12: //    "dst_from",
      break;
    case 13: //    "dst_until", 
      break;
    case 14: //    "client_ip", // 14
      break;
  }
}

void WorldTime::processJson(char *p, const char **jsonList)
{
  char *pPair[2]; // param:data pair
  int8_t brace = 0;
  int8_t bracket = 0;
  int8_t inBracket = 0;
  int8_t inBrace = 0;

  while(*p)
  {
    p = skipwhite(p);
    if(*p == '{'){p++; brace++;}
    if(*p == '['){p++; bracket++;}
    if(*p == ',') p++;
    p = skipwhite(p);

    bool bInQ = false;
    if(*p == '"'){p++; bInQ = true;}
    pPair[0] = p;
    if(bInQ)
    {
       while(*p && *p!= '"') p++;
       if(*p == '"') *p++ = 0;
    }else
    {
      while(*p && *p != ':') p++;
    }
    if(*p != ':')
      return;

    *p++ = 0;
    p = skipwhite(p);
    bInQ = false;
    if(*p == '{') inBrace = brace+1; // data: {
    else if(*p == '['){p++; inBracket = bracket+1;} // data: [
    else if(*p == '"'){p++; bInQ = true;}
    pPair[1] = p;
    if(bInQ)
    {
       while(*p && *p!= '"') p++;
       if(*p == '"') *p++ = 0;
    }else if(inBrace)
    {
      while(*p && inBrace != brace){
        p++;
        if(*p == '{') inBrace++;
        if(*p == '}') inBrace--;
      }
      if(*p=='}') p++;
    }else if(inBracket)
    {
      while(*p && inBracket != bracket){
        p++;
        if(*p == '[') inBracket++;
        if(*p == ']') inBracket--;
      }
      if(*p == ']') *p++ = 0;
    }else while(*p && *p != ',' && *p != '\r' && *p != '\n') p++;
    if(*p) *p++ = 0;
    p = skipwhite(p);
    if(*p == ',') *p++ = 0;

    inBracket = 0;
    inBrace = 0;
    p = skipwhite(p);

    if(pPair[0][0])
    {
      for(int i = 0; jsonList[i]; i++)
      {
        if(!strcmp(pPair[0], jsonList[i]))
        {
          uint32_t n = atol(pPair[1]);
          if(!strcmp(pPair[1], "true")) n = 1; // bool case
          callback(i, n, pPair[1]);
          break;
        }
      }
    }

  }
}

char *WorldTime::skipwhite(char *p)
{
  while(*p == ' ' || *p == '\t' || *p =='\r' || *p == '\n')
    p++;
  return p;
}
