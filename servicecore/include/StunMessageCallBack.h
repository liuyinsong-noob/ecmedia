//
//  StunMessageCallback.h
//  servicecore
//
//  Created by Lee Sean on 13-7-1.
//
//

#ifndef servicecore_StunMessageCallback_h
#define servicecore_StunMessageCallback_h
namespace  cloopenwebrtc
{
    class ServiceCoreCallBack {
    public:
        virtual  void onStunPacket(const char* call_id, void*data,int len,const char *fromIP ,int fromPort, bool isRTCP = 0, bool isVideo = 0) = 0;
        virtual void onAudioData(const char *call_id, const void *inData, int inLen, void *outData, int &outLen, bool send) = 0;
        virtual void onOriginalAudioData(const char *call_id, const void *inData, int inLen, int sampleRate, int numChannels, bool send) = 0;
        virtual void onVideoConference(int channelID, int status, int payload) = 0;
        virtual void onDtmf(const char *callid, char dtmf) = 0;
        virtual void onStopPlayPreRing() = 0;
		virtual void onReceiverStats(const char *callid, const int framerate, const int bitrate)=0; //获取视频解码统计信息
    };
}
#endif
