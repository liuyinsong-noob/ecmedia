/**
 实现的功能：
	通话过程中，超时收不到音频rtp包就挂机。 超时时间 AUDIO_RTP_PACKET_TIMEOUT
 **/

#ifndef VOE_OBSERVER_H
#define VOE_OBSERVER_H

#include "voe_base.h"
#include "../system_wrappers/include/event_wrapper.h"
#include "../system_wrappers/include/thread_wrapper.h"
#include <stdio.h>

using namespace yuntongxunwebrtc;

typedef int (*onVoeCallbackOnError)(int channelid, int errCode);

class VoeObserver:public VoiceEngineObserver
{
public:	
    //implement VoiceEngineObserver
    virtual void CallbackOnError(const int channel, const int errCode);

    void SetCallback(int channel, onVoeCallbackOnError callback);
    
private:
    //std::map<int, onVoeCallbackOnError> _channelCallbackMap;
	int _channelID;
	onVoeCallbackOnError _voeCallback;
};

#endif