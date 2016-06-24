#ifndef _HTTP_H_
#define _HTTP_H_

#include <string>
#include <map>

#define  HTTP_METHOD_UNKNOWN  -1
#define  HTTP_METHOD_GET      0
#define  HTTP_METHOD_PUT      1
#define  HTTP_METHOD_DELETE   2
#define  HTTP_METHOD_POST     3

void SplitString(const std::string& src , std::map<std::string,std::string> &param_map);

class THttp
{
protected:
    THttp();
    ~THttp();
public:    
    void  SetContentType(const std::string &contentType);
    void  SetContentData(const std::string &data);
	void  SetAgentData(const std::string &strAgentData);
	void  SetUserAgentData(const std::string &strUserAgentData);
	void  SetServerData(const std::string &strServerData);
	void  SetHost(const std::string &strHost);
	void  SetAccept(const std::string &strAccept);
	void  SetAuthenticateData(const std::string &strAuthenticate);
	void  SetAuthorizationData(const std::string &strAuthorization);
	void  SetMobileNumData(const std::string &strMobileNum);

    const std::string & GetContentType();
    const std::string & GetContentData();
	const std::string & GetAgentData();
	const std::string & GetUserAgentData();
	const std::string & GetAuthorizationData();
	const std::string & GetMobileNumData();
    int   GetContentLength();

protected:
    int   EncodeMessage(std::string &outputData,int& length);
    int   DecodeMessage(const char* const inputData,int length);

    std::string m_strContentType;
    std::string m_strContentData;
	std::string m_strAgentData;
	std::string m_strUserAgentData;
	std::string m_strServerData;
	std::string m_strHost;
	std::string m_strAuthenticateData;
	std::string m_strAuthorizationData;
	std::string m_strMobileNumData;
	std::string m_strAccept;
    int   m_nContentLen;
};
class THttpRequest : public THttp
{
public:
    THttpRequest();
    ~THttpRequest();

    void  SetMethod(int method);
    void  SetURI(const std::string & uri);
    
    int   GetMethod();
    const std::string &GetURI();

    int   Encode(std::string & outputData,int& length);
    int   Decode(const char* const inputData,int length);
private:
    int   m_nMethod;
    std::string m_strURI;
};

class THttpResponse : public THttp
{
public:
    THttpResponse();
    ~THttpResponse();

    void  SetStatusCode(int statusCode);
    int   GetStatusCode();

    int   Encode(std::string & outputData,int& length);
    int   Decode(const char* const inputData,int length);
private:
    int   m_nStatusCode;
};
#endif

