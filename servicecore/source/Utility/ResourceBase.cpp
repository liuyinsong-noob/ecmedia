#include "ResourceBase.h"

namespace kernel
{


#ifndef WIN32

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

// Cleanup function, modify the threadId
void threadCleanUp(void *param)
{
	CTThread *pThis = (CTThread *)param;
	if(pThis != NULL)
	{
		pThis->m_thread = INVALID_HANDLE_VALUE;
		pThis->m_terminatedEvent.Set();
		
	}
}

// the wrapped thread address
THREAD_HANDLER_DECL(WrappedEntryAddress, param)
{
	CTThread *pThis = (CTThread *)param;

	if(pThis != NULL)
	{
		// remember a cleanup function
		pthread_cleanup_push(threadCleanUp, param);
		if (pThis->m_userEntryAddress)
		{
			pThis->m_userEntryAddress(pThis->m_userParam);
		}
		pthread_cleanup_pop(1);
	}
	return 0;
}

void OnSIGUSR1Proc(int signal)
{
	pthread_exit(0);
}

CTThread::CTThread(	THREADFUNC entryAddress, void *thrdParam,
					long creationFlag /*= PTHREAD_CREATE_DETACHED*/,
					void *stackAddr /*= NULL*/, 
					size_t stackSize /*= 0*/, 
					const char *thrName /*= NULL*/)
{
	m_userEntryAddress = entryAddress;
	m_userParam = thrdParam;

#ifndef __linux__
	sigset(SIGQUIT, OnSIGUSR1Proc);   //To catch SIGUSR1 signal
#endif	
	pthread_attr_init(&m_threadAttr);
	
	if (creationFlag & PTHREAD_CREATE_DETACHED )
		pthread_attr_setdetachstate(&m_threadAttr, PTHREAD_CREATE_DETACHED);
	else
		pthread_attr_setdetachstate(&m_threadAttr, PTHREAD_CREATE_JOINABLE);

	if(stackAddr)
		pthread_attr_setstack(&m_threadAttr, stackAddr,stackSize>0?stackSize:1024*1024);
	if(stackSize)
		pthread_attr_setstacksize(&m_threadAttr, stackSize);

	int ret = pthread_create(&m_thread, &m_threadAttr, 
							  WrappedEntryAddress, this);
	if (ret != 0)
	{
		printf("pthread_create fail! ret=%d,errmsg=%s\r\n",ret,strerror(ret));
		m_thread = INVALID_HANDLE_VALUE;
		pthread_attr_destroy(&m_threadAttr);
	}
}

bool CTThread::GetExitCode(unsigned long &exitCode)
{
	exitCode = (m_thread == INVALID_HANDLE_VALUE ? 0: STILL_ACTIVE);
	return true;
}

bool CTThread::Terminate(int exitCode/*=0*/) 
{ 
	if(IsValid()) 
	{
		m_terminatedEvent.Reset();
		bool ret = (pthread_cancel(m_thread) == 0);
		m_thread = INVALID_HANDLE_VALUE;
		
		// if cancel successfully, wait the cancel callback function finished!
		if(ret == 0)
			m_terminatedEvent.Wait(1000);	// at most wait for 1s

		pthread_attr_destroy(&m_threadAttr);
		return (ret == 0);
	}
	return true;
}

bool CTThread::SetPriority(int newPri)
{
	struct sched_param priorityParams;
	int policy;

	if( pthread_attr_setschedparam(&m_threadAttr, &priorityParams) == 0 &&
		pthread_attr_getschedpolicy(&m_threadAttr, &policy) == 0)
	{
		return pthread_setschedparam( m_thread, newPri, &priorityParams) == 0;
	}
	else
	{
		return false;
	}
}

int CTThread::GetPriority(void) 
{ 
	struct sched_param priorityParams;
	pthread_attr_getschedparam(&m_threadAttr, &priorityParams);
	return priorityParams.sched_priority;
};


#endif
}
