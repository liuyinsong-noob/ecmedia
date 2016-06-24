#ifdef WIN32
#include "stdafx.h"
#else
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#endif
#include "QueueInfo.h"


TCommandBody::TCommandBody(char* str,char* pAddress,unsigned int iPort,SOCKET fd)
{
    pStrMsg=new char[strlen(str)+1];
    strcpy(pStrMsg,str);

    strcpy(Ip,pAddress);
    Port=iPort;
    mySockFD=fd;
}

THttpBody::THttpBody(char* str,SOCKET fd,void*ssl,int len,void* pSessionInfo)
{
	if(len < 0)
	{
		nStrLen=strlen(str);
	    pStrMsg=new char[nStrLen+1];
	    strcpy(pStrMsg,str);
	}
	else
	{
		nStrLen=len;
		pStrMsg=new char[len+1];
		memset(pStrMsg,0,len+1);
	    memcpy(pStrMsg,str,len);
		pStrMsg[len] = 0;
	}
	memset(mfromIP,0,sizeof(mfromIP));
    mport=0;
    context.nSocketFD=fd;
	context.pSSL = ssl;
	context.pSession = pSessionInfo;
}

THttpBody::THttpBody(char* str,SOCKET fd,void*ssl,char* addr,int port,int len,void* pSessionInfo)
{
   if(len==-1)
	{
		nStrLen=strlen(str);
	    pStrMsg=new char[nStrLen+1];
	    strcpy(pStrMsg,str);
	}
	else
	{
		nStrLen=len;
		pStrMsg=new char[len+1];
		memset(pStrMsg,0,len+1);
	    memcpy(pStrMsg,str,len);
		pStrMsg[len] = 0;
	}
    if(addr)
    {
		strcpy(mfromIP,addr);
	}
	mport = port;
    context.nSocketFD=fd;
	context.pSSL=ssl;
	context.pSession = pSessionInfo;
}



TIphonePushBody::TIphonePushBody(int nCerTag,char* strDeviceToken,char* strMsgBuf)
{
	memset(m_strDeviceToken,0,128);
	memset(m_strMsgBuf,0,256);
	m_nCerTag = nCerTag;
	m_nType  = 0;

	if (strDeviceToken)
	{
		if (strlen(strDeviceToken)<128)
		{
			strcpy(m_strDeviceToken,strDeviceToken);
		}
	}

	if (strMsgBuf)
	{
		if (strlen(strMsgBuf)<256)
		{
			strcpy(m_strMsgBuf,strMsgBuf);
		}
	}
}



TTimerBody::TTimerBody(unsigned long timerid,unsigned long ownerid,unsigned long type)
{
    TimerID  = timerid;
    OwnerID = ownerid;
    RequestType=type;
}

TTimerBody2::TTimerBody2(unsigned long timerid,char* ownerid,unsigned long type)
{
    memset(OwnerID, 0, sizeof(OwnerID));
    strcpy(OwnerID, ownerid);
    TimerID  = timerid;
    RequestType=type;
}

CQueueInfo::CQueueInfo()
{
    MsgType=_MESSAGE_UNKNOWN;
    pBody=NULL;
}
CQueueInfo::CQueueInfo(int type,void* body)
{ 
    MsgType=type;
    pBody=body;
}

CQueueInfo::~CQueueInfo()
{
    
}

CQueueInfo & CQueueInfo::operator =(CQueueInfo &src)
{
    MsgType= src.MsgType;
    pBody  = src.pBody;
	return *this;    
}

void CQueueInfo::FreeBody()
{
    if(pBody == NULL)
        return;
    
    if(MsgType == _MESSAGE_COMMAND)
    {
        TCommandBody* p=(TCommandBody*)pBody;
        if(p->pStrMsg)
        {
            delete p->pStrMsg;
			p->pStrMsg=NULL;
        }
        delete p;
        
        pBody=NULL;
        return;
    }
    if(MsgType == _MESSAGE_HTTP)
    {
        THttpBody* p=(THttpBody*)pBody;
        if(p->pStrMsg)
        {
            delete p->pStrMsg;
			p->pStrMsg=NULL;
        }
        delete p;
        
        pBody=NULL;
        return;
    }



    if(MsgType == _MESSAGE_UDP_TIMER)
    {
    	 TTimerBody2* p=(TTimerBody2*)pBody;
        delete p;
        
        pBody=NULL;
        return;
    }


    if(MsgType == _MESSAGE_TIMER)
    {
        TTimerBody* p=(TTimerBody*)pBody;
        delete p;
        
        pBody=NULL;
        return;
    }

	if(MsgType == _MESSAGE_IPHONEPUSH)
    {
        TIphonePushBody* p=(TIphonePushBody*)pBody;
        delete p;        
        pBody=NULL;
        return;
    }

    if(MsgType == _MESSAGE_FEEDBACK)
    {
    	 char  *p = (char *)pBody;
	 delete p;
	 pBody = NULL;
	 return;
    }
}
