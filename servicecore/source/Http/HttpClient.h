#ifndef _HTTP_CLIENT_H
#define _HTTP_CLIENT_H

#include <string>
#include "http.h"
#ifdef WIN32
#include <WinSock2.h>
#include <WS2tcpip.h>
#endif

#ifdef SUPPORT_SSL
#include "openssl/bio.h" 
#include "openssl/ssl.h" 
#include "openssl/err.h" 
#endif

typedef void (*HttpResponseCallBack)(THttpResponse &response, bool success);
typedef void (*TRACE_FUNC) (const char *, ...);
class THttpClient {

public:
	THttpClient(const std::string &server, int port, bool bUseSSL);
	~THttpClient();
	bool SynHttpRequest(THttpRequest& reqest,THttpResponse &response );
	bool AsynHttpRequest(THttpRequest& reqest,HttpResponseCallBack callback );
	const std::string &GetErrorMessage(){ return m_errorMsg;}
	void	SetTraceFunc(TRACE_FUNC trace_func);

private:
	int sendHttpRequest(THttpRequest& request);
	int recvHttpResponse(THttpResponse& response);
	void Trace(const char*fmt,...);

	std::string m_server;
	std::string m_errorMsg;
	int m_port;
	bool m_bUseSSL;
	int m_socket;
	TRACE_FUNC m_pTraceFun;
#ifdef SUPPORT_SSL
	SSL_CTX* m_ssl_ctx;
	SSL *m_ssl;
#endif

};

#endif