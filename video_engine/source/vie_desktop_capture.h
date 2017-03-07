
#ifndef WEBRTC_VIDEO_DESKTOP_SHARE_H_
#define WEBRTC_VIDEO_DESKTOP_SHARE_H_

#include "vie_desktop_share.h"
#include "vie_frame_provider_base.h"
#include "common_types.h"
#include "typedefs.h"
#include "vie_defines.h"
#include "bitrate_controller.h"
#include "rtp_rtcp_defines.h"
#include "scoped_ptr.h"
#include "video_processing.h"
#include "desktop_capturer.h"
#include "screen_capturer.h"
#include "window_capturer.h"
#include "desktop_and_cursor_composer.h"
#include "vie_frame_provider_base.h"

namespace cloopenwebrtc {

class CriticalSectionWrapper;
class EventWrapper;
class ThreadWrapper;
class ViEEncoder;
class ViEEffectFilter;

class VieDesktopCapturer
    :public ViEFrameProviderBase
    ,public DesktopCapturer::Callback
    
{
public:
    VieDesktopCapturer(int id,int engine_id);
    virtual ~VieDesktopCapturer();
    //ViEFrameProviderBase
    int FrameCallbackChanged(){return 0;};
    virtual int RegisterFrameCallback(int observer_id, ViEFrameCallback* callbackObject);
    virtual int DeregisterFrameCallback(const ViEFrameCallback* callbackObject);
    bool IsFrameCallbackRegistered(const ViEFrameCallback* callbackObject);

    //DesktopCapturer::Callback
    virtual SharedMemory* CreateSharedMemory(size_t size) ;
    virtual void OnCaptureCompleted(DesktopFrame* frame, CaptureErrCode errCode = kCapture_Ok, DesktopRect *window_rect = NULL);

    static VieDesktopCapturer* CreateCapture(int capture_id,const DesktopShareType desktop_share_type,int engine_id);

    bool GetScreenList(ScreenList& screens);
    bool SelectScreen(ScreenId id);
	bool GetWindowList(WindowList& windows);
    bool SelectWindow(WindowId id);


    bool GetDesktopShareCaptureRect(int &width, int &height);

    int  StartDesktopShareCapture(const int fps);
    int  StopDesktopShareCapture();

    int EnableDenoising(const bool enable);

	int setCaptureErrCodeCb(int channelid, onDesktopCaptureErrCode errCodeCb);
	int setShareWindowChangeCb(int channelid, onDesktopShareFrameChange frameChangeCb);
    
    void SetScreenShareActivity(void * activity);
    
protected:
    // Thread functions for deliver desktop captured frames to receivers.
    static bool ViEDesktopCaptureThreadFunction(void* obj);
    bool ViEDesktopCaptureProcess();

    int SelectCapture(const DesktopShareType capture_type);
    void DeliverFrame();
    int CaptrueShareFrame(cloopenwebrtc::I420VideoFrame& video_frame);

    void CopyScreenList(ScreenCapturer::ScreenList &src_screen,ScreenList& dst_screen);
    void CopyWindowsList(WindowCapturer::WindowList &src_windows,WindowList& dst_windows);

    int CreateDesktopCapture();
    int CreateWindowCapture();

    // Help function used for keeping track of VideoImageProcesingModule.
    // Creates the module if it is needed, returns 0 on success and guarantees
    // that the image proc module exist.
    WebRtc_Word32 IncImageProcRefCount();
    WebRtc_Word32 DecImageProcRefCount();

    DesktopShareErrCode GetErrCode(CaptureErrCode errCode);

private:
    int     desktop_share_id_;
    int     last_width_;
    int     last_height_;

    scoped_ptr<CriticalSectionWrapper> observer_cs_;

    bool                                has_share_frame_;
    scoped_ptr<I420VideoFrame>          share_frame_;
    scoped_ptr<I420VideoFrame>          temp_frame_;
    int64_t                             last_capture_time_;

    scoped_ptr<CriticalSectionWrapper>  shared_capture_cs_;
    bool                                shared_capture_enable_;

    DesktopShareType                share_capture_type_;              
    MouseCursorMonitor*             screen_mouse_cursor_;
	MouseCursorMonitor*				screen_mouse_cursor_for_windows_;
    ScreenCapturer*                 screen_capturer_;  
    WindowCapturer*                 windows_capture_;

    scoped_ptr<DesktopAndCursorComposer> 		screen_mouse_blender_;
    scoped_ptr<DesktopAndCursorComposer> 		window_mouse_blender_;

    scoped_ptr<CriticalSectionWrapper> deliver_cs_;
    ViEEncoder*     vie_encoder_;
    VideoCodec      codec_;
    bool            denoising_enabled_;

    VideoProcessingModule*  image_proc_module_;
    int                     image_proc_module_ref_counter_;

    // Capture thread.
    ThreadWrapper&  desktop_capture_thread_;
    EventWrapper&   desktop_capture_event_;
    scoped_ptr<CriticalSectionWrapper> wait_time_cs_;
    int thread_wait_time_ms_; 

	onDesktopCaptureErrCode capture_err_code_cb_; 
	onDesktopShareFrameChange capture_frame_change_cb_;
	int callback_id_;
};

} //cloopenwebrtc


#endif // WEBRTC_VIDEO_DESKTOP_SHARE_H_
