#ifndef ECMEDIA_H
#define ECMEDIA_H

#ifdef WEBRTC_ANDROID
#include "jni.h"
//#define ECMEDIA_API JNIEXPORT
#define ECMEDIA_API __attribute__((visibility("default")))
#elif defined(WIN32)
#define ECMEDIA_API _declspec(dllexport)
#else
#define ECMEDIA_API
#endif

#include "ec_common_types.h"
#include "sdk_common.h"

// TODO： for test, remove later
#include <windows.h>
#include <wingdi.h>
#include <sstream>
#include <string>
#include <vector>

#ifdef __cplusplus
extern "C" {
#endif
/***************************************************************************/
/*** 函数名: 日志打印设置                                                ***/
/*** 功能:   获取媒体库实例                                              ***/
/*** 返回值: 类型  bool                                                  ***/
/*** 函数参数1: path                         const char*                 ***/
/*** 函数参数2: level(1:INFO,3:WARNNING,4:ERROR,5:NONE)         int      ***/
/***************************************************************************/
ECMEDIA_API bool ECMedia_set_trace(const char* path ,const int level);
/***************************************************************************/
/*** 函数名: 初始化                                                      ***/
/*** 功能:   获取媒体库实例                                              ***/
/*** 返回值: 类型  int                                                   ***/
/*** 函数参数: 无                                                        ***/
/***************************************************************************/
ECMEDIA_API int ECMedia_init();

/****************************************************************************/
/*** 函数名: 反初始化                                                     ***/
/*** 功能:   释放媒体库实例                                               ***/
/*** 返回值: 类型  int                                                   ***/
/*** 函数参数: 无                                                         ***/
/****************************************************************************/
ECMEDIA_API int ECMedia_uninit();

/****************************************************************************/
/*** 函数名: 生成媒体通道id                                               ***/
/*** 功能:   媒体库初始化并生成通道ID                                     ***/
/*** 返回值: 类型    bool   true  成功      false  失败                   ***/
/*** 函数参数: 名称  channel_id    类型   int                             ***/
/****************************************************************************/
ECMEDIA_API bool ECMedia_generate_channel_id(int& channel_id);

/****************************************************************************/
/*** 函数名: 释放媒体通道id                                               ***/
/*** 功能:   释放通道ID                                     ***/
/*** 返回值: 类型    bool   true  成功      false  失败                   ***/
/*** 函数参数: 名称  channel_id    类型   int                             ***/
/****************************************************************************/
ECMEDIA_API bool ECMedia_release_channel_id(int channel_id);

/****************************************************************************/
/*** 函数名: 创建传输                                                     ***/
/*** 功能:   创建媒体流传输                                               ***/
/*** 返回值: 类型  bool    true  成功      false  失败                    ***/
/*** 函数参数1: 名称    l_addr本地地址       类型   const char*           ***/
/*** 函数参数2: 名称    l_port本地端口       类型   int                   ***/
/*** 函数参数3: 名称    r_addr远端地址       类型   const char*           ***/
/*** 函数参数4: 名称    r_port远端端口       类型   int                   ***/
/*** 函数参数5: 名称    id 传输ID            类型   const char*           ***/
/****************************************************************************/
ECMEDIA_API bool ECMedia_create_transport(const char* l_addr,
                                          int l_port,
                                          const char* r_addr,
                                          int r_port,
                                          const char* id);

/****************************************************************************/
/*** 函数名: 创建通道                                                     ***/
/*** 功能:   媒体库创建逻辑通道                                           ***/
/*** 返回值: 类型  bool   true  成功      false  失败                     ***/
/*** 函数参数1: 名称    tid                  类型   char*                 ***/
/*** 函数参数2: 名称    channel_id           类型   int                   ***/
/*** 函数参数3: 名称    is_video             类型   bool                  ***/
/****************************************************************************/
ECMEDIA_API bool ECMedia_create_channel(const char* tid, int& channel_id, bool is_video= true);



ECMEDIA_API bool ECMedia_create_video_channel(const char* tid, int& channel_id);


ECMEDIA_API bool ECMedia_create_voice_channel(const char* tid, int& channel_id);

/****************************************************************************/
/*** 函数名: 释放通道                                                     ***/
/*** 功能:   媒体库释放逻辑通道                                           ***/
/*** 返回值: 类型  void                                                   ***/
/*** 函数参数1: 名称    channel_id           类型   int                   ***/
/*** 函数参数2: 名称    is_video             类型   bool                  ***/
/****************************************************************************/
ECMEDIA_API void ECMedia_destroy_channel( int& channel_id,bool is_video = true);

/****************************************************************************/
/*** 函数名: 开始通道                                                     ***/
/*** 功能:   开始channel_id逻辑                                           ***/
/*** 返回值: 类型  bool     true  成功         false  失败                ***/
/*** 函数参数1: 名称    channel_id           类型   int                   ***/
/*** 函数参数2: 名称    is_video             类型   bool                  ***/
/****************************************************************************/
ECMEDIA_API bool ECMedia_start_channel(int channel_id);

/****************************************************************************/
/*** 函数名: 停止通道                                                     ***/
/*** 功能:   停止channel_id逻辑                                           ***/
/*** 返回值: 类型  bool   true  成功      false  失败                     ***/
/*** 函数参数1: 名称    channel_id           类型   int                   ***/
/*** 函数参数2: 名称    is_video             类型   bool                  ***/
/****************************************************************************/
ECMEDIA_API bool ECMedia_stop_channel(int channel_id, bool is_video = true);

/****************************************************************************/
/*** 函数名: ECMedia_delete_channel                                       ***/
/*** 功能:   删除通道                                                     ***/
/*** 返回值: 类型  int        0   成功   其他  失败                       ***/
/*** 函数参数1: 类型  int        peer_id                                  ***/
/***未实现                       ***/
/****************************************************************************/
ECMEDIA_API int ECMedia_delete_channel(int peer_id);

/****************************************************************************/
/*** 函数名: 增加本地渲染窗口                                             ***/
/*** 功能:   增加本地视频渲染窗口                                         ***/
/*** 返回值: 类型  bool    true  成功      false  失败                    ***/
/*** 函数参数1: 名称    channel_id           类型   int                   ***/
/*** 函数参数2: 名称    video_window         类型   void*                 ***/
/****************************************************************************/
ECMEDIA_API bool ECMedia_add_local_render(int chanel_id, void* video_window);

/****************************************************************************/
/*** 函数名: 增加远端渲染窗口                                             ***/
/*** 功能:   增加远端接收视频渲染窗口                                     ***/
/*** 返回值: 类型  bool    true  成功      false  失败                    ***/
/*** 函数参数1: 名称    channel_id           类型   int                   ***/
/*** 函数参数2: 名称    video_window         类型   void*                 ***/
/****************************************************************************/
ECMEDIA_API bool ECMedia_add_remote_render(int peer_id, void* video_window);

/****************************************************************************/
/*** 函数名: 设置本地视频流ssrc                                           ***/
/*** 功能:   设置通道channel_id本地媒体流ssrc                             ***/
/*** 返回值: 类型  bool    true  成功      false  失败                    ***/
/*** 函数参数1: 名称    channel_id           类型   int                   ***/
/*** 函数参数2: 名称    ssrc                 类型   unsigned int          ***/
/****************************************************************************/
ECMEDIA_API bool ECMedia_video_set_local_ssrc(int channel_id, unsigned int ssrc);


/****************************************************************************/
/*** 函数名: 设置远端视频流ssrc                                           ***/
/*** 功能:   设置通道channel_id远端媒体流ssrc                             ***/
/*** 返回值: 类型  bool    true  成功      false  失败                    ***/
/*** 函数参数1: 名称    channel_id           类型   int                   ***/
/*** 函数参数2: 名称    ssrc                 类型   unsigned int          ***/
/****************************************************************************/
ECMEDIA_API bool ECMedia_video_set_remote_ssrc(int channel_id, unsigned int ssrc);

/****************************************************************************/
/*** 函数名: 设置本地音频流ssrc                                           ***/
/*** 功能:   设置通道channel_id远端媒体流ssrc                             ***/
/*** 返回值: 类型  bool    true  成功      false  失败                    ***/
/*** 函数参数1: 名称    channel_id           类型   int                   ***/
/*** 函数参数2: 名称    ssrc                 类型   unsigned int          ***/
/****************************************************************************/
ECMEDIA_API bool ECMedia_audio_set_local_ssrc(int channel_id, unsigned int ssrc);

/****************************************************************************/
/*** 函数名: 设置远端音频流ssrc                                           ***/
/*** 功能:   设置通道channel_id远端媒体流ssrc                             ***/
/*** 返回值: 类型  bool    true  成功      false  失败                    ***/
/*** 函数参数1: 名称    channel_id           类型   int                   ***/
/*** 函数参数2: 名称    ssrc                 类型   unsigned int          ***/
/****************************************************************************/
ECMEDIA_API bool ECMedia_audio_set_remote_ssrc(int channel_id, unsigned int ssrc);

/****************************************************************************/
/*** 函数名: 创建本地音频track                                            ***/
/*** 功能:   创建本地音频源track                                          ***/
/*** 返回值: 类型  void*    返回创建的track指针                           ***/
/*** 函数参数1: 名称    track_id             类型   const char            ***/
/*** 函数参数2: 名称    voice_index          类型   int                   ***/
/****************************************************************************/
ECMEDIA_API void* ECMedia_create_audio_track(const char* track_id, int voice_index=0);

/****************************************************************************/
/*** 函数名: 释放本地音频track                                            ***/
/*** 功能:   释放本地音频源track                                          ***/
/*** 返回值: 类型  void                                                   ***/
/*** 函数参数1: 名称    track               类型   void*                  ***/
/****************************************************************************/
ECMEDIA_API void ECMedia_destroy_audio_track(void* track);

/****************************************************************************/
/*** 函数名: 创建本地视频track                                            ***/
/*** 功能:   创建本地视频源track                                          ***/
/*** 返回值: 类型  void*    返回创建的track指针                           ***/
/*** 函数参数1: 名称    video_mode           类型   int                   ***/
/*** 函数参数2: 名称    track_id             类型   const char            ***/
/*** 函数参数3: 名称    camera_index         类型   int                   ***/
/****************************************************************************/
//ECMEDIA_API void* ECMedia_create_video_track(int video_mode, const char* track_id, int camera_index);
ECMEDIA_API void* ECMedia_create_video_track(const char* track_params);

/****************************************************************************/
/*** 函数名: 释放本地视频track                                            ***/
/*** 功能:   释放本地视频源track                                          ***/
/*** 返回值: 类型  void                                                   ***/
/*** 函数参数1: 名称    track               类型   void*                  ***/
/****************************************************************************/
ECMEDIA_API void ECMedia_destroy_video_track(void* track);

/****************************************************************************/
/*** 函数名: 预览本地视频track                                            ***/
/*** 功能:   开启预览本地视频源track                                      ***/
/*** 返回值: 类型  bool    true  成功      false  失败                    ***/
/*** 函数参数1: 名称    window_id           类型   int                    ***/
/*** 函数参数2: 名称    track         类型   void*                  ***/
/****************************************************************************/
ECMEDIA_API bool ECMedia_preview_video_track(int window_id, void* track);

/****************************************************************************/
/*** 函数名: 选择视频源                                                   ***/
/*** 功能:   绑定视频源track与逻辑通道channel_id                          ***/
/*** 返回值: 类型  bool    true  成功      false  失败                    ***/ 
/*** 函数参数1: 名称    tid                  类型   const char*           ***/
/*** 函数参数2: 名称    channel_id           类型   int                   ***/
/*** 函数参数3: 名称    track_id             类型   const char*           ***/
/*** 函数参数4: 名称    video_track          类型   void*                 ***/
/*** 函数参数5: 名称    stream_ids           类型 std::vector<std::string>***/
/****************************************************************************/
ECMEDIA_API bool ECMedia_select_video_source(
    const char* tid,
    int channelid,
    const char* track_id,
    void* video_track,
    const char* stream_ids);


/****************************************************************************/
/*** 函数名: 选择音频源                                                   ***/
/*** 功能:   绑定音频源track与逻辑通道channel_id                          ***/
/*** 返回值: 类型  bool    true  成功      false  失败                    ***/
/*** 函数参数1: 名称    tid                  类型   const char*           ***/
/*** 函数参数2: 名称    channel_id           类型   int                   ***/
/*** 函数参数3: 名称    track_id             类型   const char*           ***/
/*** 函数参数4: 名称    audio_track          类型   void*                 ***/
/*** 函数参数5: 名称    stream_ids           类型 std::vector<std::string>***/
/****************************************************************************/
ECMEDIA_API bool ECMedia_select_audio_source(const char* tid, int channelid, const char* track_id, void* audio_track, const std::vector<std::string>& stream_ids);

/****************************************************************************/
/*** 函数名: 停止所有链接                                                 ***/
/*** 功能:   停止传输及所有通道                                           ***/
/*** 返回值: 类型  bool    true  成功      false  失败                    ***/
/*** 函数参数: 无                                                         ***/
/****************************************************************************/
ECMEDIA_API bool ECMedia_stop_connect();


/****************************************************************************/
/*** 函数名: 设置本地静音                                                 ***/
/*** 功能:   将本地音频源静音                                             ***/
/*** 返回值: 类型  bool    true  成功      false  失败                    ***/
/*** 函数参数1: 名称    channel_id            类型    int                 ***/
/*** 函数参数2: 名称    bMute                 类型    bool                 ***/
/****************************************************************************/
ECMEDIA_API bool ECMedia_set_local_audio_mute(int channel_id, bool bMute);

/****************************************************************************/
/*** 函数名: 设置远端静音                                                 ***/
/*** 功能:   将远端音频静音                                               ***/
/*** 返回值: 类型  bool    true  成功      false  失败                    ***/
/*** 函数参数1: 名称    channel_id            类型    int                 ***/
/*** 函数参数2: 名称    bMute                 类型    bool                ***/
/****************************************************************************/
ECMEDIA_API bool ECMedia_set_remote_audio_mute(int channel_id, bool bMute);

/****************************************************************************/
/*** 函数名: ECMedia_request_remote_ssrc                                  ***/
/*** 功能:   请求远端ssrc                                                 ***/
/*** 返回值: 类型  bool        true   成功   false  失败                  ***/
/*** 函数参数1: 类型  int       channel_id                                ***/
/*** 函数参数2: 类型  int       ssrc                                      ***/
/****************************************************************************/
ECMEDIA_API bool ECMedia_request_remote_ssrc(int channel_id, int ssrc);

/****************************************************************************/
/*** 函数名: 获取视频编码                                                 ***/
/*** 功能:   获取媒体库支持的视频编码格式                                 ***/
/*** 返回值: 类型  bool    true  成功      false  失败                    ***/
/*** 函数参数1: 名称   jsonVideoCodecInfos    类型    char*               ***/
/*** 函数参数2: 名称   length                 类型    int                 ***/
/****************************************************************************/
ECMEDIA_API bool ECMedia_get_video_codecs(char* jsonVideoCodecInfos,int* length);

/****************************************************************************/
/*** 函数名: 获取音频编码                                                 ***/
/*** 功能:   获取媒体库支持的音频编码格式                                 ***/
/*** 返回值: 类型  bool    true  成功      false  失败                    ***/
/*** 函数参数1: 名称   jsonAudioCodecInfos    类型    char*               ***/
/*** 函数参数2: 名称   length                 类型    int                 ***/
/****************************************************************************/
ECMEDIA_API bool ECMedia_get_audio_codecs(char* jsonAudioCodecInfos,int* length);

/****************************************************************************/
/*** 函数名: 设置视频NACK状态                                             ***/
/*** 功能:   设置视频是否使用NACK重传机制                                 ***/
/*** 返回值: 类型  bool    true  成功      false  失败                    ***/
/*** 函数参数1: 名称   channelId              类型    int                 ***/
/*** 函数参数2: 名称   enable_nack            类型    bool                ***/
/****************************************************************************/
ECMEDIA_API bool ECMedia_set_video_nack_status(const int channelId, const bool enable_nack);

/****************************************************************************/
/*** 函数名: 设置视频的Fec功能                                            ***/
/*** 功能:   开启或关闭视频Fec功能                                        ***/
/*** 返回值: 类型  bool    true  成功      false  失败                    ***/
/*** 函数参数1: 名称    channel_id       类型   int                       ***/
/*** 函数参数2: 名称    enable    	   类型   bool                      ***/
/*** 函数参数3: 名称    payloadtype_red  类型   uint8_t                   ***/
/*** 函数参数4: 名称    payloadtype_fec  类型   uint8_t                   ***/
/****************************************************************************/
ECMEDIA_API bool ECMedia_set_video_ulpfec_status(const int channelId,
                          const bool enable,
                          const uint8_t payloadtype_red,
                          const uint8_t payloadtype_fec);

/****************************************************************************/
/*** 函数名: 设置视频的DegradationMode功能                                ***/
/*** 功能:   开启或关闭视频DegradationMode功能                            ***/
/*** 返回值: 类型  bool    true  成功      false  失败                    ***/
/*** 函数参数1: 名称    channel_id       类型   int                       ***/
/*** 函数参数2: 名称    mode      	   类型   DegradationPreference       ***/
/****************************************************************************/
ECMEDIA_API bool ECMedia_set_video_degradation_mode(const int channelId,
                                                    int mode);

/****************************************************************************/
/*** 函数名: 关键帧发送                                                   ***/
/*** 功能:   发送关键帧                                                   ***/
/*** 返回值: 类型  bool    true  成功      false  失败                    ***/
/*** 函数参数1: 名称    channel_id       类型   int                       ***/
/****************************************************************************/
ECMEDIA_API bool ECMedia_send_key_frame(const int channelId);

/****************************************************************************/
/*** 函数名: 设置关键帧回调请求                                           ***/
/*** 功能:   设置请求关键帧回调                                           ***/
/*** 返回值: 类型  bool    true  成功      false  失败                    ***/
/*** 函数参数1: 名称    channel_id       类型   int                       ***/
/*** 函数参数1: 名称    cb        类型  OnRequestKeyFrameCallback函数指针 ***/
/****************************************************************************/
ECMEDIA_API bool ECMedia_set_key_frame_request_callback(const int channelId,
                                                        void* cb);

/****************************************************************************/
/*** 函数名: 设置回声消除                                                 ***/
/*** 功能:   音频通话回音消除                                             ***/
/*** 返回值: 类型  bool    true  成功      false  失败                    ***/
/*** 函数参数1: 名称    enable       类型   bool                          ***/
/*** 此函数需要在CreateChannel之前使用                                    ***/
/****************************************************************************/
ECMEDIA_API bool ECMedia_set_aec(bool enable);

/****************************************************************************/
/*** 函数名: 设置语音自动增益功能                                         ***/
/*** 功能:   语音自动增益                                                 ***/
/*** 返回值: 类型  bool    true  成功      false  失败                    ***/
/*** 函数参数1: 名称    enable       类型   bool                          ***/
/*** 此函数需要在CreateChannel之前使用                                    ***/
/****************************************************************************/
ECMEDIA_API bool ECMedia_set_agc(bool enable);

/****************************************************************************/
/*** 函数名: 设置语音噪声抑制功能                                         ***/
/*** 功能:   语音噪声抑制                                                 ***/
/*** 返回值: 类型  bool    true  成功      false  失败                    ***/
/*** 函数参数1: 名称    enable       类型   bool                          ***/
/*** 此函数需要在CreateChannel之前使用                                    ***/
/****************************************************************************/
ECMEDIA_API bool ECMedia_set_ns(bool enable);

/****************************************************************************/
/*** 函数名: 创建音频设备对象                                             ***/
/*** 功能:   获取底层AudioDeviceModule对象                                ***/
/*** 返回值: 类型  void*                                                  ***/
/****************************************************************************/
ECMEDIA_API void* ECMedia_create_audio_device();


/****************************************************************************/
/*** 函数名: 设置录音音量                                                 ***/
/*** 功能:   设置录音设备录音音量                                         ***/
/*** 返回值: 类型  bool        true  成功      false   失败               ***/
/*** 函数参数1: 名称   vol                类型    uint32_t                ***/
/****************************************************************************/
ECMEDIA_API bool ECMedia_set_audio_recording_volume(uint32_t vol);

/****************************************************************************/
/*** 函数名: 获取录音设备列表                                              ***/
/*** 功能:   获取录音设备列表字符串                                        ***/
/*** 返回值: 类型  bool        true  成功      false   失败                ***/
/*** 函数参数1: 名称   json                类型    char*                   ***/
/*** 函数参数2: 名称   len                 类型    int*                    ***/
/*****************************************************************************/
ECMEDIA_API char* ECMedia_get_audio_device_list(int* len);

/****************************************************************************/
/*** 函数名: 设置录音设备                                                  ***/
/*** 功能:   根据索引选择需要使用的录音设备                                ***/
/*** 返回值: 类型  bool        true  成功      false   失败                ***/
/*** 函数参数1: 名称   i                   类型    int                     ***/
/*****************************************************************************/
ECMEDIA_API bool ECMedia_set_audio_recording_device(int index);

/****************************************************************************/
/*** 函数名: 设置播放设备                                                  ***/
/*** 功能:   根据索引选择需要使用的播放设备                                ***/
/*** 返回值: 类型  bool        true  成功      false   失败                ***/
/*** 函数参数1: 名称   index               类型    int                     ***/
/*****************************************************************************/
ECMEDIA_API bool ECMedia_set_audio_playout_device(int index);

/****************************************************************************/
/*** 函数名: 获取视频设备                                                 ***/
/*** 功能:   获取本地视频摄像头信息                                       ***/
/*** 返回值: 类型  bool    true  成功      false  失败                    ***/
/*** 函数参数1: 名称   devices                类型    char*               ***/
/*** 函数参数2: 名称   *len  devices内存长度   类型    int*               ***/
/****************************************************************************/
ECMEDIA_API bool ECMedia_get_video_devices(char* devices, int* len);

/****************************************************************************/
/*** 函数名: ECMdeia_num_of_capture_devices                               ***/
/*** 功能:   获取当前系统所有视频设备                                     ***/
/*** 返回值: 类型  int         视频设备个数                               ***/
/****************************************************************************/
ECMEDIA_API int ECMdeia_num_of_capture_devices();

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ECMEDIA_API int ECMedia_init_audio();
// ECMEDIA_API int ECMedia_uninit_audio();
// ECMEDIA_API int ECMedia_init_video();
// ECMEDIA_API int ECMedia_uninit_video();
//ECMEDIA_API int ECMedia_audio_create_channel(int& peer_id, bool is_video);
//ECMEDIA_API int ECMedia_delete_channel(int& peer_id, bool is_video);



/****************************************************************************/
/*** 函数名: ECMedia_get_capture_device                                   ***/
/*** 功能:   根据索引获取摄像头信息                                       ***/
/*** 返回值: 类型  int         0 成功      -1失败                         ***/
/*** 函数参数1: 名称   index                类型    int                   ***/
/*** 函数参数2: 名称   name                 类型    char*                 ***/
/*** 函数参数3: 名称   name_len             类型    int                   ***/
/*** 函数参数4: 名称   id                   类型    char*                 ***/
/*** 函数参数5: 名称   id_len               类型    int                   ***/
/****************************************************************************/
ECMEDIA_API int ECMedia_get_capture_device(int index,
                                           char* name,
                                           int name_len,
                                           char* id,
                                           int id_len);

/****************************************************************************/
/*** 函数名: ECMedia_num_of_capabilities                                  ***/
/*** 功能:   根据摄像头id获取摄像头能力个数                               ***/
/*** 返回值: 类型  int         摄像头能力个数     -1 失败                 ***/
/*** 函数参数1: 名称   id                   类型    char*                 ***/
/*** 函数参数2: 名称   id_len               类型    int                   ***/
/****************************************************************************/
ECMEDIA_API int ECMedia_num_of_capabilities(const char* id, int id_len);

/****************************************************************************/
/*** 函数名: ECMedia_get_capture_capability                               ***/
/*** 功能:   根据摄像头id和索引获取摄像头能力                             ***/
/*** 返回值: 类型  int         0      成功                                ***/
/*** 函数参数1: 名称   id                   类型    char*                 ***/
/*** 函数参数2: 名称   id_len               类型    int                   ***/
/*** 函数参数3: 名称   index                类型    int                   ***/
/*** 函数参数4: 名称   capability           类型   CameraCapability&      ***/
/****************************************************************************/
ECMEDIA_API int ECMedia_get_capture_capability(const char* id,
                                               int id_len,
                                               int index,
                                               CameraCapability& capability);

/****************************************************************************/
/*** 函数名: ECMedia_allocate_capture_device                              ***/
/*** 功能:   根据摄像头id获取分配摄像头                                   ***/
/*** 返回值: 类型  int        0  成功             -1 失败                 ***/
/*** 函数参数1: 名称   id                   类型    char*                 ***/
/*** 函数参数2: 名称   len                  类型    int                   ***/
/*** 函数参数3: 名称   deviceid             类型    int&                  ***/
/****************************************************************************/
ECMEDIA_API int ECMedia_allocate_capture_device(const char* id,
                                                int len,
                                                int& deviceid);

/****************************************************************************/
/*** 函数名: ECMedia_connect_capture_device                               ***/
/*** 功能:   连接摄像头                                                   ***/
/*** 返回值: 类型  int        0  成功             -1 失败                 ***/
/*** 函数参数1: 名称   deviceid             类型    int                   ***/
/*** 函数参数2: 名称   peer_id              类型    int                   ***/
/****************************************************************************/
ECMEDIA_API int ECMedia_connect_capture_device(int deviceid, int peer_id);

/****************************************************************************/
/*** 函数名: ECMedia_start_capture                                        ***/
/*** 功能:   开启摄像头                                                   ***/
/*** 返回值: 类型  int        0  成功             -1 失败                 ***/
/*** 函数参数1: 名称   deviceid             类型    int                   ***/
/*** 函数参数2: 名称   cam                  类型    CameraCapability      ***/
/****************************************************************************/
ECMEDIA_API int ECMedia_start_capture(int deviceid, CameraCapability cam);

/****************************************************************************/
/*** 函数名: ECMedia_stop_capture                                        ***/
/*** 功能:   关闭摄像头                                                   ***/
/*** 返回值: 类型  int        0  成功             -1 失败                 ***/
/*** 函数参数1: 名称   deviceid             类型    int                   ***/
/****************************************************************************/
ECMEDIA_API int ECMedia_stop_capture(int deviceid);

//ECMEDIA_API int ECMedia_set_local_video_window(int deviceid,
//                                               void* video_window);

// NOTE: add_render not only add render, but also start rendering as well.
// maybe we should change the add_xxx into start_xxx

/****************************************************************************/
/*** 函数名: ECMedia_stop_local_render                                    ***/
/*** 功能:   停止渲染本地视频                                             ***/
/*** 返回值: 类型  int        0  成功             -1 失败                 ***/
/*** 函数参数1: 名称   peer_id             类型    int                    ***/
/*** 函数参数2: 名称   deviceid            类型    int                    ***/
/****************************************************************************/
ECMEDIA_API int ECMedia_stop_local_render(int peer_id, int deviceid);

/****************************************************************************/
/*** 函数名: ECMedia_stop_remote_render                                   ***/
/*** 功能:   停止渲染远端视频                                             ***/
/*** 返回值: 类型  int        0  成功             -1 失败                 ***/
/*** 函数参数1: 名称   peer_id             类型    int                    ***/
/*** 函数参数2: 名称   deviceid            类型    int                    ***/
/****************************************************************************/
ECMEDIA_API int ECMedia_stop_remote_render(int peer_id, int deviceid);

// ECMEDIA_API int ECMedia_num_of_supported_codecs_video();

// ECMEDIA_API int ECMedia_num_of_supported_codecs_audio();

/****************************************************************************/
/*** 函数名: ECMedia_start_mic                                            ***/
/*** 功能:   开启麦克风                                                   ***/
/*** 返回值: 类型  int        0  成功             -1 失败                 ***/
/*** 函数参数1: 名称   peer_id             类型    int                    ***/
/*** 函数参数2: 名称   deviceid            类型    int                    ***/
/****************************************************************************/
ECMEDIA_API int ECMedia_start_mic(int peer_id, int deviceid);


/****************************************************************************/
/*** 函数名: ECMedia_start_sendrecv                                       ***/
/*** 功能:   开始接收发送                                                 ***/
/*** 返回值: 类型  int        0  成功             -1 失败                 ***/
/*** 函数参数1: 名称   peer_id             类型    int                    ***/
/****************************************************************************/
ECMEDIA_API int ECMedia_start_sendrecv(int peer_id);

/****************************************************************************/
/*** 函数名: ECMedia_video_start_receive                                  ***/
/*** 功能:   开始接收视频流                                               ***/
/*** 返回值: 类型  int        0  成功             -1 失败                 ***/
/*** 函数参数1: 名称   peer_id             类型    int                    ***/
/****************************************************************************/
/***未实现***/
ECMEDIA_API int ECMedia_video_start_receive(int peer_id);

/****************************************************************************/
/*** 函数名: ECMedia_video_stop_receive                                   ***/
/*** 功能:   停止接收视频流                                               ***/
/*** 返回值: 类型  int        0  成功             -1 失败                 ***/
/*** 函数参数1: 名称   peer_id             类型    int                    ***/
/****************************************************************************/
/***未实现***/
ECMEDIA_API int ECMedia_video_stop_receive(int peer_id);

/****************************************************************************/
/*** 函数名: ECMedia_video_start_send                                     ***/
/*** 功能:   开始发送视频流                                               ***/
/*** 返回值: 类型  int        0  成功             -1 失败                 ***/
/*** 函数参数1: 名称   peer_id             类型    int                    ***/
/****************************************************************************/
ECMEDIA_API int ECMedia_video_start_send(int peer_id);

/****************************************************************************/
/*** 函数名: ECMedia_video_stop_send                                     ***/
/*** 功能:   停止发送视频流                                               ***/
/*** 返回值: 类型  int        0  成功             -1 失败                 ***/
/*** 函数参数1: 名称   peer_id             类型    int                    ***/
/****************************************************************************/
ECMEDIA_API int ECMedia_video_stop_send(int peer_id);

/****************************************************************************/
/*** 函数名: ECMedia_audio_start_receive                                  ***/
/*** 功能:   开始接受音频流                                               ***/
/*** 返回值: 类型  int        0  成功             -1 失败                 ***/
/*** 函数参数1: 名称   peer_id             类型    int                    ***/
/****************************************************************************/
/***未实现***/
ECMEDIA_API int ECMedia_audio_start_receive(int peer_id);

/****************************************************************************/
/*** 函数名: ECMedia_audio_stop_receive                                  ***/
/*** 功能:   停止接受音频流                                               ***/
/*** 返回值: 类型  int        0  成功             -1 失败                 ***/
/*** 函数参数1: 名称   peer_id             类型    int                    ***/
/****************************************************************************/
/***未实现***/
ECMEDIA_API int ECMedia_audio_stop_receive(int peer_id);

/****************************************************************************/
/*** 函数名: ECMedia_audio_start_send                                     ***/
/*** 功能:   开始发送音频流                                               ***/
/*** 返回值: 类型  int        0  成功             -1 失败                 ***/
/*** 函数参数1: 名称   peer_id             类型    int                    ***/
/****************************************************************************/
/***未实现***/
ECMEDIA_API int ECMedia_audio_start_send(int peer_id);

/****************************************************************************/
/*** 函数名: ECMedia_audio_stop_send                                      ***/
/*** 功能:   停止发送音频流                                               ***/
/*** 返回值: 类型  int        0  成功             -1 失败                 ***/
/*** 函数参数1: 名称   peer_id             类型    int                    ***/
/****************************************************************************/
/***未实现***/
ECMEDIA_API int ECMedia_audio_stop_send(int peer_id);

/****************************************************************************/
/*** 函数名: ECMedia_start_connect                                        ***/
/*** 功能:   开始连接音频和视频通道                                       ***/
/*** 返回值: 类型  bool        true  成功           false 失败           ***/
/*** 函数参数1: 名称   audio_channel_id             类型    int           ***/
/*** 函数参数2: 名称   video_channel_id             类型    int           ***/
/****************************************************************************/
/***未实现***/
ECMEDIA_API bool ECMedia_start_connect(int audio_channel_id, int video_channel_id);

/****************************************************************************/
/*** 函数名: ECMedia_get_supported_codecs_video                               ***/
/*** 功能:   获取支持的视频编码参数                                           ***/
/*** 返回值: 类型  int        0  成功            -1 失败                     ***/
/*** 函数参数1: 名称：video_codecs  类型   std::vector<ecmedia::VideoCodec>*  ***/
/****************************************************************************/
ECMEDIA_API int ECMedia_get_supported_codecs_video(
    std::vector<ecmedia::VideoCodec>* video_codecs);

/****************************************************************************/
/*** 函数名: ECMedia_get_supported_codecs_audio                               ***/
/*** 功能:   获取支持的音频编码参数                                           ***/
/*** 返回值: 类型  int        0  成功            -1 失败                      ***/
/*** 函数参数1: 名称：audio_codecs  类型   std::vector<ecmedia::AudioCodec>*  ***/
/****************************************************************************/
ECMEDIA_API int ECMedia_get_supported_codecs_audio(
    std::vector<ecmedia::AudioCodec>* audio_codecs);

/****************************************************************************/
/*** 函数名: ECMedia_set_send_codec_video                                 ***/
/*** 功能:   设置发送端视频编码                                           ***/
/*** 返回值: 类型  int        0  成功            -1 失败                  ***/
/*** 函数参数1: 名称：peer_id       类型   ecmedia::int                   ***/
/*** 函数参数2: 名称：video_codecs  类型   ecmedia::VideoCodec*           ***/
/****************************************************************************/
ECMEDIA_API int ECMedia_set_send_codec_video(int peer_id,
                                             ecmedia::VideoCodec* video_codec);

/****************************************************************************/
/*** 函数名: ECMedia_set_receive_codec_video                              ***/
/*** 功能:   设置接收端视频编码                                           ***/
/*** 返回值: 类型  int        0  成功            -1 失败                  ***/
/*** 函数参数1: 名称：peer_id       类型   ecmedia::int                   ***/
/*** 函数参数2: 名称：video_codecs  类型   ecmedia::VideoCodec*           ***/
/****************************************************************************/
ECMEDIA_API int ECMedia_set_receive_codec_video(
    int peer_id,
    ecmedia::VideoCodec* video_codec);

/****************************************************************************/
/*** 函数名: ECMedia_set_send_codec_audio                                 ***/
/*** 功能:   设置发送端音频编码                                           ***/
/*** 返回值: 类型  int        0  成功            -1 失败                  ***/
/*** 函数参数1: 名称：peer_id       类型   ecmedia::int                   ***/
/*** 函数参数2: 名称：audio_codecs  类型   ecmedia::AudioCodec*           ***/
/****************************************************************************/
ECMEDIA_API int ECMedia_set_send_codec_audio(int peer_id,
                                             ecmedia::AudioCodec* audio_codec);

/****************************************************************************/
/*** 函数名: ECMedia_set_receive_codec_audio                              ***/
/*** 功能:   设置接收端音频编码                                           ***/
/*** 返回值: 类型  int        0  成功            -1 失败                  ***/
/*** 函数参数1: 名称：peer_id       类型   ecmedia::int                   ***/
/*** 函数参数2: 名称：audio_codecs  类型   ecmedia::AudioCodec*           ***/
/****************************************************************************/
ECMEDIA_API int ECMedia_set_receive_playloadType_audio(
    int peer_id,
    ecmedia::AudioCodec* audio_codec);

/****************************************************************************/
/*** 函数名: ECMedia_video_set_send_destination                           ***/
/*** 功能:   设置视频发送地址                                             ***/
/*** 返回值: 类型  int        0  成功            -1 失败                  ***/
/*** 函数参数1: 名称：peer_id       类型   ecmedia::int                   ***/
/*** 函数参数2: 名称：r_addr        类型   char*                          ***/
/*** 函数参数3: 名称：l_addr        类型   char*                          ***/
/*** 函数参数4: 名称：port          类型   int                            ***/
/****************************************************************************/
/*未实现*/
ECMEDIA_API int ECMedia_video_set_send_destination(int peer_id,
                                                   const char* r_addr,
                                                   const char* l_addr,
                                                   int port);

/****************************************************************************/
/*** 函数名: ECMedia_audio_set_send_destination                           ***/
/*** 功能:   设置音频发送地址                                             ***/
/*** 返回值: 类型  int        0  成功            -1 失败                  ***/
/*** 函数参数1: 名称：peer_id       类型   ecmedia::int                   ***/
/*** 函数参数2: 名称：r_addr        类型   char*                          ***/
/*** 函数参数3: 名称：l_addr        类型   char*                          ***/
/*** 函数参数4: 名称：port          类型   int                            ***/
/****************************************************************************/
/*未实现*/
ECMEDIA_API int ECMedia_audio_set_send_destination(int peer_id,
                                                   const char* r_addr,
                                                   const char* l_addr,
                                                   int port);
/****************************************************************************/
/*** 函数名: ECMedia_set_video_protect_mode                               ***/
/*** 功能:   设置视频保护模式                                             ***/
/*** 返回值: 类型  int        0  成功            -1 失败                  ***/
/*** 函数参数1: 名称：mode                 类型     int                   ***/
/****************************************************************************/
/*未实现*/
ECMEDIA_API int ECMedia_set_video_protect_mode(int mode);

#if defined(WEBRTC_ANDROID)
ECMEDIA_API bool ECMedia_SaveLocalVideoTrack(int channelId, void* track);
ECMEDIA_API void* ECMedia_GetLocalVideoTrackPtr(int channelId);
ECMEDIA_API bool ECMedia_RemoveLocalVideoTrack(int channelId);

ECMEDIA_API bool ECMedia_SaveRemoteVideoSink(int channelId,
	JNIEnv* env,
	jobject javaSink);
ECMEDIA_API bool ECMedia_RemoveRemoteVideoSink(int channelId);

ECMEDIA_API int ECMedia_InitializeJVM();
#endif

//ECMEDIA_API void ECMedia_add_tracks();
#ifdef __cplusplus
}
#endif

#endif
