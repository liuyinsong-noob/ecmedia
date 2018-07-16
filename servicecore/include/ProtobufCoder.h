#ifndef PROTOBUF_CODER_H_
#define PROTOBUF_CODER_H_


#include <google/protobuf/message_lite.h>
#include <stdlib.h>
#include <vector>
#include "CCPClient.h"
#define PROTOBUF_CODER_OK   0
////错误码定义
//enum {
//	ERR_PROTOBUF_CODER_NULL =170100,//空指针
//	ERR_PROTOBUF_CODER_ENCODE_SERRIALIZE,//编码序列化失败
//	ERR_PROTOBUF_CODER_DECODE_READ_VARINT32,//解码读取变长失败
//	ERR_PROTOBUF_CODER_DECODE_MERGE,//解码反序列化失败
//	ERR_PROTOBUF_CODER_DECODE_CONSUMED,//解码反序列化非法
//};
/*
protobuf 编码的一些猜测
例如
message MsgLiteInner {
	optional uint32 protoType = 1;  // 消息类型  参考文件：方法类型数值说明.txt
	optional bytes protoData = 2;  // 消息数据
	optional uint32 protoClientNo= 3;  // 用于存放客户端流水号
	optional string protoBackExp = 4;  // 用于存放后台模块的扩展信息
	optional string protoSource = 5;  // 用于存放生产者源地址
	optional uint32 protoErrorCode = 6; // 服务器返回错误码
	optional uint32 protoEncrypt = 7;  // 是否加密，有值就是加密
	optional string protoToken = 8;  // 精简认证token
}

 prototype=29,protoerrorcode=200,protoclientno=4

16进制数
07 08 ID 18 04 30 C8 01

解码：
07 代表数据长度为7
08 二进制0000 1000，从前5位是序列号，即0000 1 代表1,protoType = 1;  // 消息类型
1D 十进制29(protobuf特有的变长整型，字节首位为进制占位，后7位为数据位，即128进制，例如1D=00011101 =29*128^0)
18 即0001 1000代表3，protoClientNo= 3;  // 用于存放客户端流水号
04 流水号值4
30 即0011 0000代表6，protoErrorCode = 6; // 服务器返回错误码
C8 01 错误码值200,考虑到小端字节序即11001000去掉首位1，做低位，00000001去掉首位，左移7位，做高位；11001000 00000001->0000001 1001000=C8=200
                       或者 1*128^1+72*128^0=200
                              


又例如
message JoinGroupInner {
        optional string joiner = 1; //申请加入者
        optional string groupId= 2; //群组ID
	optional string declared = 3;//申请加入理由，最长200个字符
}
prototype=33,protoclientno=48 
抓包即分析：
109	2015-05-29 14:18:07.147745000	172.26.153.4	192.168.178.226	TCP	119	39605→8085 [PSH, ACK] Seq=807 Ack=922 Win=115 Len=65
data：
40:08:21:12:3a:39:0a:0b:31:35:38:31:30:37:36:38:38:32:39:12:0d:67:31:30:30:30:30:31:31:30:30:31:33:32:1a:1b:40:2a:25:7e:2f:61:62:
63:64:e7:94:b3:e8:af:b7:e5:8a:a0:e5:85:a5:e7:be:a4:e7:bb:84:18:30
解码如下
40 代表实际内容长度为64
08 即 00001 000,代表类型字段protoType = 1
21 代表类型十进制33
12  即00010 010，前5位值为2，代表 protoData = 2;后3位值代表内容JoinGroupInner有2个值
3a  代表protoData 数据长度为58
。。。。。数据
18  即00011 000，代表protoClientNo= 3;
30  代表流水号是48
*/
namespace CcpClientYTX{
class TProtobufCoder
{
public: 
    TProtobufCoder();
    ~TProtobufCoder();
    
	int   EncodeMessage(cloopen_google::protobuf::MessageLite* pmLite);
    int   DecodeMessage(cloopen_google::protobuf::MessageLite* pmLite,char* inputData,int length);
    /*
    int   DecodeSyncMutilMessage(std::vector<MessageContent*> *messages, char* inputData,int length);
    void  MemcpyStringToChar(std::string string, char** chString);
     */
public:
    char* m_sEncodedData;
    int   m_nEncodedLen;

protected:

};

}//end namespace CcpClientYTX
#endif