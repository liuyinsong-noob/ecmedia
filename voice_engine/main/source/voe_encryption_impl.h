/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_VOICE_ENGINE_VOE_ENCRYPTION_IMPL_H
#define WEBRTC_VOICE_ENGINE_VOE_ENCRYPTION_IMPL_H

#include "voe_encryption.h"

#include "shared_data.h"

namespace yuntongxunwebrtc {

class VoEEncryptionImpl : public VoEEncryption
{
public:
    // SRTP
	virtual int EnableSRTPSend(int channel,
		ccp_srtp_crypto_suite_t crypt_type,
		const char* key);

    virtual int DisableSRTPSend(int channel);

	virtual int EnableSRTPReceive(int channel,
		ccp_srtp_crypto_suite_t crypt_type,
		const char* key);

    virtual int DisableSRTPReceive(int channel);

    // External encryption
    virtual int RegisterExternalEncryption(
        int channel,
        Encryption& encryption);

    virtual int DeRegisterExternalEncryption(int channel);

    //sean20130425
    virtual int CcpSrtpInit(int channel);
    virtual int CcpSrtpShutdown(int channel);
protected:
    VoEEncryptionImpl(voe::SharedData* shared);
    virtual ~VoEEncryptionImpl();

private:
    voe::SharedData* _shared;
};

}   // namespace yuntongxunwebrtc

#endif  // #ifndef WEBRTC_VOICE_ENGINE_VOE_ENCRYPTION_IMPL_H
