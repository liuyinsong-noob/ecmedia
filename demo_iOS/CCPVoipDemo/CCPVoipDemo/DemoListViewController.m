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

#import "DemoListViewController.h"
#import "VoIPCallViewController.h"
#import "InterphoneViewController.h"
#import "UIselectContactsViewController.h"
#import "RoomListViewController.h"
#import "IMListViewController.h"
#import "SettingViewController.h"
#import "LandingCallsViewController.h"
#import "VoiceCodeViewController.h"
#import "counselorViewController.h"
#import "ContactsViewController.h"
#import "MultiVideoConfIntroduction.h"
#import "MultiVideoConfListViewController.h"
#import "CameraViewController.h"

#define TAG_DEMO_GOTO_VOIP          100
#define TAG_DEMO_GOTO_INTERCOME     101
#define TAG_DEMO_GOTO_VOICE_VERIFY  102
#define TAG_DEMO_GOTO_CHAT_ROOM     103

#define TAG_DEMO_GOTO_VIDEO         104
#define TAG_DEMO_GOTO_MultiVideoConf 105

#define TAG_DEMO_GOTO_MARKET_CALL   106
#define TAG_DEMO_GOTO_counselorView 107

#define TAG_DEMO_GOTO_XX_Message    108
#define TAG_DEMO_GOTO_Contacts      109
#define TAG_DEMO_GOTO_LIVE_STREAM   110
#define TAG_DEMO_GOTO_SETTING       111



@interface DemoListViewController ()

@end

@implementation DemoListViewController

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
{
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self) {
        // Custom initialization
    }
    return self;
}

- (void)loadView
{
    self.title = @"演示列表";
    self.view = [[[UIView alloc] initWithFrame:[UIScreen mainScreen].applicationFrame] autorelease];
    self.view.backgroundColor = [UIColor colorWithRed:34.0f/255.0f green:34.0f/255.0f blue:34.0f/255.0f alpha:1.0f];
    int value = 0;
    if ([[[UIDevice currentDevice] systemVersion] floatValue] >= 7) {
        value = 20;
    }
    UIBarButtonItem *back = [[UIBarButtonItem alloc] init];
    back.title = @"返回";
    self.navigationItem.backBarButtonItem = back;
    [back release];
    
//    UIImageView *imageView = [[UIImageView alloc] initWithImage:[UIImage imageNamed:@"logoDemo.png"]];
//    imageView.frame = CGRectMake(11, 21+value, 133, 49);
//    [self.view addSubview:imageView];
//    [imageView release];
    
    UILabel* lb2 = [[UILabel alloc] initWithFrame:CGRectMake(210 , 53+value, 100, 25)];
    lb2.textAlignment = NSTextAlignmentRight;
    lb2.backgroundColor = [UIColor clearColor];
    lb2.font = [UIFont boldSystemFontOfSize:20];
    lb2.textColor = [UIColor whiteColor];
    lb2.text = @"能力演示";
    [self.view addSubview:lb2];
    [lb2 release];
    
    NSString* fileStr = Nil;
    CGRect range;
    if (IPHONE5)
    {
        fileStr = @"videoConfBg_1136.png";
        range = CGRectMake(0, 92+value, 320, 576);
    }
    else
    {
        fileStr = @"videoConfBg.png";
        range = CGRectMake(0, 92+value, 320, 480);
    }
    
    UIImage* imBg = [UIImage imageNamed:fileStr];
    UIImageView* ivBg = [[UIImageView alloc] initWithImage:imBg];
    ivBg.frame = range;
    [self.view addSubview:ivBg];
    [ivBg release];
    
    CGRect frame = CGRectMake(0, 92+value, 320, [UIScreen mainScreen].applicationFrame.size.height - 92+value);

    UIScrollView* scrollView = [[UIScrollView alloc] initWithFrame:frame];
    [self.view addSubview:scrollView];
    [scrollView setContentSize:CGSizeMake(320, 700)];
    scrollView.backgroundColor = [UIColor clearColor];
    [scrollView release];
    
    UIImageView* iv1 = [[UIImageView alloc] initWithFrame:CGRectMake(11, 11, 21, 21)];
    [scrollView addSubview:iv1];
    iv1.image = [UIImage imageNamed:@"videoConf32.png"];
    [iv1 release];
    
    UILabel* lb1 = [[UILabel alloc] initWithFrame:CGRectMake(44, 15, 265, 16)];
    [scrollView addSubview:lb1];
    lb1.textColor = [UIColor colorWithRed:204./255 green:204./255 blue:204./255 alpha:1];
    lb1.font = [UIFont systemFontOfSize:13];
    lb1.backgroundColor =  [UIColor clearColor];
    lb1.text = @"请点击各个功能按钮，体验各项功能演示。";
    [lb1 release];
   
    
    for (int i=0;i<12;i++)
    {
        int x = 114;
        int y = 43;
        if (i % 2 == 0)
        {
            x = 114;
        }
        else
        {
            x = 217;
        }
        int j = i / 2;
        y = y+j*103;
        [self createButtonWithRect:CGRectMake(x, y, 92, 92) andTag:TAG_DEMO_GOTO_VOIP+i andBGView:scrollView];
        
        if (i == 0 )
        {
            [self createBlackViewWithRect:CGRectMake(x-103, y, 92, 92) andText:@"Voice" andBGView:scrollView];
        }
        else if (i == 4)
        {
            [self createBlackViewWithRect:CGRectMake(x-103, y, 92, 92) andText:@"Video" andBGView:scrollView];
        }
        else if (i == 6)
        {
            [self createBlackViewWithRect:CGRectMake(x-103, y, 92, 92) andText:@"IVR" andBGView:scrollView];
        }
        else if (i == 8)
        {
            [self createBlackViewWithRect:CGRectMake(x-103, y, 92, 92) andText:@"MORE" andBGView:scrollView];
        }
        else if( i == 10)
        {
            [self createBlackViewWithRect:CGRectMake(x-103, y, 92, 92) andText:@"NEW" andBGView:scrollView];
        }
    }
    if (self.modelEngineVoip.addressBookContactList == nil)
    {
        self.modelEngineVoip.addressBookContactList = [[AddressBookContactList alloc] init];
    }
    
    if ([[NSUserDefaults standardUserDefaults] objectForKey:@"loss"]) {
        int loss = [[[NSUserDefaults standardUserDefaults] objectForKey:@"loss"] intValue];
        [self.modelEngineVoip setLoss:loss];
    }
    else
    {
        [self.modelEngineVoip setLoss:0];
    }
    
    if ([[[NSUserDefaults standardUserDefaults] objectForKey:@"FecSet"] isEqualToString:@"on"]) {
        [self.modelEngineVoip setOpusFec:TRUE];
    }
    else
    {
        [self.modelEngineVoip setOpusFec:FALSE];
    }
}

- (void)createBlackViewWithRect:(CGRect) frame andText:(NSString*) text andBGView:(UIView*) bgView
{
    UIView* view = [[UIView alloc] initWithFrame:frame];
    view.backgroundColor = [UIColor clearColor];
    [bgView addSubview:view];
    
    UIView* transparentView = [[UIView alloc] initWithFrame:CGRectMake(0, 0, frame.size.width, frame.size.height)];
    transparentView.backgroundColor = [UIColor blackColor];
    transparentView.alpha = 0.3;
    [view addSubview: transparentView];
    [transparentView release];
    
    UILabel* lb2 = [[UILabel alloc] initWithFrame:CGRectMake(6 , 68, 62, 15)];
    lb2.backgroundColor = [UIColor clearColor];
    lb2.font = [UIFont systemFontOfSize:16];
    lb2.textAlignment = NSTextAlignmentRight;
    lb2.textColor = [UIColor whiteColor];
    lb2.text = text;
    [view addSubview:lb2];
    [lb2 release];
    
    UIImage* image = [UIImage imageNamed:@"new57.png"];
    UIImageView* iv = [[UIImageView alloc] initWithFrame:CGRectMake(75, 72, 4, 8)];
    iv.image = image;
    [view addSubview:iv];
    [iv release];
    
    [view release];
}

- (void)createButtonWithRect:(CGRect) frame andTag:(NSInteger) tag andBGView:(UIView*) bgView
{
    UIButton* button= [UIButton buttonWithType:(UIButtonTypeCustom)];
    button.frame = frame;
    button.tag = tag;
    [button addTarget:self action:@selector(goToDemo:)forControlEvents:UIControlEventTouchUpInside];
    [bgView addSubview:button];
    if (button.tag == TAG_DEMO_GOTO_VOIP)
    {
        //进入VoIP演示
        [button setImage:[UIImage imageNamed:@"new201.png"] forState:(UIControlStateNormal)];
        [button setImage:[UIImage imageNamed:@"new201_on.png"] forState:(UIControlStateSelected)];
    }
    else if (button.tag == TAG_DEMO_GOTO_VIDEO)
    {
        //进入视频演示
        [button setImage:[UIImage imageNamed:@"new190.png"] forState:(UIControlStateNormal)];
        [button setImage:[UIImage imageNamed:@"new190_on.png"] forState:(UIControlStateSelected)];
    }
    else if (button.tag == TAG_DEMO_GOTO_INTERCOME)
    {
        //进入实时对讲演示
        [button setImage:[UIImage imageNamed:@"new188.png"] forState:(UIControlStateNormal)];
        [button setImage:[UIImage imageNamed:@"new188_on.png"] forState:(UIControlStateSelected)];
    }
    else if (button.tag == TAG_DEMO_GOTO_CHAT_ROOM)
    {
        //进入多人聊天室演示
        [button setImage:[UIImage imageNamed:@"new200.png"] forState:(UIControlStateNormal)];
        [button setImage:[UIImage imageNamed:@"new200_on.png"] forState:(UIControlStateSelected)];
    }
    else if (button.tag == TAG_DEMO_GOTO_XX_Message)
    {
        //进入IM演示
        [button setImage:[UIImage imageNamed:@"new186.png"] forState:(UIControlStateNormal)];
        [button setImage:[UIImage imageNamed:@"new186_on.png"] forState:(UIControlStateSelected)];
    }
    else if (button.tag == TAG_DEMO_GOTO_SETTING)
    {
        //进入设置页面
        [button setImage:[UIImage imageNamed:@"new184.png"] forState:(UIControlStateNormal)];
        [button setImage:[UIImage imageNamed:@"new184_on.png"] forState:(UIControlStateSelected)];
    }
    else if (button.tag == TAG_DEMO_GOTO_MARKET_CALL)
    {
        //进入外呼通知演示
        [button setImage:[UIImage imageNamed:@"new197.png"] forState:(UIControlStateNormal)];
        [button setImage:[UIImage imageNamed:@"new197_on.png"] forState:(UIControlStateSelected)];
    }
    else if (button.tag == TAG_DEMO_GOTO_VOICE_VERIFY)
    {
        //进入语音验证演示
        [button setImage:[UIImage imageNamed:@"new182.png"] forState:(UIControlStateNormal)];
        [button setImage:[UIImage imageNamed:@"new182_on.png"] forState:(UIControlStateSelected)];
    }
    else if (button.tag == TAG_DEMO_GOTO_counselorView)
    {
        //咨询呼叫
        [button setImage:[UIImage imageNamed:@"new199.png"] forState:(UIControlStateNormal)];
        [button setImage:[UIImage imageNamed:@"new199_on.png"] forState:(UIControlStateSelected)];
    }
    else if (button.tag == TAG_DEMO_GOTO_Contacts)
    {
        //通讯录
        [button setImage:[UIImage imageNamed:@"contact_management.png"] forState:(UIControlStateNormal)];
        [button setImage:[UIImage imageNamed:@"contact_management_on.png"] forState:(UIControlStateSelected)];
    }
    else if (button.tag == TAG_DEMO_GOTO_MultiVideoConf)
    {
        //多路视频会议
        [button setImage:[UIImage imageNamed:@"new202.png"] forState:(UIControlStateNormal)];
        [button setImage:[UIImage imageNamed:@"new202_on.png"] forState:(UIControlStateSelected)];
    }
    else if (button.tag == TAG_DEMO_GOTO_LIVE_STREAM)
    {
        //多路视频会议
        [button setImage:[UIImage imageNamed:@"new202.png"] forState:(UIControlStateNormal)];
        [button setImage:[UIImage imageNamed:@"new202_on.png"] forState:(UIControlStateSelected)];
    }
    
}

- (void)viewDidLoad
{
    [super viewDidLoad];
	// Do any additional setup after loading the view.
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

- (void)viewWillAppear:(BOOL)animated
{
    [super viewWillAppear:animated];
    [self.navigationController setNavigationBarHidden:YES animated:YES];
    [self.modelEngineVoip setModalEngineDelegate:self];
}

- (void)viewWillDisappear:(BOOL)animated
{
    [self.navigationController setNavigationBarHidden:NO animated:YES];
    [super viewWillDisappear:animated];
}

- (void)goToDemo:(id)sender
{
    UIButton *button = (UIButton*)sender;
    
    if (button.tag == TAG_DEMO_GOTO_VOIP)
    {
        //进入VoIP演示
        VoIPCallViewController *voipDemo = [[VoIPCallViewController alloc] init];
        [self.navigationController pushViewController:voipDemo animated:YES];
        [voipDemo release];
    }
    else if (button.tag == TAG_DEMO_GOTO_VIDEO)
    {
        //进入视频演示
        UIselectContactsViewController* view = [[UIselectContactsViewController alloc] initWithAccountList:self.modelEngineVoip.accountArray andSelectType:ESelectViewType_Video];
        view.backView = self;
        [self.navigationController pushViewController:view animated:YES];
        [view release];
    }
    else if (button.tag == TAG_DEMO_GOTO_INTERCOME)
    {
        //进入实时对讲演示
        InterphoneViewController *interphoneDemo = [[InterphoneViewController alloc] init];
        [self.navigationController pushViewController:interphoneDemo animated:YES];
        [interphoneDemo release];
    }
    else if (button.tag == TAG_DEMO_GOTO_CHAT_ROOM)
    {
        //进入多人聊天室演示
        RoomListViewController* view = [[RoomListViewController alloc] init];
        [self.navigationController pushViewController:view animated:YES];
        [view release];
    }
    else if (button.tag == TAG_DEMO_GOTO_XX_Message)
    {
        //进入IM演示
        IMListViewController* view = [[IMListViewController alloc] init];
        [self.navigationController pushViewController:view animated:YES];
        [view release];
    }
    else if (button.tag == TAG_DEMO_GOTO_SETTING)
    {
        //进入设置页面
        SettingViewController *view = [[SettingViewController alloc] init];
        [self.navigationController pushViewController:view animated:YES];
        [view release];
    }
    else if (button.tag == TAG_DEMO_GOTO_MARKET_CALL)
    {
        //进入外呼通知演示
        LandingCallsViewController *view = [[LandingCallsViewController alloc] init];
        [self.navigationController pushViewController:view animated:YES];
        [view release];
    }
    else if (button.tag == TAG_DEMO_GOTO_VOICE_VERIFY)
    {        
        //进入语音验证演示
        VoiceCodeViewController* view = [[VoiceCodeViewController alloc] init];
        [self.navigationController pushViewController:view animated:YES];
        [view release];
    }
     else if (button.tag == TAG_DEMO_GOTO_counselorView)
    {
        counselorViewController* view = [[counselorViewController alloc] init];
        [self.navigationController setNavigationBarHidden:NO animated:NO];
        [self.navigationController pushViewController:view animated:YES];
        [view release];
    }
    else if (button.tag == TAG_DEMO_GOTO_Contacts)
    {
        if ([self isContactsAccessGranted])
        {
            ContactsViewController* view = [[ContactsViewController alloc] init];
            [self.navigationController pushViewController:view animated:YES];
            [view release];
        }
    }
    else if (button.tag == TAG_DEMO_GOTO_MultiVideoConf)
    {
        MultiVideoConfListViewController* view = [[MultiVideoConfListViewController alloc] init];
        view.backView = self;
        [self.navigationController pushViewController:view animated:YES];
        [view release];
    }
    else if( button.tag == TAG_DEMO_GOTO_LIVE_STREAM )
    {
        CameraViewController *view = [[ CameraViewController alloc] init];
        [self.navigationController pushViewController:view animated:YES];
        [view release];
    }
}


@end
