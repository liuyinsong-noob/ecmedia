#include "g729_interface.h"	
#include "encoder.h"
#include "decoder.h"
#include "typedef.h"
#include <stdlib.h>

int16_t WebRtcG729_Encode(G729_encinst_t* encInst, int16_t* input,int16_t len, uint8_t* output)
{
	bcg729Encoder((bcg729EncoderChannelContextStruct*)encInst,input,output);
	return 0;
}

int16_t WebRtcG729_FreeEnc(G729_encinst_t* inst)
{
	closeBcg729EncoderChannel((bcg729EncoderChannelContextStruct*)inst);
	return 0;
}

int16_t WebRtcG729_FreeDec(G729_encinst_t* inst)
{
	closeBcg729DecoderChannel((bcg729DecoderChannelContextStruct*)inst);
	return 0;
}

int16_t WebRtcG729_EncoderInit(G729_encinst_t* encInst, int16_t mode)
{
	return initBcg729EncoderChannel((bcg729EncoderChannelContextStruct*)encInst);
}

int16_t WebRtcG729_DecoderInit(G729_decinst_t* decInst)
{
	return initBcg729DecoderChannel((bcg729DecoderChannelContextStruct*)decInst);
}

int16_t WebRtcG729_CreateEnc(G729_encinst_t** inst)
{
	*inst=(G729_encinst_t*)malloc(sizeof(bcg729EncoderChannelContextStruct));
	if (*inst!=NULL) {
		return(0);
	} else {
		return(-1);
	}

}

int16_t WebRtcG729_CreateDec(G729_decinst_t** inst)
{
	*inst=(G729_decinst_t*)malloc(sizeof(bcg729DecoderChannelContextStruct));
	if (*inst!=NULL) {
		return(0);
	} else {
		return(-1);
	}
	return 0;
}

void WebRtcG729_Version(char *versionStr, short len)
{

}

int16_t WebRtcG729fix_Decode(G729_decinst_t *decInst,
                                  int16_t *encoded,
                                  int16_t len,
                                  int16_t *decoded,
                                  int16_t *speechType)
{
	
	bcg729Decoder((bcg729DecoderChannelContextStruct*)decInst, (uint8_t*)encoded, 1/*uint8_t frameErasureFlag*/, decoded);
		*speechType=1;

	return 0;
}


