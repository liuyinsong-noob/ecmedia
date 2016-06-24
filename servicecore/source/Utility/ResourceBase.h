/////////////////////////////////////////////////////////////////////
#ifndef __RESOURCEBASE_H__
#define __RESOURCEBASE_H__



#ifdef WIN32

// for WIN32 platform 

#include <assert.h>
#include <string.h>
#include <iostream>
#include <process.h>
#include <windows.h>
#include <winbase.h>

/////////////////////////////////////////////////////////////////////
// CTCriticalSection
// CTCounter
// CTSingleObject
//     |
//     +-----> CTThread
//     |
//     +-----> CTMutex -----> CTInterMutex <---------------+
//     |                                                   |
//     +-----> CTSemaphore -> CTInterSemaphore <-----------+
//     |                                                   |
//     +-----> CTEvent -----> CTInterEvent <---------------+
//                                                         |
// CTInterObject -----------------------------------------+
//                                                        |
// CTMultiObject                                          |
//     |                                                  |
//     +-----> CTMultiMutex -----> CTInterMultiMutex <----+
//     |                                                  |
//     +-----> CTMultiSemaphore -> CTInterMultiSemaphore <-+
//     |                                                 |
//     +-----> CTMultiEvent -----> CTInterMultiEvent <-----+
//

///////////////////////////////////////////////////////////////////////////////
// CTCriticalSection Class
namespace kernel
{

class CTCriticalSection
{
public:
	CTCriticalSection()  { ::InitializeCriticalSection( &m_cs );	}
	~CTCriticalSection() { ::DeleteCriticalSection( &m_cs );		}

	void Lock()			 { ::EnterCriticalSection( &m_cs ); }
	void Unlock()		 { ::LeaveCriticalSection( &m_cs ); }
	void Enter()		 { Lock();	}
	void Leave()		 { Unlock();}

private:
	CRITICAL_SECTION m_cs;

};

///////////////////////////////////////////////////////////////////////////////
// CTCounter Class
class CTCounter
{

public:
	CTCounter( long cnt = 0 ) 
		: m_counter(cnt) { }

		long Get()			{ return m_counter;	}
		long Set(long cnt)	{ return ::InterlockedExchange(&m_counter, cnt);	}

		long Inc()			{ return ::InterlockedIncrement( &m_counter ); }
		long Dec()			{ return ::InterlockedDecrement( &m_counter ); }
		long operator = (int rval) { Set( rval ); return Get(); }

private:
	long m_counter;
};

///////////////////////////////////////////////////////////////////////////////
// CTSingleObject Class
class CTSingleObject
{
public:
	CTSingleObject() 
		: m_handle(INVALID_HANDLE_VALUE)	
	{
	}
	virtual ~CTSingleObject()	
	{
		::CloseHandle(m_handle);
		m_handle = INVALID_HANDLE_VALUE;
	}

protected:
	HANDLE m_handle;
};

///////////////////////////////////////////////////////////////////////////////
// CTThread Class
#define THREAD_HANDLER_DECL(function,param) DWORD WINAPI function(void *param)
typedef unsigned (__stdcall *THREADFUNC)(void *);

class CTThread : public CTSingleObject
{
public:
	CTThread(
		LPTHREAD_START_ROUTINE entryAddress,
		LPVOID                 thrdParam,
		DWORD                  creationFlag = 0,	// run immediately
		DWORD                  stackSize = 0		// 0 means default
		)
		: m_entryAddress( entryAddress )
		, m_thrdParam( thrdParam )
	{
		// 如果要判断创建线程是否成功，
		// 应用程序应该在构造函数之后调用IsValid()方法判断
		m_handle = (HANDLE)_beginthreadex( 0, stackSize, (THREADFUNC)entryAddress,
			thrdParam, creationFlag, (unsigned *)& m_threadId );
	}

	bool IsValid()
	{
		return m_handle != INVALID_HANDLE_VALUE;
	}

	bool GetExitCode(DWORD &exitCode)
	{
		return ::GetExitCodeThread(m_handle, &exitCode) == TRUE;
	}

	bool Terminate(DWORD exitCode)
	{
		return ::TerminateThread(m_handle, exitCode) == TRUE;
	}

	bool SetPriority( int nPriority )
	{
		return ::SetThreadPriority( m_handle, nPriority ) == TRUE;
	}

	int GetPriority()
	{
		return ::GetThreadPriority(m_handle);
	}

	// 下面的全部函数没有对应的LINUX实现，请注意使用
	DWORD Resume()
	{
		return ::ResumeThread(m_handle);
	}

	DWORD Continue()
	{
		return ::ResumeThread(m_handle);
	}

	DWORD Suspend()
	{
		return ::SuspendThread(m_handle);
	}

	DWORD GetRealId()
	{
		return m_threadId;
	}

	DWORD GetID()
	{
		return ::GetCurrentThreadId();
	}

	// 如果是自己的线程调用Exit，那么使用_endthreadex
	// 其他线程调用的Exit，则是结束线程
	void Exit(DWORD exitCode)
	{
		if(GetCurrentThreadId() == m_threadId)
		{
			_endthreadex((unsigned)exitCode);
		}
		else
		{
			Terminate((unsigned)exitCode);
		}
	}

	BOOL Kill(DWORD exitCode)
	{
		return ::TerminateThread(m_handle, exitCode);
	}

private:
	DWORD                  m_threadId ;
	LPTHREAD_START_ROUTINE m_entryAddress ;
	LPVOID                 m_thrdParam ;

} ;

///////////////////////////////////////////////////////////////////////////////
// TMutex Class
class CTMutex : public CTSingleObject
{
public:
	CTMutex(const char *name = NULL, BOOL initOwner = FALSE )
	{
		m_handle = ::CreateMutexA(NULL, initOwner, name);
	}

	DWORD Lock(DWORD timeout = INFINITE )
	{ 
		return ::WaitForSingleObject(m_handle, timeout ); 
	}

	BOOL Unlock()
	{ 
		return ::ReleaseMutex(m_handle); 
	}

	int TryLock() 
	{
		if (WAIT_TIMEOUT==::WaitForSingleObject(m_handle, 0))
			return -1;
		return 0;
	}
	BOOL IsLocked()
	{ 
		if (WAIT_TIMEOUT==::WaitForSingleObject(m_handle, 0))
			return TRUE; 
		Unlock();
		return FALSE;
	}
};

const long constMAXIMUMWAITCOUNT = 2048;
///////////////////////////////////////////////////////////////////////////////
// TSemaphore Class
class CTSemaphore : public CTSingleObject
{
public:
	CTSemaphore(long  initCnt,
		long  maxCnt = constMAXIMUMWAITCOUNT,
		const char *name = NULL)
	{
		assert(initCnt <= maxCnt);
		m_handle = ::CreateSemaphoreA(NULL, initCnt, maxCnt, name);
	}

	DWORD Wait(DWORD timeout = INFINITE )
	{ 
		return ::WaitForSingleObject(m_handle, timeout); 
	}

	BOOL Post(long relCnt = 1)
	{ 
		return ::ReleaseSemaphore(m_handle, relCnt, 0); 
	}

	DWORD Lock(DWORD timeout = INFINITE)
	{ 
		return Wait( timeout );
	}

	BOOL Unlock() 
	{	
		return Post(1);
	}

	BOOL IsLocked()
	{ 
		if (WAIT_TIMEOUT==::WaitForSingleObject( m_handle, 0 ))
			return TRUE; 
		Unlock();
		return FALSE;
	}

	/* 此函数实现不正确，将要删除。
	// -1 - error; 0~maxCnt - ok
	int GetCount()
	{
		long count;
		if (TRUE == ::ReleaseSemaphore( m_handle, 0, &count ) )
		{
			return (int)count;
		}
		return -1;
	}
	*/
};

///////////////////////////////////////////////////////////////////////////////
// TEvent Class
class CTEvent : public CTSingleObject
{
public:
	CTEvent(BOOL manualReset = TRUE,
		BOOL initState = TRUE,
		const char *name = NULL)
	{
		m_handle = ::CreateEventA( 0, manualReset, initState, name);
	}

	bool Set()	{ return ::SetEvent(m_handle)   == TRUE;	}
	bool Reset(){ return ::ResetEvent(m_handle) == TRUE;	}

	DWORD Wait( DWORD timeout = INFINITE )
	{ return ::WaitForSingleObject( m_handle, timeout ); }
};

///////////////////////////////////////////////////////////////////////////////
// TInterObject Class
class CTInterObject
{
public:
	CTInterObject( char* aName )
		: AlreadyExist( 0 )
	{
		if ( aName )
		{
			strncpy( Name, aName, MAX_PATH );
			Name[MAX_PATH] = 0;
		}
		else
		{
			exit( -1 );
		}
	}

	char* GetName() { return Name; }
	int IsFirstOne() { return !AlreadyExist; }

protected:
	char Name[MAX_PATH+1];
	int  AlreadyExist;
};

///////////////////////////////////////////////////////////////////////////////
// TInterMutex Class
class CTInterMutex : public CTMutex, public  CTInterObject
{
public:
	CTInterMutex( char* aName = 0 )
		: CTInterObject( aName ), CTMutex( aName )
	{
		// See if it is the first one
		if ( ERROR_ALREADY_EXISTS==GetLastError() )
		{
			AlreadyExist = 1;
		}
		else
		{
			AlreadyExist = 0;
		}
	}
};

///////////////////////////////////////////////////////////////////////////////
// TInterSemaphore Class
class CTInterSemaphore : public CTSemaphore, public CTInterObject
{
public:
	CTInterSemaphore( long  initCnt,
		long  maxCnt = constMAXIMUMWAITCOUNT,
		char* aName = 0 )
		: CTInterObject( aName ),
		CTSemaphore( initCnt, maxCnt, aName )
	{
		// See if it is the first one
		if ( ERROR_ALREADY_EXISTS==GetLastError() )
		{
			AlreadyExist = 1;
		}
		else
		{
			AlreadyExist = 0;
		}
	}
};

///////////////////////////////////////////////////////////////////////////////
// TInterEvent Class
class CTInterEvent : public CTEvent, public CTInterObject
{
public:
	CTInterEvent(BOOL manualReset= TRUE,
		BOOL  initState = TRUE,
		char* aName = 0 )
		: CTInterObject( aName ),
		CTEvent( manualReset, initState, aName )
	{
		// See if it is the first one
		if ( ERROR_ALREADY_EXISTS==GetLastError() )
		{
			AlreadyExist = 1;
		}
		else
		{
			AlreadyExist = 0;
		}
	}
};


///////////////////////////////////////////////////////////////////////////////
// TMultiObject Class
class CTMultiObject
{
protected:
	HANDLE*  m_handle;
	int      Size;
	CTCounter Counter;

public:
	CTMultiObject( int aSize );
	~CTMultiObject();
};

///////////////////////////////////////////////////////////////////////////////
// CTMultiEvent Class
class CTMultiEvent
{
private:
	HANDLE*  m_hMultiEvent;
	BOOL     m_manualReset;
	int      m_iEventSize;
	CTCounter m_Counter;

public:		
	CTMultiEvent(int iEventSize, BOOL manualReset = TRUE, BOOL initState = TRUE )
		: m_manualReset(manualReset)
	{
		assert( 0<iEventSize && iEventSize<=MAXIMUM_WAIT_OBJECTS );
		m_iEventSize  = iEventSize;
		m_hMultiEvent = new HANDLE[iEventSize];
		for( int i=0; i<m_iEventSize; i++ )
		{
			m_hMultiEvent[i] = ::CreateEvent( 0, manualReset, initState, 0 );
		}
		m_Counter.Set(0);
	}

	~CTMultiEvent()
	{
		for( int i=0; i<m_iEventSize; i++ )
		{
			::CloseHandle( m_hMultiEvent[i] );
			m_hMultiEvent[i] = 0;
		}
		delete [] m_hMultiEvent;
		m_hMultiEvent  = 0;
		m_manualReset = FALSE;
	}

	DWORD WaitAll( DWORD dwTimeOut = INFINITE )
	{
		return WaitForMultipleObjects( m_iEventSize, m_hMultiEvent, TRUE, dwTimeOut );
	}

	DWORD WaitOne( DWORD dwTimeOut = INFINITE )
	{
		return WaitForMultipleObjects( m_iEventSize, m_hMultiEvent, FALSE, dwTimeOut );
	}

	BOOL Set( const int index )
	{
		assert( 0<=index && index<m_iEventSize );
		return ::SetEvent( m_hMultiEvent[index] ) ;
	}

	BOOL Reset( int& index )
	{
		if ( m_Counter.Get()<0 || m_Counter.Get()>=m_iEventSize )
		{
			return FALSE;
		}
		index = m_Counter.Get();
		m_Counter.Inc();
		return ::ResetEvent( m_hMultiEvent[index] );
	}

	BOOL Pulse( const int index )
	{
		assert( 0<=index && index<m_iEventSize );
		return ::PulseEvent( m_hMultiEvent[index] ) ;
	}
};
}

#else  //end of WIN32

#if	defined( __linux__ ) && !defined( _GNU_SOURCE )
#	error	You must defined macro _GNU_SOURCE in your makefile ! 
#endif	//	__linux__

#include <pthread.h>
#include <errno.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <signal.h>
#include <sys/time.h>
#include <unistd.h>
#include <semaphore.h>

namespace kernel
{

#define stricmp           strcasecmp

// begin of *nix

#ifndef _REENTRAINT
#define _REENTRAINT
#endif

// define some WIN32 type
#ifndef DWORD
#define DWORD   unsigned long
#endif

#ifndef INVALID_SOCKET
#define INVALID_SOCKET   -1
#endif

#ifndef INVALID_HANDLE_VALUE
#define INVALID_HANDLE_VALUE ((pthread_t)-1)
#endif

#ifndef SOCKET_ERROR
#define SOCKET_ERROR   -2
#endif

#ifndef MAX_PATH
#define MAX_PATH   256
#endif

#ifndef E_FAIL
#define E_FAIL   -1
#endif

#ifndef INFINITE
#define INFINITE ((DWORD)-1)
#endif

#ifndef STILL_ACTIVE 
#define STILL_ACTIVE 259
#endif

#if !defined( PTHREAD_MUTEX_RECURSIVE_NP )
#	define PTHREAD_MUTEX_RECURSIVE_NP  PTHREAD_MUTEX_RECURSIVE
#endif

class CTCriticalSection
{
public:
	CTCriticalSection()
	{
		pthread_mutexattr_init(&m_mutexattr);
		pthread_mutexattr_settype(&m_mutexattr,  PTHREAD_MUTEX_RECURSIVE_NP);
		pthread_mutex_init(&m_mutex, &m_mutexattr);
	};
	~CTCriticalSection()
	{
		pthread_mutex_destroy(	&m_mutex );
	};
	void	Lock()		{ pthread_mutex_lock(&m_mutex);		};
	void	Unlock()	{ pthread_mutex_unlock(&m_mutex);	};
	void	Enter()     { Lock();	}
	void    Leave()     { Unlock(); }

private:
	pthread_mutex_t	m_mutex;
	pthread_mutexattr_t	m_mutexattr;
};

// CTCounter missed!

class CTMutex
{
public:
	CTMutex()
	{ 
		pthread_mutex_init(&m_mutex, NULL);
	};
	~CTMutex() 
	{
		pthread_mutex_destroy(&m_mutex); 
	};
	int Lock()		{ return pthread_mutex_lock(&m_mutex);		};
	int Unlock()	{ return pthread_mutex_unlock(&m_mutex);	};
	int TryLock()	{ return pthread_mutex_trylock(&m_mutex);	};
private:
	pthread_mutex_t m_mutex;
};

#if defined( __linux__ ) || defined( hpux )
#include <semaphore.h>

class CTSemaphore
{
public:
	CTSemaphore(int initCount, int maxCount){ sem_init(&m_sem, 0, initCount);	};
	~CTSemaphore()			{ sem_destroy(&m_sem);			};
	int Post()				{ return sem_post(&m_sem);		};
	int Wait()				{ return sem_wait(&m_sem);		};
	int Lock()				{ return Wait();				};
	int Unlock()			{ return Post();				};
	int GetCount()
	{
		int val = -1;
		sem_getvalue(&m_sem, &val);
		return val;
	}
	bool IsLocked() { return GetCount()<=0 ;}
private:
	sem_t m_sem;
};

#else	//other unix

#include <synch.h>    
#include <semaphore.h>

class CTSemaphore 
{
public:
	CTSemaphore()				
	{ 
		sema_init(&m_sem, 0, USYNC_THREAD, NULL);	
	};
	CTSemaphore(int count, int type=USYNC_THREAD)
	{ 
		sema_init(&m_sem, count, type, NULL);		
	};
	~CTSemaphore()				
	{ 
		sema_destroy(&m_sem);						
	};

	int Post() 
	{ 
		return sema_post(&m_sem); 
	};
	int Wait()
	{
		int ret;
		while ( (ret=sema_wait(&m_sem))==EINTR );
		return ret;
	};
	int Lock()				{ return Wait();				};
	int Unlock()			{ return Post();				};
	int GetCount()
	{
		int val = -1;
		sem_getvalue((sem_t*)(&m_sem), &val);
		return val;
	}

private:
	sema_t m_sem;
};
#endif	// !__linux__

class CTEvent
{
public:
	CTEvent(void)
	{
		init();
	}
	~CTEvent(void)
	{
		destroy();
	}
	bool Set()
	{
		pthread_cond_broadcast(&m_cond);
		return true;
	}
	bool Reset()
	{
		destroy();
		init();
		return true;
	}
	DWORD Wait( DWORD milliseconds = INFINITE)
	{
		if(milliseconds == INFINITE)
		{
			pthread_mutex_lock(&m_mutex);
			pthread_cond_wait(&m_cond, &m_mutex);
			pthread_mutex_unlock(&m_mutex);
		}
		else
		{
			struct timeval  now;
			struct timezone tz;
			struct timespec timeout;

			pthread_mutex_lock(&m_mutex);
			gettimeofday(&now, &tz);
			timeout.tv_sec  = now.tv_sec + milliseconds/1000;
			timeout.tv_nsec = now.tv_usec * 1000 + milliseconds%1000;

			pthread_cond_timedwait(&m_cond, &m_mutex, &timeout);
			pthread_mutex_unlock(&m_mutex);
		}
		return 0;
	}

private:
	void init()
	{
		pthread_cond_init(&m_cond, NULL);
		pthread_mutex_init(&m_mutex, NULL);
	}
	void destroy()
	{
		pthread_cond_destroy(&m_cond);
		pthread_mutex_destroy(&m_mutex);
	}
private:
	pthread_cond_t  m_cond;
	pthread_mutex_t m_mutex;
};

#define THREAD_HANDLER_DECL(function,param) void* function(void *param)
#define CREATE_SUSPENDED  THR_SUSPENDED
typedef void * (*THREADFUNC)(void*);  

class CTThread 
{
public:
	CTThread(THREADFUNC entryAddress, 
		void *thrdParam,
		long creationFlag = PTHREAD_CREATE_DETACHED,
		void *stackAddr = NULL, 
		size_t stackSize = 0, 
		const char *thrName = NULL);
	~CTThread() { Terminate(); }

	bool	  IsValid()  { return m_thread != INVALID_HANDLE_VALUE; }
	pthread_t GetRealId(){ return m_thread; }
	pthread_t GetID()	 { return m_thread; }

	bool	  GetExitCode(unsigned long &exitCode);
	bool	  Terminate		(int exitCode=0);
	bool	  SetPriority	(int newPri);
	int		  GetPriority	(void);

private:
	pthread_t      m_thread;
	THREADFUNC	   m_userEntryAddress;
	void *		   m_userParam;
	pthread_attr_t m_threadAttr;
	CTEvent		   m_terminatedEvent;

	friend	THREAD_HANDLER_DECL(WrappedEntryAddress, param);
	friend	void threadCleanUp(void *param);
};

}
#endif // !WIN32


#endif // !__RESOURCEBASE_H__
