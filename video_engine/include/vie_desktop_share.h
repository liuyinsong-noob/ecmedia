#ifndef WEBRTC_VIDEO_ENGINE_INCLUDE_VIE_DESKTOP_SHARE_H_
#define WEBRTC_VIDEO_ENGINE_INCLUDE_VIE_DESKTOP_SHARE_H_


#include "common_types.h"
#include <string>
#include <vector>

namespace cloopenwebrtc {

class VideoEngine;
class VideoCaptureModule;
class ViEFrameCallback;


typedef intptr_t WindowId;
typedef intptr_t ScreenId;

struct Window {
    WindowId id;
    // Title of the window in UTF-8 encoding.
    std::string title;
};
typedef std::vector<Window> WindowList;
typedef std::vector<ScreenId> ScreenList;

typedef int (*onDesktopCaptureErrCode)(int desktop_capture_id, int errCode);
typedef int (*onDesktopShareFrameChange)(int desktop_capture_id, int width, int height);

enum DesktopShareType
{
    ShareNone = -1,
    ShareScreen = 0,
    ShareWindow = 1
};

enum DesktopShareErrCode
{
    kDesktopShare_Ok = 0,
    kDesktopShare_Window_Overlapped,
    kDesktopShare_Window_IsIconic,
    kDesktopShare_Window_Closed,
    kDesktopShare_Window_Hidden,
    kDesktopShare_CaptureImage,
    kDesktopShare_Unknown
};

class WEBRTC_DLLEXPORT ViEDesktopShare {
public:
    // Factory for the ViECodec sub©\API and increases an internal reference
    // counter if successful. Returns NULL if the API is not supported or if
    // construction fails.
    static ViEDesktopShare* GetInterface(VideoEngine* video_engine);

    // Releases the ViEDesktopShare sub-API and decreases an internal reference
    // counter.
    // Returns the new reference count. This value should be zero
    // for all sub-API:s before the VideoEngine object can be safely deleted.
    virtual int Release() = 0;

    // Allocates a capture to be used in VideoEngine. 
    virtual int  AllocateDesktopShareCapturer(int& desktop_capture_id, const DesktopShareType capture_type) = 0;

    // Releases a capture device and makes it available for other applications.
    virtual int  ReleaseDesktopShareCapturer(const int desktop_capture_id) = 0;
   
    // This function connects a capture device with a desktop capture. Multiple desktop capture
    // can be connected to the same capture device.
    virtual int ConnectDesktopCaptureDevice(const int desktop_capture_id,const int video_channel) = 0;

    // Removes desktop capture from capture device.
    virtual int DisConnectDesktopCaptureDevice(const int video_channel) = 0;

	// Get number of window on the screen;
	virtual int NumberOfWindow(const int desktop_capture_id) = 0;

	// Get number of screen on this device;
	virtual int NumberOfScreen(const int desktop_capture_id) = 0;

    // Get screen or window width and height.
    virtual bool GetDesktopShareCaptureRect(const int desktop_capture_id, int &width, int &height) = 0;

    // Get the list of screens (not containing kFullDesktopScreenId). Returns
    // false in case of a failure.
    virtual bool GetScreenList(const int desktop_capture_id,ScreenList& screens) = 0;

    // Select the screen to be captured. Returns false in case of a failure (e.g.
    // if there is no screen with the specified id). If this is never called, the
    // full desktop is captured.
    virtual bool SelectScreen(const int desktop_capture_id, const ScreenId screen_id) = 0;

    // Get list of windows. Returns false in case of a failure.
    virtual bool GetWindowList(const int desktop_capture_id, WindowList& windows) = 0;

    // Select window to be captured. Returns false in case of a failure (e.g. if
    // there is no window with the specified id).
    virtual bool SelectWindow(const int desktop_capture_id,const WindowId id) = 0;

    // Makes a capture device start capturing desktop_capture_vide frames.
    virtual int StartDesktopShareCapture(const int desktop_capture_id, const int fps) = 0;

    // Stops a started capture device from capturing desktop_capture_vide frames.
    virtual int StopDesktopShareCapture(const int desktop_capture_id) = 0;

	virtual int setCaptureErrCb(int desktop_capture_id, int channelid, onDesktopCaptureErrCode capture_err_code_cb) = 0;
	virtual int setShareWindowChangeCb(int desktop_capture_id, int channelid, onDesktopShareFrameChange capture_frame_change_cb) = 0;

 virtual int SetScreenShareActivity(int desktop_capture_id, void * activity)  = 0;
protected:
    ViEDesktopShare() {}
    virtual ~ViEDesktopShare() {}

};

}

#endif  //WEBRTC_VIDEO_ENGINE_INCLUDE_VIE_DESKTOP_SHARE_H_