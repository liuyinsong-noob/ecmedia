//
//  SrtpModule.cc
//  voice_engine
//
//  Created by Lee Sean on 13-4-23.
//
#include "voe_encry_srtp.h"
#include "../../third_party/srtp/include/srtp.h"
#include "SrtpModule.h"

namespace yuntongxunwebrtc {

SrtpModule* SrtpModule::CreateSrtpModule(const WebRtc_Word32 id)
    
    {
#ifdef WEBRTC_SRTP
        return new VoeEncrySrtp(id);
#else
		return NULL;
#endif
    }
void SrtpModule::DestroySrtpModule(SrtpModule* module)
    {
        delete module;
    }


//int SrtpModule::EnableSRTPSend(
//                           int channel,
//                           CipherTypes cipherType,
//                           int cipherKeyLength,
//                           AuthenticationTypes authType,
//                           int authKeyLength,
//                           int authTagLength,
//                           SecurityLevels level,
//                           const unsigned char key[kMaxSrtpKeyLength],
//                           bool useForRTCP)
//    {
//        
//    }
//
//int SrtpModule::DisableSRTPSend(int channel)
//    {
//        
//    }
//
//int SrtpModule::EnableSRTPReceive(
//                              int channel,
//                              CipherTypes cipherType,
//                              int cipherKeyLength,
//                              AuthenticationTypes authType,
//                              int authKeyLength,
//                              int authTagLength,
//                              SecurityLevels level,
//                              const unsigned char key[kMaxSrtpKeyLength],
//                              bool useForRTCP)
//    {
//        
//    }
//
//int SrtpModule::DisableSRTPReceive(int channel)
//    {
//        
//    }
//
//// External encryption
//int SrtpModule::RegisterExternalEncryption(
//                                       int channel,
//                                       Encryption& encryption)
//    {
//        
//    }
//
//int SrtpModule::DeRegisterExternalEncryption(int channel)
//    {
//        
//    }
}