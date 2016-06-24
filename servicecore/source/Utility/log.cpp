#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include "log.h"

#ifndef WIN32
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#else
#define stat _stat32
#endif

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


TLogFile::TLogFile(const char* module)
{
	m_pLogFile = NULL;
	if (module == NULL)
	{
		return;
	}
	memset(m_WriteBuf,0,sizeof(m_WriteBuf));
	strcpy(m_szFilePathBase,module);

	time_t  atimer = time(NULL);
    struct tm *area = localtime( &atimer );
	
	memset(m_szFileDate,0,32);
	sprintf(m_szFileDate,"%04d%02d%02d",area->tm_year+1900,area->tm_mon+1,area->tm_mday);

	sprintf(m_szFilePath,"%s_%s.log",m_szFilePathBase,m_szFileDate);
    
	m_pLogFile=fopen(m_szFilePath,"a+");
	if(!m_pLogFile) {
		printf("open %s failed\n",m_szFilePath);
		return;
	}
    fprintf(m_pLogFile,"\n\n====================================================\n");
    fprintf(m_pLogFile,"====================START RUN=======================\n");
    fprintf(m_pLogFile,"====================================================\n\n");
	fflush(m_pLogFile);
    
}

TLogFile::~TLogFile()
{
    if(m_pLogFile)
    {
        fclose(m_pLogFile);
        m_pLogFile = NULL;
    }
}

void TLogFile::mvFile()
{
}

void TLogFile::printf(const char* fmt, ... )
{
    if(m_pLogFile == NULL)
        return;

	m_Mutex.Lock();
    va_list args;
    va_start(args, fmt);
    vsnprintf(m_WriteBuf,4095, fmt, args);
    va_end(args);

    time_t atimer =  time( NULL );
    tm *area = localtime( &atimer );

	char strNowDate[32]={0};
	sprintf(strNowDate,"%04d%02d%02d",area->tm_year+1900,area->tm_mon+1,area->tm_mday);
	
	if((m_nFileSize > 102400000)||(strcmp(strNowDate,m_szFileDate)!=0))
    {
        fclose(m_pLogFile);
		strcpy(m_szFileDate,strNowDate);
		strcpy(m_szFilePath,m_szFilePathBase);
		
		char TempFileName[256];
		sprintf(TempFileName,"_%s.log",strNowDate);
		
		strcat(m_szFilePath,TempFileName);
        m_pLogFile = fopen(m_szFilePath,"a+");
        m_nFileSize =0;
    }
    m_nFileSize += fprintf(m_pLogFile,"%04d%02d%02d %02d:%02d:%02d:%s",area->tm_year+1900,area->tm_mon+1,area->tm_mday,area->tm_hour,area->tm_min,area->tm_sec,m_WriteBuf);
    fflush(m_pLogFile);
    m_Mutex.Unlock();
}

void TLogFile::vprintf(const char *fmt, va_list arg)
{
	char buf[4096] ={0};
	vsprintf(buf,fmt,arg);
	printf("%s",buf);
}

