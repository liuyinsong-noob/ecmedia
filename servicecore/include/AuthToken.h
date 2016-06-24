#ifndef __AUTHTOKEN__H__
#define  __AUTHTOKEN__H__

#include <stdio.h>

#define MAX_AUTHTOKEN_LEN   10240
#define KEY_COUNT  5

//{"calling":"1","called":"1","msgflag":"1","createconf":"1","joinconf":"1"}
const char KEY_TOKEN[KEY_COUNT][32] = {"calling", "called", "msgflag", "createconf", "joinconf"};

/************************************************************************
* 函  数  名：DecodeAuthToken
* 功     能：解析获取能力级信息
* 入     参：加密的能力级字符串
* 入     参：解密的密码，暂时没有使用
* 出     参：能力级信息串，格式为010101  0代表没有该能力，1代表有，2 代表异常
* 返  回  值：-1 失败  0 成功
* 作     者：
************************************************************************/
int  DecodeAuthToken(const char * token, const char* key, char * output);


/************************************************************************
* 函  数  名：EncodeAuthToken
* 功     能：获取用户的能力级信息
* 入     参：能力级信息串
* 出     参：编码后的能力级信息
* 返  回  值：-1 失败  0 成功
* 作     者：
************************************************************************/

int  EncodeAuthToken(const char * token, char *output);

#endif
