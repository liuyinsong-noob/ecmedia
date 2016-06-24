#ifndef MODULES_AUDIO_CODING_CODECS_ARMNB_MAIN_INTERFACE_ARMNB_INTERFACE_H_
#define MODULES_AUDIO_CODING_CODECS_ARMNB_MAIN_INTERFACE_ARMNB_INTERFACE_H_
#ifdef __cplusplus
extern "C" {
#endif

//#ifndef AMRNB_WRAPPER_INTERNAL
///* Copied from enc/src/gsmamr_enc.h */
//enum Mode {
//	MR475 = 0,/* 4.75 kbps */
//	MR515,    /* 5.15 kbps */
//	MR59,     /* 5.90 kbps */
//	MR67,     /* 6.70 kbps */
//	MR74,     /* 7.40 kbps */
//	MR795,    /* 7.95 kbps */
//	MR102,    /* 10.2 kbps */
//	MR122,    /* 12.2 kbps */
//	MRDTX,    /* DTX       */
//	N_MODES   /* Not Used  */
//};
//#endif

short WebRtcAmr_CreateEnc(void** encInst);
short WebRtcAmr_CreateDec(void** decInst);
short WebRtcAmr_FreeEnc(void* encInst);
short WebRtcAmr_FreeDec(void* decInst);
short WebRtcAmr_Encode(void* encInst,
                          short* input,
                          short len,
                          short*output,
                          short mode);
short WebRtcAmr_EncoderInit(void* encInst,
                               short dtxMode);
short WebRtcAmr_EncodeBitmode(void* encInst,
                                 int format);
short WebRtcAmr_Decode(void* decInst,short* encoded,
                                                short len, short* decoded,
                                                short* speechType);
short WebRtcAmr_DecodePlc(void* decInst);
short WebRtcAmr_DecoderInit(void* decInst);
short WebRtcAmr_DecodeBitmode(void* decInst,
                                 int format);
short WebRtcAmr_Version(char *versionStr, short len);


#ifdef __cplusplus
}
#endif


#endif
