#include "com_CCP_phone_NativeInterface.h"
#include <android/log.h>
#include <stdio.h>
#include "callback.h"
#include "CCPClient.h"
#include "CCPClient_internal.h"
#include <string.h>
#include "jni.h"
#include "ECMedia.h"

JNIEXPORT jint JNI_OnLoad(JavaVM* vm, void* reserved)
{
    setJavaVM(vm);
    return JNI_VERSION_1_4;
}

JNIEXPORT void JNI_OnUnload(JavaVM* vm, void* reserved)
{
    recycle();
    bytesRecycle();
    unInitialize();
}

JNIEXPORT jint JNICALL Java_com_CCP_phone_NativeInterface_initialize
(JNIEnv * env, jclass cls)
{
    CCallbackInterface callBackInterface;
    memset(&callBackInterface, 0, sizeof(CCallbackInterface));
    callBackInterface.onConnected = onConnected;
    callBackInterface.onConnectError = onConnectError;
    callBackInterface.onIncomingCallReceived = onIncomingCallReceived;
    callBackInterface.onCallProceeding = onCallProceeding;
    callBackInterface.onCallAlerting = onCallAlerting;
    callBackInterface.onCallAnswered = onCallAnswered;
    callBackInterface.onMakeCallFailed = onMakeCallFailed;
    callBackInterface.onCallPaused = onCallPaused;
    callBackInterface.onCallPausedByRemote = onCallPausedByRemote;
    callBackInterface.onCallReleased = onCallReleased;
    callBackInterface.onCallTransfered = onCallTransfered;
    callBackInterface.onDtmfReceived = onDtmfReceived;
    callBackInterface.onTextMessageReceived = onTextMessageReceived;
//    callBackInterface.onGroupTextMessageReceived = onGroupTextMessageReceived;
    callBackInterface.onGetCapabilityToken = onGetCapabilityToken;
    callBackInterface.onLogInfo = onLogInfo;
//    callBackInterface.onDisconnect = onDisconnect;
    callBackInterface.onMessageSendReport = onMessageSendReport;
    callBackInterface.onNotifyGeneralEvent = onGeneralEvent;
    callBackInterface.onCallMediaUpdateRequest = onCallMediaUpdateRequest;
    callBackInterface.onCallMediaUpdateResponse = onCallMediaUpdateResponse;
    callBackInterface.onDeliverVideoFrame = onDeliverVideoFrame;
    callBackInterface.onRecordVoiceStatus = onRecordVoiceStatus;
    callBackInterface.onAudioData=onAudioData;
    callBackInterface.onOriginalAudioData=onOriginalAudioData;
    callBackInterface.onRequestSpecifiedVideoFailed=onRequestSpecifiedVideoFailed;
    callBackInterface.onEnableSrtp = onEnableSrtp;
    callBackInterface.onRemoteVideoRatioChanged = onRemoteVideoRatioChanged;
    callBackInterface.onLogOut = onLogOut;
    callBackInterface.oneXosipThreadStop = onReinit;
    return initialize(&callBackInterface);

}

JNIEXPORT jint JNICALL Java_com_CCP_phone_NativeInterface_setAudioContext
(JNIEnv * env, jclass cls, jobject c)
{
    JavaVM* vm;
    env->GetJavaVM(&vm);
    jobject t = env->NewGlobalRef(c);
    return setAndroidObjects((void*)vm, (void*)env, (void*)t);
}

JNIEXPORT jint JNICALL Java_com_CCP_phone_NativeInterface_unInitialize
(JNIEnv * env, jclass cls)
{
    recycle();
    bytesRecycle();
    return unInitialize();
}


JNIEXPORT jint JNICALL Java_com_CCP_phone_NativeInterface_connectToCCP
(JNIEnv * env, jclass cls, jstring proxy_addr, jint proxy_port, jstring account, jstring password,jstring capability_token)
{
    if (NULL == proxy_addr || NULL == account || NULL == password || NULL == capability_token) {
        return -1;
    }
    const char* cproxy_addr = env->GetStringUTFChars(proxy_addr, 0);
    const char* caccount = env->GetStringUTFChars(account, 0);
    const char* cpassword = env->GetStringUTFChars(password, 0);
    const char* ctoken = env->GetStringUTFChars(capability_token, 0);
    int ret = connectToCCP(cproxy_addr, proxy_port, caccount, cpassword, ctoken);

    env->ReleaseStringUTFChars(proxy_addr, cproxy_addr);
    env->ReleaseStringUTFChars(account, caccount);
    env->ReleaseStringUTFChars(password, cpassword);
    env->ReleaseStringUTFChars(capability_token, ctoken);
    return ret;
}

JNIEXPORT jint JNICALL Java_com_CCP_phone_NativeInterface_connectToCCPWithXML
(JNIEnv * env, jclass cls, jstring proxy_addr, jstring account, jstring password,jstring capability_token)
{
    if (NULL == proxy_addr || NULL == account || NULL == password || NULL == capability_token) {
        return -1;
    }
    const char* cproxy_addr = env->GetStringUTFChars(proxy_addr, 0);
    const char* caccount = env->GetStringUTFChars(account, 0);
    const char* cpassword = env->GetStringUTFChars(password, 0);
    const char* ctoken = env->GetStringUTFChars(capability_token, 0);
    int ret = connectToCCPWithXML(cproxy_addr, caccount, cpassword, ctoken);

    env->ReleaseStringUTFChars(proxy_addr, cproxy_addr);
    env->ReleaseStringUTFChars(account, caccount);
    env->ReleaseStringUTFChars(password, cpassword);
    env->ReleaseStringUTFChars(capability_token, ctoken);
    return ret;
}

JNIEXPORT jint JNICALL Java_com_CCP_phone_NativeInterface_disConnectToCCP
(JNIEnv * env, jclass cls)
{
    int ret = disConnectToCCP();
    return ret;
}

JNIEXPORT jint JNICALL Java_com_CCP_phone_NativeInterface_setUserName
(JNIEnv * env, jclass cls, jstring username)
{
    if (NULL == username) {
        return -1;
    }
    const char* cname = env->GetStringUTFChars(username, 0);
    int ret = setUserName(cname);
    env->ReleaseStringUTFChars(username, cname);
    return ret;
}

JNIEXPORT jstring JNICALL Java_com_CCP_phone_NativeInterface_makeCall
(JNIEnv * env, jclass cls, jint callType, jstring called)
{
    if (NULL == called) {
        return NULL;
    }
    const char* ccalled = env->GetStringUTFChars(called, 0);
    const char* callid = makeCall(callType, ccalled);
    env->ReleaseStringUTFChars(called, ccalled);
    return env->NewStringUTF(callid);
}

JNIEXPORT jint JNICALL Java_com_CCP_phone_NativeInterface_releaseCall
(JNIEnv * env, jclass cls, jstring callid, jint reason)
{
    if (NULL == callid) {
        return -1;
    }
    const char* ccallid = env->GetStringUTFChars(callid, 0);
    int ret = releaseCall(ccallid, reason);
    env->ReleaseStringUTFChars(callid, ccallid);
    return ret;
}

JNIEXPORT jint JNICALL Java_com_CCP_phone_NativeInterface_acceptCall
(JNIEnv * env, jclass cls, jstring callid)
{
    if (NULL == callid) {
        return -1;
    }
    const char* ccallid = env->GetStringUTFChars(callid, 0);
    int ret = acceptCall(ccallid);
    env->ReleaseStringUTFChars(callid, ccallid);
    return ret;
}


JNIEXPORT jint JNICALL Java_com_CCP_phone_NativeInterface_rejectCall
(JNIEnv * env, jclass cls, jstring callid, jint reason)
{
    if (NULL == callid) {
        return -1;
    }
    const char* ccallid = env->GetStringUTFChars(callid, 0);
    int ret = rejectCall(ccallid, reason);
    env->ReleaseStringUTFChars(callid, ccallid);
    return ret;
}


JNIEXPORT jint JNICALL Java_com_CCP_phone_NativeInterface_pauseCall
(JNIEnv * env, jclass cls, jstring callid)
{
    if (NULL == callid) {
        return -1;
    }
    const char* ccallid = env->GetStringUTFChars(callid, 0);
    int ret = pauseCall(ccallid);
    env->ReleaseStringUTFChars(callid, ccallid);
    return ret;
}

JNIEXPORT jint JNICALL Java_com_CCP_phone_NativeInterface_resumeCall
(JNIEnv * env, jclass cls, jstring callid)
{
    if (NULL == callid) {
        return -1;
    }
    const char* ccallid = env->GetStringUTFChars(callid, 0);
    int ret = resumeCall(ccallid);
    env->ReleaseStringUTFChars(callid, ccallid);
    return ret;
}


JNIEXPORT jint JNICALL Java_com_CCP_phone_NativeInterface_transferCall
(JNIEnv * env, jclass cls, jstring callid, jstring destionation, jint type)
{
    if (NULL == callid || NULL == destionation) {
        return -1;
    }
    const char* ccallid = env->GetStringUTFChars(callid, 0);
    const char* cdest = env->GetStringUTFChars(destionation, 0);
    int ret = transferCall(ccallid, cdest, type);
    env->ReleaseStringUTFChars(callid, ccallid);
    env->ReleaseStringUTFChars(destionation, cdest);
    return ret;
}


JNIEXPORT jint JNICALL Java_com_CCP_phone_NativeInterface_cancelCall
(JNIEnv * env, jclass cls, jstring callid)
{
    if (NULL == callid) {
        return -1;
    }
    const char* ccallid = env->GetStringUTFChars(callid, 0);
    env->ReleaseStringUTFChars(callid, ccallid);
    return 0;
}


JNIEXPORT jint JNICALL Java_com_CCP_phone_NativeInterface_sendDTMF
(JNIEnv * env, jclass cls, jstring callid, jchar dtmf)
{
    if (NULL == callid) {
        return -1;
    }
    const char* ccallid = env->GetStringUTFChars(callid, 0);
    int ret = sendDTMF(ccallid, dtmf);
    env->ReleaseStringUTFChars(callid, ccallid);
    return ret;
}

JNIEXPORT jstring JNICALL Java_com_CCP_phone_NativeInterface_sendTextMessage
(JNIEnv * env, jclass cls, jstring receiver, jstring message, jstring userdata)
{
    if (NULL == receiver || NULL == message) {
        return NULL;
    }
    const char* crecv = env->GetStringUTFChars(receiver, 0);
    const char* cmessage = env->GetStringUTFChars(message, 0);
    const char* cuserdata = NULL;
    if(userdata)
        cuserdata = env->GetStringUTFChars(userdata, 0);
    const char *msgid = sendTextMessage(crecv, cmessage, cuserdata);
    env->ReleaseStringUTFChars(receiver, crecv);
    env->ReleaseStringUTFChars(message, cmessage);
    if(userdata)
        env->ReleaseStringUTFChars(userdata, cuserdata);
    return env->NewStringUTF(msgid);
}


JNIEXPORT jint JNICALL Java_com_CCP_phone_NativeInterface_getCallState
(JNIEnv * env, jclass cls, jstring callid)
{
    if (NULL == callid) {
        return -1;
    }
    const char* ccallid = env->GetStringUTFChars(callid, 0);
    int state = getCallState(ccallid);
    env->ReleaseStringUTFChars(callid, ccallid);
    return state;
}


JNIEXPORT void JNICALL Java_com_CCP_phone_NativeInterface_setCallBackParams
(JNIEnv * env, jclass cls, jobject receiver, jstring methodName, jstring methodSig)
{
    if (NULL == methodName || NULL == methodSig) {
        return;
    }
    jobject globalObj = env->NewGlobalRef(receiver);

    const char* temp1 = env->GetStringUTFChars(methodName, 0);
    const char* temp2 = env->GetStringUTFChars(methodSig, 0);

    __android_log_print(ANDROID_LOG_DEBUG,"JNI", "callbackparams setted:%s", temp1);
    setCallBackParams(globalObj, temp1, temp2);

    env->ReleaseStringUTFChars(methodName, temp1);

    env->ReleaseStringUTFChars(methodSig, temp2);
}

JNIEXPORT void JNICALL Java_com_CCP_phone_NativeInterface_setBytesCallBackParams
(JNIEnv * env, jclass cls, jobject receiver, jstring methodName, jstring methodSig)
{
    if (NULL == methodName || NULL == methodSig) {
        return;
    }
    jobject globalObj = env->NewGlobalRef(receiver);

    const char* temp1 = env->GetStringUTFChars(methodName, 0);
    const char* temp2 = env->GetStringUTFChars(methodSig, 0);

    __android_log_print(ANDROID_LOG_DEBUG,"JNI", "callbackparams setted:%s", temp1);
    setBytesCallBackParams(globalObj, temp1, temp2);

    env->ReleaseStringUTFChars(methodName, temp1);

    env->ReleaseStringUTFChars(methodSig, temp2);
}

JNIEXPORT void JNICALL Java_com_CCP_phone_NativeInterface_setLogLevel
(JNIEnv * env, jclass cls, jint level)
{
    setLogLevel(level);
}

JNIEXPORT jint JNICALL Java_com_CCP_phone_NativeInterface_enableLoudsSpeaker
(JNIEnv * env, jclass cls, jboolean enable)
{
    return enableLoudsSpeaker(enable);
}

JNIEXPORT jboolean JNICALL Java_com_CCP_phone_NativeInterface_getLoudsSpeakerStatus
(JNIEnv * env, jclass cls)
{
    return getLoudsSpeakerStatus();
}

JNIEXPORT jint JNICALL Java_com_CCP_phone_NativeInterface_setMute
(JNIEnv * env, jclass cls, jboolean on)
{
    return setMute(on);
}

JNIEXPORT jboolean JNICALL Java_com_CCP_phone_NativeInterface_getMuteStatus
(JNIEnv * env, jclass cls)
{
    return getMuteStatus();
}

JNIEXPORT jint JNICALL Java_com_CCP_phone_NativeInterface_setRing
(JNIEnv * env, jclass cls, jstring filename)
{
    if (NULL == filename) {
        return -1;
    }
    const char* fn = env->GetStringUTFChars(filename, 0);
    int ret = setRing(fn);
    env->ReleaseStringUTFChars(filename, fn);
    return ret;
}

JNIEXPORT jint JNICALL Java_com_CCP_phone_NativeInterface_setRingback
(JNIEnv * env, jclass cls, jstring filename)
{
    if (NULL == filename) {
        return -1;
    }
    const char* fn = env->GetStringUTFChars(filename, 0);
    int ret = setRingback(fn);
    env->ReleaseStringUTFChars(filename, fn);
    return ret;
}

JNIEXPORT jstring JNICALL Java_com_CCP_phone_NativeInterface_getCurrentCall
(JNIEnv * env, jclass cls)
{
    const char* callid = getCurrentCall();
    return env->NewStringUTF(callid);
}

JNIEXPORT jint JNICALL Java_com_CCP_phone_NativeInterface_setUserData
(JNIEnv * env, jclass cls, jint type, jstring userData)
{
    if (NULL == userData) {
        return -1;
    }
    const char* data = env->GetStringUTFChars(userData, 0);
    int ret = setUserData(type, data);
    env->ReleaseStringUTFChars(userData, data);
    return ret;
}

JNIEXPORT jstring JNICALL Java_com_CCP_phone_NativeInterface_getUserData
(JNIEnv * env, jclass cls, jint type)
{
    char buffer[1024];
    getUserData(type, buffer, 1024);
    return env->NewStringUTF(buffer);
}

JNIEXPORT jint JNICALL Java_com_CCP_phone_NativeInterface_setCapabilityToken
(JNIEnv * env, jclass cls, jstring token)
{
    if (NULL == token) {
        return -1;
    }
    const char* data = env->GetStringUTFChars(token, 0);
    int ret = 0; //setCapabilityToken(data);
    env->ReleaseStringUTFChars(token, data);
    return ret;
}

static jobject globalRemoteVideoWindow = 0;
static jobject globalLocalViewWindow = 0;
static char remoteUserID[126];
JNIEXPORT jint JNICALL Java_com_CCP_phone_NativeInterface_setVideoView
(JNIEnv * env, jclass cls,jobject remoteView, jobject localView)
{
    const char* userid = env->GetStringUTFChars((jstring)remoteView, 0);
    sprintf(remoteUserID, "%s", userid);
    int temp = setVideoView((void*)&remoteUserID, NULL);
    return temp;

 /*   jobject globalRemoteObj = 0;
    if(remoteView)
    	globalRemoteObj = env->NewGlobalRef(remoteView);
    jobject globalLocalObj = 0;
    if(localView)
    		globalLocalObj = env->NewGlobalRef(localView);
    int temp = setVideoView((void*)globalRemoteObj, (void*)globalLocalObj);
    if(globalRemoteVideoWindow)
        env->DeleteGlobalRef(globalRemoteVideoWindow);
    if(globalLocalViewWindow)
    		env->DeleteGlobalRef(globalLocalViewWindow);
    globalRemoteVideoWindow = globalRemoteObj;
    globalLocalViewWindow = globalLocalObj;
    return temp; */
}

JNIEXPORT void JNICALL Java_com_CCP_phone_NativeInterface_setNetworkType
(JNIEnv * env, jclass cls, jint type,jboolean connected,jboolean reconnect)
{
	 setNetworkType(type,connected,reconnect);
   return;
}

JNIEXPORT jint JNICALL Java_com_CCP_phone_NativeInterface_selectCamera
(JNIEnv * env, jclass cls, jint cameraIndex, jint capabilityIndex, jint fps, jint rotate, jboolean force)
{
    return selectCamera(cameraIndex, capabilityIndex, fps, rotate, force);
}

JNIEXPORT jobjectArray JNICALL Java_com_CCP_phone_NativeInterface_getCameraInfo
(JNIEnv * env, jclass cls)
{
    jclass cameraInfoClass = env->FindClass("com/CCP/phone/CameraInfo");
    if(!cameraInfoClass)
    {
        __android_log_print(ANDROID_LOG_DEBUG, "JNI", "CameraInfo class not found");
        return 0;
    }
    __android_log_print(ANDROID_LOG_DEBUG, "JNI", "CameraInfo class found");
    jclass capabilityClass = env->FindClass("com/CCP/phone/CameraCapbility");
    if(!capabilityClass)
    {
        __android_log_print(ANDROID_LOG_DEBUG, "JNI", "Capability class not found");
        return 0;
    }
    __android_log_print(ANDROID_LOG_DEBUG, "JNI", "Capability class found");

    jfieldID cameraIndexID = env->GetFieldID(cameraInfoClass, "index", "I");
    jfieldID cameraNameID = env->GetFieldID(cameraInfoClass, "name", "Ljava/lang/String;");
    jfieldID cameraCapsID = env->GetFieldID(cameraInfoClass, "caps", "[Lcom/CCP/phone/CameraCapbility;");

    if(!cameraIndexID || !cameraNameID || !cameraCapsID)
    {
        __android_log_print(ANDROID_LOG_DEBUG, "JNI", "One of camera info field not found");
        return 0;
    }
    __android_log_print(ANDROID_LOG_DEBUG, "JNI", "CameraInfo field all ok");

    jmethodID cameraConstructID = env->GetMethodID(cameraInfoClass, "<init>", "()V");
    if(!cameraConstructID)
    {
        __android_log_print(ANDROID_LOG_DEBUG, "JNI", "CameraInfo construct method not found");
        return 0;
    }
    __android_log_print(ANDROID_LOG_DEBUG, "JNI", "CameraInfo construct method found");

    jfieldID capIndexID = env->GetFieldID(capabilityClass, "index", "I");
    jfieldID capWID = env->GetFieldID(capabilityClass, "width", "I");
    jfieldID capHID = env->GetFieldID(capabilityClass, "height", "I");
    jfieldID capFPSID = env->GetFieldID(capabilityClass, "maxFPS", "I");

    if(!capIndexID || !capWID || !capHID || !capFPSID)
    {
        __android_log_print(ANDROID_LOG_DEBUG, "JNI", "One of Capability field not found");
        return 0;
    }

    __android_log_print(ANDROID_LOG_DEBUG, "JNI", "CameraCapability field all ok");

    jmethodID capasConstructID = env->GetMethodID(capabilityClass, "<init>", "()V");
    if(!capasConstructID)
    {
        __android_log_print(ANDROID_LOG_DEBUG, "JNI", "Capability construct not found");
        return 0;
    }

    __android_log_print(ANDROID_LOG_DEBUG, "JNI", "Capability construct found");

    CameraInfo* CAMInfos = 0;
    int cameraCount = getCameraInfo(&CAMInfos);

    __android_log_print(ANDROID_LOG_DEBUG, "JNI", "Get CameraCount:%d", cameraCount);

    jobjectArray cameraInfos = env->NewObjectArray(cameraCount, cameraInfoClass, 0);

    if(!cameraInfos)
    {
        __android_log_print(ANDROID_LOG_DEBUG, "JNI", "Alloc CameraInfo Array failed");
        return 0;
    }

    for(int i = 0; i < cameraCount; i++)
    {
        int index = CAMInfos[i].index;
        jstring nameString = env->NewStringUTF(CAMInfos[i].name);
        jobject cameraInfo = env->NewObject(cameraInfoClass, cameraConstructID, "");
        if(!cameraInfo)
        {
            __android_log_print(ANDROID_LOG_DEBUG, "JNI", "Alloc CameraInfo %d failed", i);
            return 0;
        }
        __android_log_print(ANDROID_LOG_DEBUG, "JNI", "Alloc CameraInfo %d success", i);

        env->SetObjectField(cameraInfo, cameraNameID, nameString);
        env->SetIntField(cameraInfo, cameraIndexID, index);

        int capCount = CAMInfos[i].capabilityCount;
        __android_log_print(ANDROID_LOG_DEBUG, "JNI", "CameraInfo %d Caps count:%d", i, capCount);
        jobjectArray capsInfos = env->NewObjectArray(capCount, capabilityClass, 0);
        if(!capsInfos)
        {
            __android_log_print(ANDROID_LOG_DEBUG, "JNI", "Alloc Camera %d caps array failed", i);
            return 0;
        }
        __android_log_print(ANDROID_LOG_DEBUG, "JNI", "Alloc Camera %d caps array success", i);
        for(int j = 0; j < capCount; j++)
        {
            jobject capsInfo = env->NewObject(capabilityClass, capasConstructID, "");
            if(!capsInfo)
            {
                __android_log_print(ANDROID_LOG_DEBUG, "JNI", "Alloc Camera %d caps %d failed", i, j);
                return 0;
            }
            __android_log_print(ANDROID_LOG_DEBUG, "JNI", "Alloc Camera %d caps %d success", i, j);

            env->SetIntField(capsInfo, capIndexID, j);
            env->SetIntField(capsInfo, capWID, CAMInfos[i].capability[j].width);
            env->SetIntField(capsInfo, capHID, CAMInfos[i].capability[j].height);
            env->SetIntField(capsInfo, capFPSID, CAMInfos[i].capability[j].maxfps);
            env->SetObjectArrayElement(capsInfos, j, capsInfo);
        }
        env->SetObjectField(cameraInfo, cameraCapsID, capsInfos);
        env->SetObjectArrayElement(cameraInfos, i, cameraInfo);
    }

    return cameraInfos;

}

JNIEXPORT jstring JNICALL Java_com_CCP_phone_NativeInterface_getCallStatistics
(JNIEnv *env, jclass cls,jint type)
{
    char buf[1024];
	MediaStatisticsInfo stats;
    getCallStatistics(type,&stats);
    sprintf(buf, "%d#%d#%d#%d#%d#%d#%d#%d#%d", 
        stats.fractionLost, stats.cumulativeLost, stats.extendedMax, stats.jitterSamples, stats.rttMs, stats.bytesSent, stats.packetsSent, stats.bytesReceived, stats.packetsReceived);
    return env->NewStringUTF(buf);
}

JNIEXPORT jstring JNICALL Java_com_CCP_phone_NativeInterface_getVersion
(JNIEnv *env, jclass)
{
	return env->NewStringUTF(getVersion());
}

JNIEXPORT void JNICALL Java_com_CCP_phone_NativeInterface_setKeepAliveTimeout
(JNIEnv *, jclass,jint forWifi,jint for3G)
{
	setKeepAliveTimeout(forWifi,for3G);
}

JNIEXPORT jint JNICALL Java_com_CCP_phone_NativeInterface_updateCallMedia
(JNIEnv *env, jclass, jstring callid, jint request)
{
    if (NULL == callid) {
        return -1;
    }
    const char* ccallid = env->GetStringUTFChars(callid, 0);
    return updateCallMedia(ccallid,request);
}

JNIEXPORT jint JNICALL Java_com_CCP_phone_NativeInterface_answerCallMediaUpdate
(JNIEnv *env, jclass, jstring callid, jint action)
{
    if (NULL == callid) {
        return -1;
    }
    const char* ccallid = env->GetStringUTFChars(callid, 0);
    int ret = answerCallMediaUpdate(ccallid, action);
    env->ReleaseStringUTFChars(callid,ccallid);
    return ret;
}

JNIEXPORT jint JNICALL Java_com_CCP_phone_NativeInterface_getCallMeidaType
(JNIEnv *env, jclass, jstring callid)
{
    if (NULL == callid) {
        return -1;
    }
    const char* ccallid = env->GetStringUTFChars(callid, 0);
    int ret = getCallMediaType(ccallid);
    env->ReleaseStringUTFChars(callid, ccallid);
    return ret;
}

JNIEXPORT jboolean JNICALL Java_com_CCP_phone_NativeInterface_getCodecEnabled
(JNIEnv *, jclass, jint type)
{
    return  getCodecEnabled(type);
}

JNIEXPORT jint JNICALL Java_com_CCP_phone_NativeInterface_setCodecEnabled
(JNIEnv * env, jclass, jint type, jboolean enabled)
{
    return setCodecEnabled(type,enabled);
}

JNIEXPORT jint JNICALL Java_com_CCP_phone_NativeInterface_setAgcEnabled
(JNIEnv * env, jclass, jint, jboolean enabled)
{
    return setAudioConfigEnabled(0, enabled, 0);
}

JNIEXPORT void JNICALL Java_com_CCP_phone_NativeInterface_setDtxEnabled
(JNIEnv * env, jclass,jboolean enabled)
{
    setDtxEnabled(enabled);
}

JNIEXPORT jint JNICALL Java_com_CCP_phone_NativeInterface_setSrtpEnabled
(JNIEnv * env, jclass, jboolean tls, jboolean srtp, jboolean userMode, jint cryptType, jstring key)
{
    if (NULL == key) {
        return -1;
    }
    const char* keyp = env->GetStringUTFChars(key, 0);
    int ret = setSrtpEnabled(tls, srtp, userMode, cryptType, keyp);
    env->ReleaseStringUTFChars(key, keyp);
    return ret;
}

JNIEXPORT jint JNICALL Java_com_CCP_phone_NativeInterface_setAudioConfig
(JNIEnv * env, jclass, jint configType, jboolean enabled, jint mode)
{
    return setAudioConfigEnabled(configType, enabled, mode);
}

JNIEXPORT jboolean JNICALL Java_com_CCP_phone_NativeInterface_getAudioConfig
(JNIEnv * env, jclass, jint configType)
{
    bool enabled = false;
    int mode;
    int ret = getAudioConfigEnabled(configType, &enabled, &mode);
    if(ret < 0)
        return false;
    else
        return enabled;
}

JNIEXPORT jint JNICALL Java_com_CCP_phone_NativeInterface_getAudioConfigMode
(JNIEnv * env, jclass, jint configType)
{
    bool enabled = false;
    int mode;
    int ret = getAudioConfigEnabled(configType, &enabled, &mode);
    if(ret < 0)
        return -1;
    else
        return mode;
}

JNIEXPORT void JNICALL Java_com_CCP_phone_NativeInterface_setVideoBitRates
(JNIEnv *env, jclass, jint bitrates)
{
    setVideoBitRates(bitrates);
}

JNIEXPORT jint JNICALL Java_com_CCP_phone_NativeInterface_startRtpDump
(JNIEnv * env, jclass, jstring callid, jint mediaType, jstring fileName, jint direction)
{
    if (NULL == callid || NULL == fileName) {
        return -1;
    }
    const char* ccallid = env->GetStringUTFChars(callid, 0);
    const char* cfilename = env->GetStringUTFChars(fileName, 0);
    int ret = startRtpDump(ccallid, mediaType, cfilename, direction);
    env->ReleaseStringUTFChars(callid,ccallid);
    env->ReleaseStringUTFChars(fileName,cfilename);
    return ret;
}

JNIEXPORT jint JNICALL Java_com_CCP_phone_NativeInterface_stopRtpDump
(JNIEnv * env, jclass, jstring callid, jint mediaType, jint direction)
{
    if (NULL == callid) {
        return -1;
    }
    const char* ccallid = env->GetStringUTFChars(callid, 0);
    int ret = stopRtpDump(ccallid, mediaType, direction);
    env->ReleaseStringUTFChars(callid, ccallid);
    return ret;
}

JNIEXPORT jint JNICALL Java_com_CCP_phone_NativeInterface_AmrNBCreateEnc
(JNIEnv * env, jclass cls)
{
    return ECMedia_AmrNBCreateEnc();
}

JNIEXPORT jint JNICALL Java_com_CCP_phone_NativeInterface_AmrNBCreateDec
(JNIEnv * env, jclass cls)
{
    return ECMedia_AmrNBCreateDec();
}

JNIEXPORT jint JNICALL Java_com_CCP_phone_NativeInterface_AmrNBFreeEnc
(JNIEnv * env, jclass cls)
{
    return ECMedia_AmrNBFreeEnc();
}

JNIEXPORT jint JNICALL Java_com_CCP_phone_NativeInterface_AmrNBFreeDec
(JNIEnv * env, jclass cls)
{
    return ECMedia_AmrNBFreeDec();
}

JNIEXPORT jint JNICALL Java_com_CCP_phone_NativeInterface_AmrNBEncode
(JNIEnv * env, jclass cls, jbyteArray input, jint inputLen, jbyteArray output, jint mode)
{
    jbyte *inputData = env->GetByteArrayElements(input, 0);
    short noiseBuf[3840] = {'\0'};
    jint ret = noiseSuppression((const void *)inputData,noiseBuf);
    jbyte buf[1024];
    jint outputLen;
    if (0 == ret) {
        outputLen = ECMedia_AmrNBEncode((short *)noiseBuf, (short)(inputLen/sizeof(short)), (short *)&buf[0], mode);
    }
    else
    {
        outputLen = ECMedia_AmrNBEncode((short *)inputData, (short)(inputLen/sizeof(short)), (short *)&buf[0], mode);
    }

    if(outputLen > 0)
    {
        env->SetByteArrayRegion(output, 0, outputLen-1, (jbyte*)&buf[0]);
        //__android_log_print(ANDROID_LOG_DEBUG,"JNI", "amr AmrNBEncode 2");
    }

    env->ReleaseByteArrayElements(input, inputData, 0);
//    __android_log_print(ANDROID_LOG_DEBUG,"JNI", "amr AmrNBEncode 3=%d", outputLen);

    return outputLen;
}

JNIEXPORT jint JNICALL Java_com_CCP_phone_NativeInterface_AmrNBEncoderInit
(JNIEnv * env, jclass cls, jint dtxmode)
{
    return ECMedia_AmrNBEncoderInit(dtxmode);
}

JNIEXPORT jint JNICALL Java_com_CCP_phone_NativeInterface_AmrNBDecode
(JNIEnv * env, jclass cls, jbyteArray encoded, jint encodedLen, jbyteArray decoded)
{
    jbyte *encodedData = env->GetByteArrayElements(encoded, 0);
    jbyte buf[1024];
    int decodedLen = ECMedia_AmrNBDecode((short *)encodedData, encodedLen, (short *)buf);
    if(decodedLen > 0)
        env->SetByteArrayRegion(decoded, 0, decodedLen-1, buf);
    env->ReleaseByteArrayElements(encoded, encodedData, 0);

    return decodedLen;
}

JNIEXPORT jstring JNICALL Java_com_CCP_phone_NativeInterface_AmrNBVersion
(JNIEnv * env, jclass cls)
{
    char buffer[1024];
    ECMedia_AmrNBVersion(buffer, 1024);
    return env->NewStringUTF(buffer);
}

JNIEXPORT jstring JNICALL Java_com_CCP_phone_NativeInterface_GetUniqueID
(JNIEnv * env, jclass cls)
{
    char uniqueid[32];
    getUniqueID(uniqueid, 32);
    return env->NewStringUTF(uniqueid);
}

JNIEXPORT jint JNICALL Java_com_CCP_phone_NativeInterface_SetStunServer__Ljava_lang_String_2
(JNIEnv * env, jclass, jstring server)
{
    if (NULL == server) {
        return -1;
    }
    const char* serverInChar = env->GetStringUTFChars(server, 0);
    int ret = setStunServer(serverInChar);
    env->ReleaseStringUTFChars(server, serverInChar);
    return ret;
}


JNIEXPORT jint JNICALL Java_com_CCP_phone_NativeInterface_SetStunServer__Ljava_lang_String_2I
(JNIEnv * env, jclass, jstring server, jint port)
{
    if (NULL == server) {
        return -1;
    }
    const char *serverInChar = env->GetStringUTFChars(server, 0);
    int ret = setStunServer(serverInChar,port);
    env->ReleaseStringUTFChars(server, serverInChar);
    return ret;
}


JNIEXPORT jint JNICALL Java_com_CCP_phone_NativeInterface_SetFirewallPolicy
(JNIEnv *, jclass, jint policy)
{
    return setFirewallPolicy((CCPClientFirewallPolicy)policy);
}

JNIEXPORT jint JNICALL Java_com_CCP_phone_NativeInterface_SetShieldMosaic
(JNIEnv *, jclass, jboolean flag)
{
    return setShieldMosaic(flag);
}

JNIEXPORT jint JNICALL Java_com_CCP_phone_NativeInterface_SetStartBitRateAfterP2PSucceed
(JNIEnv *, jclass, jint)
{

}

JNIEXPORT jint JNICALL Java_com_CCP_phone_NativeInterface_startDeliverVideoFrame
(JNIEnv *env, jclass, jstring callid)
{
    if (NULL == callid) {
        return -1;
    }
    const char *CallID = env->GetStringUTFChars(callid, 0);
    int ret = startDeliverVideoFrame(CallID);
    env->ReleaseStringUTFChars(callid, CallID);
    return ret;
}

JNIEXPORT jint JNICALL Java_com_CCP_phone_NativeInterface_stopDeliverVideoFrame
(JNIEnv *env, jclass, jstring callid)
{
    if (NULL == callid) {
        return -1;
    }
    const char *CallID = env->GetStringUTFChars(callid, 0);
    int ret = stopDeliverVideoFrame(CallID);
    env->ReleaseStringUTFChars(callid, CallID);
    return ret;
}

JNIEXPORT jobject JNICALL Java_com_CCP_phone_NativeInterface_getLocalVideoSnapshot
  (JNIEnv *env, jclass, jstring callid)
  {
      jclass videoSnapshotClass = env->FindClass("com/CCP/phone/VideoSnapshot");
      if(!videoSnapshotClass)
      {
          __android_log_print(ANDROID_LOG_DEBUG, "JNI", "VideoSnapshot class not found");
          return 0;
      }
      __android_log_print(ANDROID_LOG_DEBUG, "JNI", "VideoSnapshot class found");

      jfieldID snapShotWidthID = env->GetFieldID(videoSnapshotClass, "width", "I");
      jfieldID snapShotHeightID = env->GetFieldID(videoSnapshotClass, "height", "I");
      jfieldID snapShotDataID = env->GetFieldID(videoSnapshotClass, "data", "[B");
      if(!snapShotWidthID || !snapShotHeightID || !snapShotDataID)
      {
          __android_log_print(ANDROID_LOG_DEBUG, "JNI", "One of VideoSnapshot field not found");
          return 0;
      }

      jmethodID videoSnapshotConstructID = env->GetMethodID(videoSnapshotClass, "<init>", "()V");
      if(!videoSnapshotConstructID)
      {
          __android_log_print(ANDROID_LOG_DEBUG, "JNI", "VideoSnapshot construct method not found");
          return 0;
      }
      __android_log_print(ANDROID_LOG_DEBUG, "JNI", "VideoSnapshot construct method found");

      jobject videoSnapshot = env->NewObject(videoSnapshotClass, videoSnapshotConstructID, "");
      if(!videoSnapshot)
      {
          __android_log_print(ANDROID_LOG_DEBUG, "JNI", "Alloc VideoSnapshot failed");
          return 0;
      }
      __android_log_print(ANDROID_LOG_DEBUG, "JNI", "Alloc VideoSnapshot success");

      if (NULL == callid) {
          return 0;
      }
      const char *CallID = env->GetStringUTFChars(callid, 0);
      unsigned char *buf = NULL;
      unsigned int size=0, width=0, height=0;

      int ret = getLocalVideoSnapshot(CallID, &buf, &size, &width, &height);
      env->ReleaseStringUTFChars(callid, CallID);
      if(ret < 0) {
          __android_log_print(ANDROID_LOG_DEBUG, "JNI", "Get Local VideoSnapshot field");
          return 0;
      }

      jbyteArray javaData = env->NewByteArray(size);
      env->SetByteArrayRegion(javaData, 0, size, (jbyte*)buf);

      env->SetIntField(videoSnapshot, snapShotWidthID, width);
      env->SetIntField(videoSnapshot, snapShotHeightID, height);
      env->SetObjectField(videoSnapshot, snapShotDataID, javaData);

      env->DeleteLocalRef(javaData);
      free(buf);

      return videoSnapshot;
  }

/*
 * Class:     com_CCP_phone_NativeInterface
 * Method:    getRemoteVideoSnapshot
 * Signature: (Ljava/lang/String;)Lcom/CCP/phone/VideoSnapshot;
 */
JNIEXPORT jobject JNICALL Java_com_CCP_phone_NativeInterface_getRemoteVideoSnapshot
  (JNIEnv *env, jclass, jstring callid)
  {
      jclass videoSnapshotClass = env->FindClass("com/CCP/phone/VideoSnapshot");
      if(!videoSnapshotClass)
      {
          __android_log_print(ANDROID_LOG_DEBUG, "JNI", "VideoSnapshot class not found");
          return 0;
      }
      __android_log_print(ANDROID_LOG_DEBUG, "JNI", "VideoSnapshot class found");

      jfieldID snapShotWidthID = env->GetFieldID(videoSnapshotClass, "width", "I");
      jfieldID snapShotHeightID = env->GetFieldID(videoSnapshotClass, "height", "I");
      jfieldID snapShotDataID = env->GetFieldID(videoSnapshotClass, "data", "[B");
      if(!snapShotWidthID || !snapShotHeightID || !snapShotDataID)
      {
          __android_log_print(ANDROID_LOG_DEBUG, "JNI", "One of VideoSnapshot field not found");
          return 0;
      }

      jmethodID videoSnapshotConstructID = env->GetMethodID(videoSnapshotClass, "<init>", "()V");
      if(!videoSnapshotConstructID)
      {
          __android_log_print(ANDROID_LOG_DEBUG, "JNI", "VideoSnapshot construct method not found");
          return 0;
      }
      __android_log_print(ANDROID_LOG_DEBUG, "JNI", "VideoSnapshot construct method found");

      jobject videoSnapshot = env->NewObject(videoSnapshotClass, videoSnapshotConstructID, "");
      if(!videoSnapshot)
      {
          __android_log_print(ANDROID_LOG_DEBUG, "JNI", "Alloc VideoSnapshot failed");
          return 0;
      }
      __android_log_print(ANDROID_LOG_DEBUG, "JNI", "Alloc VideoSnapshot success");

      if (NULL == callid) {
          return 0;
      }
      const char *CallID = env->GetStringUTFChars(callid, 0);
      unsigned char *buf = NULL;
      unsigned int size=0, width=0, height=0;

      int ret = getRemoteVideoSnapshot(CallID, &buf, &size, &width, &height);
      env->ReleaseStringUTFChars(callid,CallID);
      if(ret < 0) {
          __android_log_print(ANDROID_LOG_DEBUG, "JNI", "Get Remote VideoSnapshot field");
          return 0;
      }

      jbyteArray javaData = env->NewByteArray(size);
      env->SetByteArrayRegion(javaData, 0, size, (jbyte*)buf);

      env->SetIntField(videoSnapshot, snapShotWidthID, width);
      env->SetIntField(videoSnapshot, snapShotHeightID, height);
      env->SetObjectField(videoSnapshot, snapShotDataID, javaData);

      env->DeleteLocalRef(javaData);
      free(buf);

      return videoSnapshot;
  }

/*
 * Class:     com_CCP_phone_NativeInterface
 * Method:    startRecordVoice
 * Signature: (Ljava/lang/String;Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_CCP_phone_NativeInterface_startRecordVoice
(JNIEnv *env, jclass, jstring callid, jstring filename)
{
    if (NULL == callid || NULL == filename) {
        return -1;
    }
    const char *CallID = env->GetStringUTFChars(callid, 0);
    const char *FileName = env->GetStringUTFChars(filename, 0);
    int ret = startRecordVoice(CallID, FileName);
    env->ReleaseStringUTFChars(callid,CallID);
    env->ReleaseStringUTFChars(filename,FileName);
    return ret;
}

/*
 * Class:     com_CCP_phone_NativeInterface
 * Method:    stopRecordVoice
 * Signature: (Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_CCP_phone_NativeInterface_stopRecordVoice
(JNIEnv *env, jclass, jstring callid)
{
    if (NULL == callid) {
        return -1;
    }
    const char *CallID = env->GetStringUTFChars(callid, 0);
    int ret = stopRecordVoice(CallID);
    env->ReleaseStringUTFChars(callid, CallID);
    return ret;
}

/*
 * Class:     com_CCP_phone_NativeInterface
 * Method:    startRecordVoiceEx
 * Signature: (Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_CCP_phone_NativeInterface_startRecordVoiceEx
(JNIEnv *env, jclass, jstring callid, jstring rFileName, jstring lFileName)
{
    if (NULL == callid) {
        return -1;
    }
    const char *CallID = env->GetStringUTFChars(callid, 0);
    const char *RemoteFileName = env->GetStringUTFChars(rFileName, 0);
    const char *LocalFileName = env->GetStringUTFChars(lFileName, 0);
    int ret = startRecordVoiceEx(CallID, RemoteFileName, LocalFileName);
    env->ReleaseStringUTFChars(callid,CallID);
    env->ReleaseStringUTFChars(rFileName,RemoteFileName);
    env->ReleaseStringUTFChars(lFileName,LocalFileName);
    return ret;
}

/*
 * Class:     com_CCP_phone_NativeInterface
 * Method:    startRecordVoip
 * Signature: (Ljava/lang/String;Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_CCP_phone_NativeInterface_startRecordVoip
(JNIEnv *env, jclass, jstring callid, jstring filename)
{
    if (NULL == callid || filename == NULL) {
        return -1;
    }
    const char *CallID = env->GetStringUTFChars(callid, 0);
    const char *FileName = env->GetStringUTFChars(filename, 0);
    int ret = -1;//startRecordVoip(CallID, FileName);
    env->ReleaseStringUTFChars(callid,CallID);
    env->ReleaseStringUTFChars(filename,FileName);
    return ret;
}

/*
 * Class:     com_CCP_phone_NativeInterface
 * Method:    stopRecordVoip
 * Signature: (Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_CCP_phone_NativeInterface_stopRecordVoip
(JNIEnv *env, jclass, jstring callid)
{
    if (NULL == callid) {
        return -1;
    }
    const char *CallID = env->GetStringUTFChars(callid, 0);
    int ret = -1;//stopRecordVoip(CallID);
    env->ReleaseStringUTFChars(callid, CallID);
    return ret;
}

/*
 * Class:     com_CCP_phone_NativeInterface
 * Method:    setTraceFlag
 * Signature: (Z)V
 */
JNIEXPORT void JNICALL Java_com_CCP_phone_NativeInterface_setTraceFlag
(JNIEnv *env, jclass, jboolean enable)
{
    setTraceFlag(enable);
}

/*
 * Class:     com_CCP_phone_NativeInterface
 * Method:    checkUserOnline
 * Signature: (Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_CCP_phone_NativeInterface_checkUserOnline
(JNIEnv *env, jclass, jstring account)
{
    if (NULL == account) {
        return -1;
    }
    const char *Account = env->GetStringUTFChars(account, 0);
    int ret = checkUserOnline(Account);
    env->ReleaseStringUTFChars(account,Account);
    return ret;
}

/*
 * Class:     com_CCP_phone_NativeInterface
 * Method:    getNetworkStatistic
 * Signature: (Ljava/lang/String;)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_CCP_phone_NativeInterface_getNetworkStatistic
(JNIEnv *env, jclass, jstring callid)
{
    if (NULL == callid) {
        return NULL;
    }
    const char *CallID = env->GetStringUTFChars(callid, 0);
    long long duration=0, send_total_sim=0, recv_total_sim=0, send_total_wifi=0, recv_total_wifi=0;
    char temp[256];

    int ret = getNetworkStatistic(CallID, &duration, &send_total_sim, &recv_total_sim, &send_total_wifi, &recv_total_wifi);
    env->ReleaseStringUTFChars(callid,CallID);
    sprintf(temp, "<duration>%lld</duration><sendsim>%lld</sendsim><recvsim>%lld</recvsim><sendwifi>%lld</sendwifi><recvwifi>%lld</recvwifi>", duration, send_total_sim, recv_total_sim, send_total_wifi, recv_total_wifi);
    return env->NewStringUTF(temp);
}


/*
 * Class:     com_CCP_phone_NativeInterface
 * Method:    setProcessDataEnabled
 * Signature: (Ljava/lang/String;Z)I
 */
JNIEXPORT jint JNICALL Java_com_CCP_phone_NativeInterface_setProcessDataEnabled
(JNIEnv *env, jclass, jstring callid, jboolean flag)
{
    if (NULL == callid) {
        return -1;
    }
    const char *CallID = env->GetStringUTFChars(callid, 0);
    int ret = setProcessDataEnabled(CallID, flag);
    env->ReleaseStringUTFChars(callid,CallID);
    return ret;
}

/*
 * Class:     com_CCP_phone_NativeInterface
 * Method:    InitAudioDevice
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_CCP_phone_NativeInterface_InitAudioDevice
(JNIEnv *, jclass)
{
    return android_media_init_audio();
}

/*
 * Class:     com_CCP_phone_NativeInterface
 * Method:    UNInitAudioDevice
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_CCP_phone_NativeInterface_UNInitAudioDevice
(JNIEnv *, jclass)
{
    return android_media_uninit_audio();
}

/*
 * Class:     com_CCP_phone_NativeInterface
 * Method:    setPrivateCloud
 * Signature: (Ljava/lang/String;Ljava/lang/String;Z)I
 */
JNIEXPORT jint JNICALL Java_com_CCP_phone_NativeInterface_setPrivateCloud
(JNIEnv *env, jclass cls, jstring companyID, jstring proxy, jboolean nativeCheck)
{
    if (NULL == companyID || NULL == proxy) {
        return -1;
    }
    const char *id = env->GetStringUTFChars(companyID, 0);
    const char *proxy_addr = env->GetStringUTFChars(proxy, 0);
    int ret = setPrivateCloud(id, proxy_addr,nativeCheck);
    return ret;
}


/*
 * Class:     com_CCP_phone_NativeInterface
 * Method:    registerAudioDevice
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_CCP_phone_NativeInterface_registerAudioDevice
(JNIEnv *, jclass)
{
    return registerAudioDevice();
}

/*
 * Class:     com_CCP_phone_NativeInterface
 * Method:    deregisterAudioDevice
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_CCP_phone_NativeInterface_deregisterAudioDevice
(JNIEnv *, jclass)
{
    return deregisterAudioDevice();
}

/*
 * Class:     com_CCP_phone_NativeInterface
 * Method:    SetNetworkGroupId
 * Signature: (Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_CCP_phone_NativeInterface_SetNetworkGroupId
(JNIEnv *env, jclass, jstring groupID)
{

    if (NULL == groupID) {
        return -1;
    }
    const char *id = env->GetStringUTFChars(groupID, 0);
    int ret = SetNetworkGroupId(id);
    return ret;
}

/*
 * Class:     com_CCP_phone_NativeInterface
 * Method:    setProcessOriginalDataEnabled
 * Signature: (Ljava/lang/String;Z)I
 */
JNIEXPORT jint JNICALL Java_com_CCP_phone_NativeInterface_setProcessOriginalDataEnabled
(JNIEnv *env, jclass, jstring callid, jboolean flag)
{
    if (NULL == callid) {
        return -1;
    }
    const char *CallID = env->GetStringUTFChars(callid, 0);
    int ret = setProcessOriginalDataEnabled(CallID, flag);
    return ret;
}

/*
 * Class:     com_CCP_phone_NativeInterface
 * Method:    StartVideoCapture
 * Signature: (Ljava/lang/String;)I
 */
JNIEXPORT int JNICALL Java_com_CCP_phone_NativeInterface_StartVideoCapture
  (JNIEnv *env, jclass, jstring callid)
{
	if (NULL == callid) {
		return -1;
	}
	const char* ccallid = env->GetStringUTFChars(callid, 0);
	int ret = startVideoCapture(ccallid);
	env->ReleaseStringUTFChars(callid, ccallid);
	return ret;
}

/*
 * Class:     com_CCP_phone_NativeInterface
 * Method:    SetVideoConferenceAddr
 * Signature: (Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_CCP_phone_NativeInterface_SetVideoConferenceAddr
(JNIEnv *env, jclass, jstring ip)
{
    if (NULL == ip) {
        return -1;
    }
    const char *cip = env->GetStringUTFChars(ip, 0);
    int ret = setVideoConferenceAddr(cip);
    env->ReleaseStringUTFChars(ip, cip);
    return ret;
}

/*
 * Class:     com_CCP_phone_NativeInterface
 * Method:    requestMemberVideo
 * Signature: (Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/Object;I)I
 */
//static jobject globalConfVideoWindow = 0;
JNIEXPORT jint JNICALL Java_com_CCP_phone_NativeInterface_requestMemberVideo
(JNIEnv *env, jclass, jstring conferenceNo, jstring conferencePasswd, jstring remoteSipNo, jobject videoWindow, jint port)
{
    if (NULL == remoteSipNo) {
        return -1;
    }
    if (NULL == conferenceNo) {
        return -3;
    }
    if (NULL == conferencePasswd) {
        return -4;
    }
    jobject globalConfVideoObj = 0;
    if (videoWindow) {
//        globalConfVideoObj = env->NewGlobalRef(videoWindow);
    }
    else
    {
        return -2;
    }

    const char * cconferenceNo = env->GetStringUTFChars(conferenceNo, 0);
    const char * cconferencePasswd = env->GetStringUTFChars(conferencePasswd, 0);
    const char * cremoteSipNo = env->GetStringUTFChars(remoteSipNo, 0);
    int ret = requestMemberVideo(cconferenceNo, cconferencePasswd, cremoteSipNo, (void *)videoWindow, port);
    env->ReleaseStringUTFChars(conferenceNo, cconferenceNo);
    env->ReleaseStringUTFChars(conferencePasswd,cconferencePasswd);
    env->ReleaseStringUTFChars(remoteSipNo, cremoteSipNo);
//    if (globalConfVideoWindow) {
//        env->DeleteGlobalRef(globalConfVideoWindow);
//    }
//    globalConfVideoWindow = globalConfVideoObj;
    return ret;
}

/*
 * Class:     com_CCP_phone_NativeInterface
 * Method:    stopMemberVideo
 * Signature: (Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_CCP_phone_NativeInterface_stopMemberVideo
(JNIEnv *env, jclass, jstring conferenceNo, jstring conferencePasswd, jstring remoteSipNo)
{
    if (NULL == remoteSipNo) {
        return -1;
    }
    if (NULL == conferenceNo) {
        return -3;
    }
    if (NULL == conferencePasswd) {
        return -4;
    }
    const char * cconferenceNo = env->GetStringUTFChars(conferenceNo, 0);
    const char * cconferencePasswd = env->GetStringUTFChars(conferencePasswd, 0);
    const char * cremoteSipNo = env->GetStringUTFChars(remoteSipNo, 0);
    int ret = stopMemberVideo(cconferenceNo, cconferencePasswd, cremoteSipNo);
    env->ReleaseStringUTFChars(conferenceNo, cconferenceNo);
    env->ReleaseStringUTFChars(conferencePasswd,cconferencePasswd);
    env->ReleaseStringUTFChars(remoteSipNo, cremoteSipNo);

    return ret;

}

/*
 * Class:     com_CCP_phone_NativeInterface
 * Method:    setSilkRate
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_com_CCP_phone_NativeInterface_setSilkRate
(JNIEnv *, jclass, jint rate)
{
    return setSilkRate(rate);
}

/*
 * Class:     com_CCP_phone_NativeInterface
 * Method:    resetVideoConfWindow
 * Signature: (Ljava/lang/String;Ljava/lang/Object;)I
 */
//static jobject globalNewVideoWindow = 0;
JNIEXPORT jint JNICALL Java_com_CCP_phone_NativeInterface_resetVideoConfWindow
(JNIEnv *env, jclass, jstring sip, jobject newWindow)
{
    __android_log_print(ANDROID_LOG_DEBUG,"JNI", "DEBUG: resetVideoConfWindow called\n");
    if (NULL == sip) {
        return -2;
    }
    if (NULL == newWindow) {
        return -3;
    }
//    jobject globalNewVideoObj = 0;
//    globalNewVideoObj = env->NewGlobalRef(newWindow);
    const char * csip = env->GetStringUTFChars(sip, 0);
    int ret = resetVideoConfWindow(csip, (void *)newWindow);
    env->ReleaseStringUTFChars(sip, csip);
//    if (globalNewVideoWindow) {
//        env->DeleteGlobalRef(globalNewVideoWindow);
//    }
//    globalNewVideoWindow = globalNewVideoObj;
    return ret;
}

JNIEXPORT jint JNICALL Java_com_CCP_phone_NativeInterface_setHaiyuntongEnabled
(JNIEnv *, jclass, jboolean flag)
{
    __android_log_print(ANDROID_LOG_DEBUG,"JNI", "[DEBUG HAIYUNTONG] setHaiyuntongEnabled called\n");

    int ret = setHaiyuntongEnabled(flag);
    return ret;
}


/*
 * Class:     com_CCP_phone_NativeInterface
 * Method:    haiyuntongFileEncrypt
 * Signature: (Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;
 */
JNIEXPORT jint JNICALL Java_com_CCP_phone_NativeInterface_haiyuntongSetDeviceID
(JNIEnv *env, jclass, jstring deviceid, jstring appid,jboolean testFlag)
{
    __android_log_print(ANDROID_LOG_DEBUG,"JNI", "[DEBUG HAIYUNTONG] haiyuntongSetDeviceID called\n");
    if (NULL == deviceid) {
        __android_log_print(ANDROID_LOG_ERROR,"JNI", "[ERROR HAIYUNTONG] haiyuntongFileEncrypt called, deviceid is NULL\n");
        return -1;
    }

    const char * cdeviceid = env->GetStringUTFChars(deviceid, 0);
    const char * cappid = env->GetStringUTFChars(appid, 0);
    int ret = setDeviceID(cdeviceid, cappid, testFlag);
    env->ReleaseStringUTFChars(deviceid,cdeviceid);
    env->ReleaseStringUTFChars(appid,cappid);
    return ret;
}


/*
 * Class:     com_CCP_phone_NativeInterface
 * Method:    haiyuntongFileEncrypt
 * Signature: (Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;
 */
JNIEXPORT jint JNICALL Java_com_CCP_phone_NativeInterface_haiyuntongFileEncrypt
(JNIEnv *env, jclass, jbyteArray file, jstring remoteSip, jbyteArray encryptedFile)
{
    __android_log_print(ANDROID_LOG_DEBUG,"JNI", "[DEBUG HAIYUNTONG] haiyuntongFileEncrypt called\n");
    if (NULL == file) {
        __android_log_print(ANDROID_LOG_ERROR,"JNI", "[ERROR HAIYUNTONG] haiyuntongFileEncrypt called, file is NULL\n");
        return -1001;
    }
    if (NULL == remoteSip) {
        __android_log_print(ANDROID_LOG_ERROR,"JNI", "[ERROR HAIYUNTONG] haiyuntongFileEncrypt called, remoteSip is NULL\n");
        return -1002;
    }
    jbyte * byteFile = env->GetByteArrayElements(file, 0);
    int byteFileLen = env->GetArrayLength(file);
    const char * csip = env->GetStringUTFChars(remoteSip, 0);
    if (!byteFile) {
        __android_log_print(ANDROID_LOG_ERROR,"JNI", "[ERROR HAIYUNTONG] haiyuntongFileEncrypt called, file, failed to extract byteFile\n");
        return -1003;
    }
    int malloclen = byteFileLen*1.5+256;
    unsigned char *encrypted  = new unsigned char[malloclen];
    memset(encrypted,0,malloclen);
    long encryptedLen = 0;
    int ret = haiyuntongFileEncrypt((unsigned char *)byteFile, byteFileLen, (char *)csip, strlen(csip), encrypted, &encryptedLen);
    env->ReleaseByteArrayElements(file,byteFile,0);
    env->ReleaseStringUTFChars(remoteSip, csip);
    if (0 == ret && encryptedLen >0) {
        __android_log_print(ANDROID_LOG_DEBUG,"JNI", "[ERROR HAIYUNTONG] haiyuntongFileEncrypt succeed\n");
        env->SetByteArrayRegion(encryptedFile, 0, encryptedLen, (jbyte *)encrypted);
        delete [] encrypted;
        return encryptedLen;
    }
    else
    {
        delete [] encrypted;
        __android_log_print(ANDROID_LOG_ERROR,"JNI", "[ERROR HAIYUNTONG] haiyuntongFileEncrypt failed, ret:%d\n",ret);
        return -1004;
    }
}

/*
 * Class:     com_CCP_phone_NativeInterface
 * Method:    haiyuntongFileDecrypt
 * Signature: (Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;
 */
JNIEXPORT jint JNICALL Java_com_CCP_phone_NativeInterface_haiyuntongFileDecrypt
(JNIEnv *env, jclass, jbyteArray file, jstring remoteSip, jbyteArray decryptedFile)
{
    __android_log_print(ANDROID_LOG_DEBUG,"JNI", "[DEBUG HAIYUNTONG] haiyuntongFileDecrypt called\n");
    if (NULL == file) {
        __android_log_print(ANDROID_LOG_ERROR,"JNI", "[ERROR HAIYUNTONG] haiyuntongFileDecrypt called, file is NULL\n");
        return -1001;
    }
    if (NULL == remoteSip) {
        __android_log_print(ANDROID_LOG_ERROR,"JNI", "[ERROR HAIYUNTONG] haiyuntongFileDecrypt called, remoteSip is NULL\n");
        return -1002;
    }
    jbyte * byteFile = env->GetByteArrayElements(file, 0);
    int byteFileLen = env->GetArrayLength(file);
    const char * csip = env->GetStringUTFChars(remoteSip, 0);
    if (!byteFile) {
        __android_log_print(ANDROID_LOG_ERROR,"JNI", "[ERROR HAIYUNTONG] haiyuntongFileEncrypt called, file, failed to convert jstring to char\n");
        return -1003;
    }
    unsigned char *decrypted  = new unsigned char[byteFileLen];
    memset(decrypted,0,byteFileLen);
    long decryptedLen = 0;
    int ret = haiyuntongFileDecrypt((unsigned char *)byteFile, byteFileLen, (char *)csip, strlen(csip), decrypted, &decryptedLen);
    env->ReleaseByteArrayElements(file,byteFile,0);
    env->ReleaseStringUTFChars(remoteSip, csip);
    if (0 == ret && decryptedLen > 0) {
        __android_log_print(ANDROID_LOG_DEBUG,"JNI", "[ERROR HAIYUNTONG] haiyuntongFileDecrypt succeed\n");

        env->SetByteArrayRegion(decryptedFile, 0, decryptedLen, (jbyte*)decrypted);
        delete [] decrypted;
        return decryptedLen;
    }
    else
    {
        delete [] decrypted;
         __android_log_print(ANDROID_LOG_ERROR,"JNI", "[ERROR HAIYUNTONG] haiyuntongFileDecrypt failed, ret:%d\n",ret);
        return -1004;
    }
}

/*
 * Class:     com_CCP_phone_NativeInterface
 * Method:    haiyuntongGroupFileEncrypt
 * Signature: (Ljava/lang/String;[Ljava/lang/String;I)Ljava/lang/String;
 */
JNIEXPORT jint JNICALL Java_com_CCP_phone_NativeInterface_haiyuntongGroupFileEncrypt
(JNIEnv *env, jclass, jbyteArray file, jobjectArray userList, jint numOfUsers, jbyteArray encryptedFile)
{
    __android_log_print(ANDROID_LOG_DEBUG,"JNI", "[DEBUG HAIYUNTONG] haiyuntongGroupFileEncrypt called\n");
    if (NULL == file) {
        __android_log_print(ANDROID_LOG_ERROR,"JNI", "[ERROR HAIYUNTONG] haiyuntongGroupFileEncrypt called, file is NULL\n");
        return -1001;
    }
    if (NULL == userList) {
        __android_log_print(ANDROID_LOG_ERROR,"JNI", "[ERROR HAIYUNTONG] haiyuntongGroupFileEncrypt called, userList is NULL\n");
        return -1002;
    }
    jbyte * cfile = env->GetByteArrayElements(file, 0);
    int byteFileLen = env->GetArrayLength(file);
    if (!cfile) {
        __android_log_print(ANDROID_LOG_ERROR,"JNI", "[ERROR HAIYUNTONG] haiyuntongGroupFileEncrypt called,  file, failed to convert jstring to jbyte\n");
        -1003;
    }
    int size = env->GetArrayLength(userList);
    int mallocSize = 0;
    long *eachLen;
    eachLen = new long[size];
    char **userListInChar = new char*[size];
    int cursor = 0;
    for (int i = 0;i < size;i++)
    {
        jstring tempString = (jstring)env->GetObjectArrayElement(userList,i);
        const char *tempChar = env->GetStringUTFChars(tempString, 0);
        if(tempChar)
        {
            *(eachLen+i) = strlen(tempChar);
            userListInChar[cursor] = new char[*(eachLen+i)];
            memcpy(userListInChar[cursor], tempChar, *(eachLen+i));
            cursor++;
            env->ReleaseStringUTFChars(tempString, tempChar);
        }
    }

    unsigned char *encrypted = NULL;
    int malloclen = byteFileLen*1.5+256*numOfUsers;
    encrypted = new unsigned char[malloclen];
    if (!encrypted)
    {
        __android_log_print(ANDROID_LOG_ERROR,"JNI", "[ERROR HAIYUNTONG] haiyuntongGroupFileEncrypt called, malloc encrypted failed\n");
        return -1004;
    }
    memset(encrypted, 0, malloclen);
    long encryptedLen = 0;

    int ret = haiyuntongGroupFileEncrypt((const unsigned char *)cfile, byteFileLen, userListInChar, eachLen, size,encrypted, &encryptedLen);
    env->ReleaseByteArrayElements(file,cfile,0);
    for (int i = 0; i < cursor; i++)
    {
        delete [] userListInChar[i];
    }

    delete [] eachLen;
    delete [] userListInChar;
    if (0 == ret) {
        __android_log_print(ANDROID_LOG_DEBUG,"JNI", "[ERROR HAIYUNTONG] haiyuntongGroupFileEncrypt succeed\n");
        env->SetByteArrayRegion(encryptedFile, 0, encryptedLen, (jbyte*)encrypted);
        delete [] encrypted;
        return encryptedLen;
    }
    else
    {
        __android_log_print(ANDROID_LOG_ERROR,"JNI", "[ERROR HAIYUNTONG] haiyuntongGroupFileEncrypt failed, ret:%d\n",ret);
        delete [] encrypted;
        return -1005;
    }
}

/*
 * Class:     com_CCP_phone_NativeInterface
 * Method:    haiyuntongGroupFileDecrypt
 * Signature: (Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;
 */
JNIEXPORT jint JNICALL Java_com_CCP_phone_NativeInterface_haiyuntongGroupFileDecrypt
(JNIEnv *env, jclass, jbyteArray file, jstring selfSip, jbyteArray decryptedFile)
{
    __android_log_print(ANDROID_LOG_DEBUG,"JNI", "[DEBUG HAIYUNTONG] haiyuntongGroupFileDecrypt called\n");
    if (NULL == file) {
        __android_log_print(ANDROID_LOG_ERROR,"JNI", "[ERROR HAIYUNTONG] haiyuntongGroupFileDecrypt called, file is NULL\n");
        return -1001;
    }
    if (NULL == selfSip) {
        __android_log_print(ANDROID_LOG_ERROR,"JNI", "[ERROR HAIYUNTONG] haiyuntongGroupFileDecrypt called, selfSip is NULL\n");
        return -1002;
    }
    jbyte * cfile = env->GetByteArrayElements(file, 0);
    int byteFileLen = env->GetArrayLength(file);
    const char * cserver = env->GetStringUTFChars(selfSip, 0);
    if (NULL == cfile || NULL == cserver) {
        __android_log_print(ANDROID_LOG_ERROR,"JNI", "[ERROR HAIYUNTONG] haiyuntongGroupFileDecrypt called, error happens, return NULL\n");
        return -1003;
    }
    unsigned char *decrypted = new unsigned char[byteFileLen];
    if (!decrypted) {
        __android_log_print(ANDROID_LOG_ERROR,"JNI", "[ERROR HAIYUNTONG] haiyuntongGroupFileDecrypt called, error happens,length = %d, return NULL\n",byteFileLen);
        env->ReleaseByteArrayElements(file,cfile,0);
        env->ReleaseStringUTFChars(selfSip, cserver);
        return -1004;
    }
    memset(decrypted, 0, byteFileLen);
    long decryptedLen = 0;
    int ret = haiyuntongGroupFileDecrypt((const unsigned char *)cfile, byteFileLen, (char *)cserver, strlen(cserver), decrypted, &decryptedLen);
    env->ReleaseByteArrayElements(file,cfile,0);
    env->ReleaseStringUTFChars(selfSip, cserver);
    if (0 == ret) {
        __android_log_print(ANDROID_LOG_DEBUG,"JNI", "[ERROR HAIYUNTONG] haiyuntongGroupFileDecrypt succeed\n");
        env->SetByteArrayRegion(decryptedFile, 0, decryptedLen, (jbyte*)decrypted);
        delete decrypted;
        return decryptedLen;
    }
    else
    {
        __android_log_print(ANDROID_LOG_ERROR,"JNI", "[ERROR HAIYUNTONG] haiyuntongGroupFileDecrypt failed, ret:%d\n",ret);
        return -1005;
    }
}

/*
 * Class:     com_CCP_phone_NativeInterface
 * Method:    haiyuntongAddContact
 * Signature: (Ljava/lang/String;)I
 */
/*JNIEXPORT jint JNICALL Java_com_CCP_phone_NativeInterface_haiyuntongAddContact
(JNIEnv *env, jclass, jstring remoteSip)
{
    __android_log_print(ANDROID_LOG_ERROR,"JNI", "[DEBUG HAIYUNTONG] haiyuntongAddContact called\n");

    if (NULL == remoteSip) {
        __android_log_print(ANDROID_LOG_ERROR,"JNI", "[ERROR HAIYUNTONG] haiyuntongAddContact called, remoteSip is NULL\n");
        return -1002;
    }
    const char * csip = env->GetStringUTFChars(remoteSip, 0);

    int ret = haiyuntongAddContact((char *)csip, strlen(csip));
    env->ReleaseStringUTFChars(remoteSip, csip);
    if (0 == ret) {
        __android_log_print(ANDROID_LOG_ERROR,"JNI", "[ERROR HAIYUNTONG] haiyuntongAddContact succeed\n");
    }
    else
    {
        __android_log_print(ANDROID_LOG_ERROR,"JNI", "[ERROR HAIYUNTONG] haiyuntongAddContact failed, ret:%d\n",ret);
    }
    return ret;
}*/

JNIEXPORT jint JNICALL Java_com_CCP_phone_NativeInterface_haiyuntongAddContact
(JNIEnv *env, jclass, jobjectArray userList, jint numOfUsers)
{

    if (NULL == userList) {
        __android_log_print(ANDROID_LOG_ERROR,"JNI", "[ERROR HAIYUNTONG] haiyuntongAddContact called, userList is NULL\n");
        return -1002;
    }

    int size = env->GetArrayLength(userList);
    int mallocSize = 0;
    long *eachLen;
    eachLen = new long[size];
    char **userListInChar = new char*[size];
    int cursor = 0;
    for (int i = 0;i < size;i++)
    {
        jstring tempString = (jstring)env->GetObjectArrayElement(userList,i);
        const char *tempChar = env->GetStringUTFChars(tempString, 0);

        if(tempChar)
        {
            __android_log_print(ANDROID_LOG_ERROR,"JNI", "seansean [ERROR HAIYUNTONG] haiyuntongAddContact called, current sip:%s, length=%d\n",tempChar,strlen(tempChar));

            *(eachLen+i) = strlen(tempChar);
            userListInChar[cursor] = new char[*(eachLen+i)];
            memcpy(userListInChar[cursor], tempChar, *(eachLen+i));
            cursor++;
            env->ReleaseStringUTFChars(tempString, tempChar);
        }
    }


    for (int ii=0; ii<size; ii++) {
        __android_log_print(ANDROID_LOG_ERROR,"JNI", "seansean [ERROR HAIYUNTONG] haiyuntongAddContact called, current cursor:%d, length=%ld\n",ii,*(eachLen+ii));
    }


    int ret = haiyuntongAddContact( userListInChar, eachLen, size);

    for (int i = 0; i < cursor; i++)
    {
        delete [] userListInChar[i];
    }

    delete [] eachLen;
    delete [] userListInChar;
    if (0 == ret) {
    	__android_log_print(ANDROID_LOG_DEBUG,"JNI", "[ERROR HAIYUNTONG] haiyuntongAddContact succeed\n");
    }
    else
    {
        __android_log_print(ANDROID_LOG_ERROR,"JNI", "[ERROR HAIYUNTONG] haiyuntongAddContact failed, ret:%d\n",ret);
    }
    return ret;
}


/*
 * Class:     com_CCP_phone_NativeInterface
 * Method:    haiyuntongDelContact
 * Signature: (Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_CCP_phone_NativeInterface_haiyuntongDelContact
(JNIEnv *env, jclass, jstring remoteSip)
{
    __android_log_print(ANDROID_LOG_DEBUG,"JNI", "[DEBUG HAIYUNTONG] haiyuntongDelContact called\n");

    if (NULL == remoteSip) {
        __android_log_print(ANDROID_LOG_ERROR,"JNI", "[ERROR HAIYUNTONG] haiyuntongDelContact called, remoteSip is NULL\n");
        return -1002;
    }
    const char * csip = env->GetStringUTFChars(remoteSip, 0);

    int ret = haiyuntongDelContact((char *)csip, strlen(csip));
    env->ReleaseStringUTFChars(remoteSip, csip);
    if (0 == ret) {
        __android_log_print(ANDROID_LOG_DEBUG,"JNI", "[ERROR HAIYUNTONG] haiyuntongDelContact succeed\n");
    }
    else
    {
        __android_log_print(ANDROID_LOG_ERROR,"JNI", "[ERROR HAIYUNTONG] haiyuntongDelContact failed, ret:%d\n",ret);
    }
    return ret;
}

JNIEXPORT jint JNICALL Java_com_CCP_phone_NativeInterface_setNackEnabled
  (JNIEnv *env, jclass, jboolean audioEnabled, jboolean videoEnabled)
{
	return setNackEnabled(audioEnabled, videoEnabled);

}

JNIEXPORT jint JNICALL Java_com_CCP_phone_NativeInterface_isExistOfCert
(JNIEnv *env, jclass, jstring sip)
{
    const char * csip = env->GetStringUTFChars(sip, 0);
    if (!csip) {
        __android_log_print(ANDROID_LOG_ERROR,"JNI", "[ERROR HAIYUNTONG] isExistOfCert called, sip, failed to convert jstring to char\n");
        return -1001;
    }
    int ret = haiyuntongIsExistCert(csip, strlen(csip));
    env->ReleaseStringUTFChars(sip,csip);
    return ret;
}

JNIEXPORT jint JNICALL Java_com_CCP_phone_NativeInterface_openTraceFile
(JNIEnv *env, jclass, jstring filePath)
{
 #if 0
    const char *cfilePath = env->GetStringUTFChars(filePath, 0);
    if (!cfilePath) {
        __android_log_print(ANDROID_LOG_ERROR,"JNI", "[ERROR] isExistOfCert openTraceFile, filePath, failed to convert jstring to char\n");
    }

    int ret = openTraceFile(cfilePath);
    env->ReleaseStringUTFChars(filePath, cfilePath);
    return ret;
 #endif
 return 0;
}

JNIEXPORT jint JNICALL Java_com_CCP_phone_NativeInterface_openTraceFile2
(JNIEnv *env, jclass, jstring filePath)
{
#if 0
    const char *cfilePath = env->GetStringUTFChars(filePath, 0);
    if (!cfilePath) {
        __android_log_print(ANDROID_LOG_ERROR,"JNI", "[ERROR] isExistOfCert openTraceFile, filePath, failed to convert jstring to char\n");
    }

    int ret = openTraceFile2(cfilePath);
    env->ReleaseStringUTFChars(filePath, cfilePath);
    return ret;
#endif
    return 0;
}

JNIEXPORT jint JNICALL Java_com_CCP_phone_NativeInterface_setReconnectFlag
(JNIEnv *, jclass, jboolean flag)
{
    return setReconnectFlag(flag);
}


JNIEXPORT jint JNICALL Java_com_CCP_phone_NativeInterface_startRecord
(JNIEnv *env, jclass, jstring callid)
{
    if (NULL == callid) {
		return -1;
	}
	const char* ccallid = env->GetStringUTFChars(callid, 0);
	int ret = StartRecord();
	env->ReleaseStringUTFChars(callid, ccallid);
    return ret;
}


JNIEXPORT jint JNICALL Java_com_CCP_phone_NativeInterface_stopRecord
(JNIEnv *env, jclass, jstring callid)
{
    if (NULL == callid) {
		return -1;
	}
	const char* ccallid = env->GetStringUTFChars(callid, 0);
	int ret = StopRecord();
	env->ReleaseStringUTFChars(callid, ccallid);
    return ret;
}

/*
 * record local video as mp4 file
 */
JNIEXPORT jint JNICALL Java_com_CCP_phone_NativeInterface_startRecordLocalMedia
(JNIEnv *env, jclass, jstring fileName, jobject localView) {
    jobject globalLocalObj = 0;
    if(localView) {
        globalLocalObj = env->NewGlobalRef(localView);
    }
    const char* mp4_filename = env->GetStringUTFChars(fileName, 0);
    return startRecordLocalMedia(mp4_filename, (void*)globalLocalObj);
}

JNIEXPORT void JNICALL Java_com_CCP_phone_NativeInterface_stopRecordLocalMedia() {
    stopRecordLocalMedia();
}

JNIEXPORT jint JNICALL Java_com_CCP_phone_NativeInterface_setScreenShareActivity
(JNIEnv *env, jclass, jstring callid, jobject activity)
{
    if (NULL == callid) {
		return -1;
	}

    jobject gActivity = env->NewGlobalRef(activity);
	const char* ccallid = env->GetStringUTFChars(callid, 0);
    int ret = setScreeShareActivity((char*)ccallid, (void*)gActivity);

     __android_log_print(ANDROID_LOG_DEBUG,"JNI", "DEBUG: d setScreenShareActivity callid:%s activity:%0x\n",
        ccallid, (void*)activity);
	env->ReleaseStringUTFChars(callid, ccallid);
    return ret;
}

/**********  ec live video api begin ***************/
JNIEXPORT void JNICALL Java_com_CCP_phone_NativeInterface_createLiveStream() {
  void * handle = createLiveStream();
}

JNIEXPORT jint JNICALL Java_com_CCP_phone_NativeInterface_configLiveVideoStream(JNIEnv *env, jclass, jint camera_index, jint fps, jint resolution, jboolean auto_br) {
  LiveVideoStreamConfig config;
  config._auto_bitrate = auto_br;
  config._camera_index = camera_index;
  config._fps = fps;
  config._resolution = EC_LiveVideoResolution(resolution);
  void * handle = createLiveStream();
  return configLiveVideoStream(handle, config);
}

JNIEXPORT jint JNICALL Java_com_CCP_phone_NativeInterface_selectCameraLiveStream(JNIEnv *env, jclass, jint camera_index) {
  void * handle = createLiveStream();
  return selectCameraLiveStream(handle, camera_index);
}

JNIEXPORT jint JNICALL Java_com_CCP_phone_NativeInterface_playLiveStream(JNIEnv *env, jclass, jstring live_url, jobject renderView) {
  const char* clive_url = env->GetStringUTFChars(live_url, 0);
  void * handle = createLiveStream();
  int ret = playLiveStream(handle, clive_url, NULL);
  env->ReleaseStringUTFChars(live_url, clive_url);
  return ret;
}

JNIEXPORT jint JNICALL Java_com_CCP_phone_NativeInterface_pushLiveStream(JNIEnv *env, jclass, jstring live_url, jobject renderView) {
  const char* clive_url = env->GetStringUTFChars(live_url, 0);
  void * handle = createLiveStream();
  int ret = pushLiveStream(handle, clive_url, NULL);
  env->ReleaseStringUTFChars(live_url, clive_url);
  return ret;
}
JNIEXPORT jint JNICALL Java_com_CCP_phone_NativeInterface_stopLiveStream() {
  void * handle = createLiveStream();
  stopLiveStream(handle);
}

JNIEXPORT void JNICALL Java_com_CCP_phone_NativeInterface_releaseLiveStream() {
  void * handle = createLiveStream();
  releaseLiveStream(handle);
}
/**********  ec live video api end ***************/
