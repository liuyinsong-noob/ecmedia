#include "curl_post.h"
#include <assert.h>
#include <algorithm>
#include "trace.h"

#include <cms/ConnectionFactory.h>
#include <activemq/library/ActiveMQCPP.h>

////////////////////////////////////////////////////////////////////////////////
const char* DEFAULT_BROKER_URI = "failover:(tcp://127.0.0.1:61616"")";
const char* DEFAULT_QUEUE_NAME = "user1009.stats.data";

CurlPost::CurlPost(std::string brokerUri, std::string username, std::string password)
	:brokerURI_(brokerUri),
	username_(username),
	password_(password),
	post_thread_(*ThreadWrapper::CreateThread(CurlPostThreadFunction,
		this, kHighPriority,
		"CurlPostThread")),
	clock_(Clock::GetRealTimeClock()),
	prev_time(-1),
	push_count(0),
	pop_count(0)
{
	activemq::library::ActiveMQCPP::initializeLibrary();

	// Create a connection.
	try {
		// Create a ConnectionFactory
		std::auto_ptr<cms::ConnectionFactory> connectionFactory(
			cms::ConnectionFactory::createCMSConnectionFactory(DEFAULT_BROKER_URI));
		// Create a Connection
		connection = connectionFactory->createConnection();
		connection->start();

		// Create a Session
		session = connection->createSession(cms::Session::AUTO_ACKNOWLEDGE);
		destination = session->createQueue("TEST.FOO");

		// Create a MessageProducer from the Session to the Topic or Queue
		producer = session->createProducer(destination);
		producer->setDeliveryMode(cms::DeliveryMode::NON_PERSISTENT);
	}
	catch (cms::CMSException& ex) {
		ex.printStackTrace();
		return;
	}	
	
	unsigned int t_id = 0;
	if (!post_thread_.Start(t_id)) {
		assert(false);
	}
}

CurlPost::~CurlPost()
{
	try {
		if (connection != NULL) {
			this->connection->close();
		}
	}
	catch (cms::CMSException& ex) {
		ex.printStackTrace();
	}

	// Destroy resources.
	try {
		delete destination;
		destination = NULL;
		delete producer;
		producer = NULL;
		delete session;
		session = NULL;
		delete connection;
		connection = NULL;
	}
	catch (cms::CMSException& e) {
		e.printStackTrace();
	}
	activemq::library::ActiveMQCPP::shutdownLibrary();

	post_thread_.SetNotAlive();
	
	if (post_thread_.Stop())
	{
		delete &post_thread_;
	}
 	else
 		assert(false);

// 	SharedQueue<std::string> empty;
// 	std::swap(post_queue_, empty);

//	post_queue_ = SharedQueue<std::string>();
}

void CurlPost::AddMessage(int report_type, std::string &message)
{
	std::stringstream ss;
	ss << "body=accountId:1009";
	switch (report_type)
	{
	case 0:
		ss << ",report_type:audio_send_statistics";
		break;
	case 1:
		ss << ",report_type:audio_receive_statistics";
		break;
	case 2:
		ss << ",report_type:video_send_statistics";
		break;
	case 3:
		ss << ",report_type:video_receive_statistics";
		break;
	case 4:
		ss << ",report_type:video_codec_setting";
		break;
	default:
		break;
	}
	ss << message;
	
	post_queue_.push_back(ss.str());

	push_count++;
	if (prev_time == -1)
	{
		prev_time = clock_->TimeInMilliseconds();
	}
}

bool CurlPost::CurlPostThreadFunction(void* obj)
{
	return static_cast<CurlPost*>(obj)->CurlPostProcess();
}

bool CurlPost::CurlPostProcess()
{
	if (post_queue_.empty())
	{
		return true;
	}
	std::string message = post_queue_.front();

	try {
		std::auto_ptr<cms::TextMessage> message(
			this->session->createTextMessage(message));
		this->producer->send(message.get());

	}
	catch (cms::CMSException& ex) {
		exit(1);
	}

	post_queue_.pop_front();
	pop_count++;

	int64_t now_time = clock_->TimeInMilliseconds();
	int64_t time_diff = now_time - prev_time;
	const int64_t kUpdateIntervalMs = 1000;
	if ((time_diff > kUpdateIntervalMs) && (prev_time!=-1))
	{
		WEBRTC_TRACE(kTraceError, kTraceUtility, 0,
			"push_count=%d ", push_count);
		WEBRTC_TRACE(kTraceError, kTraceUtility, 0,
			"pop_count=%d ", pop_count);
		prev_time = now_time;

		WEBRTC_TRACE(kTraceError, kTraceUtility, 0,
			"queue_size=%d ", post_queue_.size());
		prev_time = now_time;
		push_count = 0;
		pop_count = 0;
	}
	return true;
}

