#ifndef _TCP_SERVER_H
#define _TCP_SERVER_H

#define MAX_LENGTH         1024000
#include "Utility/ResourceBase.h"
#include "Utility/QueueInfo.h"
#include <list>

#ifndef WIN32
#include <tr1/memory>
#endif

#ifdef SUPPORT_SSL
#include "openssl/bio.h" 
#include "openssl/ssl.h" 
#include "openssl/err.h" 
#endif

class TSocketInfo
{
public:
    TSocketInfo(int fd,char* ip,int port,void* ssl);
	~TSocketInfo();
	int GetStartTime() { return m_nStartTime;};
	char *GetRemoteIP() { return m_strIP;}
	int GetRemotePort() {return m_nPort;}
	int GetSockID() { return m_sockFD;}
	int GetMesssageLen() {return m_nLength;}
	void SetMessage(const char*buf,int len);
	void AppendMessage(const char*buf, int len);
	char * GetMessage() {return m_pMessage;}
	void ClearMessage();
	bool GetSSLAccepted(){return m_bSSLAccepted;}
	void SetSSLAccepted(bool accepted=true){m_bSSLAccepted=accepted;}
	void *GetSSL(){
#ifdef SUPPORT_SSL
	return m_ssl;
#else
	return NULL;
#endif
	}
private:
    int    m_sockFD;
    char   m_strIP[32];
    int	   m_nPort;
    
    char   *m_pMessage;
    int    m_nLength;
    int    m_nStartTime;
	bool   m_bSSLAccepted;
#ifdef SUPPORT_SSL
	SSL	   *m_ssl;
#endif

};
typedef void (*TRACE_FUNC) (const char *, ...);
class THttpServer
{
public:
    THttpServer(const char *listenAddress, int listenPort ,
			int nRecvThreadNum=1,int nSendThreadNum=1,bool bUseSSL=false);
    ~THttpServer();
    int    StartServer(TMsgQueue* pUpMsgQueue,TMsgQueue* pDownMsgQueue);
	void	SetTraceFunc(TRACE_FUNC trace_func);
    void   StopServer();
    
    void   ListenThread();
    void   ReadThread();
    void   SendThread();

private:
	void	Trace(const char *,...);
	bool    OpenServer();
    bool    IsFullPacket(char* inputData,int nTotalLen);
	bool    IsBlockError(void *ssl,int socketID,int ret);
	int     m_ListenFD;
    int      m_bCanRun;
	std::list< std::tr1::shared_ptr<TSocketInfo>> m_SocketList;

	int        m_nRecvThreadNum;
	int        m_nSendThreadNum;
	std::string m_listenAddress;
	int			m_listenPort;
    
    kernel::CTMutex    m_Mutex;
    kernel::CTThread*  m_pListenThread;
    kernel::CTThread**  m_pReadThread;
    kernel::CTThread**  m_pSendThread;
    
    TMsgQueue* m_pUpMsgQueue;
    TMsgQueue* m_pDownMsgQueue;
    TLogFile*  m_pSystemLog;
	TRACE_FUNC m_pTraceFun;

	bool m_bUseSSL;
#ifdef SUPPORT_SSL
	SSL_CTX* m_ssl_ctx;
#endif
};

#endif
