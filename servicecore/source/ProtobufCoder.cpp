

#include <google/protobuf/io/zero_copy_stream_impl_lite.h>
#include <google/protobuf/io/coded_stream.h>

#include "ProtobufCoder.h"
#include "SyncMsgResp.pb.h"
#include "CCPClient.h"
#include "serphoneinterface.h"

namespace CcpClientYTX{
    enum {//ProtobufCoder err
        ERR_PROTOBUF_CODER_NULL =171100,//空指针,空内容
        ERR_PROTOBUF_CODER_ENCODE_SERRIALIZE=171101,//编码序列化失败
        ERR_PROTOBUF_CODER_DECODE_READ_VARINT32=171102,//解码读取变长失败
        ERR_PROTOBUF_CODER_DECODE_MERGE=171103,//解码反序列化失败
        ERR_PROTOBUF_CODER_DECODE_CONSUMED=171104,//解码反序列化非法
    };
    
    enum {
        LOG_LEVEL_ERR    =10 ,//10以前预留给中间层日志
        LOG_LEVEL_WARNING=11,
        LOG_LEVEL_INFO=12,
        LOG_LEVEL_DEBUG=13,
        LOG_LEVEL_DEBUG_WBSS=14,//白板日志开启级别
        LOG_LEVEL_MEDIA_ERR=20,//媒体库日志,打印ECmedia调用日志和媒体库Error日志；
        LOG_LEVEL_MEDIA_WARNING=21,//添加严重错误的日志；
        LOG_LEVEL_MEDIA_INFO=22,//添加警告日志；
        LOG_LEVEL_MEDIA_DEFAULT=23,//媒体库默认日志，一般打开这个；
        LOG_LEVEL_MEDIA_DEBUG=24,//添加调试信息；
        LOG_LEVEL_MEDIA_ALL=25,//大于24： 所有日志都打开，级别最好，把媒体流的信息都打印出来
        LOG_LEVEL_END=99
    };
TProtobufCoder::TProtobufCoder()
{
    m_sEncodedData=NULL;
    m_nEncodedLen =0;
}

TProtobufCoder::~TProtobufCoder()
{
    if(m_sEncodedData)
    {
		delete[] m_sEncodedData;
		m_sEncodedData=NULL;
	}
	m_nEncodedLen =0;
}

int TProtobufCoder::EncodeMessage(yuntongxun_google::protobuf::MessageLite* pmLite)
{
	if(NULL==pmLite)
	{
		return ERR_PROTOBUF_CODER_NULL;
	}
	 if(m_sEncodedData)
    {
		delete[] m_sEncodedData;
		m_sEncodedData=NULL;
	}
	m_nEncodedLen =0;

	int ret=PROTOBUF_CODER_OK;
	int size = pmLite->ByteSize()+8;//变长 8*8 与7*10基本等价
    m_sEncodedData = new char[size];
	if(NULL==m_sEncodedData)
	{
//        PrintConsole((char*)__FILE__, __LINE__,(char*)__FUNCTION__,LOG_LEVEL_ERR,"ret=%d,m_sEncodedData new char retturn NULL",ERR_PROTOBUF_CODER_NULL);
		return ERR_PROTOBUF_CODER_NULL;
	}
    memset(m_sEncodedData, 0, size);
    yuntongxun_google::protobuf::io::ArrayOutputStream array_stream(m_sEncodedData, size);
    yuntongxun_google::protobuf::io::CodedOutputStream output_stream(&array_stream);
    output_stream.WriteVarint32(pmLite->ByteSize());
    if(pmLite->SerializeToCodedStream(&output_stream))
	{
		m_nEncodedLen = output_stream.ByteCount();
	}else
	{
		ret=ERR_PROTOBUF_CODER_ENCODE_SERRIALIZE;
	}
    
	return ret;
}

int TProtobufCoder::DecodeMessage(yuntongxun_google::protobuf::MessageLite* pmLite,char* inputData,int length)
{
	if(NULL==pmLite||NULL==inputData||length<=0)
	{
//        PrintConsole((char*)__FILE__, __LINE__,(char*)__FUNCTION__,LOG_LEVEL_ERR,"ret=%d,pmLite=%p,inputData=%p,length=%d",ERR_PROTOBUF_CODER_NULL,pmLite,inputData,length);
		return ERR_PROTOBUF_CODER_NULL;
	}
	int ret=PROTOBUF_CODER_OK;
    yuntongxun_google::protobuf::io::CodedInputStream input((yuntongxun_google::protobuf::uint8*)inputData,length);
    
    yuntongxun_google::protobuf::uint32 sizea;
    if (!input.ReadVarint32(&sizea)) {
		ret=ERR_PROTOBUF_CODER_DECODE_READ_VARINT32;
//        PrintConsole((char*)__FILE__, __LINE__,(char*)__FUNCTION__,LOG_LEVEL_ERR,"ret=%d",ret);
		return ret;
    };
    //printf("len=%d\r\n",sizea);
    // Tell the stream not to read beyond that size.
    yuntongxun_google::protobuf::io::CodedInputStream::Limit limit = input.PushLimit(sizea);
    
    // Parse the message.
    if (!pmLite->MergeFromCodedStream(&input)) {
		ret= ERR_PROTOBUF_CODER_DECODE_MERGE;
//        PrintConsole((char*)__FILE__, __LINE__,(char*)__FUNCTION__,LOG_LEVEL_ERR,"ret=%d",ret);
		return ret;
    }
  //  printf("type=%d\r\n",msgLitea->prototype());
    if (!input.ConsumedEntireMessage()) {
		ret=ERR_PROTOBUF_CODER_DECODE_CONSUMED;
//        PrintConsole((char*)__FILE__, __LINE__,(char*)__FUNCTION__,LOG_LEVEL_ERR,"ret=%d",ret);    
		return ret;
    }
      
	// Release the limit.
    input.PopLimit(limit);
	return ret;
}

/*
void TProtobufCoder::MemcpyStringToChar(std::string string, char** chString)
{
    if (string.length()<=0) {
        return;
    }
    unsigned long size = string.length();
    *chString = (char*)malloc(size+1);
    memset(*chString, 0, size+1);
    memcpy(*chString, string.c_str(), size);
}


int TProtobufCoder::DecodeSyncMutilMessage(std::vector<MessageContent*> *messages, char* inputData,int length)
{
    if(NULL==messages || NULL==inputData || length<=0)
    {
        return ERR_PROTOBUF_CODER_NULL;
    }
    
    int parseRet=PROTOBUF_CODER_OK;
    google::protobuf::io::CodedInputStream input((google::protobuf::uint8*)inputData,length);
    
    while (true) {
        google::protobuf::uint32 sizea;
        if (!input.ReadVarint32(&sizea)) {
            return PROTOBUF_CODER_OK;
        };
        
        // Tell the stream not to read beyond that size.
        google::protobuf::io::CodedInputStream::Limit limit = input.PushLimit(sizea);
        
        // Parse the message.
        SyncMsgRespInner *syncmsgReqInner = new SyncMsgRespInner();
        if (!syncmsgReqInner->MergeFromCodedStream(&input)) {
            parseRet = ERR_PROTOBUF_CODER_DECODE_MERGE;
        }
        
        if (!input.ConsumedEntireMessage()) {
            parseRet = ERR_PROTOBUF_CODER_DECODE_CONSUMED;
        }
        
        // Release the limit.
        input.PopLimit(limit);
        
        if (parseRet == PROTOBUF_CODER_OK) {
            MessageContent* msgInfo = new MessageContent();
            msgInfo->version = syncmsgReqInner->version();
            msgInfo->msgType = syncmsgReqInner->msgtype();
            
            if (syncmsgReqInner->has_msgid()) {
                MemcpyStringToChar(syncmsgReqInner->msgid(), &(msgInfo->msgId));
            }
            if (syncmsgReqInner->has_msgcontent()){
                MemcpyStringToChar(syncmsgReqInner->msgcontent(), &(msgInfo->content));
            }
            if (syncmsgReqInner->has_msgsender()){
                MemcpyStringToChar(syncmsgReqInner->msgsender(), &(msgInfo->sender));
            }
            if (syncmsgReqInner->has_msgreceiver()){
                MemcpyStringToChar(syncmsgReqInner->msgreceiver(), &(msgInfo->receiver));
            }
            if (syncmsgReqInner->has_msgdomain()){
                MemcpyStringToChar(syncmsgReqInner->msgdomain(), &(msgInfo->domain));
            }
            if (syncmsgReqInner->has_msgfilename()){
                MemcpyStringToChar(syncmsgReqInner->msgfilename(), &(msgInfo->fileName));
            }
            if (syncmsgReqInner->has_msgfileurl()){
                MemcpyStringToChar(syncmsgReqInner->msgfileurl(), &(msgInfo->fileUrl));
            }
            if (syncmsgReqInner->has_msgdatecreated()){
                MemcpyStringToChar(syncmsgReqInner->msgdatecreated(), &(msgInfo->dateCreated));
            }

            messages->push_back(msgInfo);
            
            delete syncmsgReqInner;
            syncmsgReqInner = NULL;
            
        }else{
            delete syncmsgReqInner;
            syncmsgReqInner = NULL;
            break;
        }
        
    }
    return parseRet;
}
*/


}//end namespace CcpClientYTX
