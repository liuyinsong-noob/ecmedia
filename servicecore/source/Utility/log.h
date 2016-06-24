#ifndef  _LOG_FILE_H 
#define  _LOG_FILE_H

#include "ResourceBase.h"
class TLogFile
{
private:
    char  m_szFilePath[256];
	char  m_szFilePathBase[256];
    int   m_nFileSize;
	char  m_szFileDate[32];

    FILE*	m_pLogFile;
    kernel::CTMutex m_Mutex;
    char        m_WriteBuf[4096];//5120000
public:
    TLogFile(const char* filePath);
    ~TLogFile();

    void printf(const char* fmt, ... );
	void vprintf(const char* fmt, va_list arg );
    void mvFile();
	void ReadLog(char* strOutputBuffer);
};

#endif
