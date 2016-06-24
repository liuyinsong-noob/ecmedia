#ifndef _QUEUE_INFO_H
#define _QUEUE_INFO_H


#define _MESSAGE_UNKNOWN   0
#define _MESSAGE_HTTP      2  
#define _MESSAGE_DB        3
#define _MESSAGE_TIMER     4
#define _MESSAGE_COMMAND   5
#define _MESSAGE_MEM       6
#define _MESSAGE_UDP_HTTP  7
#define _MESSAGE_DB2       8
#define _MESSAGE_UDP_TIMER    9
#define _MESSAGE_IPHONEPUSH   10
#define _MESSAGE_FEEDBACK  11

//Add by Kyd for Memcached
#define MEM_SET           0
#define MEM_GET           1
#define MEM_DEL           2
#define MEM_MULSET        3
#define MEM_MULGET        4
#define MEM_MULDEL        5
//Kyd End

#ifndef WIN32
#define  SOCKET int
#endif

#include "BaseQueue.h"

struct TCommandBody
{
    char*    pStrMsg;
    char     Ip[16];
    unsigned int Port;
    SOCKET   mySockFD;

    TCommandBody(char* str,char* pAddress,unsigned int iPort,SOCKET fd);
};
struct THttpContext
{
	THttpContext() {
		nSocketFD = -1;
		pSSL = NULL;
		pSession = NULL;
	};
	SOCKET   nSocketFD;
	void *pSSL;
	void* pSession;
};
struct THttpBody
{
	THttpContext context;
    char*    pStrMsg;
   	char mfromIP[32];
	int mport;
	int nStrLen;
	THttpBody(char* str,SOCKET fd,void *ssl,int len=-1,void* pSessionInfo=NULL);
    THttpBody(char* str,SOCKET fd,void *ssl,char* addr,int port,int len=-1,void* pSessionInfo=NULL);
    
};

struct TIphonePushBody
{
	int   m_nCerTag;
	int   m_nType;
	char  m_strDeviceToken[128];
	char  m_strMsgBuf[256];
	TIphonePushBody(int nCerTag,char* strDeviceToken,char* strMsgBuf);    
};

struct TTimerBody
{
    unsigned long  TimerID;
    unsigned long  OwnerID;
    unsigned long  RequestType;
    
    TTimerBody(unsigned long timerid,unsigned long  ownerid,unsigned long type);
};

struct TTimerBody2
{
    unsigned long  TimerID;
    char  OwnerID[128];
    unsigned long  RequestType;
    
    TTimerBody2(unsigned long timerid,char*  ownerid,unsigned long type);
};

class CQueueInfo
{
public:
    int           MsgType;
    void*         pBody;

public:
    CQueueInfo();
    CQueueInfo(int type,void* body);
    ~CQueueInfo(void);
    
    CQueueInfo & operator =(CQueueInfo &src);
    void FreeBody();
};

typedef kernel::BaseQueue<CQueueInfo> TMsgQueue;
#endif
