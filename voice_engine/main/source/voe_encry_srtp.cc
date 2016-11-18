//
//  voe_encry_srtp.cc
//  voice_engine
//
//  Created by Lee Sean on 13-4-22.
//
//

#include "voe_encry_srtp.h"
#include <b64.h>
#include "rtp.h"
#include "../../third_party/oRTP/include/ortp/rtcp.h"
#define RTP_FIXED_HEADER_SIZE 12
#define SRTP_PAD_BYTES (SRTP_MAX_TRAILER_LEN + 4)

namespace cloopenwebrtc {
    
    int VoeEncrySrtp::srtp_init_done = 0;
    
    int64_t VoeEncrySrtp::TimeUntilNextProcess()
    {
        return 0;
    }
    int32_t VoeEncrySrtp::Process()
    {
        return 0;
    }
    
    int VoeEncrySrtp::Release()
    {
        return 0;
    }
    
    int VoeEncrySrtp::CcpSrtpInit(int channel)
    {
        return ccp_srtp_init();
    }
    
    int VoeEncrySrtp::CcpSrtpShutdown(int channel)
    {
        return ccp_srtp_shutdown();
    }
    
    int VoeEncrySrtp::EnableSRTPSend(int channel, 
									ccp_srtp_crypto_suite_t crypt_type,
									const char* key,
									const WebRtc_UWord32 ssrc)
    {
		WEBRTC_TRACE(kTraceDebug, kTraceVoice, 0, "EnableSRTPSend channel = %d;crypt_type = %d;\n", channel, crypt_type);
		_suite = crypt_type;

		switch (_suite)
		{
		case CCPAES_128_SHA1_80:
			WEBRTC_TRACE(kTraceDebug, kTraceVoice, 0, "EnableSRTPReceive _suite = AES_128_SHA1_80\n");
			break;
		case CCPAES_128_SHA1_32:
			WEBRTC_TRACE(kTraceDebug, kTraceVoice, 0, "EnableSRTPReceive _suite = AES_128_SHA1_32\n");
			break;
		case CCPAES_256_SHA1_80:
			WEBRTC_TRACE(kTraceDebug, kTraceVoice, 0, "EnableSRTPReceive _suite = AES_256_SHA1_80\n");
			break;
		case CCPAES_256_SHA1_32:
			WEBRTC_TRACE(kTraceDebug, kTraceVoice, 0, "EnableSRTPReceive _suite = AES_256_SHA1_32\n");
			break;
		case CCPAES_128_NO_AUTH:
			WEBRTC_TRACE(kTraceDebug, kTraceVoice, 0, "EnableSRTPReceive _suite = AES_128_NO_AUTH\n");
			break;
		case CCPNO_CIPHER_SHA1_80:
			WEBRTC_TRACE(kTraceDebug, kTraceVoice, 0, "EnableSRTPReceive _suite = NO_CIPHER_SHA1_80\n");
			break;
		default:
			WEBRTC_TRACE(kTraceError, kTraceVoice, 0, "ERROR: NOT SUPPORT!\n");
			return -1;
		}
        int ret = ccp_srtp_configure_outgoing(ssrc, reinterpret_cast<const char*>(key));
        if (ret != 0) {
            return -1;
        }
        return 0;
    }
    
    int VoeEncrySrtp::DisableSRTPSend(int channel)
    {
        return 0;
    }
    
    int VoeEncrySrtp::EnableSRTPReceive(int channel, ccp_srtp_crypto_suite_t crypt_type, const char* key)
    {
		WEBRTC_TRACE(kTraceDebug, kTraceVoice, 0, "EnableSRTPReceive channel = %d;crypt_type = %d;\n", channel, crypt_type);        
		_suite = crypt_type;

		switch (_suite)
		{
		case CCPAES_128_SHA1_80:
			WEBRTC_TRACE(kTraceDebug, kTraceVoice, 0, "EnableSRTPReceive _suite = AES_128_SHA1_80\n");
			break;
		case CCPAES_128_SHA1_32:
			WEBRTC_TRACE(kTraceDebug, kTraceVoice, 0, "EnableSRTPReceive _suite = AES_128_SHA1_32\n");
			break;
		case CCPAES_256_SHA1_80:
			WEBRTC_TRACE(kTraceDebug, kTraceVoice, 0, "EnableSRTPReceive _suite = AES_256_SHA1_80\n");
			break;
		case CCPAES_256_SHA1_32:
			WEBRTC_TRACE(kTraceDebug, kTraceVoice, 0, "EnableSRTPReceive _suite = AES_256_SHA1_32\n");
			break;
		case CCPAES_128_NO_AUTH:
			WEBRTC_TRACE(kTraceDebug, kTraceVoice, 0, "EnableSRTPReceive _suite = AES_128_NO_AUTH\n");
			break;
		case CCPNO_CIPHER_SHA1_80:
			WEBRTC_TRACE(kTraceDebug, kTraceVoice, 0, "EnableSRTPReceive _suite = NO_CIPHER_SHA1_80\n");
			break;
		default:
			WEBRTC_TRACE(kTraceError, kTraceVoice, 0, "ERROR: NOT SUPPORT!\n");
			return -1;
		}

        int ret = ccp_srtp_configure_incoming(reinterpret_cast<const char*>(key));
        if (ret != 0) {
			WEBRTC_TRACE(kTraceError, kTraceVoice, 0, "VoeEncrySrtp::EnableSRTPReceive srtp config failed\n");
			return -1;
        }
        return 0;
    }
    
    int VoeEncrySrtp::DisableSRTPReceive(int channel)
    {
        WEBRTC_TRACE(kTraceDebug, kTraceVoice, 0,"VoeEncrySrtp::DisableSRTPReceive\n");
        return 0;
    }
    
    // External encryption
    int VoeEncrySrtp::RegisterExternalEncryption(
                                           int channel,
                                           Encryption& encryption)
    {
        WEBRTC_TRACE(kTraceDebug, kTraceVoice, 0,"VoeEncrySrtp::RegisterExternalEncryption\n");
        return 0;
    }
    
    int VoeEncrySrtp::DeRegisterExternalEncryption(int channel)
    {
        WEBRTC_TRACE(kTraceDebug, kTraceVoice, 0,"VoeEncrySrtp::DeRegisterExternalEncryption\n");
        return 0;
    }

    
    
    VoeEncrySrtp::~VoeEncrySrtp()
    {
        
    }
    VoeEncrySrtp::VoeEncrySrtp(WebRtc_Word32 id):_id(id),
    //_cipherTypes(kCipherAes128CounterMode),
    //_authenticationTypes(kAuthHmacSha1),
    //_authkeyLength(160),
    //_authtagLength(80),//AES_CM_128_HMAC_SHA1_80
    _suite(CCPAES_128_SHA1_80),
    _ssrc(0),
    session(NULL),
    _srtpCreate(false)
    {
        
    }
    
    void VoeEncrySrtp::encrypt(
                 int /*channel*/,
                 unsigned char* in_data,//rtp packet
                 unsigned char* out_data,//srtp packet
                 int bytes_in,
                 int* bytes_out)
    {
        err_status_t err;
        
        memcpy(out_data, in_data, bytes_in);

        err=srtp_protect(session,out_data,&bytes_in);
        
        if (err==err_status_ok){
            *bytes_out = bytes_in;
            return;
        }
        WEBRTC_TRACE(kTraceDebug, kTraceVoice, 0,"VoeEncrySrtp->encrypt srtp_protect() failed (%d)\n", err);
        return ;
    }
    
    void VoeEncrySrtp::decrypt(
                         int /*channel*/,
                         unsigned char* in_data,
                         unsigned char* out_data,
                         int bytes_in,
                         int* bytes_out)
    {
        err_status_t srtp_err;
        /* keep NON-RTP data unencrypted */
        rtp_header_t *rtp;
        if (bytes_in>=RTP_FIXED_HEADER_SIZE)
        {
            rtp = (rtp_header_t*)in_data;
            if (rtp->version!=2)
            {
                WEBRTC_TRACE(kTraceError, kTraceVoice, 0,"rtp->version!=2, rtp->version = %d\n",rtp->version);
                return;
            }
        }

		memcpy(out_data, in_data, bytes_in);
        srtp_err = srtp_unprotect(session,out_data,&bytes_in);
        if (srtp_err==err_status_ok)
        {   
             *bytes_out = bytes_in;
            return;
        }
        else {
            WEBRTC_TRACE(kTraceError, kTraceVoice, 0,"VoeEncrySrtp->decrypt srtp_unprotect() failed (%d)\n", srtp_err);
            return;
        }
        return;
    }
    
    // Encrypts a RTCP packet. Otherwise, this method has the same contract as
    // encrypt().
    void VoeEncrySrtp::encrypt_rtcp(
                              int /*channel*/,
                              unsigned char* in_data,
                              unsigned char* out_data,
                              int bytes_in,
                              int* bytes_out)
    {
        err_status_t srtp_err;
        /* enlarge the buffer for srtp to write its data */
        *bytes_out = bytes_in;
        memcpy(out_data, in_data, bytes_in);
        
        srtp_err=srtp_protect_rtcp(session,out_data,&bytes_in);
        if (srtp_err==err_status_ok)
        {
            WEBRTC_TRACE(kTraceDebug, kTraceVoice, 0,"VoeEncrySrtp->encrypt_rtcp srtp_protect_rtcp() err_status_ok\n");
            *bytes_out = bytes_in;
            return;
        }
        WEBRTC_TRACE(kTraceError, kTraceVoice, 0,"VoeEncrySrtp->encrypt_rtcp srtp_protect_rtcp() failed (%d)\n", srtp_err);
        return;
    }
    
    // Decrypts a RTCP packet. Otherwise, this method has the same contract as
    // decrypt().
    void VoeEncrySrtp::decrypt_rtcp(
                              int /*channel*/,
                              unsigned char* in_data,
                              unsigned char* out_data,
                              int bytes_in,
                              int* bytes_out)
    {
        int err = 0;
        err_status_t srtp_err;
        /* keep NON-RTP data unencrypted */
        rtcp_common_header_t *rtcp;
        if (err>=RTCP_COMMON_HEADER_SIZE)
        {
            rtcp = (rtcp_common_header_t*)in_data;
            if (rtcp->version!=2)
            {
                WEBRTC_TRACE(kTraceError, kTraceVoice, 0,"VoeEncrySrtp->decrypt_rtcp VERSION NOT MATCH!\n");
                return;
            }
        }
        
        memcpy(out_data, in_data, bytes_in);
        
        srtp_err=srtp_unprotect_rtcp(session,out_data,bytes_out);
        if (srtp_err==err_status_ok)
        {
            WEBRTC_TRACE(kTraceDebug, kTraceVoice, 0,"VoeEncrySrtp->encrypt_rtcp srtp_unprotect_rtcp() err_status_ok\n");
            *bytes_out = bytes_in;
            return ;
        }
        else {
            
            WEBRTC_TRACE(kTraceDebug, kTraceVoice, 0,"VoeEncrySrtp->decrypt_rtcp srtp_unprotect_rtcp() failed (%d)\n", srtp_err);
            return;
        }
        return;
    }
    err_status_t VoeEncrySrtp::ccp_srtp_init(void)
    {
        err_status_t st=(err_status_t)0;
//        printf("srtp init\n");
        
//        printf("srtp_init_done = %d\n",srtp_init_done);
        WEBRTC_TRACE(kTraceDebug, kTraceVoice, 0,"srtp init\n");
        if (!srtp_init_done) {
//            printf("srtp init not init\n");
            WEBRTC_TRACE(kTraceDebug, kTraceVoice, 0,"srtp init not init\n");
            st=srtp_init();
//            printf("st = %d\n",st);
            if (st==0) {
                srtp_init_done++;
            }else{
                WEBRTC_TRACE(kTraceError, kTraceVoice, 0,"Couldn't initialize SRTP library.");
                err_reporting_init("oRTP");
            }
        }else srtp_init_done++;
//        printf("srtp_init_done = %d after\n",srtp_init_done);
        return st;
    }
    err_status_t VoeEncrySrtp::ccp_srtp_create(const srtp_policy_t *policy)
    {
//        printf("sean111111 %s begins ...\n",__FUNCTION__);
        int i;
//        err_status_t err;
//        err = ccp_srtp_init();
//        if (err) {
//            printf("error: srtp init failed with error code %d\n", err);
//            exit(1);
//        }
        
        i = srtp_create(&session, policy);
//        printf("sean111111 %s ends status = %d ...\n",__FUNCTION__,i);
        return (err_status_t)i;
    }
    err_status_t VoeEncrySrtp::ccp_srtp_dealloc(srtp_t session)
    {
        if (session) {
            return srtp_dealloc(session);
            session = NULL;
        }
        return (err_status_t)0;
        
    }
    err_status_t VoeEncrySrtp::ccp_srtp_add_stream(srtp_t session, const srtp_policy_t *policy)
    {
        return srtp_add_stream(session, policy);
    }
    err_status_t VoeEncrySrtp::ccp_crypto_get_random(uint8_t *tmp, int size)
    {
        return crypto_get_random(tmp, size);
    }
    bool VoeEncrySrtp::ccp_srtp_supported(void)
    {
        return true;
    }

    
    err_status_t VoeEncrySrtp::ccp_srtp_create_configure_session(uint32_t ssrc, const char* snd_key, const char* rcv_key)
    {
        err_status_t err;
		
        err = ccp_srtp_create(NULL);
        if (err != err_status_ok) {
            WEBRTC_TRACE(kTraceError, kTraceVoice, 0,"Failed to create srtp session (%d)\n", err);
            return err;
        }
        
        // incoming stream
        {
            ssrc_t incoming_ssrc;
            srtp_policy_t policy;
            
            memset(&policy, 0, sizeof(srtp_policy_t));
            incoming_ssrc.type = ssrc_any_inbound;
            
            if (!ccp_init_srtp_policy(session, &policy, incoming_ssrc, rcv_key)) {
                ccp_srtp_dealloc(session);
				session = NULL;
				return err_status_fail;
            }
        }
        // outgoing stream
        {
            ssrc_t outgoing_ssrc;
            srtp_policy_t policy;
            
            memset(&policy, 0, sizeof(srtp_policy_t));
            
            outgoing_ssrc.type = ssrc_specific;
            outgoing_ssrc.value = ssrc;
            
            if (!ccp_init_srtp_policy(session, &policy, outgoing_ssrc, snd_key)) {
                ccp_srtp_dealloc(session);
				session = NULL;
				return err_status_fail;
            }
        }
        
        return err_status_ok;
    }
    
    bool VoeEncrySrtp::ccp_init_srtp_policy(srtp_t srtp, srtp_policy_t* policy, ssrc_t ssrc, const char* b64_key)
    {
        uint8_t* key;
        int key_size;
        err_status_t err;
        unsigned b64_key_length = strlen(b64_key);
//        printf("b64_key_length = %d\n",b64_key_length);
		
//        printf("check key after basd64 = %s\n",b64_key);

        switch (_suite) {
            case CCPAES_128_SHA1_32:
            {
                WEBRTC_TRACE(kTraceDebug, kTraceVoice, 0,"ccp_init_srtp_policy AES_128_SHA1_32\n");
                crypto_policy_set_aes_cm_128_hmac_sha1_32(&policy->rtp);
                // srtp doc says: not adapted to rtcp...
                crypto_policy_set_aes_cm_128_hmac_sha1_32(&policy->rtcp);
            }
                break;
            case CCPAES_128_NO_AUTH:
            {
                WEBRTC_TRACE(kTraceDebug, kTraceVoice, 0,"ccp_init_srtp_policy AES_128_NO_AUTH\n");
                crypto_policy_set_aes_cm_128_null_auth(&policy->rtp);
                // srtp doc says: not adapted to rtcp...
                crypto_policy_set_aes_cm_128_null_auth(&policy->rtcp);
            }
                break;
            case CCPNO_CIPHER_SHA1_80:
            {
                WEBRTC_TRACE(kTraceDebug, kTraceVoice, 0,"ccp_init_srtp_policy NO_CIPHER_SHA1_80\n");
                crypto_policy_set_null_cipher_hmac_sha1_80(&policy->rtp);
                crypto_policy_set_null_cipher_hmac_sha1_80(&policy->rtcp);
            }
                break;
            case CCPAES_256_SHA1_80:
            {
                WEBRTC_TRACE(kTraceDebug, kTraceVoice, 0,"ccp_init_srtp_policy AES_256_SHA1_80\n");
                crypto_policy_set_aes_cm_256_hmac_sha1_80(&policy->rtp);
                // srtp doc says: not adapted to rtcp...
                crypto_policy_set_aes_cm_256_hmac_sha1_80(&policy->rtcp);
            }
                break;
            case CCPAES_256_SHA1_32:
            {
                WEBRTC_TRACE(kTraceDebug, kTraceVoice, 0,"ccp_init_srtp_policy AES_256_SHA1_32\n");
                crypto_policy_set_aes_cm_256_hmac_sha1_32(&policy->rtp);
                // srtp doc says: not adapted to rtcp...
                crypto_policy_set_aes_cm_256_hmac_sha1_32(&policy->rtcp);
            }
                break;
            case CCPAES_128_SHA1_80: /*default mode*/
            default:
            {
                WEBRTC_TRACE(kTraceDebug, kTraceVoice, 0,"ccp_init_srtp_policy AES_128_SHA1_80/default\n");
                crypto_policy_set_rtp_default(&policy->rtp);
                crypto_policy_set_rtcp_default(&policy->rtcp);
            }
        }
        key_size = b64::b64_decode(b64_key, b64_key_length, 0, 0);

        key = (uint8_t*) malloc(key_size+2); /*srtp uses padding*/

        int final_key_size = b64::b64_decode(b64_key, b64_key_length, key, key_size);
//        printf("Here we use b64_decode key = %s\n",key);
        if (final_key_size != policy->rtp.cipher_key_len) {
            WEBRTC_TRACE(kTraceError, kTraceVoice, 0,"Key size (%d) doesn't match the selected srtp profile (required %d)\n",
                       key_size,
                       policy->rtp.cipher_key_len);
            free(key);
			return false;
        }
        
//        if (b64::b64_decode(b64_key, b64_key_length, key, key_size) != key_size) {
//            printf("Error decoding key\n");
//            free(key);
//            return false;
//        }

        policy->ssrc = ssrc;
        policy->key = key;
        policy->next = NULL;

		policy->allow_repeat_tx = true;
		policy->window_size = 1024;
        
        err = ccp_srtp_add_stream(srtp, policy);
        if (err != err_status_ok) {
            WEBRTC_TRACE(kTraceDebug, kTraceVoice, 0,"Failed to add incoming stream to srtp session (%d)\n", err);
            free(key);
            return false;
        }
		
        free(key);
        return true;
    }
    
    int VoeEncrySrtp::ccp_srtp_shutdown(void)
    {
        err_status_t err = err_status_ok;
        srtp_init_done--;
//        printf("srtp_init_done = %d\n",srtp_init_done);
        if (srtp_init_done==0){
//            printf("We go into srtp shutdown\n");
//#ifdef HAVE_SRTP_SHUTDOWN
//            printf("We Have srtp shotdown\n");
            err = srtp_shutdown();
//#endif
        }
        return err;
    }
    //正常情况返回0
    //其他返回值参考err.h
    int VoeEncrySrtp::ccp_srtp_configure_incoming(const char* rcv_key)
    {
        err_status_t err;
		
        if (!_srtpCreate) {
            err = ccp_srtp_create(NULL);
            if (err != err_status_ok) {
                WEBRTC_TRACE(kTraceError, kTraceVoice, 0,"Failed to create srtp session (%d)\n", err);
                return err;
            }
            _srtpCreate = true;
        }
        ssrc_t incoming_ssrc;
        srtp_policy_t policy;
        
        memset(&policy, 0, sizeof(srtp_policy_t));
        incoming_ssrc.type = ssrc_any_inbound;

        if (!ccp_init_srtp_policy(session, &policy, incoming_ssrc, rcv_key)) {
            ccp_srtp_dealloc(session);
			session = NULL;
            return err_status_fail;
        }
        return 0;
        
        
        
        
    }
    int VoeEncrySrtp::ccp_srtp_configure_outgoing(uint32_t ssrc, const char* snd_key)
    {
        
        err_status_t err;
		
        if (!_srtpCreate) {
            err = ccp_srtp_create(NULL);
            if (err != err_status_ok) {
                WEBRTC_TRACE(kTraceError, kTraceVoice, 0,"Failed to create srtp session (%d)\n", err);
                return err;
            }
            _srtpCreate = true;
        }
        ssrc_t outgoing_ssrc;
        srtp_policy_t policy;
        
        memset(&policy, 0, sizeof(srtp_policy_t));
        
        outgoing_ssrc.type = ssrc_specific;
        outgoing_ssrc.value = ssrc;
       
//        printf("ssrc in %s %u",__FUNCTION__,ssrc);
        if (!ccp_init_srtp_policy(session, &policy, outgoing_ssrc, snd_key)) {
            ccp_srtp_dealloc(session);
			session = NULL;
			return err_status_fail;
        }
        return 0;
    }
    
}
