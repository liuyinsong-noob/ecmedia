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

#import "CCPRestService.h"
#import "ASI/CLOPHTTPRequest.h"
#import "CCPCallService.h"
#import "GDataXMLParser.h"
#import "CommonClass.h"
#import <CommonCrypto/CommonDigest.h>

#define TAG_SIP_KEY         @"siptag"
#define TAG_CALLBACK_KEY    @"callbacktag"

//#define defHttps        @"https://"
#define defHttps [self getHttpOrHttps]
@interface CCPRestService()

@property (retain,nonatomic)NSMutableDictionary *tagDictionary;

- (NSInteger)createRestTag;
//获取时间戳,rest服务器需要13位的
- (NSString *)getTimestamp;
//获取url后面sig的MD5加密
- (NSString *)getSig:(NSString *)timestamp;
//获取头里面Authorization的base64加密
- (NSString *)getAuthorization:(NSString *)timestamp;
//上报消息
- (void)postEventType:(ERequestType)type withData:(NSMutableDictionary *)userData;
//上报日志
- (void)postLoginfo:(NSString *)loginfo;
@end

@implementation CCPRestService

@synthesize account;
@synthesize password;
@synthesize accountSid;
@synthesize authToken;
@synthesize restip;
@synthesize restport;
@synthesize tagDictionary;
@synthesize myAppId;
@synthesize myMainAccount;
@synthesize myMainToken;
@synthesize chunkedDelegate;
@synthesize libSDK;
@synthesize deviceVersion;
@synthesize PrivateCloudFlag;
@synthesize ValidateStr;
- (id)init
{
    self = [super init];
    NSMutableDictionary *tempDic = [[NSMutableDictionary alloc] init];
    self.tagDictionary = tempDic;
    [tempDic release];
    return self;
}

- (void)dealloc
{
    self.tagDictionary  = nil;
    self.account = nil;
    self.password = nil;
    self.restip = nil;
    self.authToken = nil;
    self.chunkedDelegate = nil;
    self.myMainAccount = nil;
    self.myMainToken = nil;
    self.myAppId = nil;
    self.libSDK = nil;
    self.deviceVersion = nil;
    self.PrivateCloudFlag = nil;
    self.ValidateStr = nil;
    [super dealloc];
}

#pragma mark - private
-(NSString*)getHttpOrHttps
{
    if ([self.restip hasPrefix:@"http"])
    {
        return @"";
    }
    else
    {
        return @"https://";
    }
}

- (NSInteger)createRestTag
{
    static NSInteger tagNum = 100000;
    //保证是6位数字
    if(tagNum >= 1000000)
        tagNum = 100000;
    tagNum++;
    
    return tagNum;
}

- (NSString *)getTimestamp
{
    NSDateFormatter *dateFormatter = [[NSDateFormatter alloc] init];
    [dateFormatter setDateFormat:@"yyyyMMddHHmmss"];
    
    NSString *currentDateStr = [dateFormatter stringFromDate:[NSDate date]];
    [dateFormatter release];
    
    [self postLoginfo:[NSString stringWithFormat:@"[CCPRestService] getTimestamp currentDateStr=%@",currentDateStr]];
    return currentDateStr;
}

- (NSString *)getSig:(NSString *)timestamp
{
    NSString *sigString = [NSString stringWithFormat:@"%@%@%@", self.accountSid, self.authToken, timestamp];
    
    [self postLoginfo:[NSString stringWithFormat:@"[CCPRestService] getSig sig=%@",sigString]];
    
    const char *cStr = [sigString UTF8String];
	unsigned char result[16];
	CC_MD5(cStr, (CC_LONG)strlen(cStr), result);
	return [NSString stringWithFormat:@"%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X",result[0], result[1], result[2], result[3], result[4], result[5], result[6], result[7],result[8], result[9], result[10], result[11],result[12], result[13], result[14], result[15]];
}

-(unsigned long long) fileSizeAtPath:(NSString*) filePath
{
    NSFileManager* manager = [NSFileManager defaultManager];
    if ([manager fileExistsAtPath:filePath])
    {
        return [[manager attributesOfItemAtPath:filePath error:nil] fileSize];
    }
    return 0;
}

- (NSString *)getAuthorization:(NSString *)timestamp;
{
    NSString *authorizationString = [NSString stringWithFormat:@"%@:%@",self.accountSid,timestamp];
    return [CLOPHTTPRequest base64forData:[authorizationString dataUsingEncoding:NSUTF8StringEncoding]];
}

#pragma mark - xml解析
+ (void)parseVoiceMessage:(NSString*) xmlStr andVoiceInfo:(NSMutableDictionary*) voiceMsgDic andVoipId:(NSString*) voipId
{
    GDataXMLDocument *xmldoc = [[GDataXMLDocument alloc] initWithXMLString:xmlStr options:0 error:nil];
    if (!xmldoc)
    {
        [xmldoc release];
    }
    GDataXMLElement *rootElement = [xmldoc rootElement];
    GDataXMLNode *varAttribute = [rootElement attributeForName:@"var"];
    NSString* msgType = varAttribute.stringValue;
    NSInteger type = msgType.length>0?msgType.integerValue:0;
    //NSString* strUrl = nil;
    if (type == 101)
    {

    }
    else if(type >= 201 && type <= 206)
    {
        NSArray *interphoneIdArray = [rootElement elementsForName:@"interphoneId"];
        if (interphoneIdArray.count > 0)
        {
            NSString *interphoneId = nil;
            NSString *dateCreated = nil;
            NSString *from = nil;
            NSString *whoStr = nil;
            {
                GDataXMLElement *Element = (GDataXMLElement *)[interphoneIdArray objectAtIndex:0];
                interphoneId = Element.stringValue;
            }
            
            NSArray *dateCreatedArray = [rootElement elementsForName:@"dateCreated"];
            if (dateCreatedArray.count > 0)
            {
                GDataXMLElement *Element = (GDataXMLElement *)[dateCreatedArray objectAtIndex:0];
                dateCreated = Element.stringValue;
            }
            
            NSArray *fromArray = [rootElement elementsForName:@"from"];
            if (fromArray.count > 0)
            {
                GDataXMLElement *Element = (GDataXMLElement *)[fromArray objectAtIndex:0];
                from = Element.stringValue;
            }
            
            NSArray *whoArray = [rootElement elementsForName:@"who"];
            if (whoArray.count > 0)
            {
                GDataXMLElement *Element = (GDataXMLElement *)[whoArray objectAtIndex:0];
                whoStr = Element.stringValue;
            }
            
            [voiceMsgDic setValue:msgType forKey:KEY_VOICEMESSAGE_TYPE];
            if (type == 201)
            {
                InterphoneInviteMsg * interphonemsg = [[InterphoneInviteMsg alloc] init];
                interphonemsg.interphoneId = interphoneId;
                interphonemsg.dateCreated = dateCreated;
                interphonemsg.fromVoip = from;
                [voiceMsgDic setValue:interphonemsg forKey:KEY_VOICEMESSAGE_DATA];
                [interphonemsg release];
            }
            else if(type == 202)
            {
                InterphoneJoinMsg * interphonemsg = [[InterphoneJoinMsg alloc] init];
                interphonemsg.interphoneId = interphoneId;
                interphonemsg.joinArr = whoStr.length>0?[whoStr componentsSeparatedByString:@","]:nil;
                [voiceMsgDic setValue:interphonemsg forKey:KEY_VOICEMESSAGE_DATA];
                [interphonemsg release];
            }
            else if(type == 203)
            {
                InterphoneExitMsg * interphonemsg = [[InterphoneExitMsg alloc] init];
                interphonemsg.interphoneId = interphoneId;
                interphonemsg.exitArr = whoStr.length>0?[whoStr componentsSeparatedByString:@","]:nil;
                [voiceMsgDic setValue:interphonemsg forKey:KEY_VOICEMESSAGE_DATA];
                [interphonemsg release];
            }
            else if(type == 204)
            {
                InterphoneOverMsg * interphonemsg = [[InterphoneOverMsg alloc] init];
                interphonemsg.interphoneId = interphoneId;
                [voiceMsgDic setValue:interphonemsg forKey:KEY_VOICEMESSAGE_DATA];
                [interphonemsg release];
            }
            else if(type == 205)
            {
                InterphoneControlMicMsg * interphonemsg = [[InterphoneControlMicMsg alloc] init];
                interphonemsg.interphoneId = interphoneId;
                interphonemsg.voip = whoStr;
                [voiceMsgDic setValue:interphonemsg forKey:KEY_VOICEMESSAGE_DATA];
                [interphonemsg release];
            }
            else if(type == 206)
            {
                InterphoneReleaseMicMsg * interphonemsg = [[InterphoneReleaseMicMsg alloc] init];
                interphonemsg.interphoneId = interphoneId;
                interphonemsg.voip = whoStr;
                [voiceMsgDic setValue:interphonemsg forKey:KEY_VOICEMESSAGE_DATA];
                [interphonemsg release];
            }
        }
    }
    else if(type >= 301 && type <= 304)
    {
        NSArray *interphoneIdArray = [rootElement elementsForName:@"chatroomId"];
        if (interphoneIdArray.count > 0)
        {
            NSString *interphoneId = nil;
            NSString *dateCreated = nil;
            NSString *from = nil;
            NSString *whoStr = nil;
            {
                GDataXMLElement *Element = (GDataXMLElement *)[interphoneIdArray objectAtIndex:0];
                interphoneId = Element.stringValue;
            }
            
            NSArray *dateCreatedArray = [rootElement elementsForName:@"dateCreated"];
            if (dateCreatedArray.count > 0)
            {
                GDataXMLElement *Element = (GDataXMLElement *)[dateCreatedArray objectAtIndex:0];
                dateCreated = Element.stringValue;
            }
            
            NSArray *fromArray = [rootElement elementsForName:@"from"];
            if (fromArray.count > 0)
            {
                GDataXMLElement *Element = (GDataXMLElement *)[fromArray objectAtIndex:0];
                from = Element.stringValue;
            }
            
            NSArray *whoArray = [rootElement elementsForName:@"who"];
            if (whoArray.count > 0)
            {
                GDataXMLElement *Element = (GDataXMLElement *)[whoArray objectAtIndex:0];
                whoStr = Element.stringValue;
            }
            
            [voiceMsgDic setValue:msgType forKey:KEY_VOICEMESSAGE_TYPE];
            if(type == 301)
            {
                ChatroomJoinMsg * interphonemsg = [[ChatroomJoinMsg alloc] init];
                interphonemsg.roomNo = interphoneId;
                interphonemsg.joinArr = whoStr.length>0?[whoStr componentsSeparatedByString:@","]:nil;
                [voiceMsgDic setValue:interphonemsg forKey:KEY_VOICEMESSAGE_DATA];
                [interphonemsg release];
            }
            else if(type == 302)
            {
                ChatroomExitMsg * interphonemsg = [[ChatroomExitMsg alloc] init];
                interphonemsg.roomNo = interphoneId;
                interphonemsg.exitArr = whoStr.length>0?[whoStr componentsSeparatedByString:@","]:nil;
                [voiceMsgDic setValue:interphonemsg forKey:KEY_VOICEMESSAGE_DATA];
                [interphonemsg release];
            }
            else if(type == 303)
            {
                ChatroomDismissMsg *chatroommsg = [[ChatroomDismissMsg alloc] init];
                chatroommsg.roomNo = interphoneId;
                [voiceMsgDic setValue:chatroommsg forKey:KEY_VOICEMESSAGE_DATA];
                [chatroommsg release];
            }
            else if(type == 304)
            {
                ChatroomRemoveMemberMsg *chatroommsg = [[ChatroomRemoveMemberMsg alloc] init];
                chatroommsg.roomNo = interphoneId;
                chatroommsg.who = whoStr;
                [voiceMsgDic setValue:chatroommsg forKey:KEY_VOICEMESSAGE_DATA];
                [chatroommsg release];
            }
        }
    }
    else if (type >= 401 && type <= 408)
    {

        NSArray *groupIdArray = [rootElement elementsForName:@"groupId"];
        if (groupIdArray.count > 0)
        {
            NSString *groupId = nil;
            NSString *proposer = nil;
            NSString *declared = nil;
            NSString *dateCreated = nil;
            NSString *admin = nil;
            NSString *confirm = nil;
            NSString *member = nil;
            {
                GDataXMLElement *Element = (GDataXMLElement *)[groupIdArray objectAtIndex:0];
                groupId = Element.stringValue;
            }
            
            NSArray *proposerArray = [rootElement elementsForName:@"proposer"];
            if (proposerArray.count > 0)
            {
                GDataXMLElement *Element = (GDataXMLElement *)[proposerArray objectAtIndex:0];
                proposer = Element.stringValue;
            }
            
            NSArray *declaredArray = [rootElement elementsForName:@"declared"];
            if (declaredArray.count > 0)
            {
                GDataXMLElement *Element = (GDataXMLElement *)[declaredArray objectAtIndex:0];
                declared = Element.stringValue;
            }
            
            NSArray *dateCreatedArray = [rootElement elementsForName:@"dateCreated"];
            if (dateCreatedArray.count > 0)
            {
                GDataXMLElement *Element = (GDataXMLElement *)[dateCreatedArray objectAtIndex:0];
                dateCreated = Element.stringValue;
            }
            
            
            NSArray *adminArray = [rootElement elementsForName:@"admin"];
            if (adminArray.count > 0)
            {
                GDataXMLElement *Element = (GDataXMLElement *)[adminArray objectAtIndex:0];
                admin = Element.stringValue;
            }
            
            adminArray = [rootElement elementsForName:@"inviter"];
            if (adminArray.count > 0)
            {
                GDataXMLElement *Element = (GDataXMLElement *)[adminArray objectAtIndex:0];
                admin = Element.stringValue;
            }
            
            
            NSArray *confirmArray = [rootElement elementsForName:@"confirm"];
            if (confirmArray.count > 0)
            {
                GDataXMLElement *Element = (GDataXMLElement *)[confirmArray objectAtIndex:0];
                confirm = Element.stringValue;
            }
            
            NSArray *memberArray = [rootElement elementsForName:@"member"];
            if (memberArray.count > 0)
            {
                GDataXMLElement *Element = (GDataXMLElement *)[memberArray objectAtIndex:0];
                member = Element.stringValue;
            }
            
            [voiceMsgDic setValue:msgType forKey:KEY_VOICEMESSAGE_TYPE];
            if(type == 401)
            {
                IMProposerMsg *msg = [[IMProposerMsg alloc] init];
                msg.groupId = groupId;
                msg.proposer = proposer;
                msg.declared = declared;
                msg.dateCreated = dateCreated;
                [voiceMsgDic setValue:msg forKey:KEY_VOICEMESSAGE_DATA];
                [msg release];
            }
            else if(type == 402)
            {
                IMReplyJoinGroupMsg *msg = [[IMReplyJoinGroupMsg alloc] init];
                msg.groupId = groupId;
                msg.admin = admin;
                msg.confirm = confirm;
                msg.member = member;
                [voiceMsgDic setValue:msg forKey:KEY_VOICEMESSAGE_DATA];
                [msg release];
            }
            else if(type == 403)
            {
                IMInviterMsg *msg = [[IMInviterMsg alloc] init];
                msg.groupId = groupId;
                msg.admin = admin;
                msg.declared = declared;
                msg.confirm = confirm;
                [voiceMsgDic setValue:msg forKey:KEY_VOICEMESSAGE_DATA];
                [msg release];
            }
            else if(type == 404)
            {
                IMRemoveMemberMsg *msg = [[IMRemoveMemberMsg alloc] init];
                msg.groupId = groupId;
                msg.member = member;
                [voiceMsgDic setValue:msg forKey:KEY_VOICEMESSAGE_DATA];
                [msg release];
            }
            else if(type == 405)
            {
                IMQuitGroupMsg *msg = [[IMQuitGroupMsg alloc] init];
                msg.groupId = groupId;
                msg.member = member;
                [voiceMsgDic setValue:msg forKey:KEY_VOICEMESSAGE_DATA];
                [msg release];
            }
            else if(type == 406)
            {
                IMDismissGroupMsg *msg = [[IMDismissGroupMsg alloc] init];
                msg.groupId = groupId;
                [voiceMsgDic setValue:msg forKey:KEY_VOICEMESSAGE_DATA];
                [msg release];
            }
            else if(type == 407)
            {
                IMJoinGroupMsg *msg = [[IMJoinGroupMsg alloc] init];
                msg.groupId = groupId;
                msg.member = proposer;
                msg.declared = declared;
                msg.dateCreated = dateCreated;
                [voiceMsgDic setValue:msg forKey:KEY_VOICEMESSAGE_DATA];
                [msg release];
            }
            else if(type == 408)
            {
                IMReplyInviteGroupMsg *msg = [[IMReplyInviteGroupMsg alloc] init];
                msg.groupId = groupId;
                msg.admin = admin;
                msg.confirm = confirm;
                msg.member = member;
                [voiceMsgDic setValue:msg forKey:KEY_VOICEMESSAGE_DATA];
                [msg release];
            }
        }

    }
    else if (type == 501 || type == 901)
    {
        NSString *userData = nil;
        NSString *msgId = nil;
        NSString *dateCreated = nil;
        NSString *sender = nil;
        NSString *receiver = nil;
        NSInteger fileSize = 0;
        NSString *fileUrl = nil;
        NSString *ext = nil;
        NSString *attachType = nil;
        NSString *message = nil;
        
        NSArray *msgIdArray = [rootElement elementsForName:@"msgId"];
        {
            GDataXMLElement *Element = (GDataXMLElement *)[msgIdArray objectAtIndex:0];
            msgId = Element.stringValue;
        }
        
        NSArray *dateCreatedArray = [rootElement elementsForName:@"dateCreated"];
        if (dateCreatedArray.count > 0)
        {
            GDataXMLElement *Element = (GDataXMLElement *)[dateCreatedArray objectAtIndex:0];
            dateCreated = Element.stringValue;
        }
        
        NSArray *senderArray = [rootElement elementsForName:@"sender"];
        if (senderArray.count > 0)
        {
            GDataXMLElement *Element = (GDataXMLElement *)[senderArray objectAtIndex:0];
            sender = Element.stringValue;
        }
        
        NSArray *receiverArray = [rootElement elementsForName:@"receiver"];
        if (receiverArray.count > 0)
        {
            GDataXMLElement *Element = (GDataXMLElement *)[receiverArray objectAtIndex:0];
            receiver = Element.stringValue;
        }
        
        NSArray *fileUrlArray = [rootElement elementsForName:@"fileUrl"];
        if (fileUrlArray.count > 0)
        {
            GDataXMLElement *Element = (GDataXMLElement *)[fileUrlArray objectAtIndex:0];
            fileUrl = Element.stringValue;
        }
        
        NSArray *fileSizeArray = [rootElement elementsForName:@"fileSize"];
        if (fileSizeArray.count > 0)
        {
            GDataXMLElement *Element = (GDataXMLElement *)[fileSizeArray objectAtIndex:0];
            fileSize = Element.stringValue.integerValue;
        }
        
        NSArray *extArray = [rootElement elementsForName:@"fileExt"];
        if (extArray.count > 0)
        {
            GDataXMLElement *Element = (GDataXMLElement *)[extArray objectAtIndex:0];
            ext = Element.stringValue;
        }
        
        NSArray *typeArray = [rootElement elementsForName:@"type"];
        if (typeArray.count > 0)
        {
            GDataXMLElement *Element = (GDataXMLElement *)[typeArray objectAtIndex:0];
            attachType = Element.stringValue;
        }
        
        NSArray *messageArray = [rootElement elementsForName:@"message"];
        if (messageArray.count > 0)
        {
            GDataXMLElement *Element = (GDataXMLElement *)[messageArray objectAtIndex:0];
            message = Element.stringValue;
        }
        
        NSArray *dataArray = [rootElement elementsForName:@"userData"];
        if (dataArray.count > 0)
        {
            GDataXMLElement *Element = (GDataXMLElement *)[dataArray objectAtIndex:0];
            userData = Element.stringValue;
        }
        
        [voiceMsgDic setValue:msgType forKey:KEY_VOICEMESSAGE_TYPE];
        if(type == 501)
        {
            IMAttachedMsg *msg = [[IMAttachedMsg alloc] init];
            msg.userData = userData;
            msg.msgId = msgId;
            msg.dateCreated = dateCreated;
            msg.sender = sender;
            msg.receiver = receiver;
            msg.fileSize = fileSize;
            msg.fileUrl = fileUrl;
            msg.ext = ext;
            if ([attachType isEqualToString:@"1"])
            {
                msg.chunked = YES;
            }
            [voiceMsgDic setValue:msg forKey:KEY_VOICEMESSAGE_DATA];
            [msg release];
        }
        else if(type == 901)
        {
            IMCooperMsg *msg = [[IMCooperMsg alloc] init];
            msg.userData = userData;
            msg.msgId = msgId;
            msg.dateCreated = dateCreated;
            msg.sender = sender;
            msg.receiver = receiver;
            msg.fileSize = fileSize;
            msg.fileUrl = fileUrl;
            msg.ext = ext;
            msg.type = attachType.integerValue;
            msg.message = message;
            [voiceMsgDic setValue:msg forKey:KEY_VOICEMESSAGE_DATA];
            [msg release];
        }
    }
    else if (type >= 601 && type <= 608)
    {
        NSArray *roomIdArray = [rootElement elementsForName:@"roomId"];
        if (roomIdArray.count > 0)
        {
            NSString *roomId = nil;
            NSString *dateCreated = nil;
            NSString *from = nil;
            NSString *whoStr = nil;
            NSInteger videoState = 1;
            NSString *videoSrc = nil;
            {
                GDataXMLElement *Element = (GDataXMLElement *)[roomIdArray objectAtIndex:0];
                roomId = Element.stringValue;
            }
            
            NSArray *dateCreatedArray = [rootElement elementsForName:@"dateCreated"];
            if (dateCreatedArray.count > 0)
            {
                GDataXMLElement *Element = (GDataXMLElement *)[dateCreatedArray objectAtIndex:0];
                dateCreated = Element.stringValue;
            }
            
            NSArray *fromArray = [rootElement elementsForName:@"from"];
            if (fromArray.count > 0)
            {
                GDataXMLElement *Element = (GDataXMLElement *)[fromArray objectAtIndex:0];
                from = Element.stringValue;
            }
            
            NSArray *whoArray = [rootElement elementsForName:@"who"];
            if (whoArray.count > 0)
            {
                GDataXMLElement *Element = (GDataXMLElement *)[whoArray objectAtIndex:0];
                whoStr = Element.stringValue;
            }
            
            NSArray *videoStateArr = [rootElement elementsForName:@"videoState"];
            if (videoStateArr.count > 0)
            {
                GDataXMLElement  *Element = (GDataXMLElement *)[videoStateArr objectAtIndex:0];
                videoState = Element.stringValue.integerValue;
            }
            
            NSArray *videoSrcArr = [rootElement elementsForName:@"videoSource"];
            if (videoSrcArr.count > 0)
            {
                GDataXMLElement *element = (GDataXMLElement*) [videoSrcArr objectAtIndex:0];
                videoSrc = element.stringValue;
            }
            
            [voiceMsgDic setValue:msgType forKey:KEY_VOICEMESSAGE_TYPE];
            if(type == 601)
            {
                VideoConferenceJoinMsg * videoconferencemsg = [[VideoConferenceJoinMsg alloc] init];
                videoconferencemsg.conferenceId = roomId;
                videoconferencemsg.joinArr = whoStr.length>0?[whoStr componentsSeparatedByString:@","]:nil;
                videoconferencemsg.videoState = videoState;
                videoconferencemsg.videoSrc = videoSrc;
                [voiceMsgDic setValue:videoconferencemsg forKey:KEY_VOICEMESSAGE_DATA];
                [videoconferencemsg release];
            }
            else if(type == 602)
            {
                VideoConferenceExitMsg * videoconferencemsg = [[VideoConferenceExitMsg alloc] init];
                videoconferencemsg.conferenceId = roomId;
                videoconferencemsg.exitArr = whoStr.length>0?[whoStr componentsSeparatedByString:@","]:nil;
                [voiceMsgDic setValue:videoconferencemsg forKey:KEY_VOICEMESSAGE_DATA];
                [videoconferencemsg release];
            }
            else if(type == 603)
            {
                VideoConferenceDismissMsg *videoconferencemsg = [[VideoConferenceDismissMsg alloc] init];
                videoconferencemsg.conferenceId = roomId;
                [voiceMsgDic setValue:videoconferencemsg forKey:KEY_VOICEMESSAGE_DATA];
                [videoconferencemsg release];
            }
            else if(type == 604)
            {
                VideoConferenceRemoveMemberMsg *videoconferencemsg = [[VideoConferenceRemoveMemberMsg alloc] init];
                videoconferencemsg.conferenceId = roomId;
                videoconferencemsg.who = whoStr;
                [voiceMsgDic setValue:videoconferencemsg forKey:KEY_VOICEMESSAGE_DATA];
                [videoconferencemsg release];
            }
            else if (type == 605)
            {
                VideoConferenceSwitchScreenMsg *videoconferencemsg = [[VideoConferenceSwitchScreenMsg alloc] init];
                videoconferencemsg.conferenceId = roomId;
                videoconferencemsg.displayVoip = whoStr;
                [voiceMsgDic setValue:videoconferencemsg forKey:KEY_VOICEMESSAGE_DATA];
                [videoconferencemsg release];
            }
            else if(type == 607)
            {
                VideoConferencePublishVideoMsg *videoconferencemsg = [[VideoConferencePublishVideoMsg alloc] init];
                videoconferencemsg.conferenceId = roomId;
                videoconferencemsg.who = whoStr;
                videoconferencemsg.videoState = videoState;
                videoconferencemsg.videoSrc = videoSrc;
                [voiceMsgDic setValue:videoconferencemsg forKey:KEY_VOICEMESSAGE_DATA];
                [videoconferencemsg release];
            }
            else if (type == 608)
            {
                VideoConferenceUnpublishVideoMsg *videoconferencemsg = [[VideoConferenceUnpublishVideoMsg alloc] init];
                videoconferencemsg.conferenceId = roomId;
                videoconferencemsg.who = whoStr;
                videoconferencemsg.videoState = videoState;
                [voiceMsgDic setValue:videoconferencemsg forKey:KEY_VOICEMESSAGE_DATA];
                [videoconferencemsg release];
            }
        }
    }
    [xmldoc release];
}

- (void)parseStatuscodeElement:(GDataXMLElement*)rootElement withData:(NSMutableDictionary*)userData
{
    NSArray *statuscodeArray = [rootElement elementsForName:@"statusCode"];
    if (statuscodeArray.count > 0)
    {
        GDataXMLElement *element = (GDataXMLElement *)[statuscodeArray objectAtIndex:0];
        [userData setValue:element.stringValue forKey:KEY_RESPONSE_STATUSCODE];
        
        NSArray *statusmsgArray = [rootElement elementsForName:@"statusMsg"];
        if (statusmsgArray.count > 0)
        {
            GDataXMLElement *msgelement = (GDataXMLElement *)[statusmsgArray objectAtIndex:0];
            [userData setValue:msgelement.stringValue forKey:KEY_RESPONSE_STATUSMSG];
        }
    }
    else
    {
        [userData setValue:[NSString stringWithFormat:@"%d", ELocalReason_ErrorXml] forKey:KEY_RESPONSE_STATUSCODE];
        [userData setValue:@"解析XML错误" forKey:KEY_RESPONSE_STATUSMSG];
        
    }
}

- (void)parseSipAddress:(GDataXMLElement*)rootElement withData:(NSMutableDictionary *)userData
{
    NSArray *switchArray = [rootElement elementsForName:@"Switch"];
    if (switchArray.count > 0)
    {
        GDataXMLElement *switchElement = (GDataXMLElement*)[switchArray objectAtIndex:0];
        
        
        //clpss
        NSArray *clpssArray = [switchElement elementsForName:@"clpss"];
        if ([clpssArray count]>0)
        {
            [userData setObject:[rootElement XMLString] forKey:SERVER_GetSipAddressXML_KEY];
        }
        else//容错处理，新的Rest版本上线以后不会这么返回了
        {
            NSMutableDictionary* dict = [[NSMutableDictionary alloc] init];
            NSArray *ipArray = [switchElement elementsForName:@"ip"];
            if (ipArray.count > 0)
            {
                GDataXMLElement *ipElement = (GDataXMLElement *)[ipArray objectAtIndex:0];
                [dict setObject:ipElement.stringValue forKey:@"ip"];
            }
            NSArray *portArray = [switchElement elementsForName:@"port"];
            if (portArray.count > 0)
            {
                GDataXMLElement *portElement = (GDataXMLElement *)[portArray objectAtIndex:0];
                [dict setObject:portElement.stringValue forKey:@"port"];
            }
            [userData setObject:dict forKey:SERVERIP_PORT_KEY];
            [dict release];
        }
        
        NSArray *IPArr = [switchElement elementsForName:@"p2p"];
        if (IPArr.count > 0)
        {
            GDataXMLElement *IPElement = (GDataXMLElement *)[IPArr objectAtIndex:0];
            [userData setObject:IPElement.stringValue forKey:SERVER_P2P_KEY];
        }
        
        NSArray *controlArr = [switchElement elementsForName:@"control"];
        if (controlArr.count > 0)
        {
            GDataXMLElement *controlElement = (GDataXMLElement *)[controlArr objectAtIndex:0];
            [userData setObject:controlElement.stringValue forKey:SERVER_STR_CAPABILITY];
        }
        
        NSArray *nwgidArr = [switchElement elementsForName:@"nwgid"];
        if (nwgidArr.count > 0)
        {
            GDataXMLElement *nwgidElement = (GDataXMLElement *)[nwgidArr objectAtIndex:0];
            [userData setObject:nwgidElement.stringValue forKey:NETWORK_GROUPID];
        }
        
        NSArray *lastArr = [switchElement elementsForName:@"lastLoginTime"];
        if (lastArr.count > 0)
        {
            GDataXMLElement *lastElement = (GDataXMLElement *)[lastArr objectAtIndex:0];
            [userData setObject:lastElement.stringValue forKey:KEY_LastLoginTime];
        }
    }
}

- (void)parseNetGroupId:(GDataXMLElement*)rootElement withData:(NSMutableDictionary *)userData
{
    NSArray *interphoneIdArray = [rootElement elementsForName:@"nwgid"];
    if (interphoneIdArray.count > 0)
    {
        GDataXMLElement *idElement = (GDataXMLElement *)[interphoneIdArray objectAtIndex:0];
        [userData setValue:idElement.stringValue forKey:KEY_RESPONSE_USERDATA];
    }
}

- (void)parseCheckVoipOfflineMsg:(GDataXMLElement*)rootElement withData:(NSMutableDictionary*)userData
{
    NSArray *offlineMsgArray = [rootElement elementsForName:@"offlineMsg"];
    if (offlineMsgArray.count > 0) {
        NSMutableArray* senderArray = [[NSMutableArray alloc] init];
        for (GDataXMLElement *senderElement in offlineMsgArray)
        {
            CheckVoipOfflineMsg* offlinemsgcount = [[CheckVoipOfflineMsg alloc] init];
            GDataXMLElement *countXMLElement = [[senderElement elementsForName:@"count"] objectAtIndex:0];
            GDataXMLElement *senderXMLElement= [[senderElement elementsForName:@"sender"] objectAtIndex:0];
            offlinemsgcount.sender = senderXMLElement.stringValue;
            offlinemsgcount.count = countXMLElement.stringValue.integerValue;
            [senderArray addObject:offlinemsgcount];
        }
        [userData setValue:senderArray forKey:KEY_RESPONSE_USERDATA];
        [senderArray release];
    }
}

- (InstanceMsg*)parseVoipOfflineMsg:(GDataXMLElement*)rootElement
{
    NSString *userDataStr = nil;
    NSString *msgId = nil;
    NSString *dateCreated = nil;
    NSString *sender = nil;
    NSString *receiver = nil;
    NSInteger fileSize = 0;
    NSString *fileUrl = nil;
    NSString *ext = nil;
    NSString *message = nil;
    
    NSArray *msgIdArray = [rootElement elementsForName:@"msgId"];
    {
        GDataXMLElement *Element = (GDataXMLElement *)[msgIdArray objectAtIndex:0];
        msgId = Element.stringValue;
    }
    
    NSArray *dateCreatedArray = [rootElement elementsForName:@"dateCreated"];
    if (dateCreatedArray.count > 0)
    {
        GDataXMLElement *Element = (GDataXMLElement *)[dateCreatedArray objectAtIndex:0];
        dateCreated = Element.stringValue;
    }
    
    NSArray *senderArray = [rootElement elementsForName:@"sender"];
    if (senderArray.count > 0)
    {
        GDataXMLElement *Element = (GDataXMLElement *)[senderArray objectAtIndex:0];
        sender = Element.stringValue;
    }
    
    NSArray *receiverArray = [rootElement elementsForName:@"receiver"];
    if (receiverArray.count > 0)
    {
        GDataXMLElement *Element = (GDataXMLElement *)[receiverArray objectAtIndex:0];
        receiver = Element.stringValue;
    }
    
    NSArray *fileUrlArray = [rootElement elementsForName:@"fileUrl"];
    if (fileUrlArray.count > 0)
    {
        GDataXMLElement *Element = (GDataXMLElement *)[fileUrlArray objectAtIndex:0];
        fileUrl = Element.stringValue;
    }
    
    NSArray *fileSizeArray = [rootElement elementsForName:@"fileSize"];
    if (fileSizeArray.count > 0)
    {
        GDataXMLElement *Element = (GDataXMLElement *)[fileSizeArray objectAtIndex:0];
        fileSize = Element.stringValue.integerValue;
    }
    
    NSArray *extArray = [rootElement elementsForName:@"fileExt"];
    if (extArray.count > 0)
    {
        GDataXMLElement *Element = (GDataXMLElement *)[extArray objectAtIndex:0];
        ext = Element.stringValue;
    }
    
    NSArray *messageArray = [rootElement elementsForName:@"text"];
    if (messageArray.count > 0)
    {
        GDataXMLElement *Element = (GDataXMLElement *)[messageArray objectAtIndex:0];
        message = Element.stringValue;
    }
    
    NSArray *dataArray = [rootElement elementsForName:@"userData"];
    if (dataArray.count > 0)
    {
        GDataXMLElement *Element = (GDataXMLElement *)[dataArray objectAtIndex:0];
        userDataStr = Element.stringValue;
    }
    
    if ([ext length]>0) {
        IMAttachedMsg *msg = [[[IMAttachedMsg alloc] init] autorelease];
        msg.userData = userDataStr;
        msg.msgId = msgId;
        msg.dateCreated = dateCreated;
        msg.sender = sender;
        msg.receiver = receiver;
        msg.fileSize = fileSize;
        msg.fileUrl = fileUrl;
        msg.ext = ext;
        msg.chunked = NO;
        return msg;
    }
    else
    {
        IMTextMsg *msg = [[[IMTextMsg alloc] init] autorelease];
        msg.userData = userDataStr;
        msg.msgId = msgId;
        msg.dateCreated = dateCreated;
        msg.sender = sender;
        msg.receiver = receiver;
        msg.message = message;
        return msg;
    }
}
- (void)parseGetVoipOfflineMsg:(GDataXMLElement*)rootElement withData:(NSMutableDictionary*)userData
{
    NSString *timestamp = nil;
    NSInteger remainCount = 0;
    NSArray *offlineMsgArray = [rootElement elementsForName:@"offlineMsg"];
    OfflineMsg* voipOfflineMsgs = [[OfflineMsg alloc] init];
    if (offlineMsgArray.count > 0) {
        NSMutableArray* msgArray = [[NSMutableArray alloc] init];
        for (GDataXMLElement *offlineMsg in offlineMsgArray)
        {
            [msgArray addObject:[self parseVoipOfflineMsg:offlineMsg]];
        }
        voipOfflineMsgs.msgs = msgArray;
        [msgArray release];
    }
    NSArray *remainCountArray = [rootElement elementsForName:@"remainCount"];
    if (remainCountArray.count > 0)
    {
        GDataXMLElement *Element = (GDataXMLElement *)[remainCountArray objectAtIndex:0];
        remainCount = Element.stringValue.integerValue;
    }
    NSArray *timestampArray = [rootElement elementsForName:@"timestamp"];
    {
        GDataXMLElement *Element = (GDataXMLElement *)[timestampArray objectAtIndex:0];
        timestamp = Element.stringValue;
    }
    voipOfflineMsgs.remainCount = remainCount;
    voipOfflineMsgs.timestamp = timestamp;
    [userData setValue:voipOfflineMsgs forKey:KEY_RESPONSE_USERDATA];
    [voipOfflineMsgs release];
}

//interphone相关解析包函数
- (void)parseStartInterphone:(GDataXMLElement*)rootElement withData:(NSMutableDictionary*)userData
{
    NSArray *interphoneIdArray = [rootElement elementsForName:@"interphoneId"];
    if (interphoneIdArray.count > 0)
    {
        GDataXMLElement *idElement = (GDataXMLElement *)[interphoneIdArray objectAtIndex:0];
        [userData setValue:idElement.stringValue forKey:KEY_RESPONSE_USERDATA];
    }
}

- (void)parseControlMic:(GDataXMLElement*)rootElement withData:(NSMutableDictionary*)userData
{
    NSArray *whoArray = [rootElement elementsForName:@"voipAccount"];
    if (whoArray.count > 0)
    {
        GDataXMLElement *Element = (GDataXMLElement *)[whoArray objectAtIndex:0];
        NSString* value = Element.stringValue;
        [userData setValue:value forKey:KEY_RESPONSE_USERDATA];
    }
}

- (void)parseReleaseMic:(GDataXMLElement*)rootElement withData:(NSMutableDictionary*)userData
{
    
}

- (void)parseGetInterphoneMemberlist:(GDataXMLElement*)rootElement withData:(NSMutableDictionary*)userData
{
    
    NSArray *countArray = [rootElement elementsForName:@"count"];
    NSArray *membersArray = [rootElement elementsForName:@"member"];
    if (countArray.count > 0)
    {
        GDataXMLElement *countElement = [countArray objectAtIndex:0];
        NSInteger count = countElement.stringValue.integerValue;
        if (count > 0 && membersArray.count > 0)
        {
            NSMutableArray *membersInfo = [[NSMutableArray alloc] initWithCapacity:count];
            for (GDataXMLElement *memberElement in membersArray)
            {
                InterphoneMember *memberInfo = [[InterphoneMember alloc] init];
                
                NSArray *voipIdArray = [memberElement elementsForName:@"voipAccount"];
                if (voipIdArray.count > 0)
                {
                    GDataXMLElement *valueElement = (GDataXMLElement *)[voipIdArray objectAtIndex:0];
                    memberInfo.voipId = valueElement.stringValue;
                }
                
                NSArray *typeArray = [memberElement elementsForName:@"type"];
                if (typeArray.count > 0)
                {
                    GDataXMLElement *valueElement = (GDataXMLElement *)[typeArray objectAtIndex:0];
                    memberInfo.type = valueElement.stringValue;
                }
                
                NSArray *onlineArray = [memberElement elementsForName:@"online"];
                if (onlineArray.count > 0)
                {
                    GDataXMLElement *valueElement = (GDataXMLElement *)[onlineArray objectAtIndex:0];
                    memberInfo.online = valueElement.stringValue;
                }
                
                NSArray *micArray = [memberElement elementsForName:@"mic"];
                if (micArray.count > 0)
                {
                    GDataXMLElement *valueElement = (GDataXMLElement *)[micArray objectAtIndex:0];
                    memberInfo.mic = valueElement.stringValue;
                }
                else
                {
                    memberInfo.mic = @"0";
                }
                
                [membersInfo addObject:memberInfo];
                [memberInfo release];
                
            }
            
            [userData setObject:membersInfo forKey:KEY_RESPONSE_USERDATA];
            [membersInfo release];
        }
    }
    
}

#pragma mark - 解析聊天室
- (void)parseStartChatroom:(GDataXMLElement*)rootElement withData:(NSMutableDictionary*)userData
{
    NSArray *chatroomIdArray = [rootElement elementsForName:@"roomId"];
    if (chatroomIdArray.count > 0)
    {
        GDataXMLElement *idElement = (GDataXMLElement *)[chatroomIdArray objectAtIndex:0];
        [userData setValue:idElement.stringValue forKey:KEY_RESPONSE_USERDATA];
    }
}

- (void)parseQueryChatRoomList:(GDataXMLElement*)rootElement withData:(NSMutableDictionary*)userData
{
    NSArray *countArray = [rootElement elementsForName:@"count"];
    NSArray *roomsArray = [rootElement elementsForName:@"ChatRoom"];
    if (countArray.count > 0)
    {
        GDataXMLElement *countElement = [countArray objectAtIndex:0];
        NSInteger count = countElement.stringValue.integerValue;
        if (count > 0 && roomsArray.count > 0)
        {
            NSMutableArray *roomsInfoArray = [[NSMutableArray alloc] initWithCapacity:count];
            for (GDataXMLElement *memberElement in roomsArray)
            {
                Chatroom *roomInfo = [[Chatroom alloc] init];
                
                NSArray *roomIdArray = [memberElement elementsForName:@"roomId"];
                if (roomIdArray.count > 0)
                {
                    GDataXMLElement *valueElement = (GDataXMLElement *)[roomIdArray objectAtIndex:0];
                    roomInfo.roomNo = valueElement.stringValue;
                }
                
                NSArray *nameArray = [memberElement elementsForName:@"roomName"];
                if (nameArray.count > 0)
                {
                    GDataXMLElement *valueElement = (GDataXMLElement *)[nameArray objectAtIndex:0];
                    roomInfo.roomName = valueElement.stringValue;
                }
                
                NSArray *creatorArray = [memberElement elementsForName:@"creator"];
                if (creatorArray.count > 0)
                {
                    GDataXMLElement *valueElement = (GDataXMLElement *)[creatorArray objectAtIndex:0];
                    roomInfo.creator = valueElement.stringValue;
                }
                
                NSArray *squareArray = [memberElement elementsForName:@"square"];
                if (squareArray.count > 0)
                {
                    GDataXMLElement *valueElement = (GDataXMLElement *)[squareArray objectAtIndex:0];
                    roomInfo.square = valueElement.stringValue.integerValue;
                }
                
                NSArray *keywordsArray = [memberElement elementsForName:@"keywords"];
                if (keywordsArray.count > 0)
                {
                    GDataXMLElement *valueElement = (GDataXMLElement *)[keywordsArray objectAtIndex:0];
                    roomInfo.keywords = valueElement.stringValue;
                }

                NSArray *validateArray = [memberElement elementsForName:@"validate"];
                if (validateArray.count > 0)
                {
                    GDataXMLElement *valueElement = (GDataXMLElement *)[validateArray objectAtIndex:0];
                    roomInfo.validate = valueElement.stringValue.integerValue;
                }
                
                NSArray *joinArray = [memberElement elementsForName:@"joined"];
                if (joinArray.count > 0)
                {
                    GDataXMLElement *valueElement = (GDataXMLElement *)[joinArray objectAtIndex:0];
                    roomInfo.joinNum = valueElement.stringValue.integerValue;
                }
                
                [roomsInfoArray addObject:roomInfo];
                [roomInfo release];
                
            }
            
            [userData setObject:roomsInfoArray forKey:KEY_RESPONSE_USERDATA];
            [roomsInfoArray release];
        }
    }
}

- (void)parseQueryChatRoomMembers:(GDataXMLElement*)rootElement withData:(NSMutableDictionary*)userData
{
    NSArray *countArray = [rootElement elementsForName:@"count"];
    NSArray *membersArray = [rootElement elementsForName:@"member"];
    if (countArray.count > 0)
    {
        GDataXMLElement *countElement = [countArray objectAtIndex:0];
        NSInteger count = countElement.stringValue.integerValue;
        if (count > 0 && membersArray.count > 0)
        {
            NSMutableArray *membersInfoArray = [[NSMutableArray alloc] initWithCapacity:count];
            for (GDataXMLElement *memberElement in membersArray)
            {
                ChatroomMember *memberInfo = [[ChatroomMember alloc] init];
                
                NSArray *numberArray = [memberElement elementsForName:@"number"];
                if (numberArray.count > 0)
                {
                    GDataXMLElement *valueElement = (GDataXMLElement *)[numberArray objectAtIndex:0];
                    memberInfo.number = valueElement.stringValue;
                }
                
                NSArray *typeArray = [memberElement elementsForName:@"type"];
                if (typeArray.count > 0)
                {
                    GDataXMLElement *valueElement = (GDataXMLElement *)[typeArray objectAtIndex:0];
                    memberInfo.type = valueElement.stringValue;
                }
                               
                [membersInfoArray addObject:memberInfo];
                [memberInfo release];
                
            }
            
            [userData setObject:membersInfoArray forKey:KEY_RESPONSE_USERDATA];
            [membersInfoArray release];
        }
    }
}


#pragma mark - 多媒体IM解析
- (BOOL)parseSendMediaMsg:rootElement withFile:(NSString *)filename withData:(NSMutableDictionary *)userData withChunkedFlag:(NSString*) chunkedFlag andMsgid:(NSString*)msgid
{
    IMAttachedMsg *msg = [[IMAttachedMsg alloc] init];
    msg.msgId = msgid;
    msg.ext = [filename pathExtension];
    NSArray* voiceMsgsArray = [rootElement elementsForName:@"InstanceMessage"];
    if (voiceMsgsArray.count > 0)
    {
        GDataXMLElement *voiceMsgElement = (GDataXMLElement*)[voiceMsgsArray objectAtIndex:0];
        
        NSString* uploadUrl = nil;
        NSString* uploadToken = nil;
        
        NSArray *uploadUrlArr = [voiceMsgElement elementsForName:@"uploadUrl"];
        if (uploadUrlArr.count > 0)
        {
            GDataXMLElement *valueElement = (GDataXMLElement*)[uploadUrlArr objectAtIndex:0];
            uploadUrl = valueElement.stringValue;
        }
        
        NSArray *uploadTokenArr = [voiceMsgElement elementsForName:@"uploadToken"];
        if (uploadTokenArr.count > 0)
        {
            GDataXMLElement *valueElement = (GDataXMLElement*)[uploadTokenArr objectAtIndex:0];
            uploadToken = valueElement.stringValue;
        }
        
        NSArray *dateCreatedArr = [voiceMsgElement elementsForName:@"dateCreated"];
        if (dateCreatedArr.count > 0)
        {
            GDataXMLElement *valueElement = (GDataXMLElement*)[dateCreatedArr objectAtIndex:0];
            msg.dateCreated = valueElement.stringValue;
        }
        
        if (uploadUrl && uploadToken)
        {
            msg.fileUrl = filename;
            [self uploadMediaFile:filename to:uploadUrl andToken:uploadToken andChunkedFlag:chunkedFlag withData:msg];
            [msg release];
            return YES;
        }
    }
    else
    {
        NSArray* statusCodeArray = [rootElement elementsForName:@"statusCode"];
        if (statusCodeArray.count > 0)
        {
            GDataXMLElement *statusCodeElement = (GDataXMLElement*)[statusCodeArray objectAtIndex:0];
            if (statusCodeElement.stringValue.intValue !=0 ) {
                [userData setValue:[NSString stringWithFormat:@"%d", statusCodeElement.stringValue.intValue] forKey:KEY_RESPONSE_STATUSCODE];
                
                NSArray *statusmsgArray = [rootElement elementsForName:@"statusMsg"];
                if (statusmsgArray.count > 0)
                {
                    GDataXMLElement *msgelement = (GDataXMLElement *)[statusmsgArray objectAtIndex:0];
                    [userData setValue:msgelement.stringValue forKey:KEY_RESPONSE_STATUSMSG];
                }
                
                [userData setValue:msg forKey:KEY_USERINFO_SEND_MEDIAMSG_DATA];
                [msg release];
                return NO;
            }
        }
    }
    [userData setValue:[NSString stringWithFormat:@"%d", ELocalReason_ErrorXmlBody] forKey:KEY_RESPONSE_STATUSCODE];
    [userData setValue:@"解析XML错误" forKey:KEY_RESPONSE_STATUSMSG];
    [userData setValue:msg forKey:KEY_USERINFO_SEND_MEDIAMSG_DATA];
    [msg release];
    return NO;
}

/*
- (void)parseQueryNewMediaMessages:(GDataXMLElement*)rootElement  withData:(NSMutableDictionary*)userData
{
    NSArray *countArray = [rootElement elementsForName:@"count"];                                                            
    NSArray *newMsgsRootArray = [rootElement elementsForName:@"InstanceMessage"];
    if (countArray.count > 0)
    {
        GDataXMLElement *countElement = [countArray objectAtIndex:0];
        NSInteger count = countElement.stringValue.integerValue;
        if (count > 0 && newMsgsRootArray.count > 0)
        {
            NSMutableArray *newMsgsInfo = [[NSMutableArray alloc] initWithCapacity:count];
            for (GDataXMLElement *newMsgElement in newMsgsRootArray)
            {
                MediaMsg *msgInfo = [[MediaMsg alloc] init];
                
                NSArray *voiceMsgIdArray = [newMsgElement elementsForName:@"msgId"];
                if (voiceMsgIdArray.count > 0)
                {
                    GDataXMLElement *valueElement = (GDataXMLElement*)[voiceMsgIdArray objectAtIndex:0];
                    msgInfo.mediaMessageSid = valueElement.stringValue;
                }
                
                NSArray *dateCreatedArray = [newMsgElement elementsForName:@"dateCreated"];
                if (dateCreatedArray.count > 0)
                {
                    GDataXMLElement *valueElement = (GDataXMLElement*)[dateCreatedArray objectAtIndex:0];
                    msgInfo.dateCreated = valueElement.stringValue;
                }
                
                NSArray *fromArray = [newMsgElement elementsForName:@"from"];
                if (fromArray.count > 0)
                {
                    GDataXMLElement *valueElement = (GDataXMLElement*)[fromArray objectAtIndex:0];
                    msgInfo.from = valueElement.stringValue;
                }
                
                NSArray *groupIdArr = [newMsgElement elementsForName:@"groupId"];
                if (groupIdArr.count > 0)
                {
                    GDataXMLElement *valueElement = (GDataXMLElement*)[groupIdArr objectAtIndex:0];
                    msgInfo.groupId = valueElement.stringValue;
                }
                
                
                NSArray *extArr = [newMsgElement elementsForName:@"ext"];
                if (extArr.count > 0)
                {
                    GDataXMLElement *valueElement = (GDataXMLElement*)[extArr objectAtIndex:0];
                    msgInfo.ext = valueElement.stringValue.integerValue;
                }
                
                NSArray *filesizeArr = [newMsgElement elementsForName:@"fileSize"];
                if (filesizeArr.count > 0)
                {
                    GDataXMLElement *valueElement = (GDataXMLElement*)[filesizeArr objectAtIndex:0];
                    msgInfo.filesize = valueElement.stringValue.integerValue;
                }
                
                NSArray *fileurlArr = [newMsgElement elementsForName:@"fileUrl"];
                if (fileurlArr.count > 0)
                {
                    GDataXMLElement *valueElement = (GDataXMLElement*)[fileurlArr objectAtIndex:0];
                    msgInfo.fileurl = valueElement.stringValue;
                }
                
                [newMsgsInfo addObject:msgInfo];
                
                [msgInfo release];
            }
            
            [userData setObject:newMsgsInfo forKey:KEY_RESPONSE_USERDATA];
            [newMsgsInfo release];
        }
    }
}
*/

#pragma mark - 视频会议解析

- (void)parseStartVideoConference:(GDataXMLElement*)rootElement withData:(NSMutableDictionary*)userData
{
    NSArray *roomIdArray = [rootElement elementsForName:@"roomId"];
    if (roomIdArray.count > 0)
    {
        GDataXMLElement *idElement = (GDataXMLElement *)[roomIdArray objectAtIndex:0];
        [userData setValue:idElement.stringValue forKey:KEY_RESPONSE_USERDATA];
    }
}

- (void)parseQueryVideoConferenceList:(GDataXMLElement*)rootElement withData:(NSMutableDictionary*)userData
{
    NSArray *countArray = [rootElement elementsForName:@"count"];
    NSArray *roomsArray = [rootElement elementsForName:@"VideoConf"];
    if (countArray.count > 0)
    {
        GDataXMLElement *countElement = [countArray objectAtIndex:0];
        NSInteger count = countElement.stringValue.integerValue;
        if (count > 0 && roomsArray.count > 0)
        {
            NSMutableArray *roomsInfoArray = [[NSMutableArray alloc] initWithCapacity:count];
            for (GDataXMLElement *memberElement in roomsArray)
            {
                VideoConference *roomInfo = [[VideoConference alloc] init];
                
                NSArray *roomIdArray = [memberElement elementsForName:@"roomId"];
                if (roomIdArray.count > 0)
                {
                    GDataXMLElement *valueElement = (GDataXMLElement *)[roomIdArray objectAtIndex:0];
                    roomInfo.conferenceId = valueElement.stringValue;
                }
                
                NSArray *nameArray = [memberElement elementsForName:@"roomName"];
                if (nameArray.count > 0)
                {
                    GDataXMLElement *valueElement = (GDataXMLElement *)[nameArray objectAtIndex:0];
                    roomInfo.conferenceName = valueElement.stringValue;
                }
                
                NSArray *creatorArray = [memberElement elementsForName:@"creator"];
                if (creatorArray.count > 0)
                {
                    GDataXMLElement *valueElement = (GDataXMLElement *)[creatorArray objectAtIndex:0];
                    roomInfo.creator = valueElement.stringValue;
                }
                
                NSArray *squareArray = [memberElement elementsForName:@"square"];
                if (squareArray.count > 0)
                {
                    GDataXMLElement *valueElement = (GDataXMLElement *)[squareArray objectAtIndex:0];
                    roomInfo.square = valueElement.stringValue.integerValue;
                }
                
                NSArray *keywordsArray = [memberElement elementsForName:@"keywords"];
                if (keywordsArray.count > 0)
                {
                    GDataXMLElement *valueElement = (GDataXMLElement *)[keywordsArray objectAtIndex:0];
                    roomInfo.keywords = valueElement.stringValue;
                }
                
                NSArray *validateArray = [memberElement elementsForName:@"validate"];
                if (validateArray.count > 0)
                {
                    GDataXMLElement *valueElement = (GDataXMLElement *)[validateArray objectAtIndex:0];
                    roomInfo.validate = valueElement.stringValue.integerValue;
                }
                
                NSArray *joinArray = [memberElement elementsForName:@"joined"];
                if (joinArray.count > 0)
                {
                    GDataXMLElement *valueElement = (GDataXMLElement *)[joinArray objectAtIndex:0];
                    roomInfo.joinNum = valueElement.stringValue.integerValue;
                }
                
                NSArray *isMultiVideoArray = [memberElement elementsForName:@"isMultiVideo"];
                if (isMultiVideoArray.count > 0)
                {
                    GDataXMLElement *valueElement = (GDataXMLElement *)[isMultiVideoArray objectAtIndex:0];
                    roomInfo.isMultiVideo = valueElement.stringValue.integerValue;
                }
                
                NSArray *confAttrsArray = [memberElement elementsForName:@"confAttrs"];
                if (confAttrsArray.count > 0)
                {
                    GDataXMLElement *valueElement = (GDataXMLElement *)[confAttrsArray objectAtIndex:0];
                    roomInfo.confAttrs = valueElement.stringValue;
                }
                
                [roomsInfoArray addObject:roomInfo];
                [roomInfo release];
                
            }
            
            [userData setObject:roomsInfoArray forKey:KEY_RESPONSE_USERDATA];
            [roomsInfoArray release];
        }
    }
}

- (void)parseQueryVideoConferenceMembers:(GDataXMLElement*)rootElement withData:(NSMutableDictionary*)userData
{
    NSArray *countArray = [rootElement elementsForName:@"count"];
    NSArray *membersArray = [rootElement elementsForName:@"member"];
    if (countArray.count > 0)
    {
        GDataXMLElement *countElement = [countArray objectAtIndex:0];
        NSInteger count = countElement.stringValue.integerValue;
        if (count > 0 && membersArray.count > 0)
        {
            NSMutableArray *membersInfoArray = [[NSMutableArray alloc] initWithCapacity:count];
            for (GDataXMLElement *memberElement in membersArray)
            {
                VideoConferenceMember *memberInfo = [[VideoConferenceMember alloc] init];
                
                NSArray *numberArray = [memberElement elementsForName:@"number"];
                if (numberArray.count > 0)
                {
                    GDataXMLElement *valueElement = (GDataXMLElement *)[numberArray objectAtIndex:0];
                    memberInfo.number = valueElement.stringValue;
                }
                
                NSArray *typeArray = [memberElement elementsForName:@"type"];
                if (typeArray.count > 0)
                {
                    GDataXMLElement *valueElement = (GDataXMLElement *)[typeArray objectAtIndex:0];
                    memberInfo.type = valueElement.stringValue;
                }
                
                NSArray *screenArray = [memberElement elementsForName:@"screen"];
                if (screenArray.count > 0)
                {
                    GDataXMLElement *valueElement = (GDataXMLElement *)[screenArray objectAtIndex:0];
                    memberInfo.screenType = valueElement.stringValue.integerValue;
                }
                
                NSArray *videoStateArray = [memberElement elementsForName:@"videoState"];
                if (videoStateArray.count > 0)
                {
                    GDataXMLElement *valueElement = (GDataXMLElement *)[videoStateArray objectAtIndex:0];
                    memberInfo.videoState = valueElement.stringValue.integerValue;
                }
                
                NSArray *videoSourceArray = [memberElement elementsForName:@"videoSource"];
                if (videoSourceArray.count > 0)
                {
                    GDataXMLElement *valueElement = (GDataXMLElement *)[videoSourceArray objectAtIndex:0];
                    memberInfo.videoSource = valueElement.stringValue;
                }
                
                [membersInfoArray addObject:memberInfo];
                [memberInfo release];
                
            }
            
            [userData setObject:membersInfoArray forKey:KEY_RESPONSE_USERDATA];
            [membersInfoArray release];
        }
    }
}


- (BOOL)parseUploadPortraitToVideoConference:(GDataXMLElement*)rootElement withData:(NSMutableDictionary*)userData fileName:(NSString*)filename
{
    NSArray* voiceMsgsArray = [rootElement elementsForName:@"VideoConf"];
    if (voiceMsgsArray.count > 0)
    {
        GDataXMLElement *voiceMsgElement = (GDataXMLElement*)[voiceMsgsArray objectAtIndex:0];
        
        NSString* uploadUrl = nil;
        NSString* uploadToken = nil;
        
        NSArray *uploadUrlArr = [voiceMsgElement elementsForName:@"uploadUrl"];
        if (uploadUrlArr.count > 0)
        {
            GDataXMLElement *valueElement = (GDataXMLElement*)[uploadUrlArr objectAtIndex:0];
            uploadUrl = valueElement.stringValue;
        }
        
        NSArray *uploadTokenArr = [voiceMsgElement elementsForName:@"uploadToken"];
        if (uploadTokenArr.count > 0)
        {
            GDataXMLElement *valueElement = (GDataXMLElement*)[uploadTokenArr objectAtIndex:0];
            uploadToken = valueElement.stringValue;
        }
        
        if (uploadUrl && uploadToken)
        {
            [self uploadPortrait:filename to:uploadUrl andToken:uploadToken];
            return YES;
        }
    }
    return NO;
}

- (void)uploadPortrait:(NSString*)file to:(NSString*)fileUrl andToken:(NSString*)token
{
    NSString *fileServerIP = nil;
    NSString *fileServerPort = nil;
    NSString *fileServerUrl = nil;
    BOOL isUrlRight = NO;
    if (fileUrl.length > 0)
    {
        NSArray *urlArray = [fileUrl componentsSeparatedByString:@"http://"];
        if (urlArray.count >= 2) {
            NSString *urlString = [urlArray objectAtIndex:1];
            NSArray *addressArray = [urlString componentsSeparatedByString:@"/"];
            if (addressArray.count > 0) {
                NSString *addressString = [addressArray objectAtIndex:0];
                NSArray *ipAndPortArray = [addressString componentsSeparatedByString:@":"];
                if (ipAndPortArray.count == 2) {
                    isUrlRight = YES;
                    fileServerIP = [ipAndPortArray objectAtIndex:0];
                    fileServerPort = [ipAndPortArray objectAtIndex:1];
                    NSArray *serverUrlArray = [urlString componentsSeparatedByString:addressString];
                    fileServerUrl = [serverUrlArray objectAtIndex:1];
                    [self postLoginfo:[NSString stringWithFormat:@"uploadMediaFile ip=%@ port=%@ url=%@",fileServerIP,fileServerPort,fileServerUrl]];
                }
            }
        }
    }

    
    if (!isUrlRight)
    {
        NSMutableDictionary* userdata = [NSMutableDictionary dictionaryWithObjectsAndKeys: [NSString stringWithFormat:@"%d", ELocalReason_ErrorXmlBody], KEY_RESPONSE_STATUSCODE, nil];
        [userdata setValue:@"解析XML包体错误" forKey:KEY_RESPONSE_STATUSMSG];
        [self postEventType:ERequestType_SendMediaMsg withData:userdata];
        return;
    }
    
    NSString *requestUrl = [NSString stringWithFormat:@"%@?token=%@",fileUrl,token];

    [self postLoginfo:[NSString stringWithFormat:@"[CCPRestService] uploadMediaFile url=%@",requestUrl]];
    CLOPHTTPRequest *request = [CLOPHTTPRequest requestWithURL:[NSURL URLWithString:requestUrl]];
    request.requestType = ERequestType_UploadPortraitMsg;
    request.userInfo = [NSDictionary dictionaryWithObjectsAndKeys:file, KEY_USERINFO_MEDIAMSG_FILENAME,nil];
    [request setRequestMethod:@"POST"];
    [request addRequestHeader:@"Host" value:[NSString stringWithFormat:@"%@:%@", fileServerIP, fileServerPort]];
    [request addRequestHeader:@"Content-Type" value:@"application/octet-stream"];
    [request addRequestHeader:@"Accept" value:@"application/xml"];
    [request appendPostDataFromFile:file];
    [request setDelegate:self];
    [request setValidatesSecureCertificate:NO];
    [request startAsynchronous];
}

- (void)parseDownloadPortraitFromVideoConference:(GDataXMLElement*)rootElement withData:(NSMutableDictionary*)userData
{
    NSArray *countArray = [rootElement elementsForName:@"count"];
    NSArray *fileListsArray = [rootElement elementsForName:@"fileList"];
    if (countArray.count > 0)
    {
        GDataXMLElement *countElement = [countArray objectAtIndex:0];
        NSInteger count = countElement.stringValue.integerValue;
        if (count > 0 && fileListsArray.count > 0)
        {
            NSMutableArray *fileListInfoArray = [[NSMutableArray alloc] initWithCapacity:count];
            for (GDataXMLElement *memberElement in fileListsArray)
            {
                VideoPartnerPortrait *fileInfo = [[VideoPartnerPortrait alloc] init];
                
                NSArray *dateUpdateArray = [memberElement elementsForName:@"dateUpdate"];
                if (dateUpdateArray.count > 0)
                {
                    GDataXMLElement *valueElement = (GDataXMLElement *)[dateUpdateArray objectAtIndex:0];
                    fileInfo.dateUpdate = valueElement.stringValue;
                }
                
                NSArray *senderArray = [memberElement elementsForName:@"sender"];
                if (senderArray.count > 0)
                {
                    GDataXMLElement *valueElement = (GDataXMLElement *)[senderArray objectAtIndex:0];
                    fileInfo.voip = valueElement.stringValue;
                }
                
                NSArray *fileUrlArray = [memberElement elementsForName:@"fileUrl"];
                if (fileUrlArray.count > 0)
                {
                    GDataXMLElement *valueElement = (GDataXMLElement *)[fileUrlArray objectAtIndex:0];
                    fileInfo.fileUrl = valueElement.stringValue;
                    [fileListInfoArray addObject:fileInfo];
                }            
                
//                NSArray *fileNameArray = [memberElement elementsForName:@"fileName"];
//                if (fileNameArray.count > 0)
//                {
//                    GDataXMLElement *valueElement = (GDataXMLElement *)[fileNameArray objectAtIndex:0];
//                    NSString *fileName = valueElement.stringValue;
//                    if (fileInfo.fileUrl.length > 0 && fileName.length > 0)
//                    {
//                        fileInfo.fileUrl = [NSString stringWithFormat:@"%@?fileName=%@",fileInfo.fileUrl, fileName];
//                        [fileListInfoArray addObject:fileInfo];
//                    }
//                }                
                
                [fileInfo release];
            }
            
            [userData setObject:fileListInfoArray forKey:KEY_RESPONSE_USERDATA];
            [fileListInfoArray release];
        }
    }
}
#pragma mark - 上报消息
- (void)postEventType:(ERequestType)type withData:(NSMutableDictionary *)userData
{
    switch (type)
    {
        case ERequestType_GetSipAddress: //获取软交换地址后登录
        {
            [[NSNotificationCenter defaultCenter] postNotificationName:kGetSipAddressFinished object:nil userInfo:userData];
        }
            break;
        case ERequestType_GetNetGroupId:
        {
            [[NSNotificationCenter defaultCenter] postNotificationName:kGetNetGroupIdFinished object:nil userInfo:userData];
        }
            break;
        case ERequestType_GetCallback: //获取回拨信息后回拨
        {
            [[NSNotificationCenter defaultCenter] postNotificationName:kGetCallbackFinished object:nil userInfo:userData];
        }
            break;
        case ERequestType_UploadMediaMsg:
        {
            [[NSNotificationCenter defaultCenter] postNotificationName:kUploadMediaMsgFinished object:nil userInfo:userData];
        }
            break;
        case ERequestType_DownloadFile_Media:
        {
            [[NSNotificationCenter defaultCenter] postNotificationName:kDownloadMediaMsgFinished object:nil userInfo:userData];
        }
            break;
            
            //实时对讲
        case ERequestType_StartInterphone:
        {
            
            [[NSNotificationCenter defaultCenter] postNotificationName:kStartInterphoneFinished object:nil userInfo:userData];
        }
            break;
        case ERequestType_ControlMic:
        {
            [[NSNotificationCenter defaultCenter] postNotificationName:kControlMicFinished object:nil userInfo:userData];
        }
            break;
        case ERequestType_ReleaseMic:
        {
            [[NSNotificationCenter defaultCenter] postNotificationName:kReleaseMicFinished object:nil userInfo:userData];
            
        }
            break;
        case ERequestType_GetInterphoneMember:
        {
            [[NSNotificationCenter defaultCenter] postNotificationName:kGetInterphoneMemberFinished object:nil userInfo:userData];
        }
            break;
            
            //聊天室
        case ERequestType_GetChatRoomMember:
        {
            [[NSNotificationCenter defaultCenter] postNotificationName:kGetChatRoomMemberFinished object:nil userInfo:userData];
        }
            break;
        case ERequestType_StartChatroom:
        {
            [[NSNotificationCenter defaultCenter] postNotificationName:kStartChatroomFinished object:nil userInfo:userData];
        }
            break;
        case ERequestType_GetChatroomsList:
        {
            [[NSNotificationCenter defaultCenter] postNotificationName:kGetChatRoomsListFinished object:nil userInfo:userData];
        }
            break;
        case ERequestType_InviteJoinChatroom:
        {
            [[NSNotificationCenter defaultCenter] postNotificationName:kInviteMembersChatroomFinished object:nil userInfo:userData];
        }
            break;
                   
        case ERequestType_SendMediaMsg:
        {
            [[NSNotificationCenter defaultCenter] postNotificationName:kSendMediaMsgFinished object:nil userInfo:userData];
        }
            break;
            
        case ERequestType_ConfirmDownloadMediaMessage:
        {
            [[NSNotificationCenter defaultCenter] postNotificationName:kConfirmDownloadAttachFinished object:nil userInfo:userData];
        }
            break;
            
        case ERequestType_DismissChatroom:
        {
            [[NSNotificationCenter defaultCenter] postNotificationName:kDismissChatRoomFinished object:nil userInfo:userData];
        }
            break;
            
        case ERequestType_RemoveMemberFromChatroom:
        {
            [[NSNotificationCenter defaultCenter] postNotificationName:kRemoveMemberFromChatRoomFinished object:nil userInfo:userData];
        }
            break;
            
            //视频会议
        case ERequestType_GetVideoConferenceMember:
        {
            [[NSNotificationCenter defaultCenter] postNotificationName:kGetVideoConferenceMemberFinished object:nil userInfo:userData];
        }
            break;

        case ERequestType_StartVideoConference:
        {
            [[NSNotificationCenter defaultCenter] postNotificationName:kStartVideoConferenceFinished object:nil userInfo:userData];
        }
            break;
        case ERequestType_GetVideoConferencesList:
        {
            [[NSNotificationCenter defaultCenter] postNotificationName:kGetVideoConferenceListFinished object:nil userInfo:userData];
        }
            break;
        case ERequestType_SwitchVideoRealScreen:
        {
            [[NSNotificationCenter defaultCenter] postNotificationName:kSwitchRealScreenFinished object:nil userInfo:userData];
        }
            break;
        case ERequestType_DismissVideoConference:
        {
            [[NSNotificationCenter defaultCenter] postNotificationName:kDismissVideoConferenceFinished object:nil userInfo:userData];
        }
            break;
        case ERequestType_RemoveMemberFromVideoConference:
        {
            [[NSNotificationCenter defaultCenter] postNotificationName:kRemoveMemberFromVideoConferenceFinished object:nil userInfo:userData];
        }
            break;
            
        case ERequestType_UploadPortraitMsg:
        case ERequestType_SendPortraitToVideoConference:
        {
            [[NSNotificationCenter defaultCenter] postNotificationName:kSendVideoConferencePortraitFinished object:nil userInfo:userData];
        }
            break;
            
        case ERequestType_GetPortraitFromVideoConference:
        {
            [[NSNotificationCenter defaultCenter] postNotificationName:kGetVideoConferencePortraitFinished object:nil userInfo:userData];
        }
            break;
        case ERequestType_DownloadPortraitOfVideoConference:
        {
            [[NSNotificationCenter defaultCenter] postNotificationName:kDownloadVideoConferencePortraitFinished object:nil userInfo:userData];
        }
            break;
            
            //多路视频
        case ERequestType_StartMultiVideo:
        {
            [[NSNotificationCenter defaultCenter] postNotificationName:kStartMultiVideoFinished object:nil userInfo:userData];
        }
            break;
            
        case ERequestType_PublishVideo:
        {
            [[NSNotificationCenter defaultCenter] postNotificationName:kPublishVideoMultiVideoConferenceFinished object:nil userInfo:userData];
        }
            break;
            
        case ERequestType_UnpublishVideo:
        {
            [[NSNotificationCenter defaultCenter] postNotificationName:kUnpublishVideoMultiVideoConferenceFinished object:nil userInfo:userData];
        }
            break;
            
            //体验模式下的修改测试号码
        case ERequestType_EditTestNum:
        {
            [[NSNotificationCenter defaultCenter] postNotificationName:kEditTestNumFinished object:nil userInfo:userData];
        }
            break;

        case ERequestType_CheckVoipOfflineMsg:
        {
            [[NSNotificationCenter defaultCenter] postNotificationName:kCheckVoipOfflineMsgFinished object:nil userInfo:userData];
        }
            break;
        case ERequestType_GetVoipOfflineMsg:
        {
            [[NSNotificationCenter defaultCenter] postNotificationName:kGetVoipOfflineMsgFinished object:nil userInfo:userData];
        }
            break;
        case ERequestType_ConfirmOfflineMsg:
        {
            [[NSNotificationCenter defaultCenter] postNotificationName:kConfirmOfflineMsgFinished object:nil userInfo:userData];
        }
            break;
            
        default:
            break;
    }
}

//上报日志
- (void)postLoginfo:(NSString *)loginfo
{
    NSDictionary *userData = [[NSDictionary alloc] initWithObjectsAndKeys:loginfo,SKD_LOG_KEY, nil];
    [[NSNotificationCenter defaultCenter] postNotificationName:kGetRestLoginfo object:nil userInfo:userData];
    [userData release];
}

#pragma mark - rest接口
- (void)getSipAddress
{
    NSInteger tag = [self createRestTag];
    [tagDictionary setObject:[NSNumber numberWithInteger:tag] forKey:TAG_SIP_KEY];
    NSString *timestamp = [self getTimestamp];
    NSString *timestampMD5 = [self getSig:timestamp];
    NSUserDefaults *ud = [NSUserDefaults standardUserDefaults];
    NSString * userID = nil;
    if ([[ud objectForKey:@"user_ID"] length]>0)
    {
        userID = [ud objectForKey:@"user_ID"];
    }
    else
    {
    int x = arc4random() % 1000000;
    const char *cStr = [[NSString stringWithFormat:@"%@%6d",timestamp,x] UTF8String];
	unsigned char result[16];
	CC_MD5(cStr, (CC_LONG)strlen(cStr), result);
	NSString* MD5 =  [NSString stringWithFormat:@"%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X",result[0], result[1], result[2], result[3], result[4], result[5], result[6], result[7],result[8], result[9], result[10], result[11],result[12], result[13], result[14], result[15]];
        NSUserDefaults *ud = [NSUserDefaults standardUserDefaults];
        [ud setObject:MD5 forKey:@"user_ID"];
        [ud synchronize];
        userID = [ud objectForKey:@"user_ID"];
    }
    
    NSTimeInterval time = [[NSDate date] timeIntervalSince1970];
    long long dTime = [[NSNumber numberWithDouble:time*1000] longLongValue]; // 将double转为long long型
    NSString *curTime = [NSString stringWithFormat:@"%llu",dTime]; // 输出long long型
    NSString* strUA = [NSString stringWithFormat:@"%@;%@;%@;%@;%@;%@;%@",[[UIDevice currentDevice] systemName],[[UIDevice currentDevice] systemVersion],kSDKVersion,self.libSDK,self.deviceVersion,userID,curTime];
    
    NSString* strPrivateCloud = @"";
    if ([self.PrivateCloudFlag length] > 0 && [self.ValidateStr length] > 0)
    {
        strPrivateCloud = self.PrivateCloudFlag;
    }
    
    NSString* strBase64UA = [CLOPHTTPRequest base64forData:[strUA dataUsingEncoding:NSUTF8StringEncoding]];
    NSString *requestUrl = [NSString stringWithFormat:@"%@%@:%d/%@/Switchs/%@%@?sig=%@&deviceNo=%@&voipwd=%@&ua=%@",defHttps,self.restip,self.restport,kServerVersion,self.account,strPrivateCloud,timestampMD5,userID,self.password,strBase64UA];
    [self postLoginfo:[NSString stringWithFormat:@"[CCPRestService] getSipAddress url=%@",requestUrl]];
    NSURL *url = [NSURL URLWithString:requestUrl];
    CLOPHTTPRequest *request = [CLOPHTTPRequest requestWithURL:url];
    request.tag = tag;
    request.requestType = ERequestType_GetSipAddress;

    [request addRequestHeader:@"Accept" value:@"application/xml"];
    [request addRequestHeader:@"Content-Type" value:@"application/xml;charset=utf-8"];
    [request addRequestHeader:@"Authorization" value:[self getAuthorization:timestamp]];
    
    if ([self.PrivateCloudFlag length] > 0 && [self.ValidateStr length] > 0)
    {
        [request appendPostData:[self.ValidateStr dataUsingEncoding:NSUTF8StringEncoding]];
        [request setRequestMethod:@"POST"];
    }
    else
        [request setRequestMethod:@"GET"];
    [request setDelegate:self];
    [request setValidatesSecureCertificate:NO];
    [request startAsynchronous];
}

- (void)getNetGroupId
{
    if (self.restip == NULL)
    {
        return;
    }
    NSString *timestamp = [self getTimestamp];
    NSString *timestampMD5 = [self getSig:timestamp];
    NSUserDefaults *ud = [NSUserDefaults standardUserDefaults];
    NSString * userID = nil;
    if ([[ud objectForKey:@"user_ID"] length]>0)
    {
        userID = [ud objectForKey:@"user_ID"];
    }
    else
    {
        int x = arc4random() % 1000000;
        const char *cStr = [[NSString stringWithFormat:@"%@%6d",timestamp,x] UTF8String];
        unsigned char result[16];
        CC_MD5(cStr, (CC_LONG)strlen(cStr), result);
        NSString* MD5 =  [NSString stringWithFormat:@"%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X",result[0], result[1], result[2], result[3], result[4], result[5], result[6], result[7],result[8], result[9], result[10], result[11],result[12], result[13], result[14], result[15]];
        NSUserDefaults *ud = [NSUserDefaults standardUserDefaults];
        [ud setObject:MD5 forKey:@"user_ID"];
        [ud synchronize];
        userID = [ud objectForKey:@"user_ID"];
    }
    
    NSString *imei_mac = [NSString stringWithFormat:@"99000281566650;%@",userID];
    
    NSString *requestUrl = [NSString stringWithFormat:@"%@%@:%d/%@/SubAccounts/%@/QueryNWGID?sig=%@",defHttps,self.restip,restport,kServerVersion,self.accountSid,timestampMD5];
    
    [self postLoginfo:[NSString stringWithFormat:@"[CCPRestService] getNetGroupId url=%@",requestUrl]];
    NSURL *url = [NSURL URLWithString:requestUrl];
    
    CLOPHTTPRequest *request = [CLOPHTTPRequest requestWithURL:url];
    request.requestType = ERequestType_GetNetGroupId;
    [request setRequestMethod:@"POST"];
    [request addRequestHeader:@"Accept" value:@"application/xml"];
    [request addRequestHeader:@"Content-Type" value:@"application/xml;charset=utf-8"];
    [request addRequestHeader:@"Authorization" value:[self getAuthorization:timestamp]];
    
    NSString *xmlBody = [NSString stringWithFormat:@"<?xml version='1.0' encoding='UTF8'?><Request><sha>%@</sha></Request>",[CLOPHTTPRequest base64forData:[imei_mac dataUsingEncoding:NSUTF8StringEncoding]]];
    
    [request appendPostData:[xmlBody dataUsingEncoding:NSUTF8StringEncoding]];
    
    [request setDelegate:self];
    [request setValidatesSecureCertificate:NO];
    [request startAsynchronous];
}

- (void)getCallback:(NSString *)fromPhone withTOCall:(NSString *)toPhone andSrcSerNum:(NSString*) srcSerNum andDestSerNum:(NSString*) destSerNum;
{
    NSInteger tag = [self createRestTag];
    [tagDictionary setObject:[NSNumber numberWithInteger:tag] forKey:TAG_CALLBACK_KEY];
    
    NSString *timestamp = [self getTimestamp];
    NSString *requestUrl = [NSString stringWithFormat:@"%@%@:%d/%@/SubAccounts/%@/Calls/Callback?sig=%@",defHttps,self.restip,restport,kServerVersion,self.accountSid,[self getSig:timestamp]];
    
    [self postLoginfo:[NSString stringWithFormat:@"[CCPRestService] getCallback url=%@",requestUrl]];
    
    CLOPHTTPRequest *request = [CLOPHTTPRequest requestWithURL:[NSURL URLWithString:requestUrl]];
    request.tag = tag;
    request.requestType = ERequestType_GetCallback;
    request.userInfo = [NSDictionary dictionaryWithObjectsAndKeys:fromPhone,@"from", toPhone,@"to", nil];
    [request setRequestMethod:@"POST"];
    [request addRequestHeader:@"Accept" value:@"application/xml"];
    [request addRequestHeader:@"Content-Type" value:@"application/xml;charset=utf-8"];
    [request addRequestHeader:@"Authorization" value:[self getAuthorization:timestamp]];
    NSString* srcStr  = @"";
    NSString* destStr = @"";
    if ([srcSerNum length] > 0)
    {
        srcStr = [NSString stringWithFormat:@"<fromSerNum>%@</fromSerNum>" ,srcSerNum];
    }
    
    if ([destSerNum length] > 0)
    {
        destStr = [NSString stringWithFormat:@"<customerSerNum>%@</customerSerNum>" ,destSerNum];
    }
    
    NSString *xmlBody = [NSString stringWithFormat:@"<?xml version='1.0' encoding='UTF8'?><CallBack><subAccountSid>%@</subAccountSid><voipAccount>%@</voipAccount><from>%@</from><to>%@</to>%@%@</CallBack>",self.accountSid,self.account,fromPhone,toPhone,srcStr,destStr];
    [request appendPostData:[xmlBody dataUsingEncoding:NSUTF8StringEncoding]];
    [request setDelegate:self];
    [request setValidatesSecureCertificate:NO];
    [request startAsynchronous];
}

#pragma mark - 语音留言相关REST API函数
//
//// 上次文件到文件服务器
//- (void)uploadChunkedFile:(NSString*)file to:(NSString*)fileUrl andToken:(NSString*)token withData:(VoiceMsg*)data
//{
//    data.fileurl = file;
//    NSString *fileServerIP = nil;
//    NSString *fileServerPort = nil;
//    NSString *fileServerUrl = nil;
//    BOOL isUrlRight = NO;
//    NSArray *urlArray = [fileUrl componentsSeparatedByString:@"http://"];
//    if (urlArray.count > 0) {
//        NSString *urlString = [urlArray objectAtIndex:1];
//        NSArray *addressArray = [urlString componentsSeparatedByString:@"/"];
//        if (addressArray.count > 0) {
//            NSString *addressString = [addressArray objectAtIndex:0];
//            NSArray *ipAndPortArray = [addressString componentsSeparatedByString:@":"];
//            if (ipAndPortArray.count == 2) {
//                isUrlRight = YES;
//                fileServerIP = [ipAndPortArray objectAtIndex:0];
//                fileServerPort = [ipAndPortArray objectAtIndex:1];
//                NSArray *serverUrlArray = [urlString componentsSeparatedByString:addressString];
//                fileServerUrl = [serverUrlArray objectAtIndex:1];
//                [self postLoginfo:[NSString stringWithFormat:@"ip=%@ port=%@ url=%@",fileServerIP,fileServerPort,fileServerUrl]];
//            }
//        }
//    }
//    
//    if (!isUrlRight)
//    {
//        NSMutableDictionary* userdata = [NSMutableDictionary dictionaryWithObjectsAndKeys: ERROR_XML_BODY, KEY_RESPONSE_STATUSCODE, nil];
//        [self postEventType:ERequestType_UploadVoiceMsg withData:userdata];
//        return;
//    }
//    
//    NSString *requestUrl = [NSString stringWithFormat:@"%@?duration=%lu&token=%@",fileUrl, data.duration,token];
//    
//    [self postLoginfo:[NSString stringWithFormat:@"[CCPRestService] uploadChunkedFile url=%@",requestUrl]];
//    NSString *Url = [NSString stringWithFormat:@"%@?token=%@",fileServerUrl,token];
//    [self.chunkedDelegate sendChunkedFileWithVoiceMsg:data andServerIP:fileServerIP withServerPort:fileServerPort byUrl:Url];
//}

- (void)uploadMediaFile:(NSString*)file to:(NSString*)fileUrl andToken:(NSString*)token andChunkedFlag:(NSString*) chunkedFlag withData:(IMAttachedMsg*)data
{
    data.fileUrl = file;
    NSString *fileServerIP = nil;
    NSString *fileServerPort = nil;
    NSString *fileServerUrl = nil;
    BOOL isUrlRight = NO;
    NSArray *urlArray = [fileUrl componentsSeparatedByString:@"http://"];
    if (urlArray.count > 0) {
        NSString *urlString = [urlArray objectAtIndex:1];
        NSArray *addressArray = [urlString componentsSeparatedByString:@"/"];
        if (addressArray.count > 0) {
            NSString *addressString = [addressArray objectAtIndex:0];
            NSArray *ipAndPortArray = [addressString componentsSeparatedByString:@":"];
            if (ipAndPortArray.count == 2) {
                isUrlRight = YES;
                fileServerIP = [ipAndPortArray objectAtIndex:0];
                fileServerPort = [ipAndPortArray objectAtIndex:1];
                NSArray *serverUrlArray = [urlString componentsSeparatedByString:addressString];
                fileServerUrl = [serverUrlArray objectAtIndex:1];
                [self postLoginfo:[NSString stringWithFormat:@"uploadMediaFile ip=%@ port=%@ url=%@",fileServerIP,fileServerPort,fileServerUrl]];
            }
        }
    }
    
    if (!isUrlRight)
    {
        NSMutableDictionary* userdata = [NSMutableDictionary dictionaryWithObjectsAndKeys: [NSString stringWithFormat:@"%d", ELocalReason_ErrorXmlBody], KEY_RESPONSE_STATUSCODE, data, KEY_USERINFO_SEND_MEDIAMSG_DATA, nil];
        [userdata setValue:@"解析XML包体错误" forKey:KEY_RESPONSE_STATUSMSG];
        [self postEventType:ERequestType_SendMediaMsg withData:userdata];
        return;
    }
    
    NSString *requestUrl = [NSString stringWithFormat:@"%@?token=%@",fileUrl,token];
    
    if ([chunkedFlag isEqualToString:@"1"])
    {
        [self postLoginfo:[NSString stringWithFormat:@"[CCPRestService] uploadChunkedFile url=%@",requestUrl]];
        NSString *Url = [NSString stringWithFormat:@"%@?token=%@",fileServerUrl,token];
        data.ext = @"amr";
        [self.chunkedDelegate sendChunkedFileWithVoiceMsg:data andServerIP:fileServerIP withServerPort:fileServerPort byUrl:Url];
    }
    else
    {
        [self postLoginfo:[NSString stringWithFormat:@"[CCPRestService] uploadMediaFile url=%@",requestUrl]];        
        CLOPHTTPRequest *request = [CLOPHTTPRequest requestWithURL:[NSURL URLWithString:requestUrl]];
        request.requestType = ERequestType_UploadMediaMsg;
        request.userInfo = [NSDictionary dictionaryWithObjectsAndKeys:data, KEY_USERINFO_SEND_MEDIAMSG_DATA,nil];
        [request setRequestMethod:@"POST"];
        [request addRequestHeader:@"Host" value:[NSString stringWithFormat:@"%@:%@", fileServerIP, fileServerPort]];
        [request addRequestHeader:@"Content-Type" value:@"application/octet-stream"];
        [request addRequestHeader:@"Accept" value:@"application/xml"];
        [request appendPostDataFromFile:file];
        [request setDelegate:self];
        [request setValidatesSecureCertificate:NO];
        [request startAsynchronous];
    }
}

#pragma make - 多媒体IM

//发送IM
- (void) sendMediaMsgWithFileName:(NSString*) fileName andReceiver:(NSString*) receiver andChunked:(BOOL) chunked andUserdata:(NSString*)userdata andSendMsgId:(NSString*)msgid
{
    NSString *timestamp = [self getTimestamp];
    NSString *requestUrl = [NSString stringWithFormat:@"%@%@:%d/%@/SubAccounts/%@/IM/SendMsg?sig=%@",defHttps,self.restip,restport,kServerVersion,self.accountSid,[self getSig:timestamp]];
    
    [self postLoginfo:[NSString stringWithFormat:@"[CCPRestService] /IM/SendMsg url=%@",requestUrl]];
    
    CLOPHTTPRequest *request = [CLOPHTTPRequest requestWithURL:[NSURL URLWithString:requestUrl]];
    request.requestType = ERequestType_SendMediaMsg;
    NSString* strType = nil;
    if (chunked)
    {
        strType = @"1";
    }
    else
    {
        strType = @"0";
    }
    request.userInfo = [NSDictionary dictionaryWithObjectsAndKeys:fileName, KEY_USERINFO_MEDIAMSG_FILENAME,strType,KEY_USERINFO_MEDIAMSG_SENDTYPE,msgid,KEY_USERINFO_MEDIAMSG_MSGID,nil];
    [request setRequestMethod:@"POST"];
    [request addRequestHeader:@"Accept" value:@"application/xml"];
    [request addRequestHeader:@"Content-Type" value:@"application/xml;charset=utf-8"];
    [request addRequestHeader:@"Authorization" value:[self getAuthorization:timestamp]];
    NSString *data = userdata.length>0?userdata:@"";
    NSString *ext = [fileName pathExtension];
    NSString *xmlBody = [NSString stringWithFormat:@"<?xml version='1.0' encoding='UTF8'?><InstanceMessage><receiver>%@</receiver><fileSize>%llu</fileSize><type>%@</type><fileExt>%@</fileExt><sender>%@</sender><msgId>%@</msgId><userData>%@</userData></InstanceMessage>", receiver, [self fileSizeAtPath:fileName],strType,ext,self.account, msgid, data];
    [request appendPostData:[xmlBody dataUsingEncoding:NSUTF8StringEncoding]];
    [request setDelegate:self];
    [request setValidatesSecureCertificate:NO];
    [request startAsynchronous];
}

//确认已下载多媒体IM        
- (void) confirmInstanceMessageWithMsgId :(NSArray*) msgIds{
    NSString *timestamp = [self getTimestamp];
    NSString *requestUrl = [NSString stringWithFormat:@"%@%@:%d/%@/SubAccounts/%@/IM/Done?sig=%@",defHttps,self.restip,restport, kServerVersion,self.accountSid,[self getSig:timestamp]];
    [self postLoginfo:[NSString stringWithFormat:@"[CCPRestService] IM/Done url=%@",requestUrl]];
    CLOPHTTPRequest *request = [CLOPHTTPRequest requestWithURL:[NSURL URLWithString:requestUrl]];
    request.requestType = ERequestType_ConfirmDownloadMediaMessage;
    [request setRequestMethod:@"POST"];
    [request addRequestHeader:@"Accept" value:@"application/xml"];
    [request addRequestHeader:@"Content-Type" value:@"application/xml;charset=utf-8"];
    [request addRequestHeader:@"Authorization" value:[self getAuthorization:timestamp]];
    NSMutableString* strMsgIds = [[NSMutableString alloc] init];
    for (NSString * str in  msgIds) {
        [strMsgIds appendFormat:@"<msgId>%@</msgId>",str];
    }
    
    NSString* xmlBody = [NSString stringWithFormat:@"<?xml version='1.0' encoding='UTF8'?><InstanceMessage>%@</InstanceMessage>", strMsgIds];
    [strMsgIds release];
    [request appendPostData:[xmlBody dataUsingEncoding:NSUTF8StringEncoding]];
    [request setDelegate:self];
    [request setValidatesSecureCertificate:NO];
    [request startAsynchronous];
}

- (void) downloadAttachmentFiles:(NSArray*)urlList
{
    for (DownloadInfo *downInfo in urlList)
    {
        CLOPHTTPRequest *request = [[[CLOPHTTPRequest alloc] initWithURL:[NSURL URLWithString:downInfo.fileUrl]] autorelease];
        request.requestType = ERequestType_DownloadFile_Media;
        request.userInfo = [NSDictionary dictionaryWithObjectsAndKeys:downInfo, KEY_USERINFO_DOWNLOADFILEINFO, nil];
        [request setDownloadDestinationPath:downInfo.fileName];
        [request setDelegate:self];
        [request startAsynchronous];
    }
}

#pragma mark - 实时对讲相关函数
//启动对讲场景
//发言类型  0:都可以讲话 1：抢麦模式  默认为1
- (void) startInterphoneWithJoiner:(NSArray*)joinerArr inAppId:(NSString*)appid
{
    NSString *timestamp = [self getTimestamp];
    NSString *requestUrl = [NSString stringWithFormat:@"%@%@:%d/%@/SubAccounts/%@/interphone/create?sig=%@",defHttps,self.restip,restport,kServerVersion,self.accountSid,[self getSig:timestamp]];
    
    [self postLoginfo:[NSString stringWithFormat:@"[CCPRestService] startInterphone url=%@",requestUrl]];
    
    CLOPHTTPRequest *request = [CLOPHTTPRequest requestWithURL:[NSURL URLWithString:requestUrl]];
    request.requestType = ERequestType_StartInterphone;
    [request setRequestMethod:@"POST"];
    [request addRequestHeader:@"Accept" value:@"application/xml"];
    [request addRequestHeader:@"Content-Type" value:@"application/xml;charset=utf-8"];
    [request addRequestHeader:@"Authorization" value:[self getAuthorization:timestamp]];
    
    NSMutableString *members = [[NSMutableString alloc] init];
    for (NSString* member in joinerArr)
    {
        [members appendFormat:@"<member>%@</member>", member];
    }
    
    NSString *xmlBody = nil;
    if (members.length <= 0)
    {
        xmlBody = [NSString stringWithFormat:@"<?xml version='1.0' encoding='UTF8'?><Interphone><appId>%@</appId></Interphone>", appid];
    }
    else
    {
        xmlBody = [NSString stringWithFormat:@"<?xml version='1.0' encoding='UTF8'?><Interphone><appId>%@</appId><members>%@</members></Interphone>", appid, members];
    }
    [members release];
    
    [request appendPostData:[xmlBody dataUsingEncoding:NSUTF8StringEncoding]];
    [request setDelegate:self];
    [request setValidatesSecureCertificate:NO];
    [request startAsynchronous];
}

//发起对讲——抢麦
- (void) controlMicInConfNo:(NSString*)confNo
{
    NSString *timestamp = [self getTimestamp];
    NSString *requestUrl = [NSString stringWithFormat:@"%@%@:%d/%@/SubAccounts/%@/interphone/robMic?sig=%@",defHttps,self.restip,restport,kServerVersion,self.accountSid,[self getSig:timestamp]];
    
    [self postLoginfo:[NSString stringWithFormat:@"[CCPRestService] controlMic url=%@",requestUrl]];
    
    CLOPHTTPRequest *request = [CLOPHTTPRequest requestWithURL:[NSURL URLWithString:requestUrl]];
    request.requestType = ERequestType_ControlMic;
    [request setRequestMethod:@"POST"];
    [request addRequestHeader:@"Accept" value:@"application/xml"];
    [request addRequestHeader:@"Content-Type" value:@"application/xml;charset=utf-8"];
    [request addRequestHeader:@"Authorization" value:[self getAuthorization:timestamp]];
    
    
    NSString *xmlBody = [NSString stringWithFormat:@"<?xml version='1.0' encoding='UTF8'?><Interphone><interphoneId>%@</interphoneId></Interphone>",confNo];
    
    [request appendPostData:[xmlBody dataUsingEncoding:NSUTF8StringEncoding]];
    [request setDelegate:self];
    [request setValidatesSecureCertificate:NO];
    [request startAsynchronous];
}

//结束对讲——放麦 ???是否需要加入参数:对讲场景id
- (void) releaseMicInConfNo:(NSString*)confNo
{
    NSString *timestamp = [self getTimestamp];
    NSString *requestUrl = [NSString stringWithFormat:@"%@%@:%d/%@/SubAccounts/%@/interphone/releaseMic?sig=%@",defHttps,self.restip,restport,kServerVersion,self.accountSid,[self getSig:timestamp]];
    
    [self postLoginfo:[NSString stringWithFormat:@"[CCPRestService] controlMic url=%@",requestUrl]];
    
    CLOPHTTPRequest *request = [CLOPHTTPRequest requestWithURL:[NSURL URLWithString:requestUrl]];
    request.requestType = ERequestType_ReleaseMic;
    [request setRequestMethod:@"POST"];
    [request addRequestHeader:@"Accept" value:@"application/xml"];
    [request addRequestHeader:@"Content-Type" value:@"application/xml;charset=utf-8"];
    [request addRequestHeader:@"Authorization" value:[self getAuthorization:timestamp]];
    
    
    NSString *xmlBody = [NSString stringWithFormat:@"<?xml version='1.0' encoding='UTF8'?><Interphone><interphoneId>%@</interphoneId></Interphone>", confNo];
    
    
    [request appendPostData:[xmlBody dataUsingEncoding:NSUTF8StringEncoding]];
    [request setDelegate:self];
    [request setValidatesSecureCertificate:NO];
    [request startAsynchronous];
}

//查询参与对讲成员
- (void) getMemberListInConfNo:(NSString*)confNo
{
    NSString *timestamp = [self getTimestamp];
    NSString *requestUrl = [NSString stringWithFormat:@"%@%@:%d/%@/SubAccounts/%@/interphone/members?sig=%@",defHttps,self.restip,restport,kServerVersion,self.accountSid,[self getSig:timestamp]];
    
    [self postLoginfo:[NSString stringWithFormat:@"[CCPRestService] getMemberListInCon url=%@",requestUrl]];
    
    CLOPHTTPRequest *request = [CLOPHTTPRequest requestWithURL:[NSURL URLWithString:requestUrl]];
    request.requestType = ERequestType_GetInterphoneMember;
    [request setRequestMethod:@"POST"];
    [request addRequestHeader:@"Accept" value:@"application/xml"];
    [request addRequestHeader:@"Content-Type" value:@"application/xml;charset=utf-8"];
    [request addRequestHeader:@"Authorization" value:[self getAuthorization:timestamp]];
    
    
    NSString *xmlBody = [NSString stringWithFormat:@"<?xml version='1.0' encoding='UTF8'?><Interphone><interphoneId>%@</interphoneId></Interphone>", confNo];
    
    [self postLoginfo:[NSString stringWithFormat:@"[CCPRestService] getMemberListInConfNo url=%@ body=%@",requestUrl,xmlBody]];
    [request appendPostData:[xmlBody dataUsingEncoding:NSUTF8StringEncoding]];
    [request setDelegate:self];
    [request setValidatesSecureCertificate:NO];
    [request startAsynchronous];
}

#pragma mark - 群聊天室相关函数
- (void) startChatroomInAppId:(NSString *)appId withName:(NSString *)roomName andSquare:(NSInteger)square andKeywords:(NSString *)keywords andPassword:(NSString *)roomPwd andIsAutoClose:(BOOL)isAutoClose andVoiceMod:(NSInteger) voiceMod andAutoDelete:(BOOL) autoDelete andIsAutoJoin:(BOOL) isAutoJoin;
{
    NSString *timestamp = [self getTimestamp];
    NSString *requestUrl = [NSString stringWithFormat:@"%@%@:%d/%@/SubAccounts/%@/chatroom/create?sig=%@",defHttps,self.restip,restport,kServerVersion,self.accountSid,[self getSig:timestamp]];
    
    CLOPHTTPRequest *request = [CLOPHTTPRequest requestWithURL:[NSURL URLWithString:requestUrl]];
    request.requestType = ERequestType_StartChatroom;
    [request setRequestMethod:@"POST"];
    [request addRequestHeader:@"Accept" value:@"application/xml"];
    [request addRequestHeader:@"Content-Type" value:@"application/xml;charset=utf-8"];
    [request addRequestHeader:@"Authorization" value:[self getAuthorization:timestamp]];
   
    NSString *xmlBody = nil;
    NSString *strIsAutoClose = nil;
    if (isAutoClose)
    {
        strIsAutoClose = @"0";
    }
    else
        strIsAutoClose = @"1";
    
    NSString *strVoiceMod = @"";
    NSString *strAutodelete = @"";
    strVoiceMod = [NSString  stringWithFormat:@"<voiceMod>%d</voiceMod>",voiceMod];
    NSInteger iAuto = 0;
    if (autoDelete)
    {
        iAuto = 1;
    }
    strAutodelete = [NSString  stringWithFormat:@"<autoDelete>%d</autoDelete>",iAuto];

    if ((roomPwd == nil) || (roomPwd.length == 0))
    {
        if (keywords == nil || keywords.length <= 0)
        {
            xmlBody = [NSString stringWithFormat:@"<?xml version='1.0' encoding='UTF8'?><ChatRoom><appId>%@</appId><square>%d</square><roomName>%@</roomName><isAutoClose>%@</isAutoClose>%@%@</ChatRoom>", appId, square, roomName,strIsAutoClose,strVoiceMod,strAutodelete];
        }
        else
        {
            xmlBody = [NSString stringWithFormat:@"<?xml version='1.0' encoding='UTF8'?><ChatRoom><appId>%@</appId><square>%d</square><roomName>%@</roomName><keywords>%@</keywords><isAutoClose>%@</isAutoClose>%@%@</ChatRoom>", appId, square, roomName,keywords,strIsAutoClose,strVoiceMod,strAutodelete];
        }
    }
    else
    {
        if (keywords == nil || keywords.length <= 0)
        {
            xmlBody = [NSString stringWithFormat:@"<?xml version='1.0' encoding='UTF8'?><ChatRoom><appId>%@</appId><square>%d</square><roomName>%@</roomName><pwd>%@</pwd><isAutoClose>%@</isAutoClose>%@%@</ChatRoom>", appId, square, roomName, roomPwd,strIsAutoClose,strVoiceMod,strAutodelete];
        }
        else
        {
            xmlBody = [NSString stringWithFormat:@"<?xml version='1.0' encoding='UTF8'?><ChatRoom><appId>%@</appId><square>%d</square><roomName>%@</roomName><keywords>%@</keywords><pwd>%@</pwd><isAutoClose>%@</isAutoClose>%@%@</ChatRoom>", appId, square, roomName, keywords, roomPwd,strIsAutoClose,strVoiceMod,strAutodelete];
        }
    }
    
    
    
    [self postLoginfo:[NSString stringWithFormat:@"[CCPRestService] startChatroomInAppId url=%@ body=%@",requestUrl,xmlBody]];
    
    [request appendPostData:[xmlBody dataUsingEncoding:NSUTF8StringEncoding]];
    [request setDelegate:self];
    [request setValidatesSecureCertificate:NO];
    request.userInfo = [NSDictionary dictionaryWithObjectsAndKeys:[NSNumber numberWithBool:isAutoJoin], KEY_RESPONSE_USERDATA_CHATROOM_IS_AUTOJOIN,roomPwd, KEY_RESPONSE_USERDATA_CHATROOMPWD,nil];
    [request startAsynchronous];
}

//获取聊天室成员
- (void) queryMembersWithChatroom:(NSString *)roomNo
{
    NSString *timestamp = [self getTimestamp];
    NSString *requestUrl = [NSString stringWithFormat:@"%@%@:%d/%@/SubAccounts/%@/chatroom/members?sig=%@",defHttps,self.restip,restport,kServerVersion,self.accountSid,[self getSig:timestamp]];
    CLOPHTTPRequest *request = [CLOPHTTPRequest requestWithURL:[NSURL URLWithString:requestUrl]];
    request.requestType = ERequestType_GetChatRoomMember;
    [request setRequestMethod:@"POST"];
    [request addRequestHeader:@"Accept" value:@"application/xml"];
    [request addRequestHeader:@"Content-Type" value:@"application/xml;charset=utf-8"];
    [request addRequestHeader:@"Authorization" value:[self getAuthorization:timestamp]];
    NSString *xmlBody = [NSString stringWithFormat:@"<?xml version='1.0' encoding='UTF8'?><ChatRoom><roomId>%@</roomId></ChatRoom>", roomNo];
    [self postLoginfo:[NSString stringWithFormat:@"[CCPRestService] queryMembersWithChatroom url=%@ body=%@",requestUrl,xmlBody]];
    [request appendPostData:[xmlBody dataUsingEncoding:NSUTF8StringEncoding]];
    [request setDelegate:self];
    [request setValidatesSecureCertificate:NO];
    [request startAsynchronous];
}

//获取所有的房间列表
- (void)queryChatroomsOfAppId:(NSString *)appId withKeywords:(NSString *)keywords
{
    NSString *timestamp = [self getTimestamp];
    NSString *requestUrl = [NSString stringWithFormat:@"%@%@:%d/%@/SubAccounts/%@/chatroom/roomlist?sig=%@",defHttps,self.restip,restport,kServerVersion,self.accountSid,[self getSig:timestamp]];
    CLOPHTTPRequest *request = [CLOPHTTPRequest requestWithURL:[NSURL URLWithString:requestUrl]];
    request.requestType = ERequestType_GetChatroomsList;
    [request setRequestMethod:@"POST"];
    [request addRequestHeader:@"Accept" value:@"application/xml"];
    [request addRequestHeader:@"Content-Type" value:@"application/xml;charset=utf-8"];
    [request addRequestHeader:@"Authorization" value:[self getAuthorization:timestamp]];
    
    NSString *xmlBody = nil;
    
    if (keywords == nil || keywords.length <= 0)
    {
         xmlBody = [NSString stringWithFormat:@"<?xml version='1.0' encoding='UTF8'?><ChatRoom><appId>%@</appId></ChatRoom>", appId];
    }
    else
    {
        xmlBody = [NSString stringWithFormat:@"<?xml version='1.0' encoding='UTF8'?><ChatRoom><appId>%@</appId><keywords>%@</keywords></ChatRoom>", appId, keywords];
    }
    [self postLoginfo:[NSString stringWithFormat:@"[CCPRestService] queryChatroomsOfAppId url=%@ body=%@",requestUrl,xmlBody]];
    [request appendPostData:[xmlBody dataUsingEncoding:NSUTF8StringEncoding]];
    [request setDelegate:self];
    [request setValidatesSecureCertificate:NO];
    [request startAsynchronous];
}

//外呼邀请成员加入群聊
- (void)inviteMembers:(NSArray*)members joinChatroom:(NSString*)roomNo ofAppId:(NSString *)appid
{
    NSString *timestamp = [self getTimestamp];
    NSString *requestUrl = [NSString stringWithFormat:@"%@%@:%d/%@/SubAccounts/%@/chatroom/invite?sig=%@",defHttps,self.restip,restport,kServerVersion,self.accountSid,[self getSig:timestamp]];
    
    [self postLoginfo:[NSString stringWithFormat:@"[CCPRestService] inviteMembersjoinChatroom url=%@",requestUrl]];
    
    CLOPHTTPRequest *request = [CLOPHTTPRequest requestWithURL:[NSURL URLWithString:requestUrl]];
    request.requestType = ERequestType_InviteJoinChatroom;
    request.userInfo = [NSDictionary dictionaryWithObjectsAndKeys:roomNo, KEY_USERINFO_CHATROOMNO, nil];
    [request setRequestMethod:@"POST"];
    [request addRequestHeader:@"Accept" value:@"application/xml"];
    [request addRequestHeader:@"Content-Type" value:@"application/xml;charset=utf-8"];
    [request addRequestHeader:@"Authorization" value:[self getAuthorization:timestamp]];
    
    NSMutableString *memberstr = [[NSMutableString alloc] init];
    for (NSString* member in members)
    {
        [memberstr appendFormat:@"<mobile>%@</mobile>", member];
    }
    
    NSString *xmlBody = nil;
    if (memberstr.length <= 0)
    {
        xmlBody = [NSString stringWithFormat:@"<?xml version='1.0' encoding='UTF8'?><ChatRoom><appId>%@</appId><roomId>%@</roomId></ChatRoom>", appid, roomNo];
    }
    else
    {
        xmlBody = [NSString stringWithFormat:@"<?xml version='1.0' encoding='UTF8'?><ChatRoom><appId>%@</appId><roomId>%@</roomId><mobiles>%@</mobiles></ChatRoom>", appid, roomNo, memberstr];
    }
    [memberstr release];
    
    
    [request appendPostData:[xmlBody dataUsingEncoding:NSUTF8StringEncoding]];
    [request setDelegate:self];
    [request setValidatesSecureCertificate:NO];
    [request startAsynchronous];
}

- (void) dismissChatroom:(NSString*) roomNo ofAppId:(NSString*) appId
{
    NSString *timestamp = [self getTimestamp];
    NSString *requestUrl = [NSString stringWithFormat:@"%@%@:%d/%@/SubAccounts/%@/chatroom/dismiss?sig=%@",defHttps,self.restip,restport,kServerVersion,self.accountSid,[self getSig:timestamp]];
    
    [self postLoginfo:[NSString stringWithFormat:@"[CCPRestService] dismissChatroom url=%@",requestUrl]];
    
    CLOPHTTPRequest *request = [CLOPHTTPRequest requestWithURL:[NSURL URLWithString:requestUrl]];
    request.requestType = ERequestType_DismissChatroom;
    request.userInfo = [NSDictionary dictionaryWithObjectsAndKeys:roomNo, KEY_USERINFO_CHATROOMNO, nil];
    [request setRequestMethod:@"POST"];
    [request addRequestHeader:@"Accept" value:@"application/xml"];
    [request addRequestHeader:@"Content-Type" value:@"application/xml;charset=utf-8"];
    [request addRequestHeader:@"Authorization" value:[self getAuthorization:timestamp]];
    
    NSString *xmlBody =  [NSString stringWithFormat:@"<?xml version='1.0' encoding='UTF8'?><ChatRoom><appId>%@</appId><roomId>%@</roomId></ChatRoom>", appId, roomNo];
       
    [request appendPostData:[xmlBody dataUsingEncoding:NSUTF8StringEncoding]];
    [request setDelegate:self];
    [request setValidatesSecureCertificate:NO];
    [request startAsynchronous];
}

- (void) removeMember:(NSString*) member fromRoomNo:(NSString*) roomNo ofAppId:(NSString*) appId
{
    NSString *timestamp = [self getTimestamp];
    NSString *requestUrl = [NSString stringWithFormat:@"%@%@:%d/%@/SubAccounts/%@/chatroom/remove?sig=%@",defHttps,self.restip,restport,kServerVersion,self.accountSid,[self getSig:timestamp]];
    
    [self postLoginfo:[NSString stringWithFormat:@"[CCPRestService] reomoveMemberFromChatroom url=%@",requestUrl]];
    
    CLOPHTTPRequest *request = [CLOPHTTPRequest requestWithURL:[NSURL URLWithString:requestUrl]];
    request.requestType = ERequestType_RemoveMemberFromChatroom;
    request.userInfo = [NSDictionary dictionaryWithObjectsAndKeys:member, KEY_USERINFO_MEMBERDATA, nil];
    [request setRequestMethod:@"POST"];
    [request addRequestHeader:@"Accept" value:@"application/xml"];
    [request addRequestHeader:@"Content-Type" value:@"application/xml;charset=utf-8"];
    [request addRequestHeader:@"Authorization" value:[self getAuthorization:timestamp]];
    
    NSString *xmlBody =  [NSString stringWithFormat:@"<?xml version='1.0' encoding='UTF8'?><ChatRoom><appId>%@</appId><roomId>%@</roomId><mobile>%@</mobile></ChatRoom>", appId, roomNo, member];
    
    [request appendPostData:[xmlBody dataUsingEncoding:NSUTF8StringEncoding]];
    [request setDelegate:self];
    [request setValidatesSecureCertificate:NO];
    [request startAsynchronous];
}

#pragma mark - 视频会议 video conference

- (void) startVideoConferenceInAppId:(NSString*)appId withName:(NSString *)conferenceName andSquare:(NSInteger)square andKeywords:(NSString *)keywords andPassword:(NSString *)conferencePwd andIsAutoClose:(BOOL)isAutoClose andVoiceMod:(NSInteger) voiceMod andAutoDelete:(BOOL) autoDelete andIsAutoJoin:(BOOL) isAutoJoin
{
    NSString *timestamp = [self getTimestamp];
    NSString *requestUrl = [NSString stringWithFormat:@"%@%@:%d/%@/SubAccounts/%@/video/create?sig=%@",defHttps, restip, restport,kServerVersion,self.accountSid,[self getSig:timestamp]];
    [self postLoginfo:[NSString stringWithFormat:@"[CCPRestService] startVideoConference url=%@",requestUrl]];
    
    CLOPHTTPRequest *request = [CLOPHTTPRequest requestWithURL:[NSURL URLWithString:requestUrl]];
    request.requestType = ERequestType_StartVideoConference;
    [request setRequestMethod:@"POST"];
    [request addRequestHeader:@"Accept" value:@"application/xml"];
    [request addRequestHeader:@"Content-Type" value:@"application/xml;charset=utf-8"];
    [request addRequestHeader:@"Authorization" value:[self getAuthorization:timestamp]];
    
    NSString *strIsAutoClose = nil;
    if (isAutoClose)
    {
        strIsAutoClose = @"0";
    }
    else
        strIsAutoClose = @"1";
    
    NSString *strVoiceMod = @"";
    NSString *strAutodelete = @"";
    NSInteger iAuto = 0;
    if (autoDelete)
    {
        iAuto = 1;
    }
    strVoiceMod = [NSString  stringWithFormat:@"<voiceMod>%d</voiceMod>",voiceMod];
    strAutodelete = [NSString  stringWithFormat:@"<autoDelete>%d</autoDelete>",iAuto];
    
    NSString *xmlBody = nil;
    if ((conferencePwd == nil) || (conferencePwd.length == 0))
    {
        if (keywords == nil || keywords.length <= 0)
        {
            xmlBody = [NSString stringWithFormat:@"<?xml version='1.0' encoding='UTF8'?><VideoConf><appId>%@</appId><square>%d</square><roomName>%@</roomName><isAutoClose>%@</isAutoClose>%@%@</VideoConf>", appId, square, conferenceName,strIsAutoClose,strVoiceMod,strAutodelete];
        }
        else
        {
            xmlBody = [NSString stringWithFormat:@"<?xml version='1.0' encoding='UTF8'?><VideoConf><appId>%@</appId><square>%d</square><roomName>%@</roomName><keywords>%@</keywords><isAutoClose>%@</isAutoClose>%@%@</VideoConf>", appId, square, conferenceName, keywords,strIsAutoClose,strVoiceMod,strAutodelete];
        }
    }
    else
    {
        if (keywords == nil || keywords.length <= 0)
        {
            xmlBody = [NSString stringWithFormat:@"<?xml version='1.0' encoding='UTF8'?><VideoConf><appId>%@</appId><square>%d</square><roomName>%@</roomName><pwd>%@</pwd><isAutoClose>%@</isAutoClose>%@%@</VideoConf>", appId, square, conferenceName,  conferencePwd,strIsAutoClose,strVoiceMod,strAutodelete];
        }
        else
        {
            xmlBody = [NSString stringWithFormat:@"<?xml version='1.0' encoding='UTF8'?><VideoConf><appId>%@</appId><square>%d</square><roomName>%@</roomName><keywords>%@</keywords><pwd>%@</pwd><isAutoClose>%@</isAutoClose>%@%@</VideoConf>", appId, square, conferenceName, keywords, conferencePwd,strIsAutoClose,strVoiceMod,strAutodelete];
        }
    }
    request.userInfo = [NSDictionary dictionaryWithObjectsAndKeys:[NSNumber numberWithBool:isAutoJoin], KEY_RESPONSE_USERDATA_CHATROOM_IS_AUTOJOIN, nil];
    [request appendPostData:[xmlBody dataUsingEncoding:NSUTF8StringEncoding]];
    [request setDelegate:self];
    [request setValidatesSecureCertificate:NO];
    [request startAsynchronous];
}

- (void) queryMembersInVideoConference:(NSString *)conferenceId
{
    NSString *timestamp = [self getTimestamp];
    NSString *requestUrl = [NSString stringWithFormat:@"%@%@:%d/%@/SubAccounts/%@/video/members?sig=%@",defHttps, restip, restport,kServerVersion,self.accountSid,[self getSig:timestamp]];
    [self postLoginfo:[NSString stringWithFormat:@"[CCPRestService] queryMembersInVideoConference url=%@",requestUrl]];
    
    CLOPHTTPRequest *request = [CLOPHTTPRequest requestWithURL:[NSURL URLWithString:requestUrl]];
    request.requestType = ERequestType_GetVideoConferenceMember;
    [request setRequestMethod:@"POST"];
    [request addRequestHeader:@"Accept" value:@"application/xml"];
    [request addRequestHeader:@"Content-Type" value:@"application/xml;charset=utf-8"];
    [request addRequestHeader:@"Authorization" value:[self getAuthorization:timestamp]];
    
     NSString *xmlBody = [NSString stringWithFormat:@"<?xml version='1.0' encoding='UTF8'?><VideoConf><roomId>%@</roomId></VideoConf>", conferenceId];
    
    [request appendPostData:[xmlBody dataUsingEncoding:NSUTF8StringEncoding]];
    [request setDelegate:self];
    [request setValidatesSecureCertificate:NO];
    [request startAsynchronous];
}

- (void)queryVideoConferencesOfAppId:(NSString *)appId withKeywords:(NSString *)keywords
{
    NSString *timestamp = [self getTimestamp];
    NSString *requestUrl = [NSString stringWithFormat:@"%@%@:%d/%@/SubAccounts/%@/video/roomlist?sig=%@",defHttps,  restip, restport,kServerVersion,self.accountSid,[self getSig:timestamp]];
    [self postLoginfo:[NSString stringWithFormat:@"[CCPRestService] queryVideoConferencesOfAppId url=%@",requestUrl]];
    
    CLOPHTTPRequest *request = [CLOPHTTPRequest requestWithURL:[NSURL URLWithString:requestUrl]];
    request.requestType = ERequestType_GetVideoConferencesList;
    [request setRequestMethod:@"POST"];
    [request addRequestHeader:@"Accept" value:@"application/xml"];
    [request addRequestHeader:@"Content-Type" value:@"application/xml;charset=utf-8"];
    [request addRequestHeader:@"Authorization" value:[self getAuthorization:timestamp]];
    
    NSString *xmlBody = nil;
    if (keywords == nil || keywords.length <= 0)
    {
        xmlBody = [NSString stringWithFormat:@"<?xml version='1.0' encoding='UTF8'?><VideoConf><appId>%@</appId></VideoConf>", appId];
    }
    else
    {
        xmlBody = [NSString stringWithFormat:@"<?xml version='1.0' encoding='UTF8'?><VideoConf><appId>%@</appId><keywords>%@</keywords></VideoConf>", appId, keywords];
    }
    
    [request appendPostData:[xmlBody dataUsingEncoding:NSUTF8StringEncoding]];
    [request setDelegate:self];
    [request setValidatesSecureCertificate:NO];
    [request startAsynchronous];
}

- (void) dismissVideoConferenceWithAppId:(NSString*) appId andVideoConference:(NSString*) conferenceId
{
    NSString *timestamp = [self getTimestamp];
    NSString *requestUrl = [NSString stringWithFormat:@"%@%@:%d/%@/SubAccounts/%@/video/dismiss?sig=%@",defHttps,  restip, restport,kServerVersion,self.accountSid,[self getSig:timestamp]];
    [self postLoginfo:[NSString stringWithFormat:@"[CCPRestService] dismissVideoConference url=%@",requestUrl]];
    
    CLOPHTTPRequest *request = [CLOPHTTPRequest requestWithURL:[NSURL URLWithString:requestUrl]];
    request.requestType = ERequestType_DismissVideoConference;
    request.userInfo = [NSDictionary dictionaryWithObjectsAndKeys:conferenceId, KEY_USERINFO_VIDEOCONFERENCEID, nil];
    [request setRequestMethod:@"POST"];
    [request addRequestHeader:@"Accept" value:@"application/xml"];
    [request addRequestHeader:@"Content-Type" value:@"application/xml;charset=utf-8"];
    [request addRequestHeader:@"Authorization" value:[self getAuthorization:timestamp]];
    
    NSString *xmlBody =  [NSString stringWithFormat:@"<?xml version='1.0' encoding='UTF8'?><VideoConf><appId>%@</appId><roomId>%@</roomId></VideoConf>", appId, conferenceId];
    
    [request appendPostData:[xmlBody dataUsingEncoding:NSUTF8StringEncoding]];
    [request setDelegate:self];
    [request setValidatesSecureCertificate:NO];
    [request startAsynchronous];
}


- (void) checkVoipOfflineMsgWithTimestampStr:(NSString*) timestampStr
{
    NSString *timestamp = [self getTimestamp];
    NSString *requestUrl = [NSString stringWithFormat:@"%@%@:%d/%@/SubAccounts/%@/CheckVoipOfflineMsg?sig=%@",defHttps,  restip, restport,kServerVersion,self.accountSid,[self getSig:timestamp]];
    [self postLoginfo:[NSString stringWithFormat:@"[CCPRestService] CheckVoipOfflineMsg url=%@",requestUrl]];
    
    CLOPHTTPRequest *request = [CLOPHTTPRequest requestWithURL:[NSURL URLWithString:requestUrl]];
    request.requestType = ERequestType_CheckVoipOfflineMsg;
    [request setRequestMethod:@"POST"];
    [request addRequestHeader:@"Accept" value:@"application/xml"];
    [request addRequestHeader:@"Content-Type" value:@"application/xml;charset=utf-8"];
    [request addRequestHeader:@"Authorization" value:[self getAuthorization:timestamp]];
    NSString *strTimestamp = @"";
    if ([timestampStr length]>0) {
        strTimestamp = timestampStr;
    }
    NSString *xmlBody =  [NSString stringWithFormat:@"<?xml version='1.0' encoding='UTF8'?><Request><timestamp>%@</timestamp></Request>", strTimestamp];
    
    [request appendPostData:[xmlBody dataUsingEncoding:NSUTF8StringEncoding]];
    [request setDelegate:self];
    [request setValidatesSecureCertificate:NO];
    [request startAsynchronous];
}

- (void) getVoipOfflineMsgWithSender:(NSString*) sender andTimestampStr:(NSString*) timestampStr andLimit:(int)limit
{
    NSString *timestamp = [self getTimestamp];
    NSString *requestUrl = [NSString stringWithFormat:@"%@%@:%d/%@/SubAccounts/%@/GetVoipOfflineMsg?sig=%@",defHttps,  restip, restport,kServerVersion,self.accountSid,[self getSig:timestamp]];
    [self postLoginfo:[NSString stringWithFormat:@"[CCPRestService] GetVoipOfflineMsg url=%@",requestUrl]];
    
    CLOPHTTPRequest *request = [CLOPHTTPRequest requestWithURL:[NSURL URLWithString:requestUrl]];
    request.requestType = ERequestType_GetVoipOfflineMsg;
    [request setRequestMethod:@"POST"];
    [request addRequestHeader:@"Accept" value:@"application/xml"];
    [request addRequestHeader:@"Content-Type" value:@"application/xml;charset=utf-8"];
    [request addRequestHeader:@"Authorization" value:[self getAuthorization:timestamp]];
    NSString *xmlBody = nil;
    if (limit >0) {
        if (timestampStr ==nil) {
            xmlBody =  [NSString stringWithFormat:@"<?xml version='1.0' encoding='UTF8'?><Request><sender>%@</sender><timestamp>%@</timestamp><limit>%d</limit></Request>", sender, @"",limit];
        }
        else
            xmlBody =  [NSString stringWithFormat:@"<?xml version='1.0' encoding='UTF8'?><Request><sender>%@</sender><timestamp>%@</timestamp><limit>%d</limit></Request>", sender, timestampStr,limit];
    }
    else
    {
        xmlBody =  [NSString stringWithFormat:@"<?xml version='1.0' encoding='UTF8'?><Request><sender>%@</sender><timestamp>%@</timestamp></Request>", sender, timestampStr];
    }
    [request appendPostData:[xmlBody dataUsingEncoding:NSUTF8StringEncoding]];
    [request setDelegate:self];
    [request setValidatesSecureCertificate:NO];
    [request startAsynchronous];
}

- (void) confirmOfflineMsgWithMsgIds:(NSArray*) msgIds
{
    if (!msgIds) {
        return;
    }
    if ([msgIds count]<=0) {
        return;
    }
    NSString *timestamp = [self getTimestamp];
    NSString *requestUrl = [NSString stringWithFormat:@"%@%@:%d/%@/SubAccounts/%@/ConfirmOfflineMsg?sig=%@",defHttps,  restip, restport,kServerVersion,self.accountSid,[self getSig:timestamp]];
    [self postLoginfo:[NSString stringWithFormat:@"[CCPRestService] ConfirmOfflineMsg url=%@",requestUrl]];
    
    NSMutableString* strBuilder = [[NSMutableString alloc] init];
    for (ConfirmOfflineMsg* confirmOfflineMsg in msgIds)
    {
         [strBuilder appendFormat:@"<msgIds><msgId>%@</msgId><type>%@</type></msgIds>",confirmOfflineMsg.msgId,confirmOfflineMsg.type];
    }
    
    CLOPHTTPRequest *request = [CLOPHTTPRequest requestWithURL:[NSURL URLWithString:requestUrl]];
    request.requestType = ERequestType_ConfirmOfflineMsg;
    [request setRequestMethod:@"POST"];
    [request addRequestHeader:@"Accept" value:@"application/xml"];
    [request addRequestHeader:@"Content-Type" value:@"application/xml;charset=utf-8"];
    [request addRequestHeader:@"Authorization" value:[self getAuthorization:timestamp]];
    
    NSString *xmlBody =  [NSString stringWithFormat:@"<?xml version='1.0' encoding='UTF8'?><Request>%@</Request>", strBuilder];
    
    [request appendPostData:[xmlBody dataUsingEncoding:NSUTF8StringEncoding]];
    [request setDelegate:self];
    [request setValidatesSecureCertificate:NO];
    [request startAsynchronous];
    [strBuilder release];
}


- (void) removeMemberFromVideoConferenceWithAppId:(NSString*) appId andVideoConference:(NSString*)conferenceId andMember:(NSString*) member
{
    NSString *timestamp = [self getTimestamp];
    NSString *requestUrl = [NSString stringWithFormat:@"%@%@:%d/%@/SubAccounts/%@/video/remove?sig=%@",defHttps,  restip, restport,kServerVersion,self.accountSid,[self getSig:timestamp]];
    [self postLoginfo:[NSString stringWithFormat:@"[CCPRestService] removeMemberFromVideoConference url=%@",requestUrl]];
    
    CLOPHTTPRequest *request = [CLOPHTTPRequest requestWithURL:[NSURL URLWithString:requestUrl]];
    request.requestType = ERequestType_RemoveMemberFromVideoConference;
    request.userInfo = [NSDictionary dictionaryWithObjectsAndKeys:member, KEY_USERINFO_MEMBERDATA, nil];
    
    [request setRequestMethod:@"POST"];
    [request addRequestHeader:@"Accept" value:@"application/xml"];
    [request addRequestHeader:@"Content-Type" value:@"application/xml;charset=utf-8"];
    [request addRequestHeader:@"Authorization" value:[self getAuthorization:timestamp]];
    
    NSString *xmlBody =  [NSString stringWithFormat:@"<?xml version='1.0' encoding='UTF8'?><VideoConf><appId>%@</appId><roomId>%@</roomId><voip>%@</voip></VideoConf>", appId, conferenceId, member];
    
    [request appendPostData:[xmlBody dataUsingEncoding:NSUTF8StringEncoding]];
    [request setDelegate:self];
    [request setValidatesSecureCertificate:NO];
    [request startAsynchronous];
}

- (void) switchRealScreenToVoip:(NSString*)voip ofVideoConference:(NSString*)conferenceId inAppId:(NSString*)appId
{
    NSString *timestamp = [self getTimestamp];
    NSString *requestUrl = [NSString stringWithFormat:@"%@%@:%d/%@/SubAccounts/%@/video/changed?sig=%@",defHttps,  restip, restport,kServerVersion,self.accountSid,[self getSig:timestamp]];
    [self postLoginfo:[NSString stringWithFormat:@"[CCPRestService] startVideoConference url=%@",requestUrl]];
    
    CLOPHTTPRequest *request = [CLOPHTTPRequest requestWithURL:[NSURL URLWithString:requestUrl]];
    request.requestType = ERequestType_SwitchVideoRealScreen;
    [request setRequestMethod:@"POST"];
    [request addRequestHeader:@"Accept" value:@"application/xml"];
    [request addRequestHeader:@"Content-Type" value:@"application/xml;charset=utf-8"];
    [request addRequestHeader:@"Authorization" value:[self getAuthorization:timestamp]];
    
    NSString *xmlBody =  [NSString stringWithFormat:@"<?xml version='1.0' encoding='UTF8'?><VideoConf><appId>%@</appId><roomId>%@</roomId><toVoip>%@</toVoip></VideoConf>", appId, conferenceId, voip];
    
    [request appendPostData:[xmlBody dataUsingEncoding:NSUTF8StringEncoding]];
    [request setDelegate:self];
    [request setValidatesSecureCertificate:NO];
    [request startAsynchronous];
}

- (void) sendLocalPortrait:(NSString*)filename toVideoConference:(NSString*)conferenceId
{
    NSString *timestamp = [self getTimestamp];
    NSString *requestUrl = [NSString stringWithFormat:@"%@%@:%d/%@/SubAccounts/%@/video/upload?sig=%@", defHttps, restip, restport, kServerVersion, self.accountSid, [self getSig:timestamp]];
    [self postLoginfo:[NSString stringWithFormat:@"[CCPRestService] sendLocalPortrait url=%@",requestUrl]];
    
    CLOPHTTPRequest *request = [CLOPHTTPRequest requestWithURL:[NSURL URLWithString:requestUrl]];
    request.requestType = ERequestType_SendPortraitToVideoConference;
    request.userInfo = [NSDictionary dictionaryWithObjectsAndKeys:filename, KEY_USERINFO_MEDIAMSG_FILENAME,nil];
    [request setRequestMethod:@"POST"];
    [request addRequestHeader:@"Accept" value:@"application/xml"];
    [request addRequestHeader:@"Content-Type" value:@"application/xml;charset=utf-8"];
    [request addRequestHeader:@"Authorization" value:[self getAuthorization:timestamp]];
    
    NSString *ext = [filename pathExtension];
    NSString *xmlBody = [NSString stringWithFormat:@"<?xml version='1.0' encoding='UTF8'?><VideoConf><from>%@</from><fileSize>%llu</fileSize><roomId>%@</roomId><fileExt>%@</fileExt></VideoConf>", self.account, [self fileSizeAtPath:filename], conferenceId, ext];
    
    [request appendPostData:[xmlBody dataUsingEncoding:NSUTF8StringEncoding]];
    [request setDelegate:self];
    [request setValidatesSecureCertificate:NO];
    [request startAsynchronous];
}

- (void) getPortraitsFromVideoConference:(NSString*)conferenceId
{
    NSString *timestamp = [self getTimestamp];
    NSString *requestUrl = [NSString stringWithFormat:@"%@%@:%d/%@/SubAccounts/%@/video/download?sig=%@",defHttps,  restip, restport, kServerVersion, self.accountSid, [self getSig:timestamp]];
    [self postLoginfo:[NSString stringWithFormat:@"[CCPRestService] getPortraitsFromVideoConference url=%@",requestUrl]];
    
    CLOPHTTPRequest *request = [CLOPHTTPRequest requestWithURL:[NSURL URLWithString:requestUrl]];
    request.requestType = ERequestType_GetPortraitFromVideoConference;
    [request setRequestMethod:@"POST"];
    [request addRequestHeader:@"Accept" value:@"application/xml"];
    [request addRequestHeader:@"Content-Type" value:@"application/xml;charset=utf-8"];
    [request addRequestHeader:@"Authorization" value:[self getAuthorization:timestamp]];
    
    NSString *xmlBody = [NSString stringWithFormat:@"<?xml version='1.0' encoding='UTF8'?><VideoConf><from>%@</from><roomId>%@</roomId></VideoConf>", self.account, conferenceId];
    
    [request appendPostData:[xmlBody dataUsingEncoding:NSUTF8StringEncoding]];
    [request setDelegate:self];
    [request setValidatesSecureCertificate:NO];
    [request startAsynchronous];
}

- (void)downloadVideoConferencePortrait:(id)portrait
{
    if ([portrait isKindOfClass:[VideoPartnerPortrait class]])
    {
        VideoPartnerPortrait *data = (VideoPartnerPortrait*)portrait;
        if ([data.fileName length] > 0 && [data.fileUrl length] > 0)
        {
            CLOPHTTPRequest *request = [[[CLOPHTTPRequest alloc] initWithURL:[NSURL URLWithString:data.fileUrl]] autorelease];
            request.requestType = ERequestType_DownloadPortraitOfVideoConference;
            request.userInfo = [NSDictionary dictionaryWithObjectsAndKeys:portrait, KEY_USERINFO_DOWNLOADFILEINFO, nil];
            [request setDownloadDestinationPath:data.fileName];
            [request setDelegate:self];
            [request startAsynchronous];
        }
    }
}

#pragma mark - multiVideo method
- (void) startMultiVideoInAppId:(NSString*)appId withName:(NSString *)conferenceName andSquare:(NSInteger)square andKeywords:(NSString *)keywords andPassword:(NSString *)conferencePwd andIsAutoClose:(BOOL)isAutoClose andIsPresenter:(BOOL)isPresenter andVoiceMod:(NSInteger)voiceMod andAutoDelete:(BOOL)autoDelete andIsAutoJoin:(BOOL) isAutoJoin
{
    NSString *timestamp = [self getTimestamp];
    NSString *requestUrl = [NSString stringWithFormat:@"%@%@:%d/%@/SubAccounts/%@/video/create?sig=%@",defHttps, restip, restport,kServerVersion,self.accountSid,[self getSig:timestamp]];
    [self postLoginfo:[NSString stringWithFormat:@"[CCPRestService] startMultiVideo url=%@",requestUrl]];
    
    CLOPHTTPRequest *request = [CLOPHTTPRequest requestWithURL:[NSURL URLWithString:requestUrl]];
    request.requestType = ERequestType_StartMultiVideo;
    [request setRequestMethod:@"POST"];
    [request addRequestHeader:@"Accept" value:@"application/xml"];
    [request addRequestHeader:@"Content-Type" value:@"application/xml;charset=utf-8"];
    [request addRequestHeader:@"Authorization" value:[self getAuthorization:timestamp]];
    
    NSString *strIsAutoClose = nil;
    if (isAutoClose)
    {
        strIsAutoClose = @"<isAutoClose>0</isAutoClose>";
    }
    else
    {
        strIsAutoClose = @"<isAutoClose>1</isAutoClose>";
    }
    
    NSString *strIsPresenter = @"";
    if (isPresenter)
    {
        strIsPresenter = @"<isPresenter>1</isPresenter>";
    }
    
    NSString *strPwd = @"";
    if ((conferencePwd == nil) || (conferencePwd.length == 0))
    {
        strPwd = @"";
    }
    else
    {
        strPwd = [NSString stringWithFormat:@"<pwd>%@</pwd>", conferencePwd];
    }
    
    NSString *strKeyword = @"";
    if (keywords == nil || keywords.length <= 0)
    {
        strKeyword = @"";
    }
    else
    {
        strKeyword = [NSString stringWithFormat:@"keywords>%@</keywords>", keywords];
    }
    
    NSString *strVoiceMod = @"";
    NSString *strAutodelete = @"";
    NSInteger iAuto = 0;
    if (autoDelete)
    {
        iAuto = 1;
    }
    strVoiceMod = [NSString  stringWithFormat:@"<voiceMod>%d</voiceMod>",voiceMod];
    strAutodelete = [NSString  stringWithFormat:@"<autoDelete>%d</autoDelete>",iAuto];
    
    
    NSString *xmlBody = [NSString stringWithFormat:@"<?xml version='1.0' encoding='UTF8'?><VideoConf><appId>%@</appId><square>%d</square><roomName>%@</roomName><isMultiVideo>1</isMultiVideo>%@%@%@%@%@%@</VideoConf>", appId, square, conferenceName, strKeyword,strPwd,strIsAutoClose,strIsPresenter,strVoiceMod,strAutodelete];
    
    request.userInfo = [NSDictionary dictionaryWithObjectsAndKeys:[NSNumber numberWithBool:isAutoJoin], KEY_RESPONSE_USERDATA_CHATROOM_IS_AUTOJOIN, nil];
    [request appendPostData:[xmlBody dataUsingEncoding:NSUTF8StringEncoding]];
    [request setDelegate:self];
    [request setValidatesSecureCertificate:NO];
    [request startAsynchronous];
}

//发布视频
-(void)publishVideoInMultiVideoConference:(NSString*)conferenceId ofAppId:(NSString*)appId
{
    NSString *timestamp = [self getTimestamp];
    NSString *requestUrl = [NSString stringWithFormat:@"%@%@:%d/%@/SubAccounts/%@/video/publish?sig=%@",defHttps, restip, restport,kServerVersion,self.accountSid,[self getSig:timestamp]];
    [self postLoginfo:[NSString stringWithFormat:@"[CCPRestService] publishVideoInMultiVideoConference url=%@",requestUrl]];
    
    CLOPHTTPRequest *request = [CLOPHTTPRequest requestWithURL:[NSURL URLWithString:requestUrl]];
    request.requestType = ERequestType_PublishVideo;
    [request setRequestMethod:@"POST"];
    [request addRequestHeader:@"Accept" value:@"application/xml"];
    [request addRequestHeader:@"Content-Type" value:@"application/xml;charset=utf-8"];
    [request addRequestHeader:@"Authorization" value:[self getAuthorization:timestamp]];
    
    NSString *xmlBody = [NSString stringWithFormat:@"<?xml version='1.0' encoding='UTF8'?><VideoConf><appId>%@</appId><roomId>%@</roomId></VideoConf>", appId, conferenceId];

    [request appendPostData:[xmlBody dataUsingEncoding:NSUTF8StringEncoding]];
    [request setDelegate:self];
    [request setValidatesSecureCertificate:NO];
    [request startAsynchronous];
}

//取消发布视频
-(void)unpublishVideoInMultiVideoConference:(NSString*)conferenceId ofAppId:(NSString*)appId
{
    NSString *timestamp = [self getTimestamp];
    NSString *requestUrl = [NSString stringWithFormat:@"%@%@:%d/%@/SubAccounts/%@/video/unpublish?sig=%@",defHttps, restip, restport,kServerVersion,self.accountSid,[self getSig:timestamp]];
    [self postLoginfo:[NSString stringWithFormat:@"[CCPRestService] unpublishVideoInMultiVideoConference url=%@",requestUrl]];
    
    CLOPHTTPRequest *request = [CLOPHTTPRequest requestWithURL:[NSURL URLWithString:requestUrl]];
    request.requestType = ERequestType_UnpublishVideo;
    [request setRequestMethod:@"POST"];
    [request addRequestHeader:@"Accept" value:@"application/xml"];
    [request addRequestHeader:@"Content-Type" value:@"application/xml;charset=utf-8"];
    [request addRequestHeader:@"Authorization" value:[self getAuthorization:timestamp]];
    
    NSString *xmlBody = [NSString stringWithFormat:@"<?xml version='1.0' encoding='UTF8'?><VideoConf><appId>%@</appId><roomId>%@</roomId></VideoConf>", appId, conferenceId];
    
    [request appendPostData:[xmlBody dataUsingEncoding:NSUTF8StringEncoding]];
    [request setDelegate:self];
    [request setValidatesSecureCertificate:NO];
    [request startAsynchronous];
}

#pragma mark - CLOPHTTPRequestDelegate
- (void)requestFinished:(CLOPHTTPRequest *)request
{
    NSData *responseData = [request responseData];
    NSString *responseString = [[NSString alloc] initWithData:(NSData *)responseData  encoding:NSUTF8StringEncoding];
    [self postLoginfo:[NSString stringWithFormat:@"[CCPRestService]requestType=%d   requestFinished body=%@",request.requestType,responseString]];
    [responseString release];

    NSMutableDictionary *userData = [[NSMutableDictionary alloc] init];
    
    if (request.requestType == ERequestType_DownloadFile_Media
        || request.requestType == ERequestType_DownloadPortraitOfVideoConference)
    {
        if (request.requestType == ERequestType_DownloadFile_Media)
        {
            [userData setValuesForKeysWithDictionary:request.userInfo];
            int retCode = 0;
            if (request.responseStatusCode == 404)
            {
                retCode = EIMFileDoesNotExist;
                [userData setValue:@"文件不存在" forKey:KEY_RESPONSE_STATUSMSG];
            }
            [userData setValue:[NSString stringWithFormat:@"%d",retCode]forKey:KEY_RESPONSE_STATUSCODE];
            [self postEventType:request.requestType withData:userData];
            [userData release];
            return;
        }
        
        [userData setValuesForKeysWithDictionary:request.userInfo];
        [self postEventType:request.requestType withData:userData];
        [userData release];
        return;
    }
    
    if (request.responseStatusCode != 200)
    {
        [userData setValue:request.responseStatusMessage forKey:KEY_RESPONSE_STATUSMSG];
        [userData setValue:[NSString stringWithFormat:@"%d",request.responseStatusCode]forKey:KEY_RESPONSE_STATUSCODE];
        [self postEventType:request.requestType withData:userData];
        [userData release];
        return;
    }
    
    GDataXMLDocument *xmldoc = [[GDataXMLDocument alloc] initWithData:responseData options:0 error:nil];
    if (!xmldoc)
    {
        [xmldoc release];
        if (request.userInfo != nil)
        {
            [userData setValuesForKeysWithDictionary:request.userInfo];
        }
        
        [userData setValue:[NSString stringWithFormat:@"%d", ELocalReason_ErrorXml] forKey:KEY_RESPONSE_STATUSCODE];
        [userData setValue:@"解析XML错误" forKey:KEY_RESPONSE_STATUSMSG];
        [self postEventType:request.requestType withData:userData];
        [userData release];
        return;
    }
    
    GDataXMLElement *rootElement = [xmldoc rootElement];
    
    [self parseStatuscodeElement:rootElement withData:userData];
    
    switch (request.requestType)
    {
        case ERequestType_GetSipAddress:
        {
            NSInteger tag = [[tagDictionary objectForKey:TAG_SIP_KEY] integerValue];
            if (tag != request.tag)
            {
                [userData release];
                [xmldoc release];
                return;
            }
            [self parseSipAddress:rootElement withData:userData];
        }
            break;
            
        case ERequestType_GetNetGroupId:
        {
            [self parseNetGroupId:rootElement withData:userData];
        }
            break;
            
        case ERequestType_GetCallback:
        {
            NSInteger tag = [[tagDictionary objectForKey:TAG_CALLBACK_KEY] integerValue];
            if (tag != request.tag)
            {
                [userData release];
                [xmldoc release];
                return;
            }
            [userData setValuesForKeysWithDictionary:request.userInfo];
        }
            break;
            
        //只需要解析statuscode 并且有数据传输
        case ERequestType_DownloadFile_Media:
        case ERequestType_DownloadPortraitOfVideoConference:
        case ERequestType_UploadMediaMsg:
        case ERequestType_UploadPortraitMsg:
        case ERequestType_DismissChatroom:
        case ERequestType_DismissVideoConference:
        case ERequestType_RemoveMemberFromChatroom:
        case ERequestType_RemoveMemberFromVideoConference:
        case ERequestType_ConfirmOfflineMsg:
        {
            [userData setValuesForKeysWithDictionary:request.userInfo];
        }
            break;

        case ERequestType_CheckVoipOfflineMsg:
        {
            [self parseCheckVoipOfflineMsg:rootElement withData:userData];
        }
            break;
        case ERequestType_GetVoipOfflineMsg:
        {
            [self parseGetVoipOfflineMsg:rootElement withData:userData];
        }
            break;
        case ERequestType_StartInterphone:
        {
            [self parseStartInterphone:rootElement withData:userData];
        }
            break;
        case ERequestType_ControlMic:
        {
            [self parseControlMic:rootElement withData:userData];
        }
            break;
        case ERequestType_ReleaseMic:
        {
            [self parseReleaseMic:rootElement withData:userData];
        }
            break;
        case ERequestType_GetInterphoneMember:
        {
            [self parseGetInterphoneMemberlist:rootElement withData:userData];
        }
            break;
            
        case ERequestType_StartChatroom:
        {
            NSString *roomPwd = [request.userInfo objectForKey:KEY_RESPONSE_USERDATA_CHATROOMPWD];
            NSNumber *number =  [request.userInfo objectForKey:KEY_RESPONSE_USERDATA_CHATROOM_IS_AUTOJOIN];
            [userData setValue:number forKey:KEY_RESPONSE_USERDATA_CHATROOM_IS_AUTOJOIN];
            [userData setValue:roomPwd forKey:KEY_RESPONSE_USERDATA_CHATROOMPWD];

            [self parseStartChatroom:rootElement withData:userData];
        }
            break;
        case ERequestType_GetChatroomsList:
        {
            [self parseQueryChatRoomList:rootElement withData:userData];
        }
            break;
        case ERequestType_InviteJoinChatroom:
        {
            [userData setValuesForKeysWithDictionary:request.userInfo];
        }
            break;
        case ERequestType_GetChatRoomMember:
        {
            [self parseQueryChatRoomMembers:rootElement withData:userData];
        }
            break;
            
        case ERequestType_SendMediaMsg:
        {
            NSString *filename = [request.userInfo objectForKey:KEY_USERINFO_MEDIAMSG_FILENAME];
            NSString *strChunkedFlag = [request.userInfo objectForKey:KEY_USERINFO_MEDIAMSG_SENDTYPE];
            NSString *msgid = [request.userInfo objectForKey: KEY_USERINFO_MEDIAMSG_MSGID];
            if (filename.length>0)
            {
                if ([self parseSendMediaMsg:rootElement withFile:filename withData:userData withChunkedFlag:strChunkedFlag andMsgid:msgid])
                {
                    [userData release];
                    [xmldoc release];
                    return;
                }
            }
        }
            break;
            
        case ERequestType_SendPortraitToVideoConference:
        {
            NSString *filename = [request.userInfo objectForKey:KEY_USERINFO_MEDIAMSG_FILENAME];
            if (filename.length>0)
            {
                if ([self parseUploadPortraitToVideoConference:rootElement withData:userData fileName:filename])
                {
                    [userData release];
                    [xmldoc release];
                    return;
                }
            }
        }
            break;
            
        case ERequestType_GetPortraitFromVideoConference:
        {
            [self parseDownloadPortraitFromVideoConference:rootElement withData:userData];
        }
            break;
            
            /*
        case ERequestType_QueryNewMediaMessages:
        {
            [self parseQueryNewMediaMessages:rootElement withData:userData];
        }
            break;
             */
            
        case ERequestType_StartVideoConference:
        case ERequestType_StartMultiVideo:
        {
            NSNumber *number =  [request.userInfo objectForKey:KEY_RESPONSE_USERDATA_CHATROOM_IS_AUTOJOIN];
            [userData setValue:number forKey:KEY_RESPONSE_USERDATA_CHATROOM_IS_AUTOJOIN];
            [self parseStartVideoConference:rootElement withData:userData];
        }
            break;
        case ERequestType_GetVideoConferencesList:
        {
            [self parseQueryVideoConferenceList:rootElement withData:userData];
        }
            break;
        case ERequestType_GetVideoConferenceMember:
        {
            [self parseQueryVideoConferenceMembers:rootElement withData:userData];
        }
            break;

        default:
            break;
    }
    
    [self postEventType:request.requestType withData:userData];
    [xmldoc release];
    [userData release];
}

- (void)requestFailed:(CLOPHTTPRequest *)request
{
    switch (request.requestType)
    {
        case ERequestType_GetSipAddress:
        {
            NSInteger tag = [[tagDictionary objectForKey:TAG_SIP_KEY] integerValue];
            if (tag != request.tag)
            {
                return;
            }
        }
            break;
        case ERequestType_GetCallback:
        {
            NSInteger tag = [[tagDictionary objectForKey:TAG_CALLBACK_KEY] integerValue];
            if (tag != request.tag)
            {
                return;
            }
        }
            break;
        default:
            break;
    }
    
    NSError *error = [request error];
    
    NSString *error_info = [error.userInfo objectForKey:NSLocalizedDescriptionKey];    
    [self postLoginfo:[NSString stringWithFormat:@"[CCPRestService] requestFailed requestType=%d error=%d info=%@", request.requestType, error.code, error_info]];
    
    NSInteger errReason = ELocalReason_ErrorNet;
    if (error.code == 2)
    {
        errReason = ELocalReason_ErrorTimeOut;
    }
    NSMutableDictionary* userdata = [NSMutableDictionary dictionaryWithObjectsAndKeys:[NSString stringWithFormat:@"%d", errReason],KEY_RESPONSE_STATUSCODE, nil];
    if (errReason == ELocalReason_ErrorTimeOut)
        [userdata setValue:@"网络访问超时" forKey:KEY_RESPONSE_STATUSMSG];
    else
        [userdata setValue:@"网络错误" forKey:KEY_RESPONSE_STATUSMSG];
    
    if (request.requestType == ERequestType_GetNetGroupId)
    {
        [userdata setValue:@"100100" forKey:KEY_RESPONSE_USERDATA];
    }
    else if (request.requestType == ERequestType_SendMediaMsg)
    {
        NSString *msgid = [request.userInfo objectForKey: KEY_USERINFO_MEDIAMSG_MSGID];
        NSString *file = [request.userInfo objectForKey: KEY_USERINFO_MEDIAMSG_FILENAME];        
        NSString *strChunkedFlag = [request.userInfo objectForKey:KEY_USERINFO_MEDIAMSG_SENDTYPE];        
        IMAttachedMsg *msg = [[IMAttachedMsg alloc] init];
        msg.msgId = msgid;
        if ([strChunkedFlag isEqualToString:@"1"]) {
            msg.chunked = YES;
        }
        msg.fileUrl = file;
        msg.ext = [file pathExtension];
        [userdata setValue:msg forKey:KEY_USERINFO_SEND_MEDIAMSG_DATA];
        [msg release];
    }
    else if(request.requestType == ERequestType_SendPortraitToVideoConference)
    {
        NSString *filename = [request.userInfo objectForKey:KEY_USERINFO_MEDIAMSG_FILENAME];
        [userdata setValue:filename forKey:KEY_USERINFO_MEDIAMSG_FILENAME];
    }
    else if (error.code == 4 &&  request.requestType == ERequestType_DownloadIMChunkedFile)
    {
        [userdata setValue:[NSString stringWithFormat:@"%d",EIMFileDoesNotExist]forKey:KEY_RESPONSE_STATUSCODE];
        [userdata setValue:@"文件不存在" forKey:KEY_RESPONSE_STATUSMSG];
    }
    else if (error.code == 4 &&  request.requestType == ERequestType_DownloadFile_Media)
    {
        [userdata setValue:[NSString stringWithFormat:@"%d",EIMFileDoesNotExist]forKey:KEY_RESPONSE_STATUSCODE];
        [userdata setValue:@"文件不存在" forKey:KEY_RESPONSE_STATUSMSG];
    }
    [self postEventType:request.requestType withData:userdata];
}
//获取sig编码
- (NSString *)getMainSig:(NSString *)timestamp andMainAccount:(NSString*) mainAccount andMainToken:(NSString*)mainToken
{
    NSString *sigString = [NSString stringWithFormat:@"%@%@%@", mainAccount, mainToken, timestamp];
    const char *cStr = [sigString UTF8String];
	unsigned char result[16];
	CC_MD5(cStr, (CC_LONG)strlen(cStr), result);
	return [NSString stringWithFormat:@"%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X",result[0], result[1], result[2], result[3], result[4], result[5], result[6], result[7],result[8], result[9], result[10], result[11],result[12], result[13], result[14], result[15]];
}
//根据sid和当前时间字符串获取一个Authorization编码
- (NSString *)getMainAuthorization:(NSString *)timestamp andMainAccount:(NSString*)mainAccount
{
    NSString *authorizationString = [NSString stringWithFormat:@"%@:%@",mainAccount,timestamp];
    return [CLOPHTTPRequest base64forData:[authorizationString dataUsingEncoding:NSUTF8StringEncoding]];
}

#pragma mark - 发起绑定号码请求
- (void) editTestNumWithOldPhoneNumber:(NSString*) oldPhoneNumber andNewPhoneNumber:(NSString*) newPhoneNumber andServerIP:(NSString*) server_IP andServerPort:(NSInteger) server_Port  andServerVersion :(NSString*) serverVersion andMainAccount:(NSString*) mainAccount andMainToken:(NSString*) mainToken
{
    NSString* str = @"";
    if ([oldPhoneNumber length] > 0) {
        str = oldPhoneNumber;
    }
    NSString *timestamp = [self getTimestamp];
    NSString *requestUrl = [NSString stringWithFormat:@"%@%@:%d/%@/Accounts/%@/TestNumEdit?sig=%@",defHttps, server_IP,server_Port,serverVersion,mainAccount,[self getMainSig:timestamp andMainAccount:mainAccount andMainToken:mainToken]];
    [self postLoginfo:[NSString stringWithFormat:@"[CCPRestService] editTestNumWithOldPhoneNumber url= %@",requestUrl]];
    CLOPHTTPRequest *request = [CLOPHTTPRequest requestWithURL:[NSURL URLWithString:requestUrl]];
    request.requestType = ERequestType_EditTestNum;
    [request setRequestMethod:@"POST"];
    [request addRequestHeader:@"Accept" value:@"application/xml"];
    [request addRequestHeader:@"Content-Type" value:@"application/xml;charset=utf-8"];
    [request addRequestHeader:@"Authorization" value:[self getMainAuthorization:timestamp andMainAccount:mainAccount]];
    NSString *xmlBody = [NSString stringWithFormat:@"<?xml version='1.0' encoding='UTF8'?><Request><newNum>%@</newNum><oldNum>%@</oldNum></Request>",newPhoneNumber,str];

    [self postLoginfo:[NSString stringWithFormat:@"[CCPRestService] editTestNumWithOldPhoneNumberxmlBody= %@",xmlBody]];
    [request appendPostData:[xmlBody dataUsingEncoding:NSUTF8StringEncoding]];
    [request setDelegate:self];
    [request setValidatesSecureCertificate:NO];
    [request startAsynchronous];
}
-(NSString*)getCcpRestServerVersion
{
    if ([self.ServerVerSion length] > 0)
    {
        return self.ServerVerSion;
    }
    else if ([self.PrivateCloudFlag length] > 0 && [self.ValidateStr length] > 0)
    {
        return @"2013-03-22";
    }
    else
        return @"2013-12-26";
}
@end
