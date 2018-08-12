/*
 *  Copyright (c) 2011 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef _VIE_DESKTOP_SHARE_IMPL_H_
#define _VIE_DESKTOP_SHARE_IMPL_H_


#include "typedefs.h"
#include "vie_desktop_share.h"
#include "vie_ref_count.h"

namespace yuntongxunwebrtc {


class ViESharedData;

class ViEDesktopShareImpl
    : public ViEDesktopShare,
      public ViERefCount {
 public:

     virtual int Release();

     virtual int  AllocateDesktopShareCapturer(int& desktop_capture_id, const DesktopShareType capture_type);

     virtual int  ReleaseDesktopShareCapturer(const int desktop_capture_id);

     virtual int  ConnectDesktopCaptureDevice(const int desktop_capture_id,const int video_channel);

     virtual int  DisConnectDesktopCaptureDevice(const int video_channel);

	 virtual int NumberOfWindow(const int desktop_capture_id);

	 virtual int NumberOfScreen(const int desktop_capture_id);

     virtual bool GetDesktopShareCaptureRect(const int desktop_capture_id, int &width, int &height);

     virtual bool GetScreenList(const int desktop_capture_id,ScreenList& screens);

     virtual bool SelectScreen(const int desktop_capture_id, const ScreenId screen_id);

     virtual bool GetWindowList(const int desktop_capture_id, WindowList& windows);

     virtual bool SelectWindow(const int desktop_capture_id,const WindowId id);

     virtual int StartDesktopShareCapture(const int desktop_capture_id, const int fps);

     virtual int StopDesktopShareCapture(const int desktop_capture_id);

	 virtual int setCaptureErrCb(int desktop_capture_id, int channelid, onDesktopCaptureErrCode capture_err_code_cb) ;
	 virtual int setShareWindowChangeCb(int desktop_capture_id, int channelid, onDesktopShareFrameChange capture_frame_change_cb) ;

    virtual int SetScreenShareActivity(int desktop_capture_id, void * activity);

protected:
    ViEDesktopShareImpl(ViESharedData* shared_data);
    virtual ~ViEDesktopShareImpl();

private:
    ViESharedData* shared_data_;
};

}  // namespace yuntongxunwebrtc

#endif  // _VIE_DESKTOP_SHARE_IMPL_H_
