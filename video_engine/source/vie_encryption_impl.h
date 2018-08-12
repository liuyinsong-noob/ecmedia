/*
 *  Copyright (c) 2011 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_VIDEO_ENGINE_VIE_ENCRYPTION_IMPL_H_
#define WEBRTC_VIDEO_ENGINE_VIE_ENCRYPTION_IMPL_H_

#include "typedefs.h"
#include "vie_encryption.h"
#include "vie_ref_count.h"

namespace yuntongxunwebrtc {

class ViESharedData;

class ViEEncryptionImpl
    : public ViEEncryption,
      public ViERefCount {
public:
	// SRTP
	virtual int EnableSRTPSend(int channel, ccp_srtp_crypto_suite_t crypt_type, const char* key);

	virtual int DisableSRTPSend(int channel);

	virtual int EnableSRTPReceive(int channel,ccp_srtp_crypto_suite_t crypt_type, const char* key);

	virtual int DisableSRTPReceive(int channel);

	virtual int CcpSrtpInit(int channel);
	virtual int CcpSrtpShutdown(int channel);

 public:
  virtual int Release();

  // Implements ViEEncryption.
  virtual int RegisterExternalEncryption(const int video_channel,
                                         Encryption& encryption);
  virtual int DeregisterExternalEncryption(const int video_channel);

 protected:
  ViEEncryptionImpl(ViESharedData* shared_data);
  virtual ~ViEEncryptionImpl();

 private:
  ViESharedData* shared_data_;
          
          CriticalSectionWrapper *vie_encryption_sect_;
};

}  // namespace yuntongxunwebrtc

#endif  // WEBRTC_VIDEO_ENGINE_VIE_ENCRYPTION_IMPL_H_
