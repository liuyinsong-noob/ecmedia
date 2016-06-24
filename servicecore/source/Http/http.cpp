
#include <time.h>
#include <assert.h>
#include "http.h"
#include <algorithm>
#include <functional>
#include <cctype>
#include <sstream>


#pragma warning(disable : 4996)
#define ASSERT assert

#ifndef WIN32
#define strnicmp  strncasecmp
#define stricmp   strcasecmp
#endif

//trim from left
static std::string &ltrim(std::string &s) {
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), 
		std::not1(std::ptr_fun<int, int>(std::isspace))));
	return s;
}

// trim from end
static std::string &rtrim(std::string &s) {
	s.erase(std::find_if(s.rbegin(), s.rend(),
		std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
	return s;
}

//trim left and right
static std::string &trim(std::string &s) {
	return ltrim(rtrim(s));
} 

void SplitString(const std::string& src , std::map<std::string,std::string> &param_map)
{
	std::string key,value;
	size_t start = 0;
	do {
		size_t pos = src.find_first_of("=",start);
		if( pos == std::string::npos) {
			break;
		}
		key = src.substr(start,pos-start);
		start = pos+1;
		pos = src.find_first_of("&",start);
		if( pos == std::string::npos) {
			value=src.substr(start);
			param_map[key] = value;
			break;
		}
		value = src.substr(start,pos-start);
		param_map[key] = value;
		start = pos+1;
	}while(true);
}

THttp::THttp():m_nContentLen(-1)
{
}

THttp::~THttp()
{
}

const std::string &  THttp::GetContentType()
{
    return m_strContentType;
}

const std::string &  THttp::GetContentData()
{
    return m_strContentData;
}

const std::string &  THttp::GetAgentData()
{
    return m_strAgentData;
}

const std::string & THttp::GetUserAgentData()
{
    return m_strUserAgentData;
}

const std::string & THttp::GetAuthorizationData()
{
    return m_strAuthorizationData;
}

const std::string & THttp::GetMobileNumData()
{
    return m_strMobileNumData;
}

int THttp::GetContentLength()
{
    return m_strContentData.length();
}

void  THttp::SetContentType(const std::string & contentType)
{
	m_strContentType = contentType;
    trim(m_strContentType);
}

void  THttp::SetContentData(const std::string & data)
{
   m_strContentData = data;
   //printf("the test \n");
}

void  THttp::SetAgentData(const std::string &strAgentData)
{
    m_strAgentData = strAgentData;
	trim(m_strAgentData);
}

void  THttp::SetUserAgentData(const std::string &strUserAgentData)
{
    m_strUserAgentData = strUserAgentData;
    trim(m_strUserAgentData);
}

void  THttp::SetServerData(const std::string &strServerData)
{
    m_strServerData = strServerData;
	trim(m_strServerData);
}

void  THttp::SetHost(const std::string & strHost)
{
    m_strHost = strHost;
    trim(m_strHost);
}

void  THttp::SetAuthenticateData(const std::string & strAuthenticateData)
{
    m_strAuthenticateData = strAuthenticateData;
    trim(m_strAuthenticateData);
}

void  THttp::SetAuthorizationData(const std::string & strAuthorizaionData)
{
	m_strAuthorizationData = strAuthorizaionData;
    trim(m_strAuthorizationData);
}


void  THttp::SetMobileNumData(const std::string &strMobileNumData)
{
	m_strMobileNumData = strMobileNumData;
	trim(m_strMobileNumData);
}

void  THttp::SetAccept(const std::string & strAccept)
{
	m_strAccept = strAccept;
	trim(m_strAccept);
}

int THttp::EncodeMessage(std::string & outputData,int& length)
{
    //set date
    char strDate[64]={0};
    time_t lt = time(NULL);
    strcpy(strDate,asctime(localtime(&lt)));
    strDate[strlen(strDate)-1]=0;

    outputData.append("Date:");
    outputData.append(strDate);
    outputData.append("\r\n");

	if (!m_strAccept.empty())
	{
		outputData.append("Accept:");
		outputData.append(m_strAccept);
		outputData.append("\r\n");
	}

	if (!m_strAuthenticateData.empty())
	{
		outputData.append("WWW-Authenticate:");
		outputData.append(m_strAuthenticateData);
		outputData.append("\r\n");
	}
  
	if(!m_strAuthorizationData.empty()){
		outputData.append("Authorization:");
		outputData.append(m_strAuthorizationData);
		outputData.append("\r\n");
	}
	
	if(!m_strHost.empty()){
		outputData.append("Host:");
		outputData.append(m_strHost);
		outputData.append("\r\n");
	}
    
	if(!m_strAgentData.empty()){
		outputData.append("Agent:");
		outputData.append(m_strAgentData);
		outputData.append("\r\n");
	}
    
	if (!m_strServerData.empty())
	{
		outputData.append("Server:");
		outputData.append(m_strServerData);
		outputData.append("\r\n");
	}
   
    //set connection
    outputData.append("Connection:close\r\n");

    //set content type
    if(!m_strContentData.empty())
    {
        if (!m_strContentType.empty())
        {
			outputData.append("Content-Type:");
			outputData.append(m_strContentType);
			outputData.append("\r\n");
        }
        else
        {
            outputData.append("Content-Type:text/plain\r\n");
        }
		outputData.append("Content-Length:");
		std::stringstream stream;
		stream << m_strContentData.length();
		outputData.append(stream.str());
		outputData.append("\r\n");
    }
    else
    {
        outputData.append("Content-Length:0\r\n");
    }
	outputData.append("\r\n");
    outputData.append(m_strContentData);
	length=outputData.length();
    return 0;
}
int THttp::DecodeMessage(const char* const inputData,int length)
{
    const char*  body=strstr(inputData,"\r\n\r\n");
    if(body ==NULL)
        return -1;
    body+=4;

    if(body -  inputData >  10240)
    {
    	return -1;
    }
    char header[10240];
    int  count=0;
    for(int i=0;i<length;i++)
    {
        if(inputData[i] == '\r')
        {
            if(strncmp(inputData+i,"\r\n",2)==0)
            {
                header[count]=0;
            
                if(strnicmp(header,"content-type:",13)==0)
                {
                    SetContentType(header+13);
                }
				else if(strnicmp(header,"Agent:",6)==0)
                {
                    SetAgentData(header+6);
                }
				else if(strnicmp(header,"User-Agent:",11)==0)
                {
                    SetUserAgentData(header+11);
                }
				else if(strnicmp(header,"Authorization:",14)==0)
                {
                    SetAuthorizationData(header+14);
                }
				else if(strnicmp(header,"X-Up-Calling-Line-ID:",21)==0)
                {
                    SetMobileNumData(header+21);
                }
                else if(strnicmp(header,"content-length:",15)==0)
                {
					m_nContentLen = atoi(header+15);
//                     if(atoi(header+15) != (int)strlen(body))
//                     {
//                         return -2;
//                     }
                }
            }
            if(strncmp(inputData+i,"\r\n\r\n",4)!=0)
            {
                count=0;
                i+=1;
                continue;
            }
            break;
        }
        header[count++]=inputData[i];
    }

    SetContentData(std::string(body,m_nContentLen));
    return 0;
}

THttpRequest::THttpRequest():m_nMethod(HTTP_METHOD_UNKNOWN)
{
}

THttpRequest::~THttpRequest()
{
}

void  THttpRequest::SetMethod(const int method)
{
    ASSERT(method == HTTP_METHOD_GET || method == HTTP_METHOD_PUT || method==HTTP_METHOD_DELETE|| method==HTTP_METHOD_POST);
    m_nMethod=method;
}

void  THttpRequest::SetURI(const std::string &uri)
{
    m_strURI = uri;
}

int  THttpRequest::GetMethod()
{
    return m_nMethod;
}

const std::string & THttpRequest::GetURI()
{
    return m_strURI;
}

int  THttpRequest::Encode(std::string & outputData,int& length)
{
    static const char* MethodName[4]={"GET","PUT","DELETE","POST"};

    if(m_nMethod == HTTP_METHOD_UNKNOWN)
        return -1;

    if(m_strURI.empty())
        return -2;

    //set request line
    outputData.append(MethodName[m_nMethod]);
	outputData.append(" ");
	outputData.append(m_strURI);
	outputData.append(" HTTP/1.1\r\n");

    //set body
    EncodeMessage(outputData,length);
    return 0;
}

int  THttpRequest::Decode(const char* const inputData,int length)
{
    if(inputData ==NULL)
        return -1;

    int    method;
    char   uri[1024]="";

    char strType[128];
    char strVersion[128]="";

    const char * strend = NULL;
    strend = strstr(inputData, "\r\n");

    if(strend-inputData >=512)
		return -1;
   	
    sscanf(inputData,"%127s %1023s %127s",strType,uri,strVersion);
    if(strcmp(strType,"GET") ==0)
    {
        method=HTTP_METHOD_GET;
    }
    else if(strcmp(strType,"PUT") ==0)
    {
        method=HTTP_METHOD_PUT;
    }
    else if(strcmp(strType,"DELETE") ==0)
    {
        method=HTTP_METHOD_DELETE;
    }
	else if(strcmp(strType,"POST") ==0)
    {
        method=HTTP_METHOD_POST;
    }
    else
    {
        return -3;
    }

//     if(strcmp(strVersion,"HTTP/1.1") !=0)  //手机发出的消息，经过WAP网关后，HTTP消息的版本号有时候会变为1.0
//         return -4;
   
 
    if(DecodeMessage(inputData,length) !=0)
        return -5;

    SetMethod(method);
    SetURI(uri);
    return 0;
}


struct TResponseStatus
{
    int  code;
    const char* desc;
};

const TResponseStatus responseStats[] = \
{
  /* Informational 1xx */
  {100, "Continue"},
  {101, "Switching Protocols"},

  /* Successful 2xx */
  {200, "OK"},
  {201, "Created"},
  {202, "Accepted"},
  {203, "Non-Authoritative Information"},
  {204, "No Content"},
  {205, "Reset Content"},
  {206, "Partial Content"},

  /* Redirection 3xx */
  {300, "Multiple Choices"},
  {301, "Moved Permanently"},
  {302, "Found"},
  {303, "See Other"},
  {304, "Not Modified"},
  {305, "Use Proxy"},
  // 306 Unused
  {307, "Temporary Redirect"},
 
  /* Client Error 4xx */
  {400, "Bad Request"},
  {401, "Unauthorized"},
  {402, "Payment Required"},
  {403, "Forbidden"},
  {404, "Not Found"},
  {405, "Method Not Allowed"},
  {406, "Not Acceptable"},
  {407, "Proxy Authentication Required"},
  {408, "Request Timeout"},
  {409, "Conflict"},
  {410, "Gone"},
  {411, "Length Required"},
  {412, "Precondition Failed"},
  {413, "Request Entity Too Large"},
  {414, "Request-URI Too Long"},
  {415, "Unsupported Media Type"},
  {416, "Requested Range Not Satisfiable"},
  {417, "Expectation Failed"},

  /* Server Error 5xx */
  {500, "Internal Server Error"},
 // {501, "Not Implemented"},
  {501, "Billing Failed"},
  {502, "Bad Gateway"},
  {503, "Service Unavailable"},
  {504, "Gateway Timeout"},
  {505, "HTTP Version Not Supported"},
  {511, "Illegal User Status"},
  {512, "Exceed Max Downloads Limit"},
  {513, "Exceed Max Billing Money this Period"}
};

const char* getStatusDescription(int code)
{
    for (int i=0; i < sizeof(responseStats) / sizeof(responseStats[0]); i++)
    {
        if (responseStats[i].code == code)
        {
            return responseStats[i].desc; 
        }
    }
    return NULL;
}

THttpResponse::THttpResponse()
{
    m_nStatusCode=0;
}

THttpResponse::~THttpResponse()
{
    
}

int  THttpResponse::Encode(std::string & outputData,int& length)
{
    const char* desc=getStatusDescription(m_nStatusCode);
    if(desc ==NULL)
        return -1;

    outputData.append("HTTP/1.1 ");
	std::stringstream stream;
	stream << m_nStatusCode << " " << desc << "\r\n";
	outputData.append(stream.str());
    EncodeMessage(outputData,length);
    return 0;
}

int  THttpResponse::Decode(const char* inputData,int length)
{
    if(inputData ==NULL)
        return -1;
    
    char strVersion[256]={0};
    int  statusCode=0;
    char strDesc[256]={0};
	sscanf(inputData,"%255s %d %255s\r\n",strVersion,&statusCode,strDesc);// 使用的是statusCode 的地址？
    
//     if(strcmp(strVersion,"HTTP/1.1") !=0)   //手机发出的消息，经过WAP网关后，HTTP消息的版本号有时候会变为1.0
//         return -2;

    const char* t=getStatusDescription(statusCode);
    if(t ==NULL)
        return -3;

//     if(strcmp(strDesc,t)!=0)
//         return -4;

    if(!strstr(inputData,"\r\n\r\n"))
        return -5;

    if(DecodeMessage(inputData,length) !=0)
        return -6;

    SetStatusCode(statusCode);
    return 0;
}

void THttpResponse::SetStatusCode(int statusCode)
{
    m_nStatusCode=statusCode;
}
int THttpResponse::GetStatusCode()
{
    return m_nStatusCode;
}
