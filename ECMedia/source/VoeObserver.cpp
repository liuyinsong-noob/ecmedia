#include "VoeObserver.h"
#include "utility.h"
#include "voe_errors.h"


void VoeObserver::CallbackOnError(const int channel, const int errCode)
{
 //   std::map<int, onVoeCallbackOnError>::iterator itt = _channelCallbackMap.find(channel);
	//if (itt != _channelCallbackMap.end()) {
	//	onVoeCallbackOnError callback = itt->second;
	//	if (callback)
	//		callback(channel, errCode);
	//}
	if (_voeCallback) {
		_voeCallback(_channelID, errCode);
	}
}

void VoeObserver::SetCallback(int channel, onVoeCallbackOnError callback)
{
	//std::map<int, onVoeCallbackOnError>::iterator itt = _channelCallbackMap.find(channel);
	//if (itt != _channelCallbackMap.end()) {
	//	_channelCallbackMap[channel] = callback;
	//}
	//else {
	//	_channelCallbackMap.insert(std::pair <int, onVoeCallbackOnError>(channel, callback));
	//}
	_voeCallback = callback;
	_channelID = channel;

}


