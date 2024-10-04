// URI builder. No encoding, so only simple strings

class uriString
{
public:
  uriString(const char *pPath = NULL)
  {
    m_cnt = 0;
    s = pPath;
  }
        
  String string(void)
  {
    return s;
  }

  void addDelim()
  {
    s += (m_cnt) ? "&" : "?";
  }

  void Param(const char *key, int iVal)
  {
    addDelim();
    s += key;
    s += "=";
    s += iVal;
    m_cnt++;
  }

  void Param(const char *key, uint8_t iVal)
  {
    addDelim();
    s += key;
    s += "=";
    s += iVal;
    m_cnt++;
  }

  void Param(const char *key, uint32_t iVal)
  {
    addDelim();
    s += key;
    s += "=";
    s += iVal;
    m_cnt++;
  }

  void Param(const char *key, long int iVal)
  {
    addDelim();
    s += key;
    s += "=";
    s += iVal;
    m_cnt++;
  }

  void Param(const char *key, float fVal)
  {
    addDelim();
    s += key;
    s += "=";
    s += fVal;
    m_cnt++;
  }
  
  void Param(const char *key, bool bVal)
  {
    addDelim();
    s += key;
    s += "=";
    s += bVal ? 1:0;
    m_cnt++;
  }
  
  void Param(const char *key, const char *sVal)
  {
    addDelim();
    s += key;
    s += "=";
    s += sVal;
    m_cnt++;
  }
  
  void Param(const char *key, String sVal)
  {
    addDelim();
    s += key;
    s += "=";
    s += sVal;
    m_cnt++;
  }

protected:
  String s;
  int m_cnt;
};
