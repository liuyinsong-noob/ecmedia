/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */
#include "rtp_header_parser.h"

#include "rtp_header_extension.h"
#include "rtp_utility.h"
#include "critical_section_wrapper.h"
#include "scoped_ptr.h"

namespace cloopenwebrtc {

class RtpHeaderParserImpl : public RtpHeaderParser {
 public:
  RtpHeaderParserImpl();
  virtual ~RtpHeaderParserImpl() {}

  virtual bool Parse(const uint8_t* packet,
                     size_t length,
                     RTPHeader* header) const OVERRIDE;

  virtual bool RegisterRtpHeaderExtension(RTPExtensionType type,
                                          uint8_t id) OVERRIDE;

  virtual bool DeregisterRtpHeaderExtension(RTPExtensionType type) OVERRIDE;
  virtual int setECMediaConferenceParticipantCallback(ECMedia_ConferenceParticipantCallback* cb);
 private:
  scoped_ptr<CriticalSectionWrapper> critical_section_;
  RtpHeaderExtensionMap rtp_header_extension_map_ GUARDED_BY(critical_section_);
  ECMedia_ConferenceParticipantCallback *_participantCallback; //added by zhaoyou.
  
};

RtpHeaderParser* RtpHeaderParser::Create() {
  return new RtpHeaderParserImpl;
}

RtpHeaderParserImpl::RtpHeaderParserImpl()
    : critical_section_(CriticalSectionWrapper::CreateCriticalSection()),
    _participantCallback(nullptr) {}

bool RtpHeaderParser::IsRtcp(const uint8_t* packet, size_t length) {
  RtpUtility::RtpHeaderParser rtp_parser(packet, length);
  return rtp_parser.RTCP();
}

bool RtpHeaderParserImpl::Parse(const uint8_t* packet,
                                size_t length,
                                RTPHeader* header) const {
  RtpUtility::RtpHeaderParser rtp_parser(packet, length);
  memset(header, 0, sizeof(*header));

  RtpHeaderExtensionMap map;
  {
    CriticalSectionScoped cs(critical_section_.get());
    rtp_header_extension_map_.GetCopy(&map);
  }

  const bool valid_rtpheader = rtp_parser.Parse(*header, &map);
  if (!valid_rtpheader) {
    return false;
  }
  
  /****  callback conference csrc array when csrc array changed, added by zhaoyou ******/
  static uint8_t last_arr_csrc_count_ = 0;
  static uint32_t last_arrOfCSRCs_[kRtpCsrcSize] = {0};
  if(_participantCallback) {
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
  CriticalSectionScoped cs(critical_section_.get());
  return rtp_header_extension_map_.Register(type, id) == 0;
}

bool RtpHeaderParserImpl::DeregisterRtpHeaderExtension(RTPExtensionType type) {
  CriticalSectionScoped cs(critical_section_.get());
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
}  // namespace webrtc
