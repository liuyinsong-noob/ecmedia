//
//  faaccodec.hpp
//  ECMedia
//
//  Created by james on 16/9/23.
//  Copyright © 2016年 Sean Lee. All rights reserved.
//

#ifndef faaccodec_h
#define faaccodec_h

#include <stdio.h>

void *faad_decoder_create(int sample_rate, int channels, int bit_rate);
int faad_decode_frame(void *handle, unsigned char *pData, int nLen, unsigned char *pPCM, unsigned int *outLen);
void faad_decoder_close(void *handle);

void *faac_encoder_crate(int sample_rate,int channels, unsigned long* inputSamples);
int faac_encode_frame(void* handle, unsigned char *pPCM, unsigned char **pOut, int *outLen);
void  faac_encoder_close(void *handle);
int faac_get_decoder_info(void* handle, unsigned char **buf, unsigned long *len);
#endif /* faaccodec_h */
