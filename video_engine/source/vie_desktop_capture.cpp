#include "vie_desktop_capture.h"
#include "desktop_capture_options.h"
#include "cropping_window_capturer.h"
#include "mouse_cursor_monitor.h"
#include "screen_capturer.h"
#include "desktop_frame.h"
#include "shared_memory.h"
#ifdef WIN32
#include "screen_capturer_win_gdi.h"
#endif
#include "desktop_capturer.h"
#include "../common_video/source/libyuv/include/webrtc_libyuv.h"
#include "../system_wrappers/include/trace.h"
#include "../system_wrappers/include/event_wrapper.h"
#include "../system_wrappers/include/thread_wrapper.h"
#include "../system_wrappers/include/tick_util.h"
#include "vie_encoder.h"
//#if __APPLE__
//#include "window_capturer_ios.h"
//#endif
#ifdef WEBRTC_MAC
#include "window_capturer.h"
#include "screen_capturer.h"
#endif
#ifdef WEBRTC_IOS
#include "window_capturer_ios.h"
#endif

namespace yuntongxunwebrtc {

VieDesktopCapturer::VieDesktopCapturer(int id,int engine_id):
    ViEFrameProviderBase(id, engine_id),
    desktop_share_id_(id),
    last_width_(0),
    last_height_(0),
    observer_cs_(CriticalSectionWrapper::CreateCriticalSection()),
    has_share_frame_(false),
    shared_capture_cs_(CriticalSectionWrapper::CreateCriticalSection()),
    shared_capture_enable_(false),
    share_capture_type_(ShareNone),
    screen_capturer_(NULL),
#ifdef _WIN32
    screen_mouse_cursor_(MouseCursorMonitor::CreateForScreen(DesktopCaptureOptions::CreateDefault(),
    yuntongxunwebrtc::kFullDesktopScreenId)),
	screen_mouse_cursor_for_windows_(MouseCursorMonitor::CreateForScreen(DesktopCaptureOptions::CreateDefault(),
		yuntongxunwebrtc::kFullDesktopScreenId)),
#elif WEBRTC_MAC
    screen_mouse_cursor_(MouseCursorMonitor::CreateForScreen(DesktopCaptureOptions::CreateDefault(),
                                                             yuntongxunwebrtc::kFullDesktopScreenId)),
    screen_mouse_cursor_for_windows_(MouseCursorMonitor::CreateForScreen(DesktopCaptureOptions::CreateDefault(),
                                                                         yuntongxunwebrtc::kFullDesktopScreenId)),
#else
    screen_mouse_cursor_(NULL),
#endif
    windows_capture_(NULL),
    screen_mouse_blender_(),
    window_mouse_blender_(),
    deliver_cs_(CriticalSectionWrapper::CreateCriticalSection()),
    vie_encoder_(NULL),
    denoising_enabled_(false),
    image_proc_module_ref_counter_(0),
    image_proc_module_(NULL),
    desktop_capture_thread_(*ThreadWrapper::CreateThread(ViEDesktopCaptureThreadFunction,
    this, kRealtimePriority,
    "ViEDesktopCaptureThread")),
    desktop_capture_event_(*EventTimerWrapper::Create()),
    thread_wait_time_ms_(100),
    wait_time_cs_(CriticalSectionWrapper::CreateCriticalSection()),
	capture_err_code_cb_(NULL), 
	capture_frame_change_cb_(NULL)
{
	share_frame_.reset(new I420VideoFrame);
	temp_frame_.reset(new I420VideoFrame);
}

VieDesktopCapturer::~VieDesktopCapturer()
{
    // Stop the thread.
    shared_capture_cs_->Enter();
    desktop_capture_thread_.SetNotAlive();
    desktop_capture_event_.StopTimer();
    shared_capture_enable_ = false;
    shared_capture_cs_->Leave();

    if (desktop_capture_thread_.Stop()) {
        // Thread stopped.
        delete &desktop_capture_thread_;
    } else {
        assert(false);
        WEBRTC_TRACE(kTraceMemory, kTraceVideoRenderer,
            -1,
            "%s: Not able to stop capture thread for device %d, leaking",
            __FUNCTION__, desktop_share_id_);
    }

    delete &desktop_capture_event_;
}


SharedMemory* VieDesktopCapturer::CreateSharedMemory( size_t size )
{
    return new FakeSharedMemory(new char[size], size);
}

void VieDesktopCapturer::OnCaptureCompleted(DesktopFrame* frame, CaptureErrCode errCode, DesktopRect *window_rect/* = NULL*/ )
{
    if (frame == NULL || errCode != kCapture_Ok)
    {
        has_share_frame_ = false;
        delete frame;
        frame = NULL;
        CriticalSectionScoped cs(observer_cs_.get());
		if(capture_err_code_cb_ != NULL)
		{
			capture_err_code_cb_(callback_id_, errCode);
		}

        return;
    }

    // Set the capture time
    share_frame_->set_render_time_ms(TickTime::MillisecondTimestamp());
    if (share_frame_->render_time_ms() == last_capture_time_) 
    {
        // We don't allow the same capture time for two frames, drop this one.
        has_share_frame_ = false;
        delete frame;
        frame = NULL;
        return;
    }

    int width = frame->size().width();
    int height = frame->size().height();
    //---begin
	int stride_y = width;
	int stride_uv = (width + 1) / 2;
	int target_width = width;
	int target_height = height;
	int ret = share_frame_->CreateEmptyFrame(target_width,
		abs(target_height),
		stride_y,
		stride_uv, stride_uv);
	if (ret < 0)
	{
		WEBRTC_TRACE(yuntongxunwebrtc::kTraceError, yuntongxunwebrtc::kTraceVideo,
			-1, "Failed to create empty frame, this should only happen due to bad parameters.");
		return ;
	}
	// Set the capture time
	share_frame_->set_render_time_ms(TickTime::MillisecondTimestamp());
    
    
    VideoType src_video_type;
#if defined(_WIN32)
    src_video_type = kARGB;
#elif defined(WEBRTC_ANDROID)
    /*
     * Android java 层直接向底层传入I420数据，旧版使用 ARGB565. zhaoyou
     */
    src_video_type = kI420;
#elif defined(WEBRTC_MAC)
    src_video_type = kARGB;
#else
    src_video_type = kMJPG;
#endif
    const int conversionResult = ConvertToI420(src_video_type,
		frame->data(),
		0, 0,  // No cropping
		width, height,
		CalcBufferSize(src_video_type, width, height),
		kVideoRotation_0,
		share_frame_.get());
    
	if (conversionResult < 0)
	{
		WEBRTC_TRACE(yuntongxunwebrtc::kTraceError, yuntongxunwebrtc::kTraceVideo,
			-1,"Failed to convert capture frame from type kARGB to I420.");
        delete frame;
        frame = NULL;
        return ;
	}
	//---end
    has_share_frame_ = true;  

    if (last_height_ != height || last_width_ != width)
    {
        last_width_ = width;
        last_height_ = height;
        CriticalSectionScoped cs(observer_cs_.get());
		if(capture_frame_change_cb_ != NULL)
		{
			capture_frame_change_cb_(callback_id_, width, height);
		}
    }
    last_capture_time_ = share_frame_->render_time_ms();
    delete frame;
    frame = NULL;
}

int VieDesktopCapturer::SelectCapture(const DesktopShareType capture_type)
{
    int ret = -1;
    share_capture_type_ = capture_type;
    switch(capture_type)
    {
        case ShareScreen:
            CreateWindowCapture();
            ret = CreateDesktopCapture();
            break;
//#ifdef _WIN32
        case ShareWindow:
            CreateDesktopCapture();
            ret = CreateWindowCapture();
            break;
//#endif
        default:
            break;
    }
    
    return ret ;
    
}
    
    bool VieDesktopCapturer::GetScreenList( ScreenList& screens )
{
//#ifdef _WIN32
    if(screen_capturer_ == NULL )
    {
        WEBRTC_TRACE(yuntongxunwebrtc::kTraceError, yuntongxunwebrtc::kTraceVideo,
                     -1, "%s: screen_capturer_ not exist ", __FUNCTION__);
        return false;
    }
    
    ScreenCapturer::ScreenList screen_list;
    screen_capturer_->GetScreenList(&screen_list);
    
    CopyScreenList(screen_list,screens);
    return true;
//#endif
//    return false;
}

bool VieDesktopCapturer::SelectScreen( ScreenId id )
{
//#ifdef _WIN32
    if(screen_capturer_ == NULL )
    {
        WEBRTC_TRACE(yuntongxunwebrtc::kTraceError, yuntongxunwebrtc::kTraceVideo,
                     -1, "SelectScreen failed, screen_capturer not exist ");
        return false;
    }
    share_capture_type_ = ShareScreen;
    return screen_capturer_->SelectScreen(id);
//#endif
//    return false;
}
    
    bool VieDesktopCapturer::GetWindowList( WindowList& windows )
{
//#ifdef _WIN32
    if(windows_capture_ == NULL)
    {
        WEBRTC_TRACE(yuntongxunwebrtc::kTraceError, yuntongxunwebrtc::kTraceVideo,
                     -1, "%s: windows_capture_ not exist ", __FUNCTION__);
        return false;
    }
    WindowCapturer::WindowList window_list;
    windows_capture_->GetWindowList(&window_list);
    
    CopyWindowsList(window_list,windows);
    return true;
//#endif
//    return false;
}
    
    bool VieDesktopCapturer::SelectWindow( WindowId id )
{
//#ifdef _WIN32
    if(windows_capture_ == NULL)
    {
        WEBRTC_TRACE(yuntongxunwebrtc::kTraceError, yuntongxunwebrtc::kTraceVideo,
                     -1,"SelectWindow failed, screen_capturer not exist ");
        return false;
    }
    if(!windows_capture_->SelectWindow(id))
    {
        WEBRTC_TRACE(yuntongxunwebrtc::kTraceError, yuntongxunwebrtc::kTraceVideo,
                     -1,"SelectWindow err ");
        return false;
    }
    if(!windows_capture_->BringSelectedWindowToFront())
    {
        WEBRTC_TRACE(yuntongxunwebrtc::kTraceError, yuntongxunwebrtc::kTraceVideo,
                     -1,"BringSelectedWindowToFront err ");
        return false;
    }
    share_capture_type_ = ShareWindow;
    return true;
//#endif
//    return false;
}
    
    
    bool VieDesktopCapturer::GetDesktopShareCaptureRect( int &width, int &height )
{
#ifndef __APPLE__
    if(share_capture_type_ ==  ShareScreen)
    {
        return screen_capturer_->GetShareCaptureRect(width, height);
    }
//#ifdef _WIN32
    else if (share_capture_type_ == ShareWindow)
    {
        return windows_capture_->GetShareCaptureRect(width, height);
    }
//#endif // _WIN32
#endif // __APPLE__
    return false;
    
}
    
int VieDesktopCapturer::CaptrueShareFrame( yuntongxunwebrtc::I420VideoFrame& video_frame )
{
#ifdef _WIN32
    switch(share_capture_type_)
    {
        case ShareScreen:
        {
            screen_mouse_blender_->Capture(DesktopRegion());
        }
            break;
        case ShareWindow:
        {
            window_mouse_blender_->Capture(DesktopRegion());
        }
            break;
        default:
            break;
    }
#endif
    
#ifdef WEBRTC_ANDROID
    screen_capturer_->Capture(DesktopRegion());
#endif
    
#ifdef WEBRTC_MAC
    switch(share_capture_type_)
    {
        case ShareScreen:
        {
            screen_mouse_blender_->Capture(DesktopRegion());
        }
            break;
        case ShareWindow:
        {
            window_mouse_blender_->Capture(DesktopRegion());
        }
            break;
        default:
            break;
    }
#endif
#ifdef WEBRTC_IOS
    screen_capturer_->Capture(DesktopRegion());
#endif

    if(!has_share_frame_) {
        return -1;
    }
    video_frame.SwapFrame(share_frame_.get());
    return 0;
}

int VieDesktopCapturer::StartDesktopShareCapture(const int fps)
{
    {
        CriticalSectionScoped cs(shared_capture_cs_.get());
        if(shared_capture_enable_)
        {
            WEBRTC_TRACE(yuntongxunwebrtc::kTraceError, yuntongxunwebrtc::kTraceVideo,
                -1,"share capture already enable ");
            return -1;
        }
        shared_capture_enable_ = true;
    }

    CriticalSectionScoped cs(wait_time_cs_.get());
    thread_wait_time_ms_ = 1000/fps;
	desktop_capture_event_.StopTimer();
	desktop_capture_event_.StartTimer(true,thread_wait_time_ms_);

    unsigned int t_id = 0;
    if (desktop_capture_thread_.Start(t_id)) {
        WEBRTC_TRACE(kTraceInfo, kTraceVideo, -1,"%s: thread started: %u", __FUNCTION__, t_id);
    } else {
        assert(false);
    }
    return 0;
}

int VieDesktopCapturer::StopDesktopShareCapture()
{
    CriticalSectionScoped cs(shared_capture_cs_.get());
    if(!shared_capture_enable_)
    {
        WEBRTC_TRACE(yuntongxunwebrtc::kTraceError, yuntongxunwebrtc::kTraceVideo,
            -1,"share capture already stop ");
        return -1;
    }
	desktop_capture_thread_.SetNotAlive();
    shared_capture_enable_ = false;

#if defined _WIN32 || WEBRTC_MAC
	if (share_capture_type_ == ShareScreen) {
		screen_mouse_blender_->ResetScreenDC();
	}
#endif

    return 0;
}

VieDesktopCapturer* VieDesktopCapturer::CreateCapture( int capture_id, const DesktopShareType desktop_share_type,int engine_id)
{
    VieDesktopCapturer* capture = new VieDesktopCapturer(capture_id,engine_id);
    if (capture == NULL)
    {
        WEBRTC_TRACE(yuntongxunwebrtc::kTraceError, yuntongxunwebrtc::kTraceVideo,
            -1,"CreateCapture err ");
        return NULL;
    }
    if(capture->SelectCapture(desktop_share_type) != 0)
    {
        WEBRTC_TRACE(yuntongxunwebrtc::kTraceError, yuntongxunwebrtc::kTraceVideo,
            -1,"CreateCaSelectCapturepture err ");
        return NULL;
    }
    return capture;
}

int VieDesktopCapturer::CreateDesktopCapture()
{
    
#ifdef _WIN32
    if(screen_mouse_cursor_ == NULL)
    {
        WEBRTC_TRACE(yuntongxunwebrtc::kTraceError, yuntongxunwebrtc::kTraceVideo,
            -1,"CreateWindowCapture  get screen_mouse_cursor_  ERR ");
        return -1;
    }
    screen_capturer_ = ScreenCapturer::Create();
    if(screen_capturer_ == NULL)
    {
        WEBRTC_TRACE(yuntongxunwebrtc::kTraceError, yuntongxunwebrtc::kTraceVideo,
            -1,"ScreenCapturer::Create() ERR ");
        return -1;
    }
    screen_mouse_blender_.reset(new DesktopAndCursorComposer(screen_capturer_,screen_mouse_cursor_));
    if (screen_mouse_blender_.get() == NULL)
    {
        WEBRTC_TRACE(yuntongxunwebrtc::kTraceError, yuntongxunwebrtc::kTraceVideo,
            -1,"CreateDesktopCapture get  screen_mouse_blender_ ERR ");
        return -1;
    }
    screen_mouse_blender_->Start(this);
#endif
#ifdef WEBRTC_MAC
    if(screen_mouse_cursor_ == NULL){
        WEBRTC_TRACE(yuntongxunwebrtc::kTraceError, yuntongxunwebrtc::kTraceVideo,
                     -1,"CreateWindowCapture  get screen_mouse_cursor_  ERR ");
        return -1;
    }
    screen_capturer_ = ScreenCapturer::Create();
    if(screen_capturer_ == NULL){
        WEBRTC_TRACE(yuntongxunwebrtc::kTraceError, yuntongxunwebrtc::kTraceVideo, -1,"ScreenCapturer::Create() ERR ");
        return -1;
    }
    screen_mouse_blender_.reset(new DesktopAndCursorComposer(screen_capturer_,screen_mouse_cursor_));
    if (screen_mouse_blender_.get() == NULL){
        WEBRTC_TRACE(yuntongxunwebrtc::kTraceError, yuntongxunwebrtc::kTraceVideo, -1,"CreateDesktopCapture get  screen_mouse_blender_ ERR ");
        return -1;
    }
    screen_mouse_blender_->Start(this);
//    screen_capturer_ = ScreenCapturer::Create();
//    screen_capturer_->Start(this);
#endif
#ifdef WEBRTC_IOS
    screen_capturer_ = ScreenCapturer::Create();
    screen_capturer_->Start(this);
#endif
#ifdef WEBRTC_ANDROID
    WEBRTC_TRACE(yuntongxunwebrtc::kTraceInfo, yuntongxunwebrtc::kTraceVideo,
                 -1,"CreateWindowCapture Android");
    screen_capturer_ = ScreenCapturer::Create();
    screen_capturer_->Start(this);
#endif
    
    return 0;
}

int VieDesktopCapturer::CreateWindowCapture()
{
#ifdef _WIN32
    WEBRTC_TRACE(yuntongxunwebrtc::kTraceInfo, yuntongxunwebrtc::kTraceVideo,
                 -1,"CreateWindowCapture win32");
    
    if(screen_mouse_cursor_for_windows_ == NULL)
    {
        WEBRTC_TRACE(yuntongxunwebrtc::kTraceError, yuntongxunwebrtc::kTraceVideo,
            -1,"CreateWindowCapture  get screen_mouse_cursor_  ERR ");
        return -1;
    }
    windows_capture_ = CroppingWindowCapturer::Create(DesktopCaptureOptions::CreateDefault());
    if ( windows_capture_ == NULL)
    {
        WEBRTC_TRACE(yuntongxunwebrtc::kTraceError, yuntongxunwebrtc::kTraceVideo,
            -1,"CroppingWindowCapturer::Create ERR ");
        return -1;
    }
    window_mouse_blender_.reset(new DesktopAndCursorComposer(windows_capture_, screen_mouse_cursor_for_windows_));
    if (window_mouse_blender_.get() == NULL )
    {
        WEBRTC_TRACE(yuntongxunwebrtc::kTraceError, yuntongxunwebrtc::kTraceVideo,
            -1,"CreateDesktopCapture get  window_mouse_blender_  ERR ");
        return -1;
    }

    window_mouse_blender_->Start(this);
#endif
#ifdef WEBRTC_MAC
    if(screen_mouse_cursor_for_windows_ == NULL){
        WEBRTC_TRACE(yuntongxunwebrtc::kTraceError, yuntongxunwebrtc::kTraceVideo, -1,"CreateWindowCapture  get screen_mouse_cursor_  ERR ");
        return -1;
    }
    windows_capture_ = CroppingWindowCapturer::Create(DesktopCaptureOptions::CreateDefault());
    if ( windows_capture_ == NULL){
        WEBRTC_TRACE(yuntongxunwebrtc::kTraceError, yuntongxunwebrtc::kTraceVideo, -1,"CroppingWindowCapturer::Create ERR ");
        return -1;
    }
    window_mouse_blender_.reset(new DesktopAndCursorComposer(windows_capture_, screen_mouse_cursor_for_windows_));
    if (window_mouse_blender_.get() == NULL ){
        WEBRTC_TRACE(yuntongxunwebrtc::kTraceError, yuntongxunwebrtc::kTraceVideo,  -1,"CreateDesktopCapture get  window_mouse_blender_  ERR ");
        return -1;
    }
    window_mouse_blender_->Start(this);
//    windows_capture_ = CroppingWindowCapturer::Create(DesktopCaptureOptions::CreateDefault());
//    windows_capture_->Start(this);
#endif
#ifdef WEBRTC_IOS
    screen_capturer_ = ScreenCapturer::Create();
    screen_capturer_->Start(this);
#endif
    return 0;
}

int VieDesktopCapturer::RegisterFrameCallback(int observer_id, ViEFrameCallback* callbackObject) {
    return ViEFrameProviderBase::RegisterFrameCallback(observer_id, callbackObject);
}

int VieDesktopCapturer::DeregisterFrameCallback(
    const ViEFrameCallback* callbackObject) {
        provider_cs_->Enter();
        if (callbackObject == vie_encoder_) {
            ViEEncoder* vie_encoder = NULL;
            vie_encoder = vie_encoder_;
            vie_encoder_ = NULL;
            provider_cs_->Leave();

            // Need to take this here in order to avoid deadlock with VCM. The reason is
            // that VCM will call ::Release and a deadlock can occur.
            deliver_cs_->Enter();
            vie_encoder->DeRegisterExternalEncoder(codec_.plType);
            deliver_cs_->Leave();
            return 0;
        }
        provider_cs_->Leave();
        return ViEFrameProviderBase::DeregisterFrameCallback(callbackObject);
}

bool VieDesktopCapturer::IsFrameCallbackRegistered(
    const ViEFrameCallback* callbackObject) {
        CriticalSectionScoped cs(provider_cs_.get());
        if (callbackObject == vie_encoder_) {
            return true;
        }
        return ViEFrameProviderBase::IsFrameCallbackRegistered(callbackObject);
}

bool VieDesktopCapturer::ViEDesktopCaptureThreadFunction(void* obj)
{
    return static_cast<VieDesktopCapturer*>(obj)->ViEDesktopCaptureProcess();
}
    
bool VieDesktopCapturer::ViEDesktopCaptureProcess()
{
	if (!shared_capture_enable_)
		return false;
    CriticalSectionScoped cs(wait_time_cs_.get());
    if (desktop_capture_event_.Wait(thread_wait_time_ms_) != kEventError) 
    {   
        DeliverFrame();
    }
    return true;
}

void VieDesktopCapturer::CopyScreenList( ScreenCapturer::ScreenList &src_screen,ScreenList& dst_screen )
{
//#ifdef _WIN32
    ScreenCapturer::ScreenList::iterator iter = src_screen.begin();
    while(iter != src_screen.end())
    {
        dst_screen.push_back(iter->id);
        iter++;
    }
//#endif
}

void VieDesktopCapturer::CopyWindowsList( WindowCapturer::WindowList &src_windows,WindowList& dst_windows )
{
//#ifdef _WIN32
    WindowCapturer::WindowList::iterator iter = src_windows.begin();
    while(iter != src_windows.end())
    {
        Window wind_;
        wind_.id = iter->id;
        wind_.title = iter->title;
        dst_windows.push_back(wind_);
        iter++;
    }
//#endif
}

void VieDesktopCapturer::DeliverFrame()
{
    CriticalSectionScoped cs(shared_capture_cs_.get());
    if(!shared_capture_enable_)
    {
        WEBRTC_TRACE(yuntongxunwebrtc::kTraceError, yuntongxunwebrtc::kTraceVideo,
            -1,"don't send desktop frame ");
        return ;
    }
    I420VideoFrame frame;
    if(CaptrueShareFrame(frame) == -1)
        return;

    {
        //CriticalSectionScoped cs(deliver_cs_.get());
        //if (denoising_enabled_) {
        //    //image_proc_module_->Denoising(frame); //new version not support
        //}
    }
    ViEFrameProviderBase::DeliverFrame(&frame, std::vector<uint32_t>(),NULL);
}

int VieDesktopCapturer::EnableDenoising(const bool enable )
{
    CriticalSectionScoped cs(deliver_cs_.get());
    if (enable) {
        if (denoising_enabled_) {
            // Already enabled, nothing need to be done.
            return 0;
        }
        denoising_enabled_ = true;
        if (IncImageProcRefCount() != 0) {
            return -1;
        }
    } else {
        if (denoising_enabled_ == false) {
            // Already disabled, nothing need to be done.
            return 0;
        }
        denoising_enabled_ = false;
        DecImageProcRefCount();
    }

    return 0;
}

int VieDesktopCapturer::setCaptureErrCodeCb(int channelid, onDesktopCaptureErrCode errCodeCb)
{
	CriticalSectionScoped cs(observer_cs_.get());
	capture_err_code_cb_ = errCodeCb;
	callback_id_ = channelid;
	return 0;
}

int VieDesktopCapturer::setShareWindowChangeCb(int channelid, onDesktopShareFrameChange frameChangeCb)
{
//#ifdef _WIN32
	CriticalSectionScoped cs(observer_cs_.get());
	capture_frame_change_cb_ = frameChangeCb;
	callback_id_ = channelid;
	return 0;
//#endif
//    return -1;
}

WebRtc_Word32 VieDesktopCapturer::IncImageProcRefCount()
{
    if (!image_proc_module_) {
        assert(image_proc_module_ref_counter_ == 0);
        image_proc_module_ = VideoProcessingModule::Create(
            ViEModuleId(engine_id_, desktop_share_id_));
        if (!image_proc_module_) {
            WEBRTC_TRACE(kTraceError, kTraceVideo, ViEId(engine_id_, desktop_share_id_),
                "%s: could not create video processing module",
                __FUNCTION__);
            return -1;
        }
    }
    image_proc_module_ref_counter_++;
    return 0;
}

WebRtc_Word32 VieDesktopCapturer::DecImageProcRefCount()
{
    image_proc_module_ref_counter_--;
    if (image_proc_module_ref_counter_ == 0) {
        // Destroy module.
        VideoProcessingModule::Destroy(image_proc_module_);
        image_proc_module_ = NULL;
    }
    return 0;
}
    
void VieDesktopCapturer::SetScreenShareActivity(void * activity)
{
    WEBRTC_TRACE(kTraceError, kTraceVideo, ViEId(engine_id_, desktop_share_id_),
                 "%s: screen_capturer_:%0x activity:%0x", __FUNCTION__, screen_capturer_, activity);
    
#ifdef WEBRTC_ANDROID
    if(screen_capturer_) {
        screen_capturer_->SetScreenShareActivity(activity);
    }
#endif
}
    
}
