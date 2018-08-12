#include <sstream>
#include "video_send_stream.h"

namespace yuntongxunwebrtc {

std::string VideoSendStream::Config::EncoderSettings::ToString()
{
	std::stringstream ss;
	std::stringstream ssub;
	for (int i = 0; i < numberOfSimulcastStreams; i++)
	{
		ssub << "{simulcast " << i;
		ssub << simulcastStream[i].ToString();
		ssub << '}';
	}
	int fr = maxFramerate;
	int num = numberOfSimulcastStreams;
	//*ss << ",payload_name:" << payload_name; TODO: influxdb need to change
	ss << ",payload_type:" << payload_type;
	ss << ",width:" << width;
	ss << ",height:" << height;
	ss << ",framerate:" << (int)maxFramerate;
	ss << ",start_bitrate:" << startBitrate;
	ss << ",max_bitrate:" << maxBitrate;
	ss << ",min_bitrate:" << minBitrate;
	ss << ",target_bitrate:" << targetBitrate;
	ss << ",simulcast_num:" << (int)numberOfSimulcastStreams;
	ss << ssub.str();
	return ss.str();
}

std::string VideoSendStream::Config::ToString()
{
	return encoder_settings.ToString();
}
}
