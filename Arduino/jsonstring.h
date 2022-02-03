#include "eeMem.h"

class jsonString
{
public:
  jsonString(const char *pLabel = NULL)
  {
    m_cnt = 0;
    if(pLabel)
      s = pLabel, s += ";";
    s += "{";
  }
        
  String Close(void)
  {
    s += "}";
    return s;
  }

  void Var(char *key, int iVal)
  {
    if(m_cnt) s += ",";
    s += "\"";
    s += key;
    s += "\":";
    s += iVal;
    m_cnt++;
  }

  void Var(char *key, uint32_t iVal)
  {
    if(m_cnt) s += ",";
    s += "\"";
    s += key;
    s += "\":";
    s += iVal;
    m_cnt++;
  }

  void Var(char *key, long int iVal)
  {
    if(m_cnt) s += ",";
    s += "\"";
    s += key;
    s += "\":";
    s += iVal;
    m_cnt++;
  }

  void Var(char *key, float fVal)
  {
    if(m_cnt) s += ",";
    s += "\"";
    s += key;
    s += "\":";
    s += fVal;
    m_cnt++;
  }
  
  void Var(char *key, bool bVal)
  {
    if(m_cnt) s += ",";
    s += "\"";
    s += key;
    s += "\":";
    s += bVal ? 1:0;
    m_cnt++;
  }
  
  void Var(char *key, char *sVal)
  {
    if(m_cnt) s += ",";
    s += "\"";
    s += key;
    s += "\":\"";
    s += sVal;
    s += "\"";
    m_cnt++;
  }
  
  void Var(char *key, String sVal)
  {
    if(m_cnt) s += ",";
    s += "\"";
    s += key;
    s += "\":\"";
    s += sVal;
    s += "\"";
    m_cnt++;
  }

  void Array(char *key, String sVal[], int n)
  {
    if(m_cnt) s += ",";
    s += "\"";
    s += key;
    s += "\":[";
    for(int i = 0; i < n; i++)
    {
      if(i) s += ",";
      s += "\"";
      s += sVal[i];
      s += "\"";
    }
    s += "]";
    m_cnt++;
  }

  void Array(char *key, uint16_t iVal[], int n)
  {
    if(m_cnt) s += ",";
    s += "\"";
    s += key;
    s += "\":[";
    for(int i = 0; i < n; i++)
    {
      if(i) s += ",";
      s += iVal[i];
    }
    s += "]";
    m_cnt++;
  }

  void Array(char *key, uint32_t iVal[], int n)
  {
    if(m_cnt) s += ",";
    s += "\"";
    s += key;
    s += "\":[";
    for(int i = 0; i < n; i++)
    {
      if(i) s += ",";
      s += iVal[i];
    }
    s += "]";
    m_cnt++;
  }

  void Array(char *key, float fVal[], int n)
  {
    if(m_cnt) s += ",";
    s += "\"";
    s += key;
    s += "\":[";
    for(int i = 0; i < n; i++)
    {
      if(i) s += ",";
      if(fVal[i] == (int)fVal[i]) // reduces size
        s += (int)fVal[i];
      else
        s += fVal[i];
    }
    s += "]";
    m_cnt++;
  }

  int RLE(char *key, float fVal[], int n, int nTotal)
  {
    if(m_cnt) s += ",";
    s += "\"";
    s += key;
    s += "\":[";

    for(int cnt = 0; n < nTotal && s.length() < 750; cnt++)
    {
      if(cnt) s += ",";

      s += "[";

      float fV = fVal[n++];
      int i;

      if(fV != fVal[n]) // do bulk
      {
        int i;
        int len = 0;
        float fV2 = fVal[n++];
        int n2 = n;
        for(i = 0; fV2 != fVal[n2] && n2 < nTotal && len + s.length() < 750; i++, n2++) // lookahead, just look for 1 repeat
        {
          fV2 = fVal[n2];
          String sl = String(fV2);
          len += sl.length()+1;
        }
        s += i + 1;
        s += ",";
        s += fV;
        for(int j = 0; j < i; j++, n++)
        {
          s += ",";
          fV = fVal[n];
          if(fV == (int)fV)
            s += (int)fV;
          else
            s += fV;
        }
      }
      else // do repeat
      {
        for(i = 1; fV == fVal[n] && n < nTotal; i++, n++);
        s += -i;
        s += ",";
        if(fV == (int)fV) // remove a few chars if possible
          s += (int)fV;
        else
          s += fV;
      }
      s += "]";
    }
    s += "]";
    m_cnt++;
    return n;
  }

  void Array(char *key, Sched sch[], int n)
  {
    if(m_cnt) s += ",";
    s += "\"";
    s += key;
    s += "\":[";

    for(int i = 0; i < n; i++)
    {
      if(i) s += ",";
      s += "[\"";  s += sch[i].sname;
      s += "\","; s += sch[i].timeSch;
      s += ",";  s += sch[i].seconds;
      s += ","; s += sch[i].wday;
      s += ","; s += sch[i].level;
      s += "]";
      if(sch[i].sname[0]==0)
        break;
    }
    s += "]";
    m_cnt++;
  }

  void Array(char *key, Device dev[], DevState devst[], int n)
  {
    if(m_cnt) s += ",";
    s += "\"";
    s += key;
    s += "\":[";

    for(int i = 0; i < n && dev[i].IP[0]; i++)
    {
      if(i) s += ",";
      s += "[\"";  s += dev[i].szName;
      s += "\",\""; s += dev[i].IP[0]; s += "."; s += dev[i].IP[1]; s += "."; s += dev[i].IP[2]; s += "."; s += dev[i].IP[3];
      s += "\",";  s += dev[i].mode;
      s += ","; s += dev[i].flags;
      s += ","; s += dev[i].delay;
      s += ","; s += devst[i].bPwr;
      s += ","; s += devst[i].nLevel;
      s += "]";
    }
    s += "]";
    m_cnt++;
  }

  void Array(char *key, Energy stuff[], int n)
  {
    if(m_cnt) s += ",";
    s += "\"";
    s += key;
    s += "\":[";

    for(int i = 0; i < n; i++)
    {
      if(i) s += ",";
      s += "[";  s += stuff[i].sec;
      s += ",";  s += stuff[i].fwh;
      s += "]";
    }
    s += "]";
    m_cnt++;
  }

protected:
  String s;
  int m_cnt;
};
