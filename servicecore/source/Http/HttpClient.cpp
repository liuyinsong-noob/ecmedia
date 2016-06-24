#include "HttpClient.h"
#include <memory.h>
#include <string.h>
#include "sometools.h"

#ifdef WIN32
#pragma warning(disable:4996)
#ifdef SUPPORT_SSL 
#pragma comment(lib,"ssleay32.lib")
#pragma comment(lib,"libeay32.lib")
#pragma comment(lib,"ws2_32.lib")
#endif
#else
#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>

#define closesocket close
#define strnicmp  strncasecmp
#define TIME_OUT_TIME 20 //connect超时时间20秒
#endif

THttpClient::THttpClient(const std::string &server, int port, bool bUseSSL)
	:m_server(server),m_port(port),m_socket(-1),m_bUseSSL(bUseSSL),m_pTraceFun(NULL)
{
#ifdef WIN32
	static bool bInitSocket = false;
	if( !bInitSocket) 
	{
		bInitSocket = true;
		WORD wVersionRequested;
		WSADATA wsaData;
		wVersionRequested = MAKEWORD(2, 0);
		WSAStartup(wVersionRequested, &wsaData);
	}
#endif

#ifdef SUPPORT_SSL
	m_ssl_ctx = NULL;
	m_ssl = NULL;
#endif

}
THttpClient::~THttpClient()
{
	if(m_socket!=-1)
	{
		closesocket(m_socket);
	}
#ifdef SUPPORT_SSL
	if( m_ssl) {
		SSL_shutdown(m_ssl);
		SSL_free(m_ssl);
		SSL_CTX_free(m_ssl_ctx);
	}
#endif
}
void THttpClient::SetTraceFunc(TRACE_FUNC trace_func)
{
	m_pTraceFun = trace_func;
}

bool THttpClient::SynHttpRequest(THttpRequest& request,THttpResponse &response )
{

	m_errorMsg = "";
#ifdef SUPPORT_SSL
	if( m_bUseSSL && !m_ssl_ctx) 
	{
		SSL_library_init();
		SSL_load_error_strings();
		m_ssl_ctx = SSL_CTX_new(SSLv3_client_method());
		m_ssl =  SSL_new(m_ssl_ctx);
		SSL_CTX_set_verify(m_ssl_ctx, SSL_VERIFY_NONE, NULL);
	}
#else
	if( m_bUseSSL)
		return false;
#endif

	char cport[32]={0};
	struct addrinfo *result = NULL;
	struct addrinfo hints;
	sprintf(cport,"%d",m_port);

	memset( &hints,0,sizeof(addrinfo));
	hints.ai_family = AF_INET;
	hints.ai_protocol=IPPROTO_TCP;
	hints.ai_socktype = SOCK_STREAM;
	if((m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0){
		m_errorMsg = "create socket error";
		return false;
	}
	if( getaddrinfo(m_server.c_str(),cport, NULL, &result) != 0) {
		m_errorMsg = "get address info error for " +m_server+":"+cport;
		closesocket(m_socket);
		m_socket = -1;
		return false;
	}
	PrintConsole("address:%s\n",inet_ntoa(((sockaddr_in*)(result->ai_addr))->sin_addr));
	PrintConsole("port:%d\n",ntohs(((sockaddr_in*)(result->ai_addr))->sin_port));
	//set no block
	int flags;
	/* If they have O_NONBLOCK, use the Posix way to do it */
#if defined(O_NONBLOCK)
	/* Fixme: O_NONBLOCK is defined but broken on SunOS 4.1.x and AIX 3.2.5. */
	if (-1 == (flags = fcntl(m_socket, F_GETFL, 0)))
		flags = 0;
	fcntl(m_socket, F_SETFL, flags | O_NONBLOCK);
#else
	/* Otherwise, use the old way of doing it */
	flags = 1;
	int nobolock = ioctlsocket(m_socket, FIONBIO, (u_long *)&flags);
	if (nobolock == SOCKET_ERROR)
	{
		PrintConsole("set error happens non-block %d,:%s\n",nobolock,strerror(WSAGetLastError()));
	}
#endif
	int rett = connect(m_socket, result->ai_addr, result->ai_addrlen);
#ifdef _WIN32
	int errWin = WSAGetLastError();
	//int optVal;
	//int optLen = sizeof(optVal);
	//getsockopt(m_socket,SOL_SOCKET,SO_ERROR,(char*)&optVal,&optLen);
	if (rett < 0 && errWin != WSAEINPROGRESS && errWin != WSAEINTR && errWin != ERROR_SUCCESS && errWin != WSAEWOULDBLOCK) {
		PrintConsole("ERROR: connect error %s\n",strerror(errWin));
#else
	if(rett < 0 && (errno != EINPROGRESS && errno != EINTR)){
		PrintConsole("ERROR: connect error %s\n",strerror(errno));
#endif
		m_errorMsg = "connect to " +m_server+":"+cport+" error";
		closesocket(m_socket);
		m_socket = -1;
		freeaddrinfo( result);
		return false;
	}
	struct timeval timeout;
	timeout.tv_sec=10;
	timeout.tv_usec=5;
	fd_set set, rset;
	FD_ZERO(&set);
	FD_ZERO(&rset);
	FD_SET(m_socket, &set);
	FD_SET(m_socket, &rset);

	int res;
	res = select(m_socket+1,&rset,&set,NULL,&timeout);
	if(res < 0)
	{
		PrintConsole("ERROR: network error in connect\n");
		closesocket(m_socket);
		return false;
	}
	else if(res == 0)
	{
		PrintConsole("ERROR: connect time out\n");
		closesocket(m_socket);
		return false;
	}
#if defined(O_NONBLOCK)
	/* Fixme: O_NONBLOCK is defined but broken on SunOS 4.1.x and AIX 3.2.5. */
	if (-1 == (flags = fcntl(m_socket, F_GETFL, 0)))
		flags = 0;
	fcntl(m_socket, F_SETFL, flags&~O_NONBLOCK);
#else
	/* Otherwise, use the old way of doing it */
	flags = 0;
	int block = ioctlsocket(m_socket, FIONBIO, (u_long *)&flags);
	if (block == SOCKET_ERROR)
	{
		PrintConsole("set error happens block %d,:%s\n",block,strerror(WSAGetLastError()));
	}
#endif
#ifdef _WIN32
	int timeoutt = 5010;
	if (SOCKET_ERROR == setsockopt(m_socket,SOL_SOCKET,SO_RCVTIMEO,(char *)&timeoutt,sizeof(timeoutt))) {
		PrintConsole("set error happens receive timeout:%s\n",strerror(WSAGetLastError()));
	}
#else
	int resulttimeout = setsockopt(m_socket,SOL_SOCKET,SO_RCVTIMEO,(char *)&timeout.tv_sec,sizeof(struct timeval));
	if (resulttimeout < 0)
	{
		perror("setsockopt");
	}
#endif
	freeaddrinfo( result);
#ifdef SUPPORT_SSL
	if( m_bUseSSL) 
	{
		SSL_set_fd(m_ssl,m_socket);
		if( SSL_connect(m_ssl) < 0 )
		{
			m_errorMsg = "ssl connect to " +m_server+":"+cport+" error";
			return false;
		}
	}
#endif
	if( sendHttpRequest(request)<0 ) {
		closesocket(m_socket);
		m_socket = -1;
		return false;
	}
	if( recvHttpResponse(response)<0) {
		closesocket(m_socket);
		m_socket = -1;
		return false;
	}
	return true;
}
bool THttpClient::AsynHttpRequest(THttpRequest& request,HttpResponseCallBack callback )
{
	return true;
}

int THttpClient::sendHttpRequest(THttpRequest& request)
{
	std::string outputData ;
	int msglen = 0;
	if( request.Encode(outputData,msglen) < 0 )
	{
		m_errorMsg = " reqeust coding error";
		return -1;
	}
	Trace("[HttpClient][socket:%d] send message:\n%s\n\n",m_socket,outputData.c_str());
	int	sendlen = 0,len=0;
	while(sendlen <msglen )
	{
		if(!m_bUseSSL)
			len = send(m_socket, outputData.c_str() + sendlen, outputData.length()-sendlen, 0);
#ifdef SUPPORT_SSL
		else
			len = SSL_write(m_ssl,outputData.c_str() + sendlen,outputData.length()-sendlen);
#endif		
		if(len <= 0){
			m_errorMsg = "send message eroor";
			return -1;
		}
		sendlen += len;
	}

	return 0;
}
int THttpClient::recvHttpResponse(THttpResponse& response)
{
	int buflen = 4*1024;
	const char *body_len_header = "Content-Length:";
	char *inputData = new char[buflen];
	char lenStr[64] = {0};
	int len = 0,recvlen=0, bodylen = -1,ret=0;
	memset(inputData,0,buflen);
	char *body = NULL;
	char *unParsedHeader = inputData;
	do {
		if( !m_bUseSSL) 
			len = recv(m_socket, inputData+recvlen, buflen-recvlen, 0);
#ifdef SUPPORT_SSL
		else
			len = SSL_read(m_ssl,inputData+recvlen, buflen-recvlen);
#endif
		if( len <=0 )
			break;
		recvlen += len;
		if( bodylen < 0) 
		{
			char *header = strstr(unParsedHeader,"\r\n");
			while( header) 
			{
				if( strnicmp(header+2,body_len_header,strlen(body_len_header))) 
				{
					header = strstr(header+2,"\r\n");
					if( !header)
						break;
					unParsedHeader = header+2;
					continue;
				}

				char *endheader = strstr(header+2,"\r\n");
				if(endheader) {
					header += 2+strlen(body_len_header);
					strncpy( lenStr,header,endheader-header);
					bodylen = atoi(lenStr);
					break;
				}
			}

		} 
		// don't use else if here, because last if may change the bodylen
		if(bodylen >= 0 ) 
		{
			if(!body) {
				body = strstr(unParsedHeader-4,"\r\n\r\n");
				if( !body)
					unParsedHeader = inputData+recvlen;
				else 
					body += 4;
			}
			if( body && (body-inputData+bodylen+1>= recvlen)) {
				break;
			}
		}		
	} while(true);
	if(recvlen>0)
	{
		ret=response.Decode(inputData,recvlen);
	}else
	{
		ret=-1;
	}
	Trace("[HttpClient][socket:%d] receive message:\n%s\n\n",m_socket,inputData);
	delete[] inputData;
	return ret;
}
void  THttpClient::Trace(const char*fmt,...)
{
	va_list args;
	va_start(args, fmt);
	if( !m_pTraceFun) 
		vprintf(fmt,args);
	else
	{
		//m_pTraceFun(fmt, args);
		int len=10*1024;
		char logBuff[10*1024]={0};
#ifdef _WIN32
		_vsnprintf(logBuff,len,fmt, args);
#else
		vsnprintf(logBuff, len, fmt, args);
#endif
		m_pTraceFun(logBuff);
	}
	va_end(args);
}
