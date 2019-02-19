//
//  network_control.h
//  congestion_controller
//
//  Created by 袁丽蓉 on 2019/2/18.
//  Copyright © 2019 袁丽蓉. All rights reserved.
//

#ifndef network_control_h
#define network_control_h

#include "../module/rtp_rtcp/include/rtp_rtcp_defines.h"

namespace yuntongxunwebrtc{


class NetworkControllerInterface {
public:
    virtual ~NetworkControllerInterface() = default;

    virtual int OnTransportPacketsFeedback(const std::vector<PacketFeedback>& packet_feedback_vector) = 0;
};
    
    
}

#endif /* network_control_h */
