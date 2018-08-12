//
//  videoframe.h
//  servicecoreVideo
//
//  Created by hubin on 13-11-6.
//
//

#ifndef __servicecoreVideo__videoframe__
#define __servicecoreVideo__videoframe__

#include <iostream>
#include "vie_frame_provider_base.h"
#include "serphonecall.h"

class VideoFrameDeliver :public yuntongxunwebrtc::ViEFrameCallback {
    
public:
    VideoFrameDeliver(SerPhoneCall *call);
    
public:
    // Implementing ViEFrameCallback.
    void DeliverFrame(int id,
                      yuntongxunwebrtc::I420VideoFrame* video_frame,
 /*                     int num_csrcs = 0,*/
                      const std::vector<uint32_t>& csrcs);
    
    void DelayChanged(int id, int frame_delay){ return; }
    int GetPreferedFrameSettings(int* width, int* height, int* frame_rate) { return -1; }
    void ProviderDestroyed(int id) { return; }

public:
    SerPhoneCall *m_Call;
};

#endif /* defined(__servicecoreVideo__videoframe__) */
