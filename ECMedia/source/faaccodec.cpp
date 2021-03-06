//
//  faaccodec.cpp
//  ECMedia
//
//  Created by james on 16/9/23.
//  Copyright ? 2016年 Sean Lee. All rights reserved.
//

#include "faaccodec.h"
#include "faad.h"
#include "faac.h"
#include <stdlib.h>
#include <memory.h>


#if defined(_WIN32)
#include <winsock2.h>
#include <ws2tcpip.h>
#include <cstdint>
#else
#include <arpa/inet.h>  // ntohl()
#endif

typedef struct {
    NeAACDecHandle handle;
    int sample_rate;
    int channels;
    int bit_rate;
}FAADContext;

uint32_t _get_frame_length(const unsigned char *aac_header)
{
    uint32_t len = *(uint32_t *)(aac_header + 3);
    len = ntohl(len); //Little Endian
    len = len << 6;
    len = len >> 19;
    return len;
}

void *faad_decoder_create(int sample_rate, int channels, int bit_rate)
{
    NeAACDecHandle handle = NeAACDecOpen();
    if(!handle){
        printf("NeAACDecOpen failed\n");
        return NULL;
    }
    NeAACDecConfigurationPtr conf = NeAACDecGetCurrentConfiguration(handle);
    if(!conf){
        printf("NeAACDecGetCurrentConfiguration failed\n");
        NeAACDecClose(handle);
        return NULL;
    }
    conf->defSampleRate = sample_rate;
    conf->outputFormat = FAAD_FMT_16BIT;
    conf->dontUpSampleImplicitSBR = 1;
    NeAACDecSetConfiguration(handle, conf);
    
    FAADContext* ctx =  new FAADContext;
    ctx->handle = handle;
    ctx->sample_rate = sample_rate;
    ctx->channels = channels;
    ctx->bit_rate = bit_rate;
    return ctx;

}

int faad_decode_frame(void *pParam, unsigned char *pData, int nLen, unsigned char *pPCM, unsigned int *outLen)
{
    FAADContext* pCtx = (FAADContext*)pParam;
    NeAACDecHandle handle = pCtx->handle;
	*outLen = 0;
    /*long res = NeAACDecInit(handle, pData, nLen, (unsigned long*)&pCtx->sample_rate, (unsigned char*)&pCtx->channels);
    if (res < 0) {
        printf("NeAACDecInit failed\n");
        return -1;
    }*/
    NeAACDecFrameInfo info;
    uint32_t framelen = _get_frame_length(pData);
    unsigned char *buf = (unsigned char *)NeAACDecDecode(handle, &info, pData, nLen);
    if (buf && info.error == 0) {
		if (info.samples == 0) {
			return 0;
		}
        int tmplen = (int)info.samples * 16 / 8;
        memcpy(pPCM,buf,tmplen);
        *outLen = tmplen;
    } else {
        printf("NeAACDecDecode failed --- %s\n", NeAACDecGetErrorMessage(info.error));
        return -1;
    }
    return 0;
}

void faad_decoder_close(void *pParam)
{
    if(!pParam){
        return;
    }
    FAADContext* pCtx = (FAADContext*)pParam;
    if(pCtx->handle){
        NeAACDecClose(pCtx->handle);
    }
    free(pCtx);
}

typedef struct {
	faacEncHandle handle;
	int sample_rate;
	int channels;
	unsigned char *outputBuffer;
	unsigned long maxOutputBytes;
	unsigned long inputSamples;
}FAACContext;


void *faac_encoder_crate(int sample_rate, int channels, unsigned long* inputSamples)
{
	FAACContext *context = new FAACContext;
	context->channels = channels;
	context->sample_rate = sample_rate;
	context->handle = faacEncOpen(sample_rate, channels,
		 inputSamples, &context->maxOutputBytes );
	
	if (!context->handle) {
		delete context;
		return NULL;
	}
	context->inputSamples = *inputSamples;
	context->outputBuffer = new unsigned char[context->maxOutputBytes];
	faacEncConfigurationPtr pConfiguration = faacEncGetCurrentConfiguration(context->handle);
	pConfiguration->inputFormat = FAAC_INPUT_16BIT;
	pConfiguration->outputFormat = 1; //	// 0 = Raw; 1 = ADTS
	pConfiguration->aacObjectType = LOW;
	pConfiguration->allowMidside = 0;
	pConfiguration->useLfe = 0;
	pConfiguration->bitRate = 48000;
	pConfiguration->bandWidth = 32000;

	faacEncSetConfiguration(context->handle, pConfiguration);
	return context;
}

int faac_encode_frame(void* handle, unsigned char *pPCM, unsigned char **outbuf,  int *outLen)
{
	if (!handle)
		return -1;
	FAACContext *context = (FAACContext *)handle;
	int len  = faacEncEncode(context->handle , (int32_t *) pPCM,  context->inputSamples,
		context->outputBuffer, context->maxOutputBytes);

	*outbuf = context->outputBuffer;
	*outLen = len;
	return len;
}


void  faac_encoder_close(void *handle)
{
	if (!handle)
		return;
	FAACContext *context = (FAACContext *)handle;
	faacEncClose(context->handle);
	delete context->outputBuffer;
	delete context;
}

int faac_get_decoder_info(void* handle,unsigned char **buf, unsigned long *len)
{
	if (!handle)
		return -1;
	FAACContext *context = (FAACContext *)handle;
	return faacEncGetDecoderSpecificInfo(context->handle , buf, len);
}

int faad_decoder_init(void *pParam, unsigned char *pData, int nLen,unsigned int &sampleRate, unsigned int &channels)
{
	FAADContext* pCtx = (FAADContext*)pParam;
	NeAACDecHandle handle = pCtx->handle;
	long res = NeAACDecInit(handle, pData, nLen, (unsigned long*)&pCtx->sample_rate, (unsigned char*)&pCtx->channels);
	if (res < 0) {
		printf("NeAACDecInit failed\n");
		return -1;
	}
	sampleRate = pCtx->sample_rate;
	channels = pCtx->channels;
	return 0;
}

int faad_decoder_getinfo(char *aacconfig,unsigned int &sampleRate, unsigned int &channels)
{
	int sampleindex = ( (aacconfig[0] & 0x7) << 1) |  ((unsigned char)(aacconfig[1] & 0x80) >> 7);
	switch (sampleindex) {
	case 0 :
		sampleRate = 96000;
		break;
	case 1:
		sampleRate = 88200;
		break;
	case 2:
		sampleRate = 64000;
		break;

	case 3:
		sampleRate = 48000;
		break;
	case 4:
		sampleRate = 44100;
		break;
	case 5:
		sampleRate = 32000;
		break;
	case 6:
		sampleRate = 24000;
		break;
	case 7:
		sampleRate = 22050;
		break;
	case 8:
		sampleRate = 16000;
		break;
	case 10:
		sampleRate = 11025;
		break;
	case 11:
		sampleRate = 8000;
		break;
	default:
		break;
	}
	channels = (aacconfig[1] & 0x78) >> 3;
	return 0;

}
