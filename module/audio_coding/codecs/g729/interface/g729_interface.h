#pragma once

#ifndef _G729_INTERFACE_H_
#define _G729_INTERFACE_H_

/*
#ifdef __cplusplus
extern "C" {
#endif
*/
typedef struct G729_encinst_t_ G729_encinst_t;
typedef struct G729_decinst_t_ G729_decinst_t;


#if !defined(_MSC_VER)
#include <stdint.h>
#else
#include "typedefs.h"
#endif

int16_t WebRtcG729_Encode(G729_encinst_t* encInst, int16_t* input,
										int16_t len, uint8_t* output);

int16_t WebRtcG729_FreeEnc(G729_encinst_t* inst);
int16_t WebRtcG729_FreeDec(G729_decinst_t* inst);
int16_t WebRtcG729_EncoderInit(G729_encinst_t* encInst, int16_t mode);
int16_t WebRtcG729_DecoderInit(G729_decinst_t* decInst);
int16_t WebRtcG729_CreateEnc(G729_encinst_t** inst);
int16_t WebRtcG729_CreateDec(G729_decinst_t** inst);
void WebRtcG729_Version(char *versionStr, short len);
int16_t WebRtcG729_Decode(G729_decinst_t *decInst,
                                  const int16_t *encoded,
                                  int16_t len,
                                  int16_t *decoded,
                                  int16_t *speechType);
int16_t WebRtcG729_DecodePlc(G729_decinst_t *decInst,
                                  int16_t *decoded,
                                  int16_t frames);



#endif //_G729_INTERFACE_H_