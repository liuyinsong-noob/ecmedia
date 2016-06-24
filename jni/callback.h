//
//  callback.h
//  androidJNI
//
//  Created by gyf on 12-8-21.
//  Copyright (c) 2012年 G4Next. All rights reserved.
//

#ifndef androidJNI_callback_h
#define androidJNI_callback_h

#include <jni.h>


const int EVENT_MESSAGE			= 0x00;
const int EVENT_CONNECT         = 0x01;
const int EVENT_DISCONNECT      = 0x02;
const int EVENT_CALL				= 0x03;
const int EVENT_DTMF_RECEIVED	= 0x04;
const int EVENT_GET_CAPABILITY_TOKEN	= 0x05;
const int EVENT_LOG_INFO				= 0x06;
const int EVENT_MESSAGE_REPORT  = 0x07;
const int EVENT_GENERAL         = 0x08;
const int EVENT_GROUP_MESSAGE = 0x09;
const int EVENT_DELIVER_VIDEO_FRAME = 0x0a;
const int EVENT_RECORD_CALL_VOICE = 0x0b;
const int EVENT_AUDIO_DATA_PROCESS = 0x0c;
const int EVENT_ORIGINAL_AUDIO_DATA_PROCESS = 0x0e;
const int EVENT_REQUEST_VIDEO_RESULT = 0x0f;
const int EVENT_ENABLE_SRTP = 0x10;
const int EVENT_STOP_VIDEO_RESPONSE = 0x11;
const int EVENT_STATE_CALL_VIDEO_RATIO = 0x12;
const int EVENT_LOGOUT          = 0x13;
const int EVENT_REINIT          = 0X14;

const int CALL_STATE_MAKE_CALL_FAILED	= 0x01;
const int CALL_STATE_CALL_INCOMING		= 0x02;
const int CALL_STATE_CALL_ALERTING		= 0x03;
const int CALL_STATE_CALL_ANSWERED		= 0x04;
const int CALL_STATE_CALL_PAUSED			= 0x05;
const int CALL_STATE_CALL_REMOTE_PAUSED	= 0x06;
const int CALL_STATE_CALL_RELEASED		= 0x07;
const int CALL_STATE_CALL_TRANSFERRED	= 0x08;
const int CALL_STATE_CALL_VIDEO         = 0x09;
const int CALL_STATE_CALL_PROCEEDING	= 0x0a;
const int CALL_STATE_CALL_VIDEO_UPDATE_REQUEST	= 0x0b;
const int CALL_STATE_CALL_VIDEO_UPDATE_RESPONSE	= 0x0c;
const int CALL_STATE_CALL_VIDEO_WIDTH_HEIGHT		= 0x0d;


const int GENERAL_EVENT_STATE_EARLYMEDIA = 0x00;
const int GENERAL_EVENT_STATE_MESSAGE_COMMAND = 0x01;
const int GENERAL_EVENT_STATE_REMOTE_VIDEO_RATIO = 0x02;
const int GENERAL_EVENT_STATE_MEDIA_INIT_FAILED = 0x03;
const int GENERAL_EVENT_STATE_AUDIO_DESTINATION_CHANGED = 0x04;
const int GENERAL_EVENT_STATE_VIDEO_DESTINATION_CHANGED = 0x05;

typedef struct
{
    JNIEnv* _env;
    bool _attached;
}CBEnv;

bool createCBEnv(CBEnv* env);
void releaseCBEnv(CBEnv* env);
void setJavaVM(JavaVM* javaVM);

//
void setBytesCallBackParams(jobject obj, const char* methodName, const char* methodSig);
bool bytesCallbackValid();
void bytesRecycle();
void bytesCallback(int event, const char*message, const unsigned char*data, int size);
//

void setCallBackParams(jobject obj, const char* methodName, const char* methodSig);
bool callbackValid();
void recycle();
jobject callback(int event, const char* id, const unsigned char* message, int state, int messageLen=0);

void onConnected();
void onConnectError(int reason);
void onDisconnect();
void onIncomingCallReceived(int callType, const char*callid, const char*caller);
void onCallProceeding(const char*callid);
void onCallAlerting(const char*callid);
void onCallAnswered	(const char *callid);
void onMakeCallFailed(const char *callid, int reason);
void onCallPaused(const char* callid);
void onCallPausedByRemote(const char *callid);
void onCallReleased(const char *callid);
void onCallTransfered(const char *callid, const char *destionation);
void onDtmfReceived(const char* callid, char dtmf);
void onTextMessageReceived(const char* sender, const char* receiver, const char *sendtime, const char *msgid, const char *message, const char *userdata);
void onMessageSendReport(const char *msgid, const char *time, int status);
void onGetCapabilityToken();
void onLogInfo(const char* loginfo);
void onGeneralEvent(const char* callid, int eventid, const char* data, int flag);
void onCallMediaUpdateRequest(const char* callid, int request);
void onCallMediaUpdateResponse(const char* callid, int respone);
void onSucceedGetWidthHeight(const char * callid, const char* ratio);
void onDeliverVideoFrame(const char * callid, unsigned char *buf, int size, int width, int height);
void onRecordVoiceStatus(const char *callid, const char *fileName, int status);
void onAudioData(const char *callid, const void *inData, int inLen, void *outData, int &outLen, bool send);
void onOriginalAudioData(const char *callid,const void *inData, int inLen, int sampleRate, int numChannels, const char *codec, bool send);
void onRequestSpecifiedVideoFailed(const char *callid, const char *sip, int reason);
void onEnableSrtp(const char *sip, bool isCaller);
void onStopSpecifiedVideoResponse(const char *callid, const char *sip, int response, void *window);
void onRemoteVideoRatioChanged(const char *callid, int width, int height, bool isVideoConference, const char *sipNo);//远端视频媒体分辨率变化时上报
void onLogOut();
void onReinit();
#endif
