/*
 *  Copyright (c) 2013 The CCP project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a Beijing Speedtong Information Technology Co.,Ltd license
 *  that can be found in the LICENSE file in the root of the web site.
 *
 *                    http://www.yuntongxun.com
 *
 *  An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#import <Foundation/Foundation.h>
#define kSDKVersion  @"3.6.5"
#define kSDKDate     @"2015-1-30"
#define kGetSipAddressFinished                      @"ON_REST_GETSIPADDRESSFINISHED"
#define kGetNetGroupIdFinished                      @"ON_REST_GETNETGROUPIDFINISHED"
#define kGetCallbackFinished                        @"ON_REST_GETCALLBACKFINISHED"
#define kGetRestLoginfo                             @"ON_REST_GETRESTLOGINFO"
#define kUploadMediaMsgFinished                     @"ON_REST_UPLOADMEDIAMSG_FINISHED"

#define kStartInterphoneFinished                    @"ON_REST_STARTINTERPHONE_FINISHED"
#define kControlMicFinished                         @"ON_REST_CONTROLMIC_FINISHED"
#define kReleaseMicFinished                         @"ON_REST_RELEASEMIC_FINISHED"
#define kGetInterphoneMemberFinished                @"ON_REST_GETINTERPHONEMEMBER_FINISHED"

#define kStartChatroomFinished                      @"ON_REST_STARTCHATROOM_FINISHED"
#define kInviteMembersChatroomFinished              @"ON_REST_INVITEMEMBERSCHATROOM_FINISHED"
#define kGetChatRoomsListFinished                   @"ON_REST_GETCHATROOMSLIST_FINISHED"
#define kGetChatRoomMemberFinished                  @"ON_REST_GETCHATROOMMEMBER_FINISHED"
#define kDismissChatRoomFinished                    @"ON_REST_DISMISS_CHATROOM_FINISHED"
#define kRemoveMemberFromChatRoomFinished           @"ON_REST_RemoveMemberFromChatRoom_FINISHED"

#define kStartVideoConferenceFinished               @"ON_REST_START_VIDEO_CONFERENCE_FINISHED"
#define kGetVideoConferenceMemberFinished           @"ON_REST_GET_VIDEO_CONFERENCE_MEMBER_FINISHED"
#define kGetVideoConferenceListFinished             @"ON_REST_GET_VIDEO_CONFERENCE_LIST_FINISHED"
#define kDismissVideoConferenceFinished             @"ON_REST_DISMISS_VIDEO_CONFERENCE_FINISHED"
#define kRemoveMemberFromVideoConferenceFinished    @"ON_REST_REMOVE_MEMBER_FROM_VIDEO_CONFERENCE_FINISHED"
#define kSwitchRealScreenFinished                   @"ON_REST_SWITCH_REAL_SCREEN_FINISHED"
#define kSendVideoConferencePortraitFinished        @"ON_REST_SEND_PORTRAIT_OF_VIDEO_CONFERENCE_FINISHED"
#define kGetVideoConferencePortraitFinished         @"ON_REST_GET_PORTRAIT_OF_VIDEO_CONFERENCE_FINISHED"
#define kDownloadVideoConferencePortraitFinished    @"ON_REST_DOWNLOAD_PORTRAIT_OF_VIDEO_CONFERENCE_FINISHED"
#define kEditTestNumFinished                        @"ON_REST_EditTestNum_FINISHED"
#define kSendMediaMsgFinished                       @"ON_REST_SendMediaMsg_FINISHED"
#define kDownloadMediaMsgFinished                   @"ON_REST_DOWNLOADMEDIAMSG_FINISHED"
#define kConfirmDownloadAttachFinished              @"ON_REST_CONFIRM_DOWNLOAD_ATTACH_FINISHED"

#define kCheckVoipOfflineMsgFinished              @"ON_REST_CheckVoipOfflineMsg_FINISHED"
#define kGetVoipOfflineMsgFinished                @"ON_REST_GetVoipOfflineMsg_FINISHED"
#define kConfirmOfflineMsgFinished                @"ON_REST_ConfirmOfflineMsg_FINISHED"


#define kStartMultiVideoFinished                    @"ON_REST_START_MULTI_VIDEO_FINISHED"
#define kPublishVideoMultiVideoConferenceFinished   @"ON_REST_PUBLISH_VIDEO_IN_MULTI_VIDEO_CONFERENCE_FINISHED"
#define kUnpublishVideoMultiVideoConferenceFinished        @"ON_REST_UNPUBLISH_VIDEO_IN_MULTI_VIDEO_CONFERENCE_FINISHED"

#define kServerVersion  ((NSString *)[self getCcpRestServerVersion])
#define SERVER_GetSipAddressXML_KEY    @"server_get_sip_address_XML_KEY"
#define SERVERIP_PORT_KEY    @"serverip_and_port"
#define SERVER_P2P_KEY  @"serverP2P_IP_Port"
#define SERVER_STR_CAPABILITY @"server_strCapability"
#define NETWORK_GROUPID @"network_groupid"
#define KEY_LastLoginTime @"lastLoginTime"
#define SKD_LOG_KEY     @"LOG_STRING"

#define KEY_RESPONSE_STATUSCODE @"responsestatuscode"
#define KEY_RESPONSE_STATUSMSG @"responsestatusmessage"

#define KEY_RESPONSE_USERDATA   @"responseuserdatainfo"
#define KEY_RESPONSE_USERDATA_CHATROOMPWD   @"responseuserdatainfo_chatroom_pwd"
#define KEY_RESPONSE_USERDATA_CHATROOM_IS_AUTOJOIN   @"responseuserdatainfo_chatroom_Is_AutoJoin"
#define KEY_USERINFO_DOWNLOADFILEINFO   @"downlaodvoicemsginfo"
#define KEY_USERINFO_UPLOAD_VOICEMSG    @"uploadvoicemsgdata"
#define KEY_USERINFO_CHATROOMNO         @"chatroomnoOfinvitemembersin"
#define KEY_USERINFO_VIDEOCONFERENCEID  @"videoConference_id"
#define KEY_USERINFO_MEMBERDATA         @"chatroomnoremovemember"
#define KEY_USERINFO_MEDIAMSG_FILENAME  @"MediaMsgFilename"
#define KEY_USERINFO_MEDIAMSG_MSGID     @"MediaMsgMSGID"
#define KEY_USERINFO_MEDIAMSG_SENDTYPE  @"MediaMsgSendType"
#define KEY_USERINFO_SEND_MEDIAMSG_DATA @"sendMediaMsgdata"



#define KEY_VOICEMESSAGE_DATA   @"pushvoicemessageinfodata"
#define KEY_VOICEMESSAGE_TYPE   @"pushvoicemessagetype"

enum {
    ELocalReason_NotRegisted = 170000,
    ELocalReason_CallBusy,
    ELocalReason_ErrorXml,
    ELocalReason_ErrorStutusCode,
    ELocalReason_ErrorXmlBody,
    ELocalReason_ErrorNet,
    ELocalReason_MakeCallFailed,
    ELocalReason_SDKNotSupport,
    ELocalReason_JoinFailed,
    
    ELocalReason_UploadFailed = 170012,//170012
    ELocalReason_DwonLoadIMChunkedFailed,//170013
    ELocalReason_UploadFailedTimeIsShort,//170014
    ELocalReason_UploadConnectFailed,//170015
    ELocalReason_UploadCancel,//170016
    EIMFileDoesNotExist,//170017
    
    ELocalReason_EditTextNum_VerifyPhoneNumberErr,//170018
    ELocalReason_ErrorTimeOut,//170019
    ELocalReason_ErrorTextTooLong = 170022,
    
    //以下是pc客户端错误码
    ERR_RECORD_WAVE_IN_OPEN =171200,
    ERR_RECORD_NO_MEMORY,
    ERR_RECORD_WAVE_IN_GET_POSITION,
    ERR_RECORD_TIME_BYTES,
    ERR_RECORD_CREATE_FILE,
    ERR_RECORD_WAVE_IN_RESET,
    ERR_RECORD_WAVE_IN_UN_PREPARE_HEADER,
    ERR_RECORD_GLOBAL_FREE,
    ERR_RECORD_WAVE_IN_CLOSE,
    ERR_PLAY_CREATE_FILE,
    ERR_PLAY_NOT_AMR_FILE,
    ERR_PLAY_WAVE_OUT_OPEN,
    ERR_PLAY_WAVE_OUT_PREPARE_HEADER,
    ERR_PLAY_GLOBAL_FREE,
    ERR_RECORD_IS_RECORDING,
    ERR_PLAY_IS_PLAYING,
    ERR_NULL =171300,
    ERR_MD5_BASE64,
    ERR_SYN_HTTP_REQUEST,
    ERR_SYN_HTTP_STATAUS_CODE,
    ERR_CREATE_FILE,
    ERR_XML_NODE,
    ERR_XML_ELEMENT,
    ERR_XML_STATAUS_CODE,
    ERR_XML_FILESTATAUS_CODE,
    ERR_QUEUE_EMPTY,
    ERR_XML_SWITCHIP_NULL,
    ERR_ENCODE_HTTP,
    ERR_RECV_NULL,
    ERR_BODY_NULL,//内容空
    ERR_HEAD_LARGE,//头太长
    ERR_RESPONSE_CONTENT_LENGTH,//下载二进制包体长度
};


@protocol CCPRestDelegate<NSObject>
@optional
//发送chunked数据
- (void)sendChunkedFileWithVoiceMsg:(id) data andServerIP:(NSString *)serverIp withServerPort:(NSString *)serverPort byUrl:(NSString *)url;
@end

@interface CCPRestService : NSObject
{
}

@property (retain, nonatomic)NSString   *account;
@property (retain, nonatomic)NSString   *password;             //voip密码
@property (retain, nonatomic)NSString   *accountSid;
@property (retain, nonatomic)NSString   *authToken;            //账户授权令牌
@property (retain, nonatomic)NSString   *restip;
@property (assign, nonatomic)NSInteger  restport;
@property (retain, nonatomic)NSString   *myMainAccount;
@property (retain, nonatomic)NSString   *myMainToken;
@property (retain, nonatomic)NSString   *myAppId;
@property (retain, nonatomic)NSString   *libSDK;
@property (retain, nonatomic)NSString   *deviceVersion;
@property (retain, nonatomic)NSString   *PrivateCloudFlag;
@property (retain, nonatomic)NSString   *ValidateStr;
@property (retain, nonatomic)NSString   *ServerVerSion;
@property (assign, nonatomic)id<CCPRestDelegate> chunkedDelegate;
//@property (assign, nonatomic)NSInteger  controlMicStatus;      //0:无控麦 1:控麦中


-(NSString*)getCcpRestServerVersion;
//获取软件换地址
- (void)getSipAddress;
//获取网络群组id
- (void)getNetGroupId;
//回拨
- (void)getCallback:(NSString *)fromPhone withTOCall:(NSString *)toPhone andSrcSerNum:(NSString*) srcSerNum andDestSerNum:(NSString*) destSerNum;


#pragma mark - 语音留言相关REST API函数

+ (void)parseVoiceMessage:(NSString*) xmlStr andVoiceInfo:(NSMutableDictionary*) voiceMsgInfo andVoipId:(NSString*) voipId;

#pragma mark - 实时对讲相关函数
//启动对讲场景
//发言类型 0：抢麦模式   1:都可以讲话 默认为0
- (void) startInterphoneWithJoiner:(NSArray*)joinerArr inAppId:(NSString*)appid;


//发起对讲——抢麦
- (void) controlMicInConfNo:(NSString*)confNo;

//结束对讲——放麦
- (void) releaseMicInConfNo:(NSString*)confNo;

//查询参与对讲成员
- (void) getMemberListInConfNo:(NSString*)conNo;


#pragma mark - 聊天室相关函数
//创建聊天室
- (void) startChatroomInAppId:(NSString *)appId withName:(NSString *)roomName andSquare:(NSInteger)square andKeywords:(NSString *)keywords andPassword:(NSString *)roomPwd andIsAutoClose:(BOOL)isAutoClose andVoiceMod:(NSInteger) voiceMod andAutoDelete:(BOOL) autoDelete andIsAutoJoin:(BOOL) isAutoJoin;

//获取聊天室成员
- (void) queryMembersWithChatroom:(NSString *)roomNo;

//获取所有的房间列表
- (void)queryChatroomsOfAppId:(NSString *)appId withKeywords:(NSString *)keywords;


//外呼邀请成员加入群聊
- (void)inviteMembers:(NSArray*)members joinChatroom:(NSString*)roomNo ofAppId:(NSString *)appid;

- (void) dismissChatroom:(NSString*) roomNo ofAppId:(NSString*) appId;

- (void) removeMember:(NSString*) member fromRoomNo:(NSString*) roomNo ofAppId:(NSString*) appId;

#pragma mark - 视频会议 video conference

- (void) startVideoConferenceInAppId:(NSString*)appId withName:(NSString *)conferenceName andSquare:(NSInteger)square andKeywords:(NSString *)keywords andPassword:(NSString *)conferencePwd andIsAutoClose:(BOOL)isAutoClose andVoiceMod:(NSInteger) voiceMod andAutoDelete:(BOOL) autoDelete andIsAutoJoin:(BOOL) isAutoJoin;

- (void) queryMembersInVideoConference:(NSString *)conferenceId;

- (void) queryVideoConferencesOfAppId:(NSString *)appId withKeywords:(NSString *)keywords;

- (void) dismissVideoConferenceWithAppId:(NSString*) appId andVideoConference:(NSString*) conferenceId;

- (void) removeMemberFromVideoConferenceWithAppId:(NSString*) appId andVideoConference:(NSString*)conferenceId andMember:(NSString*) member;

- (void) switchRealScreenToVoip:(NSString*)voip ofVideoConference:(NSString*)conferenceId inAppId:(NSString*)appId;

- (void) sendLocalPortrait:(NSString*)filename toVideoConference:(NSString*)conferenceId;

- (void) getPortraitsFromVideoConference:(NSString*)conferenceId;

- (void)downloadVideoConferencePortrait:(id)portrait;

#pragma mark - 多路视频
- (void) startMultiVideoInAppId:(NSString*)appId withName:(NSString *)conferenceName andSquare:(NSInteger)square andKeywords:(NSString *)keywords andPassword:(NSString *)conferencePwd andIsAutoClose:(BOOL)isAutoClose andIsPresenter:(BOOL)isPresenter andVoiceMod:(NSInteger)voiceMod andAutoDelete:(BOOL)autoDelete andIsAutoJoin:(BOOL) isAutoJoin;

//发布视频
-(void)publishVideoInMultiVideoConference:(NSString*)conferenceId ofAppId:(NSString*)appId;

//取消发布视频
-(void)unpublishVideoInMultiVideoConference:(NSString*)conferenceId ofAppId:(NSString*)appId;

#pragma make - 多媒体IM
//发送IM
- (void) sendMediaMsgWithFileName:(NSString*) fileName andReceiver:(NSString*) receiver andChunked:(BOOL) chunked andUserdata:(NSString*)userdata andSendMsgId:(NSString*)msgid;
//查询未下载的多媒体IM
//- (void) queryNewMediaMessages;
//确认已下载多媒体IM
- (void) confirmInstanceMessageWithMsgId:(NSArray*) msgIds;
//下载附件
- (void) downloadAttachmentFiles:(NSArray*)urlList;
//获取文件大小
- (unsigned long long) fileSizeAtPath:(NSString*) filePath;



- (void) checkVoipOfflineMsgWithTimestampStr:(NSString*) timestampStr;

- (void) getVoipOfflineMsgWithSender:(NSString*) sender andTimestampStr:(NSString*) timestampStr andLimit:(int)limit;

- (void) confirmOfflineMsgWithMsgIds:(NSArray*) msgIds;


- (void) editTestNumWithOldPhoneNumber:(NSString*) oldPhoneNumber andNewPhoneNumber:(NSString*) newPhoneNumber andServerIP:(NSString*) serverIP andServerPort:(NSInteger) serverPort andServerVersion :(NSString*) serverVersion andMainAccount:(NSString*) mainAccount andMainToken:(NSString*) mainToken;
@end