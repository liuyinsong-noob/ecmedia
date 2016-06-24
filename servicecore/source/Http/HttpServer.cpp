#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <string>

#ifndef WIN32
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>
#include <strings.h>
#include <fcntl.h>
#define closesocket close
#else
#include <WinSock2.h>
#include <time.h>
#pragma warning(disable:4996)
#pragma comment(lib,"ws2_32.lib")
#define socklen_t int
#endif


#include "Utility/log.h"
#include "HttpServer.h"

#ifndef WIN32
#define strnicmp  strncasecmp
#define stricmp   strcasecmp
#else
#define strncasecmp strnicmp  
#define strcasecmp stricmp   
#define MSG_NOSIGNAL 0
#endif

THREAD_HANDLER_DECL(_ListenThread,p)
{
    THttpServer* pServer=(THttpServer*)p;
    pServer->ListenThread();
    return 0;
}

THREAD_HANDLER_DECL(_ReadThread,p)
{
    THttpServer* pServer=(THttpServer*)p;
    pServer->ReadThread();
    return 0;
}

THREAD_HANDLER_DECL(_SendThread,p)
{
    THttpServer* pServer=(THttpServer*)p;
    pServer->SendThread();
    return 0;
}

static bool IsSocketConnected(int nSockFd)
{
	if(nSockFd < 2)
	{
		return false;
	}
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;
    fd_set fdwrite;
    fd_set fdexcept;
    FD_ZERO(&fdwrite);
    FD_ZERO(&fdexcept);
    FD_SET(nSockFd,&fdwrite);
    FD_SET(nSockFd,&fdexcept);
    int ret = select(nSockFd+1,NULL,&fdwrite,&fdexcept,&timeout);
    if(ret == -1)
        return false;
    if(ret > 0)
    {
        if(FD_ISSET(nSockFd,&fdexcept))
            return false;
        else if(FD_ISSET(nSockFd,&fdwrite))
        {
            int err = 0;
            socklen_t len = sizeof(err);
            int result = getsockopt(nSockFd,SOL_SOCKET,SO_ERROR,(char*)&err,&len);
            if(result < 0 || err != 0)
                return false;
            return true;
        }
    }
    return false;
}

TSocketInfo::TSocketInfo(int fd,char* ip,int port,void* ssl)
{
	m_sockFD=fd;
	strcpy(m_strIP,ip);
	m_nPort=port;
	m_pMessage = 0;
	m_nLength=0;
	m_nStartTime= (int) time(NULL);
#ifdef SUPPORT_SSL
	m_ssl = (SSL*) ssl;
#endif
}
TSocketInfo::~TSocketInfo()
{
	if(m_pMessage)
		delete m_pMessage;
#ifdef SUPPORT_SSL
	if(m_ssl) {
		SSL_shutdown(m_ssl);
		SSL_free(m_ssl);
	}
#endif

}

void TSocketInfo::SetMessage(const char*buf,int len)
{
	if( m_pMessage)
		delete m_pMessage;
	m_nLength = len;
	m_pMessage = new char[len+1];
	memcpy(m_pMessage,buf,len);
	m_pMessage[len] = 0;
}

void TSocketInfo::AppendMessage(const char*buf, int len)
{
	if( !m_pMessage) {
		SetMessage(buf,len);
		return ;
	}
	char * newMessage = new char[m_nLength+len+1];
	memcpy(newMessage,m_pMessage,m_nLength);
	memcpy(newMessage+m_nLength,buf,len);
	delete m_pMessage;
	newMessage[m_nLength+len] = 0;
	m_pMessage = newMessage;
	m_nLength +=  len;
}

void TSocketInfo::ClearMessage()
{
	if(m_pMessage)
		delete m_pMessage;
	m_pMessage = NULL;
	m_nLength = 0;
}

THttpServer::THttpServer(const char *listenAddress, int listenPort ,
		int nRecvThread,int nSendThread,bool bUseSSL)
		:m_bUseSSL(bUseSSL),m_pTraceFun(NULL)
{
	if (nRecvThread<1)
		nRecvThread = 1;
	
	if (nSendThread<1)
		nSendThread = 1;
	
	m_nRecvThreadNum = nRecvThread;
	m_nSendThreadNum = nSendThread;
	m_listenAddress = listenAddress;
	m_listenPort = listenPort;
    m_pListenThread=NULL;

    m_ListenFD = 0;
    m_bCanRun =0;

	m_pReadThread = new kernel::CTThread*[m_nRecvThreadNum];
	if (m_pReadThread == NULL)
		return;
	
	m_pSendThread = new kernel::CTThread*[m_nSendThreadNum];
	if (m_pSendThread == NULL)
	{
		delete m_pReadThread;
		m_pReadThread = NULL;
		return;
	}

	for(int i=0;i<m_nRecvThreadNum;i++)
    	m_pReadThread[i]=NULL;
    
	for(int i=0;i<m_nSendThreadNum;i++)
    	m_pSendThread[i]=NULL;
	
#ifdef SUPPORT_SSL
	m_ssl_ctx = NULL;
#endif
        
}

THttpServer::~THttpServer()
{
	for(int i=0;i<m_nRecvThreadNum;i++)
    {
		if (m_pReadThread[i]!=NULL)
		{
			delete m_pReadThread[i];
			m_pReadThread[i]=NULL;
		}		
    }
	for(int i=0;i<m_nSendThreadNum;i++)
    {
		if (m_pSendThread[i]!=NULL)
		{
			delete m_pSendThread[i];
			m_pSendThread[i]=NULL;
		}
    }
	if (m_pReadThread != NULL)
	{
		delete m_pReadThread;
		m_pReadThread = NULL;
	}

	if (m_pSendThread != NULL)
	{
		delete m_pSendThread;
		m_pSendThread = NULL;
	}
#ifdef SUPPORT_SSL
	if( m_ssl_ctx) {
		SSL_CTX_free(m_ssl_ctx);
	}
#endif
}

bool THttpServer::IsFullPacket(char* inputData,int nTotalLen)
{
    char*  body = strstr(inputData,"\r\n\r\n");
    if(body == NULL)
        return false;

    int  headerLen = body-inputData;
    for(int i=0;i<headerLen;i++)
    {
        if(inputData[i] == '\r')
        {
            if(strncasecmp(inputData+i+2,"content-length:",15)==0)
            {
				if(nTotalLen >= headerLen + 4 + atoi(inputData+i+17))
					return true;
				else
					return false;
            }
            if(strncmp(inputData+i,"\r\n\r\n",4)==0)
            {	 
                break;
            }
        }
    }
    return true;
}
void THttpServer::SetTraceFunc(TRACE_FUNC trace_func)
{
	m_pTraceFun = trace_func;
}

bool THttpServer::OpenServer()
{	
    int fd = socket(PF_INET, SOCK_STREAM, 0);
    if(fd <=0)
    {
        closesocket(fd);
        return false;
    }
	
	int i=1;
	if(setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,(const char*) &i,sizeof(i))<0)
	{
		return false;
	}
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(m_listenAddress.c_str());
    addr.sin_port = htons(m_listenPort);
	memset(&addr.sin_addr,0,sizeof(addr.sin_addr));
	
    int res = bind(fd, (struct sockaddr *) &addr, sizeof (addr));
    if(res != 0)
    {
		Trace("bind error\n");
        closesocket(fd);
        return false;
    }
    if(listen(fd,SOMAXCONN) < 0 )
    {
		Trace("listen error\n");
        closesocket(fd);
        return false;
    }
    int  bufSize=1024000;
    setsockopt(fd,SOL_SOCKET,SO_RCVBUF,(char*)&bufSize,4);
    m_ListenFD = fd;
    return true;
}
#ifdef SUPPORT_SSL
void info_callback(const SSL *ssl,int type,int ret)
{
  /*
	const char *str;
    int w;
    w = type& ~SSL_ST_MASK;
    if (w & SSL_ST_CONNECT) 
		str="SSL_connect";
    else if (w & SSL_ST_ACCEPT) 
		str="SSL_accept";
	else
		str ="undefine";

	if (type & SSL_CB_LOOP)
    {
        printf("%s:%s\n",str,SSL_state_string_long(ssl));
    }
	else if (type & SSL_CB_ALERT)
    {
		str=(type & SSL_CB_READ)?"read":"write";
        printf("SSL3 alert %s:%s:%s\n",str,
                SSL_alert_type_string_long(ret),
                SSL_alert_desc_string_long(ret));
    }
	else if (type & SSL_CB_EXIT)
    {
        if (ret == 0)
            printf("%s:failed in %s\n",str,SSL_state_string_long(ssl));
        else if (ret < 0)
            printf("%s:error in %s\n",str,SSL_state_string_long(ssl));

    }*/
	//printf("[%04d][%s:%s] val=%d\n",
		//ssl,str,SSL_state_string_long(ssl),val);
}
#endif

int  THttpServer::StartServer(TMsgQueue* pUpMsgQueue,TMsgQueue* pDownMsgQueue)
{
#ifdef SUPPORT_SSL
	if( m_bUseSSL && !m_ssl_ctx) 
	{
		SSL_library_init();
		SSL_load_error_strings();
		m_ssl_ctx = SSL_CTX_new(SSLv23_server_method());
		SSL_CTX_set_verify(m_ssl_ctx, SSL_VERIFY_NONE, NULL);
		ERR_print_errors_fp(stderr);
		SSL_CTX_set_info_callback(m_ssl_ctx,info_callback );
		if(SSL_CTX_use_certificate_file(m_ssl_ctx, "cacert.pem",SSL_FILETYPE_PEM) <= 0) { 
			Trace("use certificate file error\n");
		}
        if (SSL_CTX_use_PrivateKey_file(m_ssl_ctx, "cakey.pem",  SSL_FILETYPE_PEM) <= 0) {
			Trace("use key file  error\n");
        }
	}
#else 
	if( m_bUseSSL)
		return -1;
#endif
    if(!OpenServer())
    {
        Trace("Open http socket failure,ip is %s,port is %d.\n",
			m_listenAddress.c_str(),m_listenPort);
        return -1;
    }
	Trace("Listen on %s : %d \n",m_listenAddress.c_str(),m_listenPort);
    m_bCanRun=1;
    m_pUpMsgQueue=pUpMsgQueue;
    m_pDownMsgQueue=pDownMsgQueue;
    
    m_pListenThread = new kernel::CTThread(_ListenThread, this );
	if (m_pListenThread == NULL ||!m_pListenThread->IsValid())
	{
		return -1;
	}    
    for(int i=0;i<m_nRecvThreadNum;i++)
    {
#ifdef WIN32
		m_pReadThread[i] =new kernel::CTThread(_ReadThread,this,NULL,NULL);
#else
		m_pReadThread[i] =new kernel::CTThread(_ReadThread,this,PTHREAD_CREATE_DETACHED,NULL,NULL);
#endif
		if (m_pReadThread[i]==NULL|| !m_pReadThread[i]->IsValid() )
		{
			return -1;
		}
    }
    
    for(int i=0;i<m_nRecvThreadNum;i++)
    {
#ifdef WIN32
	m_pSendThread[i]=new kernel::CTThread(_SendThread, this,NULL,NULL);
#else
	m_pSendThread[i]=new kernel::CTThread(_SendThread, this,PTHREAD_CREATE_DETACHED,NULL,NULL);
#endif

		if (m_pSendThread[i]==NULL || !m_pSendThread[i]->IsValid())
		{
			return -1;
		}
    }
    return 0;
}

void THttpServer::StopServer()
{
    m_bCanRun=0;
}

void THttpServer::ListenThread()
{
    sockaddr_in from;
    socklen_t   from_len=sizeof(sockaddr_in);
    fd_set ready;	
    struct timeval tv;
	
#ifndef WIN32			
	fcntl(m_ListenFD, F_SETFL, O_NONBLOCK);  // set to non-blocking
#else
	unsigned long nonBlock = 1;
	ioctlsocket(m_ListenFD, FIONBIO , &nonBlock);
#endif=
	while(m_bCanRun) 	
    {	
        FD_ZERO(&ready);
        FD_SET(m_ListenFD, &ready);
		
        tv.tv_sec = 0;
        tv.tv_usec = 300000;
		int ret = select(m_ListenFD + 1,&ready,NULL,NULL,&tv);
		if( ret < 0 ){
			Trace("listen thread select error\n");
			break;
		}
		if( 0 == ret) 
		{
			continue;
		}
        if(FD_ISSET(m_ListenFD, &ready)>0)
        {
			int  fd = accept(m_ListenFD,(struct sockaddr *)&from,&from_len);
			void *ssl = NULL;
			if(fd > 0)
            {
#ifdef SUPPORT_SSL
				if( m_bUseSSL) {
					ssl =  SSL_new(m_ssl_ctx);
					SSL_set_fd((SSL*)ssl, fd);
					SSL_set_accept_state((SSL*)ssl);
				}
#endif
				Trace("[socket:%d] new connection accepted\n",fd);
				m_Mutex.Lock();
				m_SocketList.push_back( std::tr1::shared_ptr<TSocketInfo>
				   ( new TSocketInfo(fd,inet_ntoa(from.sin_addr),ntohs(from.sin_port),ssl)));
				 m_Mutex.Unlock();
            }
        }
    }
	Trace("listen thread exit\n");
}
void  THttpServer::ReadThread()
{
	std::list<std::tr1::shared_ptr<TSocketInfo> >  socketList;
	message = new char[MAX_LENGTH+1];
    fd_set ready;
	int readyCount=0;
    int    maxfd;
    struct timeval tv;
	int nSelectErrorCount = 0;
    
    while(m_bCanRun) 	
    {
		FD_ZERO(&ready);
		readyCount =0;
        maxfd = 0; 
		
		m_Mutex.Lock();
		if( !m_SocketList.empty()) {
			socketList.push_back(m_SocketList.front());
			m_SocketList.pop_front();
		}
        m_Mutex.Unlock();
		std::list<std::tr1::shared_ptr<TSocketInfo> >::iterator it= socketList.begin();
		while( it!= socketList.end()) 
		{		
			TSocketInfo* pSockInfo=it->get();
			if(time(NULL) - pSockInfo->GetStartTime() > 120)
			{
				Trace("[socket:%d] Connection time is too long[%s:%d]\n",
					pSockInfo->GetSockID(),pSockInfo->GetRemoteIP(),pSockInfo->GetRemotePort());
				closesocket(pSockInfo->GetSockID());
				it = socketList.erase(it);
				continue;
			}
			if (!IsSocketConnected(pSockInfo->GetSockID()))
			{
				closesocket(pSockInfo->GetSockID());
				it = socketList.erase(it);
				continue;
			}
			FD_SET(pSockInfo->GetSockID(), &ready);					
			readyCount++;
			if( pSockInfo->GetSockID()> maxfd ) 
					maxfd=pSockInfo->GetSockID();

				++ it;			
        }
		
		if( 0 == readyCount ) {
#ifdef WIN32
			Sleep(100);
#else 
			usleep(100*1000);
#endif
			continue;
		}
        tv.tv_sec = 0;
        tv.tv_usec = 100000;

		int nSelectRet = 0;

		nSelectRet = select(maxfd +1,&ready,NULL,NULL,&tv);
		if( nSelectRet = 0)
			continue;

		if (nSelectRet < 0)
		{
			nSelectErrorCount++;
			Trace("nSelectRet is  %d,errno is %d,errtext is %s\r\n",nSelectRet,errno,strerror(errno));	
			continue;
		}
		it= socketList.begin();
		while( it != socketList.end())
		{
			TSocketInfo* pSockInfo=it->get();
			if( FD_ISSET(pSockInfo->GetSockID(), &ready) ==0 ) {
				++it;
				continue;
			}
			memset(message,0,MAX_LENGTH+1);
			int len =0;
			if( !m_bUseSSL) 
				len = recv(pSockInfo->GetSockID(),message,MAX_LENGTH,0);
#ifdef SUPPORT_SSL
			else {  
				len = SSL_read((SSL*)pSockInfo->GetSSL(),message,MAX_LENGTH);
			}
#endif
			if( len <= 0 )
			{ 
				if( len<0 && IsBlockError(pSockInfo->GetSSL(),pSockInfo->GetSockID(),len))
					continue;
				Trace("[socket:%d] connection closed\n",pSockInfo->GetSockID());
                closesocket(pSockInfo->GetSockID());
                it = socketList.erase(it);
				continue;
			}
			if(len + pSockInfo->GetMesssageLen() > MAX_LENGTH)
            {
				Trace("[socket:%d] Message from [%s:%d] too long,close connection\n",
					pSockInfo->GetSockID(),	pSockInfo->GetRemoteIP(),pSockInfo->GetRemotePort());
                closesocket(pSockInfo->GetSockID());
                it = socketList.erase(it);
                continue;
            }
			pSockInfo->AppendMessage(message,len);
			if(IsFullPacket(pSockInfo->GetMessage(),pSockInfo->GetMesssageLen()))
            {
				Trace("[socket:%d] recevied message from %s:%d:\n%s\n\n",pSockInfo->GetSockID(),
					pSockInfo->GetRemoteIP(),pSockInfo->GetRemotePort(),pSockInfo->GetMessage());
				THttpBody* pHttpBody = new THttpBody(pSockInfo->GetMessage(),pSockInfo->GetSockID(),pSockInfo->GetSSL(),
					pSockInfo->GetRemoteIP(),pSockInfo->GetRemotePort(),pSockInfo->GetMesssageLen());

				CQueueInfo pQueueinfo(_MESSAGE_HTTP,pHttpBody);        
                m_pUpMsgQueue->Put(pQueueinfo);
				pSockInfo->ClearMessage();
                                
            }
			++it;
        }
    }
	delete[] message;
}

void  THttpServer::SendThread()
{
    CQueueInfo queueInfo;
    
    while(m_bCanRun) 	
    {	
		queueInfo.FreeBody();
		m_pDownMsgQueue->Get(queueInfo);	
		
		if( !queueInfo.pBody) {
			Trace("Error: no message body in queueinfo");
			continue;
		}

       THttpBody* pHttpBody=(THttpBody*)queueInfo.pBody;

	   if(  pHttpBody->context.nSocketFD<= 0) {
			Trace("Error: send out socket(%d) is invalid",
				pHttpBody->context.nSocketFD);
		   continue;
		}
	  
		struct sockaddr_in in;
		int inlen = sizeof(sockaddr_in);
		getpeername(pHttpBody->context.nSocketFD, (sockaddr*)&in, (socklen_t*)&inlen);
		char* fromIP={0};
		fromIP = inet_ntoa(*(in_addr*)(&(in.sin_addr.s_addr)));
		int fromport = ntohs(in.sin_port);

		if( pHttpBody->mport!=0 && (strcmp(fromIP,pHttpBody->mfromIP) || fromport!= pHttpBody->mport))
		{
			Trace("socket ip error..new..IP and port is %s,%d\n",fromIP,fromport);
			Trace("socket ip error..old..IP and port is %s,%d\n",pHttpBody->mfromIP,pHttpBody->mport);
			continue;
		}
				
		if (pHttpBody->nStrLen <= 0) {
			Trace("Error: len(%d) is invalid",
				pHttpBody->nStrLen);
			continue;
		}
				
		bool IsSendSuccessTag = 1;
		int nBytesToSend = pHttpBody->nStrLen;
		int nBytesSended = 0;
		int nFailTryTimes = 0;
		pHttpBody->pStrMsg[nBytesToSend] = 0;
		Trace("[socket:%d] send message:\n%s\n\n",pHttpBody->context.nSocketFD,
			pHttpBody->pStrMsg);

		while ((IsSendSuccessTag == 1 || nFailTryTimes< 3)&&(nBytesToSend>0))
		{
			if(!m_bUseSSL)
				nBytesSended = send(pHttpBody->context.nSocketFD,pHttpBody->pStrMsg+nBytesSended,nBytesToSend,MSG_NOSIGNAL) ;
#ifdef SUPPORT_SSL
			else
				nBytesSended = SSL_write((SSL*)pHttpBody->context.pSSL,pHttpBody->pStrMsg+nBytesSended,nBytesToSend);
#endif		
			if( nBytesSended > 0 )
			{	
				IsSendSuccessTag = 1;
				nFailTryTimes = 0;
				nBytesToSend -= nBytesSended;
			}
			else
			{
				if( nBytesSended<0 && IsBlockError(pHttpBody->context.pSSL,
							pHttpBody->context.nSocketFD,nBytesSended))
					continue;
				IsSendSuccessTag = 0;
				nFailTryTimes++;
				Trace("ReturnHttpMsg...send faild\n");
				Trace("errno is : %d\n and socketFd is %d,%s\n",
					errno,pHttpBody->context.nSocketFD,(char*)pHttpBody->pStrMsg);
			}
		}
	}
}

void  THttpServer::Trace(const char*fmt,...)
{
	va_list args;
    va_start(args, fmt);
    if( !m_pTraceFun) 
		vprintf(fmt,args);
	else {
		char buf[4096];
		vsprintf(buf,fmt,args);
		m_pTraceFun("%s%s",m_bUseSSL?"[HttpsServer]":"[HttpServer]",buf);
	}
    va_end(args);
}

bool  THttpServer::IsBlockError(void *ssl,int socketID,int ret)
{
	int error;
	if(!m_bUseSSL) {
#ifdef WIN32
	error=WSAGetLastError();
	return (error==WSAEINTR || error==WSAEWOULDBLOCK);
#else
	return (errno==EINTR || errno==EWOULDBLOCK || errno==EAGAIN);
#endif
	}
#ifdef SUPPORT_SSL
	error = SSL_get_error((SSL*)ssl,ret);
	if( error ==SSL_ERROR_WANT_READ || error == SSL_ERROR_WANT_WRITE )
		return true;
	Trace("Error in SSL_R/W(socket:%d) erro:%d:%s\n",
		socketID, ret,ERR_reason_error_string(ERR_get_error()));
	return false;
#endif
}
