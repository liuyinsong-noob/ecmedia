#ifndef 	QUEUE_H
#define 	QUEUE_H


#include <iostream>
#include <stdlib.h>
#include <stdio.h>

#include <sys/types.h>

#ifndef WIN32 
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <assert.h>
#endif

#include "ResourceBase.h"
namespace kernel
{

const int DEFAULTQUEUESIZE = 40960 ;

template<class T>
class BaseQueue {
public: 
	BaseQueue(int size = DEFAULTQUEUESIZE);
	~BaseQueue();

public:
  int 			TryPut(T & newData);
  int 			Put(T & newData);
  int 			TryGet(T & buf);
  void 			Get(T & buf);
  
  int			full();
  bool			isEmpty();
  int 			listSize();
  void 			clear();
                                
private:

	CTMutex  mutex;
	CTSemaphore fullSemaphore;
	CTSemaphore emptySemaphore;	/*space*/
	int spos;		/*保存队列下一空位置的下标值*/
	int rpos;		/*保存队列下次被取项的下标*/
	T **KeyQueue;
	int QueueSize;
};

template<class T>
BaseQueue<T>::BaseQueue(int size)
	:fullSemaphore(CTSemaphore(0,DEFAULTQUEUESIZE)),
	emptySemaphore(CTSemaphore(DEFAULTQUEUESIZE,DEFAULTQUEUESIZE))
{
	rpos = 0;
	spos = 0;
	QueueSize = size;

	KeyQueue = (T **)malloc(sizeof(T *)*QueueSize);
	assert(KeyQueue != NULL);

}

template<class T>
BaseQueue<T>::~BaseQueue()
{
	if(KeyQueue)
	{
		clear();
		free (KeyQueue);
	}
}


template<class T>
int BaseQueue<T>::
TryPut(T & newData)
{
	if( emptySemaphore.IsLocked() )
		return -1;

	mutex.Lock();
	T * tmp = new T;
	*tmp = newData;
	KeyQueue[spos] = tmp;
	
	spos = (spos + 1)%QueueSize;
	mutex.Unlock();
	fullSemaphore.Post();
	return 0;
}

template<class T>
int BaseQueue<T>::
Put(T & newData)
{
	emptySemaphore.Wait();
	mutex.Lock();

	T * tmp = new T;
	*tmp = newData;
	KeyQueue[spos] = tmp;
	
	spos = (spos + 1)%QueueSize;
	mutex.Unlock();
	fullSemaphore.Post();

	return 0;
}

template<class T>
int BaseQueue<T>::
TryGet(T & buf)
{
	if( fullSemaphore.IsLocked())
		return -1;
	mutex.Lock();
	buf = *KeyQueue[rpos];
	delete 	KeyQueue[rpos];
	KeyQueue[rpos] = NULL;
	
	rpos = (rpos +1 )%QueueSize ;

	mutex.Unlock();
	emptySemaphore.Post();

	return 0;
}

template<class T>
void BaseQueue<T>::
Get(T & buf)
{
	fullSemaphore.Wait();
	mutex.Lock();	
	buf = *KeyQueue[rpos];
	delete 	KeyQueue[rpos];
	KeyQueue[rpos] = NULL;
	
	rpos = (rpos +1 )%QueueSize ;
	mutex.Unlock();
	emptySemaphore.Post();
}

template<class T>
void BaseQueue<T>::
clear( )
{
	int i;
	
	mutex.Lock();
	if(rpos < spos)
	{
		for(i=rpos; i<spos; i++)
		{
			if(KeyQueue[i])
				delete KeyQueue[i];
		}		
	}
	if(rpos > spos)
	{
		for(i=rpos; i<QueueSize; i++)
		{
			if(KeyQueue[i])
				delete KeyQueue[i];
		}		
		for(i=0; i<spos; i++)
		{
			if(KeyQueue[i])
				delete KeyQueue[i];
		}		
	}
	
	rpos = 0;
	spos = 0;
	mutex.Unlock();
	
}

template<class T>
int	BaseQueue<T>::
full()
{
	if((spos + 1) % QueueSize == rpos)
		return true;
	else
		return false;
}

template<class T>
bool	BaseQueue<T>::
isEmpty()
{
	if(rpos == spos )
		return true;
	else
		return false;
}

template<class T>
int	BaseQueue<T>::
listSize()
{
	//cerr << "BaseQueue: rpos=[" << rpos << "]," << "spos=[" << spos << "]" << endl;	
	if(rpos<=spos)
		return spos-rpos;
	else
		return QueueSize-rpos+spos;
}
}

#endif
