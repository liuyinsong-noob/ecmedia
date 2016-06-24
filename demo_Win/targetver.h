#pragma once

// 包括 SDKDDKVer.h 将定义可用的最高版本的 Windows 平台。

// 如果要为以前的 Windows 平台生成应用程序，请包括 WinSDKVer.h，并将
// WIN32_WINNT 宏设置为要支持的平台，然后再包括 SDKDDKVer.h。

#include <SDKDDKVer.h>
#define WEBRTC_AUDIO_PROCESSING_OFF false

#define WEBRTC_VOICE_ENGINE_HP_DEFAULT_STATE true
    // AudioProcessing HP is ON
#define WEBRTC_VOICE_ENGINE_NS_DEFAULT_STATE  WEBRTC_AUDIO_PROCESSING_OFF
    // AudioProcessing NS off
#define WEBRTC_VOICE_ENGINE_AGC_DEFAULT_STATE true
    // AudioProcessing AGC on
#define WEBRTC_VOICE_ENGINE_EC_DEFAULT_STATE  WEBRTC_AUDIO_PROCESSING_OFF
    // AudioProcessing EC off
#define WEBRTC_VOICE_ENGINE_VAD_DEFAULT_STATE WEBRTC_AUDIO_PROCESSING_OFF
    // AudioProcessing off
#define WEBRTC_VOICE_ENGINE_RX_AGC_DEFAULT_STATE WEBRTC_AUDIO_PROCESSING_OFF
    // AudioProcessing RX AGC off
#define WEBRTC_VOICE_ENGINE_RX_NS_DEFAULT_STATE WEBRTC_AUDIO_PROCESSING_OFF
    // AudioProcessing RX NS off
#define WEBRTC_VOICE_ENGINE_RX_HP_DEFAULT_STATE WEBRTC_AUDIO_PROCESSING_OFF
    // AudioProcessing RX High Pass Filter off

#define WEBRTC_VOICE_ENGINE_NS_DEFAULT_MODE NoiseSuppression::kModerate
    // AudioProcessing NS moderate suppression
#define WEBRTC_VOICE_ENGINE_AGC_DEFAULT_MODE GainControl::kAdaptiveAnalog
    // AudioProcessing AGC analog digital combined
#define WEBRTC_VOICE_ENGINE_RX_AGC_DEFAULT_MODE GainControl::kAdaptiveDigital
    // AudioProcessing AGC mode
#define WEBRTC_VOICE_ENGINE_RX_NS_DEFAULT_MODE NoiseSuppression::kModerate
    // AudioProcessing RX NS mode
