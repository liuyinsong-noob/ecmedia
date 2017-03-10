#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>
template <typename T>
class SharedQueue {
public:
	SharedQueue();
	~SharedQueue();

	T& front();
	void pop_front();
	void push_back(const T& item);
	int size();
	bool empty();
private:
	std::queue<T> queue_;
	std::mutex mutex_;
	std::condition_variable cond_;
};

template <typename T>
SharedQueue<T>::SharedQueue() {}

template <typename T>
SharedQueue<T>::~SharedQueue() {}

template <typename T>
T& SharedQueue<T>::front()
{
	std::unique_lock<std::mutex> lock(mutex_);
// 	std::chrono::microseconds timeout(20);
// 	if (queue_.empty())
// 	{
// 		if (cond_.wait_for(lock, timeout) == std::_Cv_status::no_timeout)
// 		{
// 			if (queue_.empty())
// 				return NULL;
// 		}
// 	}	
// 	if (queue_.empty())
// 		return NULL;
	T& val = queue_.front();
	return val;
}

template <typename T>
void SharedQueue<T>::pop_front()
{
	std::unique_lock<std::mutex> lock(mutex_);
	queue_.pop();
}

template <typename T>
void SharedQueue<T>::push_back(const T& item)
{
	std::unique_lock<std::mutex> lock(mutex_);
	queue_.push(item);
	lock.unlock();
	cond_.notify_one();
}

template <typename T>
int SharedQueue<T>::size()
{
	std::unique_lock<std::mutex> lock(mutex_);
	int size = queue_.size();
	lock.unlock();
	return size;
}

template <typename T>
bool SharedQueue<T>::empty()
{
	std::unique_lock<std::mutex> lock(mutex_);
	bool val = queue_.empty();
	lock.unlock();
	return val;
}
