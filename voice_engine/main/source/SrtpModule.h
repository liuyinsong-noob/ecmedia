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

#define kVoiceEngineMaxSrtpKeyLength 30

namespace cloopenwebrtc {
    class SrtpModule : public Module , public VoEEncryption , public Encryption
    {
    public:
        static SrtpModule* CreateSrtpModule(const WebRtc_Word32 id);
        static void DestroySrtpModule(SrtpModule* module);
        
        
        
//        virtual int EnableSRTPSend(
//                                   int channel,
//                                   CipherTypes cipherType,
//                                   int cipherKeyLength,
//                                   AuthenticationTypes authType,
//                                   int authKeyLength,
//                                   int authTagLength,
//                                   SecurityLevels level,
//                                   const unsigned char key[kVoiceEngineMaxSrtpKeyLength],
//                                   bool useForRTCP = false);
//        
//        virtual int DisableSRTPSend(int channel);
//        
//        virtual int EnableSRTPReceive(
//                                      int channel,
//                                      CipherTypes cipherType,
//                                      int cipherKeyLength,
//                                      AuthenticationTypes authType,
//                                      int authKeyLength,
//                                      int authTagLength,
//                                      SecurityLevels level,
//                                      const unsigned char key[kVoiceEngineMaxSrtpKeyLength],
//                                      bool useForRTCP = false);
//        
//        virtual int DisableSRTPReceive(int channel);
//        
//        // External encryption
//        virtual int RegisterExternalEncryption(
//                                               int channel,
//                                               Encryption& encryption);
//        
//        virtual int DeRegisterExternalEncryption(int channel);
    };
}


#endif
