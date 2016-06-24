#ifndef _REST_CLIENT_H
#define _REST_CLIENT_H

#include <string>
#include "Utility/tinyxml2.h"

typedef void (*TRACE_FUNC) (const char *, ...);

class TRESTClient {
	
public:
	TRESTClient(const std::string& server, int port,const std::string& accountid, 
		std::string authenToken, std::string appid);
	~TRESTClient();
	bool CreateSubAccount(std::string name ,std::string &subAccount,
				std::string & sipAccount ,std::string&sipPassword ,std::string & authenToken , bool isHttps = true);
	bool CallBackRequest(const std::string& caller, const std::string & called,
				const std::string& subaccount,const	std::string & sipid, 
				const std::string & token ,int & status, bool isHttps = true);
	bool GetLoginServer(const std::string& subaccount,const	std::string & sipid, 
				const std::string & token, std::string & softswitch_addr, int & softswitch_port, bool isHttps = true);
    bool CheckProxyValid(const std::string& proxy, const std::string& sipAccount, const std::string& key, int& status, bool isHttps = false);
    bool CheckPrivateProxyValid(const std::string& companyID, const std::string& proxy,const int port, int& status, bool isHttps = false);
    
	const std::string& GetErrorMessge() { return m_errorMsg; };
	const std::string& GetLastResponse() { return m_lastresponse;};
    
    void	SetTraceFunc(TRACE_FUNC trace_func);
private:
	void GenSignature( std::string &signature, 
		std::string &authorization, std::string subaccount ="",std::string token="");
	std::string m_server;
	int m_port;
	std::string m_accountid;
	std::string m_lastresponse;
	std::string m_authenToken;
	std::string m_appid;
	std::string m_errorMsg;
    TRACE_FUNC m_traceFun;
};

#endif

