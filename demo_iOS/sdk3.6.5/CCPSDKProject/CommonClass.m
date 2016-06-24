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

#ifndef CCPSDKProject_CommonClass_m
#define CCPSDKProject_CommonClass_m
#import "CommonClass.h"

@implementation AudioConfig
@synthesize audioMode;
@synthesize audioType;
@synthesize enable;
@end

@implementation StatisticsInfo

@end
//摄像头的信息类
@implementation CameraCapabilityInfo
@synthesize width;
@synthesize height;
@synthesize maxfps;
@end

//摄像头信息类
@implementation CameraDeviceInfo
@synthesize index;
@synthesize name;
@synthesize capabilityArray;
- (void)dealloc
{
    self.name = nil;
    self.capabilityArray = nil;
    [super dealloc];
}
@end

//下载结构文件参数结构定义
@implementation DownloadInfo;
@synthesize fileUrl;
@synthesize fileName;
@synthesize isChunked;
-(void)dealloc
{
    self.fileUrl = nil;
    self.fileName = nil;
    [super dealloc];
}
@end

//通话网络流量统计
@implementation NetworkStatistic
@synthesize duration;//媒体交互的持续时间，单位秒，可能为0；
@synthesize txBytes_sim; //在duration时间内，网络发送的总流量，单位字节；
@synthesize rxBytes_sim; //在duration时间内，网络接收的总流量，单位字节；
@synthesize txBytes_wifi; //在duration时间内，网络发送的总流量，单位字节；
@synthesize rxBytes_wifi; //在duration时间内，网络接收的总流量，单位字节；
-(void)dealloc
{
    [super dealloc];
}
@end



@implementation CloopenReason
@synthesize reason;
@synthesize msg;
-(void)dealloc
{
    self.msg = nil;
    [super dealloc];
}
@end
#pragma mark - 实时语音消息的类

//实时对讲机消息基类定义
@implementation InterphoneMsg : NSObject
@synthesize  interphoneId;
- (void)dealloc
{
    self.interphoneId = nil;
    [super dealloc];
}
@end

//实时对讲解散消息类定义
@implementation InterphoneOverMsg
@end

//邀请加入实时对讲消息类定义
@implementation InterphoneInviteMsg
@synthesize  dateCreated;
@synthesize  fromVoip;
- (void)dealloc
{
    self.dateCreated = nil;
    self.fromVoip = nil;
    [super dealloc];
}
@end

//有人加入实时对讲消息类定义
@implementation InterphoneJoinMsg
@synthesize  joinArr;
- (void)dealloc
{
    self.joinArr = nil;
    [super dealloc];
}
@end

//有人退出实时对讲消息类定义
@implementation InterphoneExitMsg
@synthesize  exitArr;
- (void)dealloc
{
    self.exitArr = nil;
    [super dealloc];
}
@end

//实时对讲控麦消息类定义
@implementation InterphoneControlMicMsg
@synthesize  voip;
- (void)dealloc
{
    self.voip = nil;
    [super dealloc];
}
@end

//实时对讲放麦消息类定义
@implementation InterphoneReleaseMicMsg
@synthesize  voip;
- (void)dealloc
{
    self.voip = nil;
    [super dealloc];
}
@end

//获取到的对讲成员信息
@implementation InterphoneMember : NSObject
@synthesize  type;
@synthesize  online;
@synthesize  voipId;
@synthesize  mic;
- (void)dealloc
{
    self.type = nil;
    self.online = nil;
    self.voipId = nil;
    self.mic = nil;
    [super dealloc];
}
@end

#pragma mark - 会议消息的类

//会议消息基类定义
@implementation ChatroomMsg : NSObject
@synthesize  roomNo;
- (void)dealloc
{
    self.roomNo = nil;
    [super dealloc];
}
@end

//有人加入会议消息类定义
@implementation ChatroomJoinMsg
@synthesize  joinArr;
- (void)dealloc
{
    self.joinArr = nil;
    [super dealloc];
}
@end

//有人退出会议消息类定义
@implementation ChatroomExitMsg
@synthesize  exitArr;
- (void)dealloc
{
    self.exitArr = nil;
    [super dealloc];
}
@end

//踢出会议成员消息类定义
@implementation ChatroomRemoveMemberMsg
@synthesize who;
- (void)dealloc
{
    self.who = nil;
    [super dealloc];
}

@end

//解散会议消息类定义
@implementation ChatroomDismissMsg
-(void)dealloc
{
    [super dealloc];
}
@end

//获取到的会议成员信息
@implementation ChatroomMember : NSObject
@synthesize  type;
@synthesize  number;
- (void)dealloc
{
    self.type = nil;
    self.number = nil;
    [super dealloc];
}
@end

//聊天室信息类
@implementation Chatroom
@synthesize roomNo;
@synthesize roomName;
@synthesize creator;
@synthesize square;
@synthesize keywords;
@synthesize joinNum;
@synthesize validate;
-(void)dealloc
{
    self.roomNo = nil;
    self.roomName = nil;
    self.creator = nil;
    self.keywords = nil;
    [super dealloc];
}
@end


#pragma mark - 实时消息相关类
@implementation InstanceMsg

@end

@implementation IMTextMsg
@synthesize msgId;
@synthesize dateCreated;
@synthesize sender;
@synthesize receiver;
@synthesize message;
@synthesize status; //-1:发送失败 0:发送成功 1:对方已收到
@synthesize userData;
- (id)init
{
    if (self = [super init])
    {
        self.status = @"0";
    }
    return self;
}

- (void)dealloc
{
    self.msgId = nil;
    self.dateCreated = nil;
    self.sender = nil;
    self.receiver = nil;
    self.message = nil;
    self.status = nil;
    self.userData = nil;
    [super dealloc];
}
@end

@implementation CheckVoipOfflineMsg
@synthesize sender;
@synthesize count;
- (id)init
{
    if (self = [super init])
    {
        self.sender = @"";
    }
    return self;
}

- (void)dealloc
{
    self.sender = nil;
    [super dealloc];
}
@end

@implementation OfflineMsg
@synthesize timestamp;
@synthesize remainCount;
@synthesize msgs;
- (id)init
{
    if (self = [super init])
    {
        self.timestamp = @"";
        self.msgs = nil;
        self.remainCount = 0;
    }
    return self;
}

- (void)dealloc
{
    self.timestamp = nil;
    self.msgs = nil;
    [super dealloc];
}
@end

@implementation ConfirmOfflineMsg
@synthesize msgId;
@synthesize type;
- (id)init
{
    if (self = [super init])
    {
        self.msgId = @"";
        self.type = 0;
    }
    return self;
}

- (void)dealloc
{
    self.msgId = nil;
    [super dealloc];
}
@end

@implementation IMAttachedMsg
@synthesize msgId;
@synthesize dateCreated;
@synthesize sender;
@synthesize receiver;
@synthesize fileSize;
@synthesize fileUrl;
@synthesize ext;
@synthesize chunked;
@synthesize userData;

- (void)dealloc
{
    self.msgId = nil;
    self.dateCreated = nil;
    self.sender = nil;
    self.receiver = nil;
    self.fileUrl = nil;
    self.ext = nil;
    self.userData = nil;
    [super dealloc];
}
@end

@implementation IMDismissGroupMsg
@synthesize groupId;
- (void)dealloc
{
    self.groupId = nil;
    [super dealloc];
}
@end

@implementation IMInviterMsg
@synthesize groupId;
@synthesize admin;
@synthesize confirm;
@synthesize declared;
- (void)dealloc
{
    self.groupId = nil;
    self.admin = nil;
    self.confirm = nil;
    self.declared = nil;
    [super dealloc];
}
@end

@implementation IMProposerMsg
@synthesize groupId;
@synthesize proposer;
@synthesize declared;
@synthesize dateCreated;
- (void) dealloc
{
    self.groupId = nil;
    self.proposer = nil;
    self.declared = nil;
    self.dateCreated = nil;
    [super dealloc];
}
@end

//有成员加入
@implementation IMJoinGroupMsg
@synthesize groupId;
@synthesize member;
@synthesize declared;
@synthesize dateCreated;
- (void) dealloc
{
    self.groupId = nil;
    self.member = nil;
    self.declared = nil;
    self.dateCreated = nil;
    [super dealloc];
}
@end

@implementation IMQuitGroupMsg
@synthesize groupId;
@synthesize member;
- (void) dealloc
{
    self.groupId = nil;
    self.member = nil;
    [super dealloc];
}
@end

@implementation IMRemoveMemberMsg
@synthesize groupId;
@synthesize member;
- (void)dealloc
{
    self.groupId = nil;
    self.member = nil;
    [super dealloc];
}
@end

@implementation IMReplyJoinGroupMsg
@synthesize groupId;
@synthesize admin;
@synthesize confirm;
@synthesize member;
- (void)dealloc
{
    self.groupId = nil;
    self.admin = nil;
    self.confirm = nil;
    self.member = nil;
    [super dealloc];
}
@end

@implementation IMReplyInviteGroupMsg
@synthesize groupId;
@synthesize admin;
@synthesize confirm;
@synthesize member;
- (void)dealloc
{
    self.groupId = nil;
    self.admin = nil;
    self.confirm = nil;
    self.member = nil;
    [super dealloc];
}
@end

@implementation IMCooperMsg
@synthesize message;
@synthesize type;
- (void)dealloc
{
    self.message = nil;
    [super dealloc];
}

@end


#pragma mark - 会议消息的类

//会议消息基类定义
@implementation VideoConferenceMsg : NSObject
@synthesize  conferenceId;
- (void)dealloc
{
    self.conferenceId = nil;
    [super dealloc];
}
@end

//有人加入会议消息类定义
@implementation VideoConferenceJoinMsg
@synthesize  joinArr;
@synthesize videoState;
@synthesize videoSrc;
- (id)init
{
    if (self = [super init])
    {
        self.videoState = 1;
    }
    return self;
}
- (void)dealloc
{
    self.joinArr = nil;
    self.videoSrc = nil;
    [super dealloc];
}
@end

//有人退出会议消息类定义
@implementation VideoConferenceExitMsg
@synthesize  exitArr;
- (void)dealloc
{
    self.exitArr = nil;
    [super dealloc];
}
@end

//踢出会议成员消息类定义
@implementation VideoConferenceRemoveMemberMsg
@synthesize who;
- (void)dealloc
{
    self.who = nil;
    [super dealloc];
}

@end

//解散会议消息类定义
@implementation VideoConferenceDismissMsg
-(void)dealloc
{
    [super dealloc];
}
@end

@implementation VideoConferenceSwitchScreenMsg

- (void)dealloc
{
    self.displayVoip = nil;
    [super dealloc];
}

@end

@implementation VideoConferencePublishVideoMsg

-(id)init
{
    if (self = [super init])
    {
        self.videoState = 1;
    }
    return self;
}

-(void)dealloc
{
    self.who = nil;
    self.videoSrc = nil;
    [super dealloc];
}

@end

@implementation VideoConferenceUnpublishVideoMsg

-(id)init
{
    if (self = [super init]) {
        self.videoState = 1;
    }
    return self;
}

-(void)dealloc
{
    self.who = nil;
    [super dealloc];
}
@end
//获取到的会议成员信息
@implementation VideoConferenceMember : NSObject
@synthesize  type;
@synthesize  number;
@synthesize  screenType;
@synthesize videoSource;
@synthesize videoState;
- (void)dealloc
{
    self.type = nil;
    self.number = nil;
    self.videoSource = nil;
    [super dealloc];
}
@end

//聊天室信息类
@implementation VideoConference
@synthesize conferenceId;
@synthesize conferenceName;
@synthesize creator;
@synthesize square;
@synthesize keywords;
@synthesize joinNum;
@synthesize validate;
@synthesize isMultiVideo;
@synthesize confAttrs;
-(void)dealloc
{
    self.conferenceId = nil;
    self.conferenceName = nil;
    self.creator = nil;
    self.keywords = nil;
    self.confAttrs = nil;
    [super dealloc];
}
@end

@implementation VideoPartnerPortrait

@synthesize dateUpdate;
@synthesize voip;
@synthesize fileName;
@synthesize fileUrl;
- (void)dealloc
{
    self.dateUpdate = nil;
    self.voip = nil;
    self.fileUrl = nil;
    self.fileName = nil;
    [super dealloc];
}

@end
#endif
