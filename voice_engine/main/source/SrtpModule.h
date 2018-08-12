//
//  SrtpModule.h
//  voice_engine
//
//  Created by Lee Sean on 13-4-23.
//
//

#ifndef voice_engine_SrtpModule_h
#define voice_engine_SrtpModule_h

#include "module.h"
#include "voe_encryption.h"
//#include "voice_engine_defines.h"

namespace yuntongxunwebrtc {
    class SrtpModule : public Module , public Encryption //, public VoEEncryption
    {
    public:
        static SrtpModule* CreateSrtpModule(const WebRtc_Word32 id);
        static void DestroySrtpModule(SrtpModule* module);
	public:
		virtual int EnableSRTPSend(int channel,
			ccp_srtp_crypto_suite_t crypt_type,
			const char* key,
			const WebRtc_UWord32 ssrc) = 0;

		virtual int DisableSRTPSend(int channel) = 0;

		virtual int EnableSRTPReceive(int channel, ccp_srtp_crypto_suite_t crypt_type, const char* key) = 0;

		virtual int DisableSRTPReceive(int channel) = 0;

		virtual int CcpSrtpInit(int channel) = 0;
		virtual int CcpSrtpShutdown(int channel) = 0;
    };
}


#endif
