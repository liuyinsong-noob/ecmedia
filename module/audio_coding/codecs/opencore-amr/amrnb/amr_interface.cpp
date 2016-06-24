
#include <stdlib.h>
#include <string.h>

#define AMRNB_WRAPPER_INTERNAL
#include <sp_dec.h>
#include <amrdecode.h>
#include <amrencode.h>
#include "amr_interface.h"
#include "interf_dec.h"
#include "interf_enc.h"

#define FRAME_SIZE            320
#define toc_get_f(toc)      ((toc) >> 7)
#define toc_get_index(toc)  ((toc>>3) & 0xf)

const int amr_frame_sizes[]={12,13,15,17,19,20,26,31,5,0 };

static int toc_list_check(const char *tl,size_t buflen)
{
	size_t s=1;
	while(toc_get_f(*tl)){
		tl++;
		s++;
		if (s>buflen){
			return -1;
		}
	}
	return s;
}

struct encoder_state {
	void* encCtx;
	void* pidSyncCtx;
};

short WebRtcAmr_CreateEnc(void** encInst)
{
	struct encoder_state* state = (struct encoder_state*) malloc(sizeof(struct encoder_state));
	*encInst = (void *)state;
	return 0;
}

short WebRtcAmr_CreateDec(void** decInst)
{
	short ret;
	ret = GSMInitDecode(decInst, (int8*)"Decoder");
	return ret;
}

short WebRtcAmr_FreeEnc(void* encInst)
{
	struct encoder_state* state = (struct encoder_state*) encInst;
	AMREncodeExit(&state->encCtx, &state->pidSyncCtx);
	free(state);
	return 0;
}

short WebRtcAmr_FreeDec(void* decInst)
{
	GSMDecodeFrameExit(&decInst);
	return 0;
}

short WebRtcAmr_Encode(void* encInst,
                          int16_t* input,
                          int16_t len,
                          int16_t*output,
                          int16_t mode)
{
	char * pOut =(char *)output;
	//len = len*sizeof(int16_t);
	//len /= FRAME_SIZE;
	//*pOut++ = (char)0xf0;
	//if (len -1)
	//{
	//	memset(pOut,0xbc,len-1);
	//	pOut += len-1;
	//}
	//int nFrame;
	//char cSav =0;
	short ret;
	//for (nFrame=0; nFrame<len; ++nFrame)
	//{
	    ret = Encoder_Interface_Encode(encInst,(enum Mode)mode,input,(unsigned char *)pOut,0);
		if ( ret<0 )
		{
			//encode fail
			return 0;
		}
		//if (nFrame)
		//	*pOut = cSav; // replace the first byte of this frame with org-val
		//input += FRAME_SIZE/sizeof(int16_t);
		//pOut += ret -1;  // for skipping the first byte of next frame
		//cSav = *pOut;
	//}
	// no frames, jump to correct pos
	//++pOut;
	//ret = pOut-(char *)output;
	return ret;
}

short WebRtcAmr_Decode(void* decInst,int16_t* encoded,
                                                int16_t len, int16_t* decoded,
                                                int16_t* speechType)
{
	char * indata = (char *)encoded;
	if ( len<2 )
	{
		return -1;
	}
	// skip payload header, ignore CMR
	const char * inEnd = (char *)indata+len;
    // see the number of TOCs :
    const unsigned char* tocs = (const unsigned char*)++indata;
	int toclen = toc_list_check((const char*)tocs,len);
    unsigned char tmp[32];
    int i;
    int index;
    int framesz;
    if (toclen==-1){
		//format is uncorrect
        return -1;
    }
    indata += toclen;
    len = 0;
// iterate through frames, following the toc list
    for(i=0; i<toclen; ++i){
        index = toc_get_index(tocs[i]);
        if (index>=9) {
             //Bad amr toc
             return -1;
        }
        framesz=amr_frame_sizes[index];
        if (indata+framesz>inEnd) {
            // Truncated amr frame!
            return -1;
        }
        tmp[0] = tocs[i];
        memcpy(&tmp[1],indata,framesz);
        Decoder_Interface_Decode(decInst,tmp,(short*)decoded,0);
        decoded += FRAME_SIZE/sizeof(short);
        len += FRAME_SIZE/sizeof(short);
        indata += framesz;
    }
    return len;
}

short WebRtcAmr_Version(char *versionStr, short len)
{
    char version[30] = "1.0.0\n";
    if (strlen(version) < (unsigned int)len)
    {
        strcpy(versionStr, version);
        return 0;
    }
    else
    {
        return -1;
    }
}

short WebRtcAmr_EncoderInit(void* encInst,
                               int16_t dtxMode)
{
	struct encoder_state* state = (struct encoder_state*) encInst;
	return AMREncodeInit(&state->encCtx, &state->pidSyncCtx, dtxMode);
}

short WebRtcAmr_EncodeBitmode(void* encInst,
                                 int format)
{
	return 0;
}

short WebRtcAmr_DecodePlc(void* decInst)
{
	return 0;
}

short WebRtcAmr_DecoderInit(void* decInst)
{
	return 0;
}

int16_t WebRtcAmr_DecodeBitmode(void* decInst,
                                 int format)
{
	return 0;
}

