#ifndef ___VOIP_CRPT__H___
#define ___VOIP_CRPT__H___
///Author: CCIT WANGTIANYU
///date: 2014-7-11

//#define FILE_VOIP_PATH "/data/data/com.voice.demo/files"
#define FILE_VOIP_PATH "/data/data/com.hisun.cas/files"
//#define FILE_VOIP_PATH "/data/data/com.ccit.test/files"
//#define FILE_VOIP_PATH "/data/data/com.hisun.chuangyuan/files"

#define VOIPCRPT_API

/************************************************************************/
/*voip客户端密码模块初始化   
[in]appid	应用id
[in]appidLen	长度
[in]testFlag                                          */
/************************************************************************/
VOIPCRPT_API int init(char *appid, long appidLen,  char *UsrId, long UsrIdLen, char *devId);
/************************************************************************/
/*设置PIN
[char*]	PIN
[in]	PIN长度
[bool]testFlag                                        */
/************************************************************************/

VOIPCRPT_API int setPIN(char *pin , int pinLen, bool testFlag);


/************************************************************************/
/*协商密钥请求
[in]desid		目标用户id
[in]desidLen	长度
[out]inviteKey		协商密钥密文格式：密文|签名，密文与签名值使用“|”隔开
[out]linviteKeyLen	  协商密钥                                            */
/************************************************************************/
VOIPCRPT_API int inviteKeyRequest(char *desid, long desidLen, char *inviteKey,long* linviteKeyLen);

/************************************************************************/
/*主叫会话密钥生成
inviteKey		协商密钥密文（格式：密文|签名值）
inviteKeyLen		长度
desid	char*	目标用户ID
ldesidLen	long	长度                                           */
/************************************************************************/
VOIPCRPT_API int transmitKeyRequest(char * inviteKey, long inviteKeyLen,char *desid, long ldesidLen);

/************************************************************************/
/*被叫会话密钥生成   
tansmitKey	char*	会话密钥密文（格式：密文|签名值）
linviteKeyLen	long	长度
desid	char*	目标用户ID
ldesidLen	long	长度
transmitKey	char*	协商密钥密文格式：密文|签名，密文与签名值使用“|”隔开
ltransmitKeyLen	long	长度                                          */
/************************************************************************/
VOIPCRPT_API int transmitKeyRequest(char *inviteKey, long linviteKeyLen,char *desid, long ldesidLen, char *transmitKey,long* ltransmitKeyLen);

/************************************************************************/
/*删除会话密钥   
Id	char*	目标用户ID
idLen	long	长度                                         */
/************************************************************************/
VOIPCRPT_API int deleteTransmitKey(char *id,long idLen);

/************************************************************************/
/*落地电话密钥协商请求
[in]serviceID		软实现服务器id
[in]serviceIDLen	软实现服务器id长度
[out]inviteKey		协商密钥密文格式：密文|签名，密文与签名值使用“|”隔开
[out]linviteKeyLen	  协商密钥                                            */
/************************************************************************/
VOIPCRPT_API int inviteVOIPKeyRequest(char *serviceID, long serviceIDLen, char *inviteKey,long* linviteKeyLen);

/************************************************************************/
/*落地电话主叫会话密钥生成
inviteKey		协商密钥密文（格式：密文|签名值）
inviteKeyLen		长度
serviceID	char*	软实现服务器id
serviceIDLen	long	软实现服务器id长度                                           */
/************************************************************************/
VOIPCRPT_API int VOIPtransmitKeyRequest(char * inviteKey, long inviteKeyLen,char *serviceID, long serviceIDLen);

/************************************************************************/
/*落地电话被叫会话密钥生成   
tansmitKey	char*	会话密钥密文（格式：密文|签名值）
linviteKeyLen	long	长度
serviceID	char*	软实现服务器id
serviceIDLen	long	软实现服务器id长度
transmitKey	char*	协商密钥密文格式：密文|签名，密文与签名值使用“|”隔开
ltransmitKeyLen	long	长度                                          */
/************************************************************************/
VOIPCRPT_API int VOIPtransmitKeyRequest(char *inviteKey, long linviteKeyLen,char *serviceID, long serviceIDLen, char *transmitKey,long* ltransmitKeyLen);

/************************************************************************/
/*落地电话删除会话密钥   
serviceID	char*	软实现服务器id
serviceIDLen	long	软实现服务器id长度                              */
/************************************************************************/
VOIPCRPT_API int deleteVOIPTransmitKey(char *serviceID,long serviceIDLen);

/************************************************************************/
/*群组语音会话密钥解密   
transmitKey	char*	服务器下发的会话密钥密文
transmitKeyLen	long	长度             
groupID	char*	群组ID标识
groupIDLen	long	群组标识长度                            */
/************************************************************************/
VOIPCRPT_API bool groupTransmitKeyDecrp (char *transmitKey,long transmitKeyLen);

/************************************************************************/
/*删除群组语音会话密钥
serviceID	char*	软实现服务器id
serviceIDLen	long	软实现服务器id长度                              */
/************************************************************************/
VOIPCRPT_API int deleteGroupTransmitKey(char *serviceID,long serviceIDLen);

/************************************************************************/
/*短消息加密（公钥直接加密方法）  
sms	char*	待加密的短消息
smsLen	long	长度
desid	char*	接收方用户ID
ldesidLen	long	长度
smsCrpt	char*	加密后的短消息密文（密文格式：密文|签名值）
smsCrptLen	long	长度                                                  */
/************************************************************************/
VOIPCRPT_API int smsCrpRequest(char *sms, long smsLen, char *desid, long desidLen, char *smsCrpt, long* smsCrptLen );

/************************************************************************/
/*短消息解密（公钥直接加密方法）
sms	char*	待解密的短消息
smsLen	long	长度
desid	char*	发送方用户ID
ldesidLen	long	长度
smsDeCrpt	char*	解密后的短消息明文
smsDeCrptLen	long	长度                                                */
/************************************************************************/
VOIPCRPT_API int smsDecrpRequest(char *sms, long smsLen, char *desid, long desidLen, char *smsDeCrpt, long* smsDeCrptLen );

/************************************************************************/
/*端到端短消息加密（数字信封方式）    
sms	char*	待加密的短消息
smsLen	long	长度
desid	char*	目标用户ID
ldesidLen	long	长度
smsCrpt	char*	加密后的短消息（带签名的P7数字信封）
smsCrptLen	long	长度                                                  */
/************************************************************************/
VOIPCRPT_API int smsCrpEnvelop(char *sms, long smsLen, char *desid, long desidLen, char *smsCrpt, long* smsCrptLen );

/************************************************************************/
/*端到端短消息解密（数字信封方式）
sms	char*	待解密的短消息
smsLen	long	长度
smsDeCrpt	char*	解密后的短消息
smsDeCrptLen	long	长度                                                */
/************************************************************************/
VOIPCRPT_API int smsDecrpEnvelop(char *sms, long smsLen, char *desid, long desidLen, char *smsDeCrpt, long* smsDeCrptLen );

/************************************************************************/
/*群组短消息加密（数字信封方式）
sms	char*	待加密的短消息
smsLen	long	长度
serverid	char*	服务器id
serveridLen	long	长度
smsCrpt	char*	加密后的短消息（带签名的P7数字信封）
smsCrptLen	long	长度                                                  */
/************************************************************************/
VOIPCRPT_API int groupSmsCrpEnvelop(char *sms, long smsLen, char *serverid, long serveridLen, char *smsCrpt, long* smsCrptLen ); 

/************************************************************************/
/*群组短消息解密
sms	char*	待解密的短消息
smsLen	long	长度
serverid	char*	发送方（服务器）ID
serveridLen	long	长度
smsDeCrpt	char*	解密后的短消息
smsDeCrptLen	long	长度                                                */
/************************************************************************/
VOIPCRPT_API int groupSmsDecrpEnvelop(char *sms, long smsLen, char *serverid, long serveridLen, char *smsDeCrpt, long* smsDeCrptLen ); 

/************************************************************************/
/*端到端文件加密（数字信封方式）  
file	char*	待加密的文件
fileLen	long	长度
desid	char*	发送方用户ID
desidLen	long	长度
fileCrpEnvelop	char*	加密后的文件
fileCrpEnvelopLen	long	长度                                           */
/************************************************************************/
VOIPCRPT_API int fileCrpEnvelop(unsigned char *file_short, long fileLen, char *desid, long desidLen, unsigned char *fileCrpEnvelop, long* fileCrpEnvelopLen ); 

/************************************************************************/
/*端到端文件解密（数字信封方式）
file	char*	待解密的文件
fileLen	long	长度
desid	char*	发送方用户ID
desidLen	long	长度
fileDeCrpt	char*	解密后的文件
fileDeCrptLen	long	长度                                                */
/************************************************************************/
VOIPCRPT_API int fileDecrpEnvelop(unsigned char *file_short, long fileLen, char *desid, long desidLen, unsigned char *fileDeCrpt, long* fileDeCrptLen ); 

/************************************************************************/
/*群组文件加密（数字信封方式）           
file	char*	待加密的文件
fileLen	long	长度
desid	char*[]	接收方用户ID
desidLen	long[]	长度
userNum int 接收方用户数量
groupFileCrpEnvelop	char*	加密后的文件
groupFileCrpEnvelopLen	long	长度                                     */
/************************************************************************/
VOIPCRPT_API int groupFileCrpEnvelop(unsigned char *file, long fileLen, char *desid[], long desidLen[], int userNum, unsigned char *groupFileCrpEnvelop, long* groupFileCrpEnvelopLen ); 

/************************************************************************/
/*群组文件解密（数字信封方式）   
file	char*	待解密的文件
fileLen	long	长度
desid	char*	发送方用户ID
desidLen	long	长度
groupFileDecrpt	char*	解密后的文件
groupFileDecrptLen	long	长度                                          */
/************************************************************************/
VOIPCRPT_API int groupFileDecrpEnvelop(unsigned char *file, long fileLen, char *desid, long desidLen, unsigned char *groupFileDecrpt, long*groupFileDecrptLen ); 

/************************************************************************/
/*媒体流加密
mediaStream	char*	待加密的媒体流
mediaStreamLen	long	长度
id	char*	发送方用户ID或者群组id
idLen	long	长度
mediaStreamCrp	char*	加密后的流媒体
mediaStreamCrpLen	long	长度                                            */
/************************************************************************/
VOIPCRPT_API int mediaStreamCrp(char *mediaStream, long mediaStreamLen, char *id, long idLen, char *mediaStreamCrp , long* mediaStreamCrpLen ); 

/************************************************************************/
/*媒体流解密
mediaStream	char*	待解密的媒体流
mediaStreamLen	long	长度
id	char*	发送方用户ID或者群组id
idLen	long	长度
mediaStreamDecrp	char*	解密后的媒体流
mediaStreamDecrpLen	long	长度                                          */
/************************************************************************/
VOIPCRPT_API int mediaStreamDecrp (char *mediaStream, long mediaStreamLen, char *id, long idLen, char *mediaStreamDecrp , long* mediaStreamDecrpLen );  

/************************************************************************/
/*添加联系人
desid	char**	联系人ID
desidLen	long[]	长度
num     int     联系人数量
*/
/************************************************************************/
VOIPCRPT_API int contactListAdd (char *desid[], long desidLen[], int num); 

/************************************************************************/
/*删除联系人
desid	char*	联系人ID
desidLen	long	长度                                                    
*/
/************************************************************************/
VOIPCRPT_API int contactListDel (char *desid, long desidLen); 

/************************************************************************/
/*检查证书是否存在
desid	char*	联系人ID
desidLen	long	长度  
return int 0 证书不存在，1 证书存在
*/
/************************************************************************/
VOIPCRPT_API int isExistOfCert (char *desid, long desidLen); 


/*********自定义函数****************/

int tool_readFile(char *filePath, char *fileContent, long *fileLength);
/***********************************/

#endif //___VOIP_CRPT__H___

