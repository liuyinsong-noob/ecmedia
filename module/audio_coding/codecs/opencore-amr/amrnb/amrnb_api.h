//
//  amrnb_api.h
//  amrnb
//
//  Created by hubin on 13-6-6.
//  Copyright (c) 2013年 hisunsray. All rights reserved.
//

#ifndef amrnb_amrnb_api_h
#define amrnb_amrnb_api_h

#ifdef __cplusplus
extern "C" {
#endif
    
int AmrNBCreateEnc();
int AmrNBCreateDec();
int AmrNBFreeEnc();
int AmrNBFreeDec();
int AmrNBEncode(short* input,
                       short len,
                       short*output,
                       short mode);
int AmrNBEncoderInit(short dtxMode);
int AmrNBDecode(short* encoded,
                       int len, short* decoded);
int AmrNBDecodeBitmode(int format);
int AmrNBVersion(char *versionStr, short len);

#ifdef __cplusplus
}
#endif

#endif
