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
    long res = NeAACDecInit(handle, pData, nLen, (unsigned long*)&pCtx->sample_rate, (unsigned char*)&pCtx->channels);
    if (res < 0) {
        printf("NeAACDecInit failed\n");
        return -1;
    }
    NeAACDecFrameInfo info;
    uint32_t framelen = _get_frame_length(pData);
    unsigned char *buf = (unsigned char *)NeAACDecDecode(handle, &info, pData, nLen);
    if (buf && info.error == 0) {
        if (info.samplerate == 44100) {
            //src: 2048 samples, 4096 bytes
            //dst: 2048 samples, 4096 bytes
            int tmplen = (int)info.samples * 16 / 8;
            memcpy(pPCM,buf,tmplen);
            *outLen = tmplen;
        } else if (info.samplerate == 22050) {
            //src: 1024 samples, 2048 bytes
            //dst: 2048 samples, 4096 bytes
            short *ori = (short*)buf;

            short* tmpbuf = new short[info.samples * 2];
            int tmplen = (int)info.samples * 16 / 8 * 2;
            for (int32_t i = 0, j = 0; i < info.samples; i += 2) {
                tmpbuf[j++] = ori[i];
                tmpbuf[j++] = ori[i + 1];
                tmpbuf[j++] = ori[i];
                tmpbuf[j++] = ori[i + 1];
            }
            memcpy(pPCM,tmpbuf,tmplen);
			delete tmpbuf;
            *outLen = tmplen;
        }else if(info.samplerate == 8000){
            //从双声道的数据中提取单通道
            for(int i=0,j=0; i<4096 && j<2048; i+=4, j+=2)
            {
                pPCM[j]= buf[i];
                pPCM[j+1]=buf[i+1];
            }
            *outLen = (unsigned int)info.samples;
        }
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
	pConfiguration->outputFormat = 0; //	// 0 = Raw; 1 = ADTS  
	pConfiguration->aacObjectType = LOW;
	pConfiguration->allowMidside = 0;
	pConfiguration->useLfe = 0;
	pConfiguration->bitRate = 48000;
	pConfiguration->bandWidth = 32000;

	faacEncSetConfiguration(context->handle, pConfiguration);
	return context;
}

int faac_encode_frame(void* handle, unsigned char *pPCM,unsigned char **outbuf,  int *outLen)
{
	if (!handle)
		return -1;
	FAACContext *context = (FAACContext *)handle;
	int len  = faacEncEncode(context->handle , (int32_t *) pPCM,context->inputSamples,
		context->outputBuffer,context->maxOutputBytes);

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
