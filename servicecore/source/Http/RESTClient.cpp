#include "RESTClient.h"
#include <time.h>
#include <string>
#include "HttpClient.h"
#include "Utility/md5.h"
#include "Utility/base64_2.h"
#include "Utility/tinyxml2.h"
#include <sstream>
#include "sometools.h"
#include "ECMedia.h"
#ifdef WIN32
#include <Windows.h>
#pragma warning(disable:4996)
#else
#define stricmp strcasecmp
#endif

extern FILE *traceFile;
extern std::string timetodate(time_t const timer);

std::string Utf8ToLocal(const char* u8string)
{
#ifdef WIN32

    int wcharcount = strlen(u8string);
	std::string tempString(u8string);
	wchar_t * tempWstr = new wchar_t[wcharcount];
	MultiByteToWideChar(CP_UTF8, 0, u8string, -1, tempWstr, wcharcount);
	int requirelen = WideCharToMultiByte(CP_ACP,0, tempWstr,-1, NULL, NULL,NULL,NULL);
	if( 0 == requirelen) {
		delete[] tempWstr;
		return tempString;
	}

	char * tempStr = new char[requirelen];
	WideCharToMultiByte(CP_ACP,0, tempWstr,-1, tempStr, requirelen,NULL,NULL);
	std::string result(tempStr);
	delete[] tempWstr;
	delete[] tempStr;
	return result;
#else
	return std::string(u8string);
#endif

}

TRESTClient::TRESTClient(const std::string& server, int port,
	const std::string& accountid,std::string authenToken, std::string appid)
	: m_server(server),m_port(port),m_accountid(accountid),m_authenToken(authenToken),m_appid(appid),
	m_traceFun(NULL)
{
}
TRESTClient::~TRESTClient()
{
}
bool TRESTClient::CreateSubAccount(std::string name ,std::string& subAccount,
		std::string & sipAccount ,std::string&sipPassword ,std::string& authenToken,bool isHttps)
{
	m_errorMsg ="";
	m_lastresponse="";

	char message[512] = {0};
	sprintf(message,"<SubAccount>\n"
			"<appId>%s</appId>\n"
			"<friendlyName>%s</friendlyName>\n"
			"<parentAccountSid>%s</parentAccountSid>\n"
			"<type>1</type>\n"
			"<status>1</status>\n"
			"</SubAccount>\n", m_appid.c_str(),name.c_str(),m_accountid.c_str());

	std::string sig, authen;
	GenSignature(sig,authen);
	std::string uri("/cloudcom/2012-08-20/SubAccounts?sig=");
	uri += sig;

	THttpClient client(m_server,m_port,true);
	THttpRequest request;
	request.SetHost(m_server.c_str());
	request.SetMethod(HTTP_METHOD_POST);
	request.SetContentType("application/xml;charset=utf-8");
	request.SetAccept("application/xml");
	request.SetContentData(message);

	request.SetAuthorizationData(authen);
	request.SetURI(uri);

	THttpResponse response;
	if( !client.SynHttpRequest(request, response) )
	{
		m_errorMsg= client.GetErrorMessage();
		return false;
	}
	m_lastresponse = response.GetContentData();
    PrintConsole("CreateSubAccount Request:%s\n",request.GetContentData().c_str());
    PrintConsole("CreateSubAccount Response:%s\n", m_lastresponse.c_str());
	if(response.GetStatusCode() != 200) 
	{
		m_errorMsg = "response error: status code : "+response.GetStatusCode();
		return false;
	}
	cloopenwebrtc::tinyxml2::XMLDocument doc;
	if( doc.Parse( response.GetContentData().c_str() )!= 0) {
		m_errorMsg = doc.GetErrorStr1();
		return false;
	}
	cloopenwebrtc::tinyxml2::XMLElement* responseElemenet 	= doc.FirstChildElement();
	if( !responseElemenet || stricmp(responseElemenet->Name(),"Response")) {
		m_errorMsg="no Response child element in response";
		return false;
	}
	cloopenwebrtc::tinyxml2::XMLElement*  subAccountElement = responseElemenet->FirstChildElement("SubAccount");
	if( !subAccountElement) {
		cloopenwebrtc::tinyxml2::XMLElement *message = responseElemenet->FirstChildElement("Message");
		if(!message) {
			m_errorMsg="no message child element in response";
			return false;
		}
		cloopenwebrtc::tinyxml2::XMLElement *code = message->FirstChildElement("code");
		cloopenwebrtc::tinyxml2::XMLElement *msg = message->FirstChildElement("msg");
		if( code && msg) {
			m_errorMsg = Utf8ToLocal(msg->GetText());
		}
		return false;
	}
	cloopenwebrtc::tinyxml2::XMLElement *accountSid = subAccountElement->FirstChildElement("accountSid");
	if( !accountSid)
	{
		m_errorMsg ="can not get accountSid in repsonse";
		return false;
	}
	subAccount = accountSid->GetText();
	cloopenwebrtc::tinyxml2::XMLElement *sipCode = subAccountElement->FirstChildElement("sipCode");
	if( !sipCode) 
	{
		m_errorMsg ="can not get sipCode in repsonse";
		return false;
	}
	sipAccount = sipCode->GetText();
	cloopenwebrtc::tinyxml2::XMLElement *password = subAccountElement->FirstChildElement("sipPwd");
	if( !password) {
		m_errorMsg ="can not get sipPwd in repsonse";
		return false;
	}
	sipPassword=password->GetText();
	cloopenwebrtc::tinyxml2::XMLElement *authen_token= subAccountElement->FirstChildElement("authToken");
	if( !authen_token) {
		m_errorMsg ="can not get authen_token in repsonse";
		return false;
	}
	authenToken=authen_token->GetText();
	return true;
}

bool TRESTClient::CallBackRequest(const std::string& caller, const std::string& called, 
	const std::string& subaccount,const std::string & sipid, const std::string & token, int& status,bool isHttps)
{
	m_errorMsg ="";
	m_lastresponse="";

	char message[512] = {0};
	sprintf(message,"<CallBack>\n"
			"<accountSid>%s</accountSid>\n"
			"<from>%s</from>\n"
			"<to>%s</to>\n"
			"<sipCode>%s</sipCode>\n"
			"</CallBack>\n", subaccount.c_str(),caller.c_str(),
			called.c_str(),sipid.c_str());

	std::string sig, authen;
	GenSignature(sig,authen,subaccount,token);

	std::string uri("/cloudcom/2012-08-20/SubAccounts/");
	uri += subaccount +"/Calls/Callback?sig="+sig;

	THttpClient client(m_server,m_port,true);
    client.SetTraceFunc(m_traceFun);
    
	THttpRequest request;
	request.SetHost(m_server.c_str());
	request.SetMethod(HTTP_METHOD_POST);
	request.SetContentType("application/xml;charset=utf-8");
	request.SetAccept("application/xml");
	request.SetContentData(message);

	request.SetAuthorizationData(authen.c_str());
	request.SetURI(uri.c_str());
	THttpResponse response;
	if( !client.SynHttpRequest(request, response) )
	{
		m_errorMsg= client.GetErrorMessage();
		return false;
	}
	m_lastresponse = response.GetContentData();
		printf("%s\n", m_lastresponse.c_str());
	if(response.GetStatusCode() != 200) 
	{
		m_errorMsg = "response error: status code : "+response.GetStatusCode();
		return false;
	}
	cloopenwebrtc::tinyxml2::XMLDocument doc;
	if( doc.Parse( response.GetContentData().c_str())!= 0) {
		m_errorMsg = doc.GetErrorStr1();
		return false;
	}
	cloopenwebrtc::tinyxml2::XMLElement* responseElement 	= doc.FirstChildElement();
	if( !responseElement || stricmp(responseElement->Name(),"Response")) {
		m_errorMsg="no Response child element in response";
		return false;
	}
	cloopenwebrtc::tinyxml2::XMLElement*  callElement = responseElement->FirstChildElement("CallBack");
	if( !callElement) {
		cloopenwebrtc::tinyxml2::XMLElement *messageElement = responseElement->FirstChildElement("Message");
		if(!messageElement) { 
			m_errorMsg="no callback element in response";
			return false;
		}
		cloopenwebrtc::tinyxml2::XMLElement* code  = messageElement->FirstChildElement("code");
		if( !code|| !strcmp(code->GetText(),""))	{
			status = 7654;
		}
		else
			status = atoi(code->GetText());

	}
	cloopenwebrtc::tinyxml2::XMLElement *statusElement = callElement->FirstChildElement("status");
	if( !statusElement) 
	{
		m_errorMsg ="can not status in repsonse";
		return false;
	}
	status = 0;
	return true;
}

bool TRESTClient::GetLoginServer(const std::string& subaccount,const std::string & sipid, 
				const std::string & token, std::string & softswitch_addr, int & softswitch_port,bool isHttps)
{
	m_errorMsg ="";
	m_lastresponse="";
	char message[512] = {0};
	std::string sig, authen;
	GenSignature(sig,authen,subaccount,token);

	std::string uri("/2013-03-22/Switchs/");
	uri += sipid +"?sig="+sig +"&deviceNo"+"=1234";

	THttpClient client(m_server,m_port,true);
    client.SetTraceFunc(m_traceFun);
    
	THttpRequest request;
	request.SetHost(m_server.c_str());
	request.SetMethod(HTTP_METHOD_GET);
	request.SetContentType("application/xml;charset=utf-8");
	request.SetAccept("application/xml");
	request.SetContentData(message);

	request.SetAuthorizationData(authen.c_str());
	request.SetURI(uri.c_str());
	THttpResponse response;
	if( !client.SynHttpRequest(request, response) )
	{
		m_errorMsg= client.GetErrorMessage();
		return false;
	}
	m_lastresponse = response.GetContentData();
		printf("%s\n", m_lastresponse.c_str());
	if(response.GetStatusCode() != 200) 
	{
		m_errorMsg = "response error: status code : "+response.GetStatusCode();
		return false;
	}
	cloopenwebrtc::tinyxml2::XMLDocument doc;
	if( doc.Parse( response.GetContentData().c_str())!= 0) {
		m_errorMsg = doc.GetErrorStr1();
		return false;
	}
	cloopenwebrtc::tinyxml2::XMLElement* responseElement 	= doc.FirstChildElement();
	if( !responseElement || stricmp(responseElement->Name(),"Response")) {
		m_errorMsg="no Response child element in response";
		return false;
	}
	cloopenwebrtc::tinyxml2::XMLElement*  switchElement = responseElement->FirstChildElement("Switch");
	if( !switchElement) {
		cloopenwebrtc::tinyxml2::XMLElement *messageElement = responseElement->FirstChildElement("Message");
		if(!messageElement) { 
			m_errorMsg="no callback element in response";
			return false;
		}
		cloopenwebrtc::tinyxml2::XMLElement* code  = messageElement->FirstChildElement("code");

	}
	cloopenwebrtc::tinyxml2::XMLElement *ipElement = switchElement->FirstChildElement("ip");
	if( !ipElement) 
	{
		m_errorMsg ="can not ip in repsonse";
		return false;
	}
	softswitch_addr = ipElement->GetText();

	cloopenwebrtc::tinyxml2::XMLElement *portElement = switchElement->FirstChildElement("port");
	if( !ipElement) 
	{
		m_errorMsg ="can not port in repsonse";
		return false;
	}
	softswitch_port = atoi(portElement->GetText());

	return true;

}

bool TRESTClient::CheckProxyValid(const std::string& proxy, const std::string& sipAccount, const std::string& key, int& status,bool isHttps)
{
    m_errorMsg ="";
	m_lastresponse="";
	char message[512] = {0};

    std::string param;
	std::string signature;
    
    std::string par;
	par.append(proxy);
	par.append(";");
	par.append(sipAccount);

    param = base64_encode(par.c_str(),par.length());
    
    std::string  sig;
	sig.append(proxy);
	sig.append(key);
	sig.append("cloopen");
	signature = md5(sig);
    
    std::stringstream domain;
	domain <<"http://" << m_server << ":" << m_port << "/2013-12-26/inner/ServerAddr" 
		<< "?param=" << param << "&sig=" << signature;

	std::string uri = domain.str();
    
	THttpClient client(m_server,m_port,false);
    client.SetTraceFunc(m_traceFun);
    
	THttpRequest request;
	request.SetHost(m_server.c_str());
	request.SetMethod(HTTP_METHOD_GET);
	request.SetContentType("application/xml;charset=utf-8");
	request.SetAccept("application/xml");
	request.SetContentData(message);
	request.SetURI(uri.c_str());
    
	THttpResponse response;
	if( !client.SynHttpRequest(request, response) )
	{
		m_errorMsg= client.GetErrorMessage();
		return false;
	}
	m_lastresponse = response.GetContentData();

	if(response.GetStatusCode() != 200)
	{
		m_errorMsg = "response error: status code : "+ response.GetStatusCode();
		return false;
	}
	cloopenwebrtc::tinyxml2::XMLDocument doc;
	if( doc.Parse( response.GetContentData().c_str())!= 0) {
		m_errorMsg = doc.GetErrorStr1();
		return false;
	}
	cloopenwebrtc::tinyxml2::XMLElement* responseElement 	= doc.FirstChildElement();
	if( !responseElement || stricmp(responseElement->Name(),"Response")) {
		m_errorMsg="no Response child element in response";
		return false;
	}
	cloopenwebrtc::tinyxml2::XMLElement*  statusCodeElement = responseElement->FirstChildElement("statusCode");
	if( !statusCodeElement) {
        m_errorMsg="no statusCode element in response";
        return false;
	}
	status = atoi(statusCodeElement->GetText());
    
	return true;
}

bool TRESTClient::CheckPrivateProxyValid(const std::string& companyID, const std::string& proxy,const int port, int& status,bool isHttps)
{
    m_errorMsg ="";
	m_lastresponse="";
    
	std::string uri ("/2013-12-26/inner/checkCorpHost");
    
	PrintConsole("CheckPrivateProxyValid m_server:%s,m_port:%d\n",m_server.c_str(),m_port);
	THttpClient client(m_server,m_port,false);
    client.SetTraceFunc(m_traceFun);

	THttpRequest request;
    std::stringstream host;
    host<<m_server<<":"<<m_port;
	request.SetHost(host.str().c_str());
	request.SetMethod(HTTP_METHOD_POST);
	request.SetContentType("application/xml;charset=utf-8");
	request.SetAccept("application/xml");
    request.SetURI(uri);
    
    char message[512] = {0};
	sprintf(message,"<Request>\n"
			"<corp_id>%s</corp_id>\n"
			"<clpss_ip>%s</clpss_ip>\n"
			"<clpss_port>%d</clpss_port>\n"
			"</Request>\n", companyID.c_str(),proxy.c_str(),port);
    
    if (traceFile) {
        time_t temp = time(NULL);
        std::string strTime = timetodate(temp);
        fwrite(strTime.c_str(), strTime.length(), 1, traceFile);
        
        fwrite(message, strlen(message), 1, traceFile);
        fflush(traceFile);
    }
    
    
	request.SetContentData(message);
    
	request.SetURI(uri.c_str());
    
	THttpResponse response;
    PrintConsole("CheckPrivateProxyValid Request:%s\n",request.GetContentData().c_str());
    
    
	if( !client.SynHttpRequest(request, response) )
	{
		m_errorMsg= client.GetErrorMessage();
        status = 11;
		return false;
	}
	m_lastresponse = response.GetContentData();
    PrintConsole("CheckPrivateProxyValid Response:%s\n", m_lastresponse.c_str());
    
    if (traceFile) {
        time_t temp = time(NULL);
        std::string strTime = timetodate(temp);
        fwrite(strTime.c_str(), strTime.length(), 1, traceFile);
        
        fwrite(m_lastresponse.c_str(), m_lastresponse.length(), 1, traceFile);
        fflush(traceFile);
    }
    
	if(response.GetStatusCode() != 200)
	{
		m_errorMsg = "response error: status code : "+ response.GetStatusCode();
		return false;
	}
	cloopenwebrtc::tinyxml2::XMLDocument doc;
	if( doc.Parse( response.GetContentData().c_str())!= 0) {
		m_errorMsg = doc.GetErrorStr1();
		return false;
	}
	cloopenwebrtc::tinyxml2::XMLElement* responseElement 	= doc.FirstChildElement();
	if( !responseElement || stricmp(responseElement->Name(),"Response")) {
		m_errorMsg="no Response child element in response";
		return false;
	}
	cloopenwebrtc::tinyxml2::XMLElement*  statusCodeElement = responseElement->FirstChildElement("statusCode");
	if( !statusCodeElement) {
        m_errorMsg="no statusCode element in response";
        return false;
	}
	status = atoi(statusCodeElement->GetText());
    cloopenwebrtc::tinyxml2::XMLElement*  licenseElement = responseElement->FirstChildElement("license");
	if( !licenseElement) {
        m_errorMsg="no license element in response";
        return false;
	}
    std::stringstream compositeString;
    compositeString<<companyID<<proxy<<port<<"cloopenyuntongxun";
    if (strcmp(licenseElement->GetText(), md5(compositeString.str()).c_str())) {
         m_errorMsg="license not match";
        status = 11;
        return false;
    }
    
	return true;
}

void TRESTClient::GenSignature( std::string &signature, std::string &authorization, 
	std::string subaccount,std::string token)
{
	char sig[128]={0},auth[128]={0};
	char timestamp[32] = {0};
	time_t now = time(NULL);
	tm *now_tm = localtime(&now);
	std::string accountid = m_accountid;
    std::string authentoken = m_authenToken;
	if( subaccount !="")	
		accountid = subaccount;
   if( token !="" )
       authentoken = token;
	
	sprintf(timestamp,"%04d%02d%02d%02d%02d%02d",now_tm->tm_year+1900,now_tm->tm_mon+1,
		now_tm->tm_mday, now_tm->tm_hour,now_tm->tm_min,now_tm->tm_sec );
	sprintf(auth,"%s:%s",accountid.c_str(),timestamp);
	authorization = base64_encode(reinterpret_cast<const char*>(auth),strlen(auth));
	sprintf(sig,"%s%s%s",accountid.c_str(),authentoken.c_str(),timestamp);
	signature = md5(sig);
	return ;
}
void TRESTClient::SetTraceFunc(TRACE_FUNC trace_func)
{
    m_traceFun = trace_func;
}
