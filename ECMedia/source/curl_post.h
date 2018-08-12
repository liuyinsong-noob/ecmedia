#pragma once

#include <string>
#include "../system_wrappers/include/shared_queue.h"
#include "../system_wrappers/include/thread_wrapper.h"
#include "../system_wrappers/include/clock.h"

#include <memory>
#include <cms/Connection.h>
#include <cms/Session.h>
#include <cms/Destination.h>
#include <cms/ExceptionListener.h>
#include <cms/CMSException.h>

using namespace yuntongxunwebrtc;

class CurlPost {
public:
	explicit CurlPost(std::string brokerUri, std::string username, std::string password);
	~CurlPost();

	void AddMessage(int report_type, std::string &message);
	static bool CurlPostThreadFunction(void* obj);
	bool CurlPostProcess();
private:
	SharedQueue<std::string> post_queue_;
	ThreadWrapper &post_thread_;

	uint32_t push_count;
	uint32_t pop_count;

	Clock*		clock_;
	int64_t		prev_time;	
private:
	cms::Connection* connection;
	cms::Session* session;
	cms::Destination* destination;
	cms::MessageProducer* producer;

	std::string brokerURI_;
	std::string username_;
	std::string password_;
	std::string topicName_;
};
