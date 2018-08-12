//
//  voe_encry_srtp.h
//  voice_engine
//
//  Created by Lee Sean on 13-4-22.
//
//

#ifndef voice_engine_voe_encry_srtp_h
#define voice_engine_voe_encry_srtp_h
#include "srtp.h"

#include "common_types.h"
#include "SrtpModule.h"
#include "../system_wrappers/include/trace.h"

//enum ccp_srtp_crypto_suite_t {
//	CCPAES_128_SHA1_80 = 1,
//	CCPAES_128_SHA1_32,
//    CCPAES_256_SHA1_80,
//    CCPAES_256_SHA1_32,
//	CCPAES_128_NO_AUTH,
//	CCPNO_CIPHER_SHA1_80
//};

namespace yuntongxunwebrtc {
    class VoeEncrySrtp : public SrtpModule
    {
    public:
        
        virtual int64_t TimeUntilNextProcess();
        virtual int32_t Process();
        
		virtual int EnableSRTPSend(int channel,
								ccp_srtp_crypto_suite_t crypt_type,
								const char* key,
								const WebRtc_UWord32 ssrc);
        
        virtual int DisableSRTPSend(int channel);
        
        virtual int EnableSRTPReceive(int channel, ccp_srtp_crypto_suite_t crypt_type, const char* key);
        
        virtual int DisableSRTPReceive(int channel);
        
        // External encryption
        virtual int RegisterExternalEncryption(int channel,
                                               Encryption& encryption);
        
        virtual int DeRegisterExternalEncryption(int channel);
        
        
        virtual int Release();
        // Encrypt the given data.
        //
        // Args:
        //   channel: The channel to encrypt data for.
        //   in_data: The data to encrypt. This data is bytes_in bytes long.
        //   out_data: The buffer to write the encrypted data to. You may write more
        //       bytes of encrypted data than what you got as input, up to a maximum
        //       of yuntongxunwebrtc::kViEMaxMtu if you are encrypting in the video engine, or
        //       yuntongxunwebrtc::kVoiceEngineMaxIpPacketSizeBytes for the voice engine.
        //   bytes_in: The number of bytes in the input buffer.
        //   bytes_out: The number of bytes written in out_data.
        virtual void encrypt(
                             int channel,
                             unsigned char* in_data,
                             unsigned char* out_data,
                             int bytes_in,
                             int* bytes_out);
        
        // Decrypts the given data. This should reverse the effects of encrypt().
        //
        // Args:
        //   channel_no: The channel to decrypt data for.
        //   in_data: The data to decrypt. This data is bytes_in bytes long.
        //   out_data: The buffer to write the decrypted data to. You may write more
        //       bytes of decrypted data than what you got as input, up to a maximum
        //       of yuntongxunwebrtc::kViEMaxMtu if you are encrypting in the video engine, or
        //       yuntongxunwebrtc::kVoiceEngineMaxIpPacketSizeBytes for the voice engine.
        //   bytes_in: The number of bytes in the input buffer.
        //   bytes_out: The number of bytes written in out_data.
        virtual void decrypt(
                             int channel,
                             unsigned char* in_data,
                             unsigned char* out_data,
                             int bytes_in,
                             int* bytes_out) ;
        
        // Encrypts a RTCP packet. Otherwise, this method has the same contract as
        // encrypt().
        virtual void encrypt_rtcp(
                                  int channel,
                                  unsigned char* in_data,
                                  unsigned char* out_data,
                                  int bytes_in,
                                  int* bytes_out);
        
        // Decrypts a RTCP packet. Otherwise, this method has the same contract as
        // decrypt().
        virtual void decrypt_rtcp(
                                  int channel,
                                  unsigned char* in_data,
                                  unsigned char* out_data,
                                  int bytes_in,
                                  int* bytes_out);
        virtual int CcpSrtpInit(int channel);
        virtual int CcpSrtpShutdown(int channel);
        
        bool ccp_srtp_supported(void);
        
        //        int srtp_transport_new(srtp_t srtp, RtpTransport **rtpt, RtpTransport **rtcpt );
        
        int ccp_srtp_shutdown(void);
        
       // int setSSRC();
        
        virtual ~VoeEncrySrtp();
        VoeEncrySrtp(WebRtc_Word32 id);
    private:
        static int srtp_init_done;
        srtp_t session;
        
        const WebRtc_UWord32 _ssrc;
        WebRtc_Word32 _id;
        
        //CipherTypes _cipherTypes;
        //AuthenticationTypes _authenticationTypes;
        //int _authkeyLength;
        //int _authtagLength;
        //SecurityLevels _securityLevels;
        enum ccp_srtp_crypto_suite_t _suite;
        
        bool _srtpCreate;

		int send_channel;
		ccp_srtp_crypto_suite_t send_crypt_type;
		char send_key[256];
		WebRtc_UWord32 send_ssrc;
        
		int recv_channel;
		ccp_srtp_crypto_suite_t recv_crypt_type;
		char recv_key[256];

    private:
        err_status_t ccp_srtp_init(void);
        err_status_t ccp_srtp_create(const srtp_policy_t *policy);
        err_status_t ccp_srtp_dealloc(srtp_t session);
        err_status_t ccp_srtp_add_stream(srtp_t session, const srtp_policy_t *policy);
        err_status_t ccp_crypto_get_random(uint8_t *tmp, int size);
        bool ccp_init_srtp_policy(srtp_t srtp, srtp_policy_t* policy, ssrc_t ssrc, const char* b64_key);
        
        //err_status_t ccp_srtp_create_configure_session(uint32_t ssrc, const char* snd_key, const char* rcv_key);
        int ccp_srtp_configure_incoming(const char* rcv_key);
        int ccp_srtp_configure_outgoing(uint32_t ssrc, const char* snd_key);
        
        
    };
    
} //namespace yuntongxunwebrtc


#endif
