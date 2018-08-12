//
//  window_capturer_ios.h
//  desktop_capture
//
//  Created by SeanLee on 2016/11/24.
//  Copyright © 2016年 SeanLee. All rights reserved.
//

#ifndef window_capturer_ios_h
#define window_capturer_ios_h
namespace yuntongxunwebrtc {
    class ScreenCapturerIos : public ScreenCapturer {
    public:
        ScreenCapturerIos();
        virtual ~ScreenCapturerIos();
        
        // WindowCapturer interface.
        bool GetShareCaptureRect(int &width, int &height);
        
        // DesktopCapturer interface.
        void Start(Callback* callback) ;
        void Capture(const DesktopRegion& region) ;
        
        virtual bool GetScreenList(ScreenList* screens);
        
        // Select the screen to be captured. Returns false in case of a failure (e.g.
        // if there is no screen with the specified id). If this is never called, the
        // full desktop is captured.
        virtual bool SelectScreen(ScreenId id);
        
        
    private:
        bool IsAeroEnabled();
        
        Callback* callback_;
        
        
        DesktopSize previous_size_;
        
        DISALLOW_COPY_AND_ASSIGN(ScreenCapturerIos);
    };
}
#endif /* window_capturer_io

s_h */
