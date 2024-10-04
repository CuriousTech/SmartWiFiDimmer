#ifndef WORLDTIME_H
#define WORLDTIME_H

#include <Arduino.h>
#ifdef ESP32
#include <AsyncTCP.h> // https://github.com/me-no-dev/AsyncTCP
#else
#include <ESPAsyncTCP.h> // https://github.com/me-no-dev/ESPAsyncTCP
#endif

class WorldTime
{
public:
  WorldTime();
  bool  update(void);
  bool getDST(void);
  uint32_t getUTCtime(void);
  int32_t getTZOffset(void);
  void setTZOffset(int32_t offset);
  bool hasUpdated(void);
private:
  AsyncClient m_ac;
  void _onConnect(AsyncClient* client);
  void _onDisconnect(AsyncClient* client);
  void _onError(AsyncClient* client, int8_t error);
  void _onTimeout(AsyncClient* client, uint32_t time);
  void _onData(AsyncClient* client, char* data, size_t len);

  void processJson(char *p, const char **jsonList);
  char *skipwhite(char *p);
  void callback(uint8_t iName, uint32_t iValue, char *psValue);

  bool m_bUpdated;
  bool m_bDST;
  uint32_t m_unixTime;
  int32_t m_rawOffet;
  int32_t m_dstOffset;
#define WT_BUFSIZE 512
  char m_buffer[WT_BUFSIZE];
  uint16_t m_bufIdx;
};

#endif // WORLDTIME_H
