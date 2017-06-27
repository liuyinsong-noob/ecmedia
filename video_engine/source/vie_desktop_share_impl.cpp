
#include "vie_desktop_share_impl.h" 
#include "../system_wrappers/include/trace.h"
#include "vie_errors.h"
#include "vie_capturer.h"
#include "vie_channel.h"
#include "vie_channel_manager.h"
#include "vie_defines.h"
#include "vie_encoder.h"
#include "vie_impl.h"
#include "vie_input_manager.h"
#include "vie_desktop_share_manager.h"
#include "vie_desktop_capture.h"
#include "vie_shared_data.h"
#include "engine_configurations.h"

namespace cloopenwebrtc {

ViEDesktopShare* ViEDesktopShare::GetInterface(VideoEngine* video_engine) 
{
#ifdef ENABLE_SCREEN_SHARE
#ifdef WEBRTC_VIDEO_ENGINE_DESKTOP_SHARE_API
        if (!video_engine) {
            return NULL;
        }
        VideoEngineImpl* vie_impl = static_cast<VideoEngineImpl*>(video_engine);
        ViEDesktopShareImpl* vie_capture_impl = vie_impl;

        // Increase ref count.
        (*vie_capture_impl)++;
        return vie_capture_impl;
#endif
#endif
        return NULL;
}

int ViEDesktopShareImpl::Release() {
    WEBRTC_TRACE(kTraceApiCall, kTraceVideo, shared_data_->instance_id(),
        "ViECapture::Release()");
    // Decrease ref count
    (*this)--;

    WebRtc_Word32 ref_count = GetCount();
    if (ref_count < 0) {
        WEBRTC_TRACE(kTraceWarning, kTraceVideo, shared_data_->instance_id(),
            "ViECapture release too many times");
        shared_data_->SetLastError(kViEAPIDoesNotExist);
        return -1;
    }
    WEBRTC_TRACE(kTraceInfo, kTraceVideo, shared_data_->instance_id(),
        "ViECapture reference count: %d", ref_count);
    return ref_count;
}

ViEDesktopShareImpl::ViEDesktopShareImpl(ViESharedData* shared_data)
    : shared_data_(shared_data) {
        WEBRTC_TRACE(kTraceMemory, kTraceVideo, shared_data_->instance_id(),
            "ViECaptureImpl::ViECaptureImpl() Ctor");
}

ViEDesktopShareImpl::~ViEDesktopShareImpl() {
    WEBRTC_TRACE(kTraceMemory, kTraceVideo, shared_data_->instance_id(),
        "ViECaptureImpl::~ViECaptureImpl() Dtor");
}


int ViEDesktopShareImpl::AllocateDesktopShareCapturer(int& desktop_capture_id, const DesktopShareType capture_type)
{
#ifdef ENABLE_SCREEN_SHARE
    if(shared_data_->desktop_share_manager()->CreateDesktopCapturer(desktop_capture_id,capture_type) !=0 )
    {
        WEBRTC_TRACE(kTraceError, kTraceVideo,
            ViEId(shared_data_->instance_id(), -1),
            "Can't create desktop capturer.");
        shared_data_->SetLastError(kViEDesktopShareInvalidChannelId);
        return -1;
    }
#endif
    return 0;

}

int  ViEDesktopShareImpl::ReleaseDesktopShareCapturer(const int desktop_capture_id)
{
#ifdef ENABLE_SCREEN_SHARE
    {
         ViEDesktopShareScoped ds(*(shared_data_->desktop_share_manager()));
        VieDesktopCapturer *desktop_capture_ = ds.DesktopCapture(desktop_capture_id);
        if(!desktop_capture_)
        {
            WEBRTC_TRACE(kTraceError, kTraceVideo,
                ViEId(shared_data_->instance_id(), desktop_capture_id),
                "Can't get desktop capturer.");
            shared_data_->SetLastError(kViEDesktopShareInvalidChannelId);
            return -1;
        }
    }

    return shared_data_->desktop_share_manager()->DestroyDesktopCapture(desktop_capture_id);
#else
	return 0;
#endif
}

bool ViEDesktopShareImpl::GetDesktopShareCaptureRect(const int desktop_capture_id, int &width, int &height)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceVideo,
        ViEId(shared_data_->instance_id(), desktop_capture_id),
        "%s(capture_id: %d)", __FUNCTION__,
        desktop_capture_id);
#ifdef ENABLE_SCREEN_SHARE
    ViEDesktopShareScoped is(*(shared_data_->desktop_share_manager()));
    VieDesktopCapturer* desktop_capture = is.DesktopCapture(desktop_capture_id);
    if (!desktop_capture) {
        WEBRTC_TRACE(kTraceError, kTraceVideo,
            ViEId(shared_data_->instance_id(), desktop_capture_id),
            "%s: Capture device %d doesn't exist", __FUNCTION__,
            desktop_capture_id);
        shared_data_->SetLastError(kViEDesktopShareDoesNotExist);
        return false;
    }
    return desktop_capture->GetDesktopShareCaptureRect(width, height);
#else
	return 0;
#endif
}

int ViEDesktopShareImpl::ConnectDesktopCaptureDevice(const int desktop_capture_id,const int video_channel)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceVideo,
        ViEId(shared_data_->instance_id(), video_channel),
        "%s(capture_id: %d, video_channel: %d)", __FUNCTION__,
        desktop_capture_id, video_channel);
#ifdef ENABLE_SCREEN_SHARE
    ViEDesktopShareScoped is(*(shared_data_->desktop_share_manager()));
    VieDesktopCapturer* desktop_capture = is.DesktopCapture(desktop_capture_id);
    if (!desktop_capture) {
        WEBRTC_TRACE(kTraceError, kTraceVideo,
            ViEId(shared_data_->instance_id(), video_channel),
            "%s: Capture device %d doesn't exist", __FUNCTION__,
            desktop_capture_id);
        shared_data_->SetLastError(kViECaptureDeviceDoesNotExist);
        return -1;
    }

    ViEChannelManagerScoped cs(*(shared_data_->channel_manager()));
    ViEEncoder* vie_encoder = cs.Encoder(video_channel);
    if (!vie_encoder) {
        WEBRTC_TRACE(kTraceError, kTraceVideo,
            ViEId(shared_data_->instance_id(), video_channel),
            "%s: Channel %d doesn't exist", __FUNCTION__,
            video_channel);
        shared_data_->SetLastError(kViECaptureDeviceInvalidChannelId);
        return -1;
    }
    if (vie_encoder->Owner() != video_channel) {
        WEBRTC_TRACE(kTraceError, kTraceVideo,
            ViEId(shared_data_->instance_id(), video_channel),
            "Can't connect capture device to a receive only channel.");
        shared_data_->SetLastError(kViECaptureDeviceInvalidChannelId);
        return -1;
    }
    //  Check if the encoder already has a connected frame provider
    if (is.FrameProvider(vie_encoder) != NULL) {
        WEBRTC_TRACE(kTraceError, kTraceVideo,
            ViEId(shared_data_->instance_id(), video_channel),
            "%s: Channel %d already connected to a capture device.",
            __FUNCTION__, video_channel);
        shared_data_->SetLastError(kViECaptureDeviceAlreadyConnected);
        return -1;
    }

    // If we don't use the camera as hardware encoder, we register the vie_encoder
    // for callbacks.
    if (desktop_capture->RegisterFrameCallback(video_channel, vie_encoder) != 0) {
            shared_data_->SetLastError(kViECaptureDeviceUnknownError);
            return -1;
    }
    return 0;
#else
	return 0;
#endif
}


int ViEDesktopShareImpl::DisConnectDesktopCaptureDevice(const int video_channel)
{
#ifdef ENABLE_SCREEN_SHARE
    WEBRTC_TRACE(kTraceApiCall, kTraceVideo,
        ViEId(shared_data_->instance_id(), video_channel),
        "%s(video_channel: %d)", __FUNCTION__, video_channel);

    ViEChannelManagerScoped cs(*(shared_data_->channel_manager()));
    ViEEncoder* vie_encoder = cs.Encoder(video_channel);
    if (!vie_encoder) {
        WEBRTC_TRACE(kTraceError, kTraceVideo,
            ViEId(shared_data_->instance_id()),
            "%s: Channel %d doesn't exist", __FUNCTION__,
            video_channel);
        shared_data_->SetLastError(kViECaptureDeviceInvalidChannelId);
        return -1;
    }

    ViEDesktopShareScoped is(*(shared_data_->desktop_share_manager()));
    ViEFrameProviderBase* frame_provider = is.FrameProvider(vie_encoder);
    if (!frame_provider) {
        WEBRTC_TRACE(kTraceWarning, kTraceVideo,
            ViEId(shared_data_->instance_id()),
            "%s: No capture device connected to channel %d",
            __FUNCTION__, video_channel);
        shared_data_->SetLastError(kViECaptureDeviceNotConnected);
        return -1;
    }
    if (frame_provider->Id() < kViEDesktopIdBase ||
        frame_provider->Id() > kViEDesktopIdMax) {
            WEBRTC_TRACE(kTraceWarning, kTraceVideo,
                ViEId(shared_data_->instance_id()),
                "%s: No capture device connected to channel %d",
                __FUNCTION__, video_channel);
            shared_data_->SetLastError(kViECaptureDeviceNotConnected);
            return -1;
    }

    if (frame_provider->DeregisterFrameCallback(vie_encoder) != 0) {
        shared_data_->SetLastError(kViECaptureDeviceUnknownError);
        return -1;
    }
#endif
    return 0;

}

int ViEDesktopShareImpl::NumberOfWindow(const int desktop_capture_id)
{
	WEBRTC_TRACE(kTraceApiCall, kTraceVideo,
		ViEId(shared_data_->instance_id(), desktop_capture_id),
		"%s(capture_id: %d)", __FUNCTION__,
		desktop_capture_id);
#ifdef ENABLE_SCREEN_SHARE
	ViEDesktopShareScoped is(*(shared_data_->desktop_share_manager()));
	VieDesktopCapturer* desktop_capture = is.DesktopCapture(desktop_capture_id);
	if (!desktop_capture) {
		WEBRTC_TRACE(kTraceError, kTraceVideo,
			ViEId(shared_data_->instance_id(), desktop_capture_id),
			"%s: Capture device %d doesn't exist", __FUNCTION__,
			desktop_capture_id);
		shared_data_->SetLastError(kViEDesktopShareDoesNotExist);
		return false;
	}
	WindowList windows;
	windows.clear();
	if (!desktop_capture->GetWindowList(windows))
	{
		WEBRTC_TRACE(kTraceError, kTraceVideo,
			ViEId(shared_data_->instance_id(), desktop_capture_id),
			"%s: get window size err %d doesn't exist", __FUNCTION__,
			desktop_capture_id);
		shared_data_->SetLastError(kViEDesktopShareDoesNotExist);
		return false;
	}
    return windows.size();
#else
	return 0;
#endif
}

int ViEDesktopShareImpl::NumberOfScreen(const int desktop_capture_id)
{
	WEBRTC_TRACE(kTraceApiCall, kTraceVideo,
		ViEId(shared_data_->instance_id(), desktop_capture_id),
		"%s(capture_id: %d)", __FUNCTION__,
		desktop_capture_id);
#ifdef ENABLE_SCREEN_SHARE
	ViEDesktopShareScoped is(*(shared_data_->desktop_share_manager()));
	VieDesktopCapturer* desktop_capture = is.DesktopCapture(desktop_capture_id);
	if (!desktop_capture) {
		WEBRTC_TRACE(kTraceError, kTraceVideo,
			ViEId(shared_data_->instance_id(), desktop_capture_id),
			"%s: Capture device %d doesn't exist", __FUNCTION__,
			desktop_capture_id);
		shared_data_->SetLastError(kViEDesktopShareDoesNotExist);
		return false;
	}
	ScreenList screens;
	screens.clear();
	if (!desktop_capture->GetScreenList(screens))
	{
		WEBRTC_TRACE(kTraceError, kTraceVideo,
			ViEId(shared_data_->instance_id(), desktop_capture_id),
			"%s: get screens size err %d doesn't exist", __FUNCTION__,
			desktop_capture_id);
		shared_data_->SetLastError(kViEDesktopShareDoesNotExist);
		return false;
	}
	return screens.size();
#else
	return 0;
#endif
}


bool ViEDesktopShareImpl::GetScreenList(const int desktop_capture_id,ScreenList& screens)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceVideo,
        ViEId(shared_data_->instance_id(), desktop_capture_id),
        "%s(capture_id: %d)", __FUNCTION__,
        desktop_capture_id);
#ifdef ENABLE_SCREEN_SHARE
    ViEDesktopShareScoped is(*(shared_data_->desktop_share_manager()));
    VieDesktopCapturer* desktop_capture = is.DesktopCapture(desktop_capture_id);
    if (!desktop_capture) {
        WEBRTC_TRACE(kTraceError, kTraceVideo,
            ViEId(shared_data_->instance_id(), desktop_capture_id),
            "%s: Capture device %d doesn't exist", __FUNCTION__,
            desktop_capture_id);
        shared_data_->SetLastError(kViEDesktopShareDoesNotExist);
        return false;
    }
    return desktop_capture->GetScreenList(screens);
#else
	return 0;
#endif
}

bool ViEDesktopShareImpl::SelectScreen(const int desktop_capture_id, const ScreenId id) 
{
    WEBRTC_TRACE(kTraceApiCall, kTraceVideo,
        ViEId(shared_data_->instance_id(), desktop_capture_id),
        "%s(capture_id: %d)", __FUNCTION__,
        desktop_capture_id);
#ifdef ENABLE_SCREEN_SHARE
    ViEDesktopShareScoped is(*(shared_data_->desktop_share_manager()));
    VieDesktopCapturer* desktop_capture = is.DesktopCapture(desktop_capture_id);
    if (!desktop_capture) {
        WEBRTC_TRACE(kTraceError, kTraceVideo,
            ViEId(shared_data_->instance_id(), desktop_capture_id),
            "%s: Capture device %d doesn't exist", __FUNCTION__,
            desktop_capture_id);
        shared_data_->SetLastError(kViEDesktopShareDoesNotExist);
        return false;
    }
    return desktop_capture->SelectScreen(id);
#else
	return 0;
#endif
}

bool ViEDesktopShareImpl::GetWindowList(const int desktop_capture_id,WindowList& windows)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceVideo,
        ViEId(shared_data_->instance_id(), desktop_capture_id),
        "%s(capture_id: %d)", __FUNCTION__,
        desktop_capture_id);
#ifdef ENABLE_SCREEN_SHARE
    ViEDesktopShareScoped is(*(shared_data_->desktop_share_manager()));
    VieDesktopCapturer* desktop_capture = is.DesktopCapture(desktop_capture_id);
    if (!desktop_capture) {
        WEBRTC_TRACE(kTraceError, kTraceVideo,
            ViEId(shared_data_->instance_id(), desktop_capture_id),
            "%s: Capture device %d doesn't exist", __FUNCTION__,
            desktop_capture_id);
        shared_data_->SetLastError(kViEDesktopShareDoesNotExist);
        return false;
    }
    return desktop_capture->GetWindowList(windows);
#else
	return 0;
#endif
}

bool ViEDesktopShareImpl::SelectWindow(const int desktop_capture_id,const WindowId id) 
{
    WEBRTC_TRACE(kTraceApiCall, kTraceVideo,
        ViEId(shared_data_->instance_id(), desktop_capture_id),
        "%s(capture_id: %d)", __FUNCTION__,
        desktop_capture_id);
#ifdef ENABLE_SCREEN_SHARE
    ViEDesktopShareScoped is(*(shared_data_->desktop_share_manager()));
    VieDesktopCapturer* desktop_capture = is.DesktopCapture(desktop_capture_id);
    if (!desktop_capture) {
        WEBRTC_TRACE(kTraceError, kTraceVideo,
            ViEId(shared_data_->instance_id(), desktop_capture_id),
            "%s: Capture device %d doesn't exist", __FUNCTION__,
            desktop_capture_id);
        shared_data_->SetLastError(kViEDesktopShareDoesNotExist);
        return false;
    }
    return desktop_capture->SelectWindow(id);
#else
	return 0;
#endif
}

int ViEDesktopShareImpl::StartDesktopShareCapture(const int desktop_capture_id, const int fps)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceVideo,
        ViEId(shared_data_->instance_id(), desktop_capture_id),
        "%s(capture_id: %d )", __FUNCTION__,
        desktop_capture_id);
#ifdef ENABLE_SCREEN_SHARE
    ViEDesktopShareScoped is(*(shared_data_->desktop_share_manager()));
    VieDesktopCapturer* desktop_capture = is.DesktopCapture(desktop_capture_id);
    if (!desktop_capture) {
        WEBRTC_TRACE(kTraceError, kTraceVideo,
            ViEId(shared_data_->instance_id(), desktop_capture_id),
            "%s: Capture device %d doesn't exist", __FUNCTION__,
            desktop_capture_id);
        shared_data_->SetLastError(kViEDesktopShareDoesNotExist);
        return false;
    }

    return desktop_capture->StartDesktopShareCapture(fps);
#else
	return 0;
#endif
}

int ViEDesktopShareImpl::StopDesktopShareCapture(const int desktop_capture_id)
{
 #ifdef ENABLE_SCREEN_SHARE
    WEBRTC_TRACE(kTraceApiCall, kTraceVideo,
        ViEId(shared_data_->instance_id(), desktop_capture_id),
        "%s(capture_id: %d )", __FUNCTION__,
        desktop_capture_id);

    ViEDesktopShareScoped is(*(shared_data_->desktop_share_manager()));
    VieDesktopCapturer* desktop_capture = is.DesktopCapture(desktop_capture_id);
    if (!desktop_capture) {
        WEBRTC_TRACE(kTraceError, kTraceVideo,
            ViEId(shared_data_->instance_id(), desktop_capture_id),
            "%s: Capture device %d doesn't exist", __FUNCTION__,
            desktop_capture_id);
        shared_data_->SetLastError(kViEDesktopShareDoesNotExist);
        return false;
    }

    return desktop_capture->StopDesktopShareCapture();
#else
	return 0;
#endif
}

int ViEDesktopShareImpl::setCaptureErrCb(int desktop_capture_id, int channelid, onDesktopCaptureErrCode capture_err_code_cb)
{
#ifdef ENABLE_SCREEN_SHARE
	ViEDesktopShareScoped is(*(shared_data_->desktop_share_manager()));
	VieDesktopCapturer* vie_desktop_capture = is.DesktopCapture(desktop_capture_id);
	if (!vie_desktop_capture) {
		WEBRTC_TRACE(kTraceError, kTraceVideo,
			ViEId(shared_data_->instance_id(), desktop_capture_id),
			"%s: Capture device %d doesn't exist", __FUNCTION__,
			desktop_capture_id);
		shared_data_->SetLastError(kViEDesktopShareDoesNotExist);
		return -1;
	}
	if (vie_desktop_capture->setCaptureErrCodeCb(channelid, capture_err_code_cb) != 0) {
		shared_data_->SetLastError(kViEDesktopShareUnknownError);
		return -1;
	}
#endif
	return 0;
}
int ViEDesktopShareImpl::setShareWindowChangeCb(int desktop_capture_id, int channelid, onDesktopShareFrameChange capture_frame_change_cb)
{
#ifdef ENABLE_SCREEN_SHARE
	ViEDesktopShareScoped is(*(shared_data_->desktop_share_manager()));
	VieDesktopCapturer* vie_desktop_capture = is.DesktopCapture(desktop_capture_id);
	if (!vie_desktop_capture) {
		WEBRTC_TRACE(kTraceError, kTraceVideo,
			ViEId(shared_data_->instance_id(), desktop_capture_id),
			"%s: Capture device %d doesn't exist", __FUNCTION__,
			desktop_capture_id);
		shared_data_->SetLastError(kViEDesktopShareDoesNotExist);
		return -1;
	}
	if (vie_desktop_capture->setShareWindowChangeCb(channelid,capture_frame_change_cb) != 0) {
		shared_data_->SetLastError(kViEDesktopShareUnknownError);
		return -1;
	}
#endif
	return 0;
}


int ViEDesktopShareImpl::SetScreenShareActivity(int desktop_capture_id, void * activity)
{
#ifdef ENABLE_SCREEN_SHARE
	ViEDesktopShareScoped is(*(shared_data_->desktop_share_manager()));
	VieDesktopCapturer* vie_desktop_capture = is.DesktopCapture(desktop_capture_id);
	if (!vie_desktop_capture) {
		WEBRTC_TRACE(kTraceError, kTraceVideo,
			ViEId(shared_data_->instance_id(), desktop_capture_id),
			"%s: Capture device %d doesn't exist", __FUNCTION__,
			desktop_capture_id);
		shared_data_->SetLastError(kViEDesktopShareDoesNotExist);
		return -1;
	}
    vie_desktop_capture->SetScreenShareActivity(activity); 
#endif
    return 0;
}

} //cloopenwebrtc 
