/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */
#include "../module/rtp_rtcp/include/rtp_header_parser.h"

#include "../base/criticalsection.h"
#include "../module/rtp_rtcp/source/rtp_header_extension.h"
#include "../module/rtp_rtcp/source/rtp_utility.h"
#include "../system_wrappers/include/clock.h"

namespace yuntongxunwebrtc {

class RtpHeaderParserImpl : public RtpHeaderParser {
 public:
  RtpHeaderParserImpl();
  virtual ~RtpHeaderParserImpl() {}

  bool Parse(const uint8_t* packet,
             size_t length,
             RTPHeader* header) override;

  bool RegisterRtpHeaderExtension(RTPExtensionType type, uint8_t id) override;

  bool DeregisterRtpHeaderExtension(RTPExtensionType type) override;

  virtual int setECMediaConferenceParticipantCallback(ECMedia_ConferenceParticipantCallback* cb);
  virtual void setECMediaConferenceParticipantCallbackTimeInterVal(int timeInterVal);

 private:
  yuntongxunwebrtc::CriticalSection critical_section_;
  RtpHeaderExtensionMap rtp_header_extension_map_ GUARDED_BY(critical_section_);
  ECMedia_ConferenceParticipantCallback *_participantCallback; //added by zhaoyou.
  int callConferenceParticipantCallbacktimeInterVal_;
    
  uint8_t last_arr_csrc_count_;
  uint32_t last_arrOfCSRCs_[kRtpCsrcSize];
  int64_t base_time;
};

RtpHeaderParser* RtpHeaderParser::Create() {
  return new RtpHeaderParserImpl;
}

RtpHeaderParserImpl::RtpHeaderParserImpl()
    : _participantCallback(nullptr),
    callConferenceParticipantCallbacktimeInterVal_(1) {
        last_arr_csrc_count_ = 0;
        memset(last_arrOfCSRCs_, 0, sizeof(uint32_t)*kRtpCsrcSize);
        base_time = 0;
    }

bool RtpHeaderParser::IsRtcp(const uint8_t* packet, size_t length) {
  RtpUtility::RtpHeaderParser rtp_parser(packet, length);
  return rtp_parser.RTCP();
}

bool RtpHeaderParserImpl::Parse(const uint8_t* packet,
                                size_t length,
                                RTPHeader* header) {
  RtpUtility::RtpHeaderParser rtp_parser(packet, length);
  memset(header, 0, sizeof(*header));

  RtpHeaderExtensionMap map;
  {
    yuntongxunwebrtc::CritScope cs(&critical_section_);
    map = rtp_header_extension_map_;
  }

  const bool valid_rtpheader = rtp_parser.Parse(header, &map);
  if (!valid_rtpheader) {
    return false;
  }
  
  /****  callback conference csrc array when csrc array changed, added by zhaoyou ******/

  bool should_send = false;
  {
      int64_t current_time = Clock::GetRealTimeClock()->TimeInMilliseconds();
      should_send = (current_time - base_time) >= callConferenceParticipantCallbacktimeInterVal_ * 1000;
      if(should_send) {
          base_time = Clock::GetRealTimeClock()->TimeInMilliseconds();
      }
  }
    if(_participantCallback && should_send) {
      if(last_arr_csrc_count_ != header->numCSRCs) {
          // ConferenceParticipantCallback
          _participantCallback(header->arrOfCSRCs, header->numCSRCs);
          last_arr_csrc_count_ = header->numCSRCs;
          for(int i = 0; i< header->numCSRCs; i++) {
              last_arrOfCSRCs_[i] = header->arrOfCSRCs[i];
          }
      } else if(header->numCSRCs != 0){
          bool need_callback = true;
          for(int i = 0; i< header->numCSRCs; i++) {
              // callback only once, when csrc list change
              if(header->arrOfCSRCs[i] != last_arrOfCSRCs_[i]) {
                  if(need_callback) {
                      _participantCallback(header->arrOfCSRCs, header->numCSRCs);
                      need_callback = false;
                  }
              }
              // copy current csrs arr.
              last_arrOfCSRCs_[i] = header->arrOfCSRCs[i];
          }
          // store last arr count.
          last_arr_csrc_count_ = header->numCSRCs;
      }
  }
  return true;
}

bool RtpHeaderParserImpl::RegisterRtpHeaderExtension(RTPExtensionType type,
                                                     uint8_t id) {
  yuntongxunwebrtc::CritScope cs(&critical_section_);
  return rtp_header_extension_map_.RegisterByType(id, type);
}

bool RtpHeaderParserImpl::DeregisterRtpHeaderExtension(RTPExtensionType type) {
  yuntongxunwebrtc::CritScope cs(&critical_section_);
  return rtp_header_extension_map_.Deregister(type) == 0;
}
    
int RtpHeaderParserImpl::setECMediaConferenceParticipantCallback(ECMedia_ConferenceParticipantCallback* cb) {
    if(cb == nullptr) {
        // LOG(LS_ERROR) << "ECMedia_ConferenceParticipantCallback is null.";
        return -1;
    }
    
    if(_participantCallback == cb) {
        // LOG(LS_WARNING) << "ECMedia_ConferenceParticipantCallback already have been set.";
        return 0;
    }
    
    _participantCallback = cb;
    return 0;
}
    
void RtpHeaderParserImpl::setECMediaConferenceParticipantCallbackTimeInterVal(int timeInterVal) {
    if(timeInterVal < 0 ) {
        // LOG(LS_ERROR) << "setECMediaConferenceParticipantCallbackTimeInterVal is null.";
        timeInterVal = 0;
    }
    
    callConferenceParticipantCallbacktimeInterVal_ = timeInterVal;
 }

} // namespace webrtc
