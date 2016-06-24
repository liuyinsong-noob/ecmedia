//
//  callback.c
//  androidJNI
//
//  Created by gyf on 12-8-21.
//  Copyright (c) 2012年 G4Next. All rights reserved.
//

#include "callback.h"
#include <android/log.h>
#include <string.h>
#include <stdio.h>
#include "cJSON.h"
#include "base64.h"

static JavaVM* _javaVM = NULL;

static char* _methodName = NULL;
static char* _methodSig = NULL;
static jobject _obj = NULL;

static char* _bytesMethodName = NULL;
static char* _bytesMethodSig = NULL;
static jobject _bytesObj = NULL;

#define MAX_CAPABILITY_TOKEN_SIZE       256

static char _capabilityToken[MAX_CAPABILITY_TOKEN_SIZE + 1];

bool createCBEnv(CBEnv* env)
{
    if(!_javaVM) {
    		__android_log_print(ANDROID_LOG_ERROR,"JNI", "NO JavaVM ");
        return false;
     }
    JNIEnv* temp = NULL;
    env->_attached = false;
    if(_javaVM->GetEnv((void**)&temp, JNI_VERSION_1_4) != JNI_OK)
    {
        jint res = _javaVM->AttachCurrentThread(&temp, NULL);
        if ((res < 0) || !env)
        {
            __android_log_print(ANDROID_LOG_ERROR,"JNI", "create env failed");
            return false;
        }
        env->_attached = true;
    }
    env->_env = temp;
    return temp != NULL;
}

void releaseCBEnv(CBEnv* env)
{
    if(!env->_attached || !_javaVM)
        return;
    _javaVM->DetachCurrentThread();
}

void setJavaVM(JavaVM* javaVM)
{
    _javaVM = javaVM;
}

void setBytesCallBackParams(jobject obj, const char* methodName, const char* methodSig)
{    
    _bytesObj = obj;
    
    int size = strlen(methodName);
    _bytesMethodName = (char*)malloc(size + 1);
    memcpy(_bytesMethodName, methodName, size);
    _bytesMethodName[size] = '\0';
    
    size = strlen(methodSig);
    _bytesMethodSig = (char*)malloc(size + 1);
    memcpy(_bytesMethodSig, methodSig, size);
    _bytesMethodSig[size] = '\0';
    
}

bool bytesCallbackValid()
{
    return _javaVM && _bytesObj && _bytesMethodName && _bytesMethodSig;
}

void bytesRecycle()
{
    CBEnv env;
    if(_obj && createCBEnv(&env))
    {
        env._env->DeleteGlobalRef(_bytesObj);
        releaseCBEnv(&env);
    }
    if(_bytesMethodName)
        free(_bytesMethodName);
    if(_bytesMethodSig)
        free(_bytesMethodSig);
    _bytesMethodSig = NULL;
    _bytesMethodName = NULL;
    _bytesObj = NULL;
}

void bytesCallback(int event, const char*message, const unsigned char*data, int size)
{
    if(!_bytesObj) {
        __android_log_print(ANDROID_LOG_ERROR,"JNI", "bytesCallback NO Java Object Set ");
        return;
    }
    if(!bytesCallbackValid()) {
        __android_log_print(ANDROID_LOG_ERROR,"JNI", "bytesCallback env is invalid");
        return;
    }
    CBEnv env;
    if(!createCBEnv(&env)) {
        __android_log_print(ANDROID_LOG_ERROR,"JNI", "Can not Create bytesCallback environment");
        return;
    }
    
    jclass cls = env._env->GetObjectClass(_bytesObj);
    
    jmethodID javaCallBack = env._env->GetMethodID(cls, _bytesMethodName, _bytesMethodSig);
    jstring javaMessage = env._env->NewStringUTF(message);
    jbyteArray javaData = env._env->NewByteArray(size);
    env._env->SetByteArrayRegion(javaData, 0, size, (jbyte*)data);   
    env._env->CallVoidMethod(_bytesObj, javaCallBack, event, javaMessage, javaData, size);
    
    env._env->DeleteLocalRef(javaMessage);
    env._env->DeleteLocalRef(javaData);
    env._env->DeleteLocalRef(cls);
    
    releaseCBEnv(&env);
}


void setCallBackParams(jobject obj, const char* methodName, const char* methodSig)
{
    _obj = obj;
    int size = strlen(methodName);
    
    _methodName = (char*)malloc(size + 1);
    memcpy(_methodName, methodName, size);
    _methodName[size] = '\0';
        
    size = strlen(methodSig);
    _methodSig = (char*)malloc(size + 1);
    
    memcpy(_methodSig, methodSig, size);
    _methodSig[size] = '\0';
}

bool callbackValid()
{
    return _javaVM && _obj && _methodName && _methodSig;
}

void recycle()
{
    CBEnv env;
    if(_obj && createCBEnv(&env))
    {
        env._env->DeleteGlobalRef(_obj);
        releaseCBEnv(&env);
    }
    if(_methodName)
        free(_methodName);
    if(_methodSig)
        free(_methodSig);
    _methodSig = NULL;
    _methodName = NULL;
    _obj = NULL;
    // Don't clean _javaVM because so only load once to get _javaVM
    //_javaVM = NULL
}

jobject callback(int event, const char* id, const unsigned char* message, int state, int messageLen)
{
    if(!_obj) {
    		__android_log_print(ANDROID_LOG_ERROR,"JNI", "NO Java Object Set \n");
        return NULL;
      }
    if(!callbackValid()) {
    		__android_log_print(ANDROID_LOG_ERROR,"JNI", "Callback env is invalid\n");
    	  return NULL;
    }
    CBEnv env;
    if(!createCBEnv(&env)) {
    		__android_log_print(ANDROID_LOG_ERROR,"JNI", "Can not Create Callback environment\n");
        return NULL;
    }

    jclass cls = env._env->GetObjectClass(_obj);
    jmethodID javaCallBack = env._env->GetMethodID(cls, _methodName, _methodSig);
    jstring javaId = env._env->NewStringUTF(id);
    jobject callbackObj = NULL;
    jobject returnObj = NULL;
    if (NULL != message) {
        jbyteArray javaArraryMessage = env._env->NewByteArray(messageLen);
        env._env->SetByteArrayRegion(javaArraryMessage,0,messageLen,(jbyte *)message);
        callbackObj = env._env->CallObjectMethod(_obj, javaCallBack, event, javaId, javaArraryMessage, state);
        env._env->DeleteLocalRef(javaArraryMessage);
    }
    else
    {
        callbackObj = env._env->CallObjectMethod(_obj, javaCallBack, event, javaId, NULL, state);
    }
    env._env->DeleteLocalRef(javaId);
    env._env->DeleteLocalRef(cls);
    if (callbackObj) { //Currently, only onAudioData return jobject and only onAudioData use it, so on ly onAudioData need to delete returnObj.
        returnObj = env._env->NewGlobalRef(callbackObj);
        env._env->DeleteLocalRef(callbackObj);
    }
    releaseCBEnv(&env);
    return returnObj;

}

void onConnected()
{
    callback(EVENT_CONNECT, NULL, NULL, 0);
}

void onConnectError(int reason)
{
    callback(EVENT_CONNECT, NULL, NULL, reason);
}

void onDisconnect()
{
    callback(EVENT_DISCONNECT, NULL, NULL, 0);
}

void onIncomingCallReceived(int callType, const char*callid, const char*caller)
{
    if(callType == 0)
        callback(EVENT_CALL, callid, (const unsigned char *)caller, CALL_STATE_CALL_INCOMING, strlen(caller));
    else
        callback(EVENT_CALL, callid, (const unsigned char *)caller, CALL_STATE_CALL_VIDEO,strlen(caller));
}
void onCallProceeding(const char*callid)
{
		callback(EVENT_CALL, callid, NULL, CALL_STATE_CALL_PROCEEDING);
}

void onCallAlerting(const char*callid)
{
    callback(EVENT_CALL, callid, NULL, CALL_STATE_CALL_ALERTING);
}

void onCallAnswered	(const char *callid)
{
    callback(EVENT_CALL, callid, NULL, CALL_STATE_CALL_ANSWERED);
}

void onMakeCallFailed(const char *callid, int reason)
{
    char reasonStr[32];
    sprintf(reasonStr, "%d", reason);
    callback(EVENT_CALL, callid, (const unsigned char *)reasonStr, CALL_STATE_MAKE_CALL_FAILED, strlen(reasonStr));
}

void onCallPaused(const char* callid)
{
    callback(EVENT_CALL, callid, NULL, CALL_STATE_CALL_PAUSED);
}

void onCallPausedByRemote(const char *callid)
{
    callback(EVENT_CALL, callid, NULL, CALL_STATE_CALL_REMOTE_PAUSED);
}

void onCallReleased(const char *callid)
{
    callback(EVENT_CALL, callid, NULL, CALL_STATE_CALL_RELEASED);
}

void onCallTransfered(const char *callid, const char *destionation)
{
    callback(EVENT_CALL, callid, (const unsigned char *)destionation, CALL_STATE_CALL_TRANSFERRED, strlen(destionation));
}

void onDtmfReceived(const char* callid, char dtmf)
{
    callback(EVENT_DTMF_RECEIVED, callid, NULL, dtmf);
}

void onTextMessageReceived(const char* sender, const char* receiver, const char *sendtime, const char* msgid, const char *message, const char *userdata)
{
    char temp[256];
    sprintf(temp, "<sender>%s</sender><receiver>%s</receiver><time>%s</time><msgid>%s</msgid>", sender, receiver, sendtime, msgid);
//    char comMsg[2048];
    char *comMsg = NULL;
    int mallocLen = 50;
    if (message) {
        mallocLen += strlen(message);
    }
    if (userdata) {
        mallocLen += strlen(userdata);
    }
    comMsg = (char *)malloc(mallocLen);
    if (!comMsg) {
        __android_log_print(ANDROID_LOG_ERROR,"JNI", "ERROR: onTextMessageReceived: Cannot alloc memory failed\n");
    }
    memset(comMsg,0, mallocLen);
    sprintf(comMsg, "<message>%s</message><userdata>%s</userdata>", message, userdata);
    callback(EVENT_MESSAGE, temp, (const unsigned char *)comMsg, 0, strlen(comMsg));
    free(comMsg);
}
//void onGroupTextMessageReceived(const char* sender, const char* groupid, const char* message)
//{
//    char temp[256];
//    sprintf(temp, "<sender>%s</sender><groupid>%s</groupid>", sender, groupid);
//    callback(EVENT_GROUP_MESSAGE, temp, message, 0);
//}
void onMessageSendReport(const char *msgid, const char *time, int status)
{
    if (time) {
        callback(EVENT_MESSAGE_REPORT, msgid, (const unsigned char *)time, status, strlen(time));
    }
    else
    {
        callback(EVENT_MESSAGE_REPORT, msgid, NULL, status, 0);
    }
	  
}

void onGetCapabilityToken()
{
    callback(EVENT_GET_CAPABILITY_TOKEN, NULL, NULL, 0);
}

void onLogInfo(const char* loginfo)
{
	  __android_log_print(ANDROID_LOG_INFO,"console", "%s\n", loginfo);
//    callback(EVENT_LOG_INFO, NULL, (const unsigned char *)loginfo, 0, strlen(loginfo));
}

void onGeneralEvent(const char* callid, int eventid, const char* data, int flag)
{
    char temp[2048] = {0};
    if (NULL == data) {
        __android_log_print(ANDROID_LOG_ERROR,"JNI", "ERROR: onGeneralEvent: Message is NULL\n");
        return;
    }
    sprintf(temp, "%d:%s", flag, data);
    callback(EVENT_GENERAL, callid, (const unsigned char *)temp, eventid, strlen(temp));
}
void onCallMediaUpdateRequest(const char* callid, int request)
{
    char requestStr[32];
    sprintf(requestStr, "%d", request);
	callback(EVENT_CALL,callid,(const unsigned char *)requestStr,CALL_STATE_CALL_VIDEO_UPDATE_REQUEST, strlen(requestStr));
}
void onCallMediaUpdateResponse(const char* callid, int respone)
{
    char responeStr[32];
    sprintf(responeStr, "%d", respone);
	callback(EVENT_CALL,callid,(const unsigned char *)responeStr,CALL_STATE_CALL_VIDEO_UPDATE_RESPONSE, strlen(responeStr));
}

void onSucceedGetWidthHeight(const char* callid, const char *ratio)
{

	callback(EVENT_CALL,callid,(const unsigned char *)ratio,CALL_STATE_CALL_VIDEO_WIDTH_HEIGHT, strlen(ratio));
}

void onDeliverVideoFrame(const char * callid, unsigned char *buf, int size, int width, int height)
{  
    char temp[128];
    sprintf(temp, "<callid>%s</callid><width>%d</width><height>%d</height>", callid, width, height);
    bytesCallback(EVENT_DELIVER_VIDEO_FRAME, temp, (const unsigned char *)buf, size);
}
void onRecordVoiceStatus(const char *callid, const char *fileName, int status)
{
    callback(EVENT_RECORD_CALL_VOICE, callid, (const unsigned char *)fileName, status, strlen(fileName));
}

void onAudioData(const char *callid, const void *inData, int inLen, void *outData, int &outLen, bool send)
{
    __android_log_print(ANDROID_LOG_ERROR,"JNI", "DEBUG: onAudioData: Original data length = %d!\n",inLen);
    jobject tempobj = callback(EVENT_AUDIO_DATA_PROCESS, callid, (const unsigned char*)inData, send, inLen);
    
    if (!tempobj) {
        __android_log_print(ANDROID_LOG_ERROR,"JNI", "ERROR: onAudioData: Back data is NULL!!!");
        return;
    }
    CBEnv env;
    if(!createCBEnv(&env)) {
        __android_log_print(ANDROID_LOG_ERROR,"JNI", "Can not Create Callback environment");
        return;
    }
    jbyteArray tempByteArray = (jbyteArray)tempobj;
    jbyte *tmpByte = (jbyte *)env._env->GetByteArrayElements(tempByteArray,0);
    if (!tmpByte) {
        __android_log_print(ANDROID_LOG_ERROR,"JNI", "ERROR: onAudioData: Cannot get data from bytearray!!!");
        return;
    }
    outLen = env._env->GetArrayLength(tempByteArray);
    
    if (outLen >= 1024*1024) {
        __android_log_print(ANDROID_LOG_ERROR,"JNI", "DEBUG: onAudioData: Data length is too big to deal! Length = %d\n",outLen);
    }
    else
    {
        __android_log_print(ANDROID_LOG_ERROR,"JNI", "DEBUG: onAudioData: Data length after deal = %d!\n",outLen);
        memcpy(outData,tmpByte,outLen);
    }
    env._env->ReleaseByteArrayElements(tempByteArray,tmpByte,0);
    env._env->DeleteGlobalRef(tempobj);
    releaseCBEnv(&env);

}

void onOriginalAudioData(const char *callid,const void *inData, int inLen, int sampleRate, int numChannels, const char *codec, bool send)
{
#if !defined(NO_VOIP_FUNCTION)
    __android_log_print(ANDROID_LOG_ERROR,"JNI", "DEBUG: onOriginalAudioData: Original data length = %d!send:%d\n",inLen,send);
    cJSON *head = cJSON_CreateObject();
    cJSON_AddNumberToObject(head,"sampleRate",sampleRate);
    cJSON_AddNumberToObject(head,"numChannels",numChannels);
    cJSON_AddStringToObject(head,"codec",codec);
    cJSON_AddBoolToObject(head,"isSend",send);
    
    int mallocLen = Base64encode_len(inLen);
    char *encoded = (char *)malloc(mallocLen);
    Base64encode(encoded,(const char *)inData,inLen);
    cJSON_AddStringToObject(head,"voiceData",encoded);
    free(encoded);
    const char *jsonStr = cJSON_PrintUnformatted(head);
    __android_log_print(ANDROID_LOG_ERROR,"JNI", "DEBUG: %s!\n",jsonStr);
    callback(EVENT_ORIGINAL_AUDIO_DATA_PROCESS, callid, (const unsigned char*)inData, 0, inLen);
    cJSON_Delete(head);
#endif
}

void onRequestSpecifiedVideoFailed(const char *callid, const char *sip, int reason)
{
    __android_log_print(ANDROID_LOG_ERROR,"JNI", "DEBUG: onRequestSpecifiedVideoResult: callid = %s, request sip = %s, result = %d\n",callid, sip, reason);
    callback(EVENT_REQUEST_VIDEO_RESULT, callid, (const unsigned char*)sip, reason, strlen(sip));
}

void onStopSpecifiedVideoResponse(const char *callid, const char *sip, int response, void *window)
{
    __android_log_print(ANDROID_LOG_ERROR,"JNI", "DEBUG: onRequestSpecifiedVideoResult: callid = %s, request sip = %s, response = %d\n",callid, sip, response);
    callback(EVENT_STOP_VIDEO_RESPONSE, callid, (const unsigned char*)sip, response, strlen(sip));
}

void onEnableSrtp(const char * sip, bool isCaller)
{
    if (!sip) {
        __android_log_print(ANDROID_LOG_ERROR,"JNI", "ERROR: onEnableSrtp sip is NULL, check it!\n");
        return;
    }
    __android_log_print(ANDROID_LOG_ERROR,"JNI", "DEBUG: onEnableSrtp sip is %s.\n",sip);
    callback(EVENT_ENABLE_SRTP, sip, NULL, isCaller);
}

void onRemoteVideoRatioChanged(const char *callid, int width, int height, bool isVideoConference, const char *sip)
{
    if ((isVideoConference && !sip) || !callid) {
        __android_log_print(ANDROID_LOG_ERROR,"JNI", "ERROR: onRemoteVideoRatioChanged sip or callid is NULL, check it!\n");
        return;
    }
    char ratio[30] = {0};
    sprintf(ratio,"%dx%d",width,height);
    if (isVideoConference) {
        __android_log_print(ANDROID_LOG_ERROR,"JNI", "DEBUG: onRemoteVideoRatioChanged sip no is %s, ratio is %s.\n",sip,ratio);
        callback(EVENT_STATE_CALL_VIDEO_RATIO, sip, (const unsigned char*)ratio, isVideoConference, strlen(ratio));
    }
    else
    {
        __android_log_print(ANDROID_LOG_ERROR,"JNI", "DEBUG: onRemoteVideoRatioChanged callid is %s, ratio is %s.\n",callid,ratio);
        callback(EVENT_STATE_CALL_VIDEO_RATIO, callid, (const unsigned char*)ratio, isVideoConference,strlen(ratio));
    }
    
}

void onLogOut()
{
    __android_log_print(ANDROID_LOG_ERROR,"JNI", "DEBUG: onLogOut called");
    callback(EVENT_LOGOUT, NULL, NULL, 0);
}

void onReinit()
{
    __android_log_print(ANDROID_LOG_ERROR,"JNI", "DEBUG: onReinit called");
    callback(EVENT_REINIT, NULL, NULL, 0);
}
