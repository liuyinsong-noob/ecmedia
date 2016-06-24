/*////////////////////////////////////////////////////////////////////////
//
// INTEL CORPORATION PROPRIETARY INFORMATION
// This software is supplied under the terms of a license agreement or
// nondisclosure agreement with Intel Corporation and may not be copied
// or disclosed except in accordance with the terms of that agreement.
// Copyright (c) 2005 Intel Corporation. All Rights Reserved.
//
//   Intel(R)  Integrated Performance Primitives
//
//     USC speech codec sample
//
// By downloading and installing this sample, you hereby agree that the
// accompanying Materials are being provided to you under the terms and
// conditions of the End User License Agreement for the Intel(R) Integrated
// Performance Primitives product previously accepted by you. Please refer
// to the file ipplic.htm located in the root directory of your Intel(R) IPP
// product installation for more information.
//
// Purpose: Wave file reading/writing  functions header file.
//
////////////////////////////////////////////////////////////////////////*/
#ifndef __WAVFILE_H__
#define __WAVFILE_H__

#define LINIAR_PCM        1
#define ALAW_PCM          6
#define MULAW_PCM         7

enum {
   FILE_READ                = 0x0002,
   FILE_WRITE               = 0x0004
};

typedef struct _WaveFormat {
   short  nFormatTag;
   short  nChannels;
   int    nSamplesPerSec;
   int    nAvgBytesPerSec;
   short  nBlockAlign;
   short  nBitPerSample;
   int    cbSize;
}WaveFormat;

typedef struct _WaveFileParams {
   int isReadMode;
   int isFirstTimeAccess;
   void *FileHandle;
   WaveFormat    waveFmt;
   unsigned int  DataSize;
   unsigned int  RIFFSize;
   unsigned int  bitrate;
} WaveFileParams;

int OpenWavFile(WaveFileParams *wfParams, char* FileName, unsigned int mode);
void InitWavFile(WaveFileParams *wfParams);
int WavFileRead(WaveFileParams *wfParams, void* Data, int size);
int WavFileWrite(WaveFileParams *wfParams, void* Data, int size);
int WavFileClose(WaveFileParams *wfParams);
int WavFileReadHeader(WaveFileParams *wfParams);
int WavFileWriteHeader(WaveFileParams *wfParams);

#endif /*__WAVFILE_H__*/
