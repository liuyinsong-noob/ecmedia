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

#import "ExperienceSelectAccountViewController.h"
#import "AccountInfoViewController.h"
#import "AccountInfo.h"
#import "DemoListViewController.h"
#define BUTTON_SUNACCOUNT_BASE_TAG 100
@interface ExperienceSelectAccountViewController ()
{
    UIButton *logoBtn;
    CGFloat account_frame_begin_y;
    NSArray *accountArr;
    UIScrollView *btn_scrollView;
}
@end

@implementation ExperienceSelectAccountViewController

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
{
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self) {
        // Custom initialization
        curIndex = -1;
    }
    return self;
}

- (void)loadView
{
    self.title = @"客户体验登录";
    self.view = [[[UIView alloc] initWithFrame:[UIScreen mainScreen].applicationFrame] autorelease];
    
    NSString* fileStr = Nil;
    CGRect range;
    if (IPHONE5)
    {
        fileStr = @"videoConfBg_1136.png";
        range = CGRectMake(0, 0, 320, 576);
    }
    else
    {
        fileStr = @"videoConfBg.png";
        range = CGRectMake(0, 0, 320, 480);
    }
    
    UIImage* imBg = [UIImage imageNamed:fileStr];
    UIImageView* ivBg = [[UIImageView alloc] initWithImage:imBg];
    ivBg.frame = range;
    [self.view addSubview:ivBg];
    [ivBg release];
    
    
    UIBarButtonItem *leftBarItem = [[UIBarButtonItem alloc] initWithCustomView:[CommonTools navigationBackItemBtnInitWithTarget:self action:@selector(popToPreView)]];
    self.navigationItem.leftBarButtonItem = leftBarItem;
    [leftBarItem release];
    
    UIBarButtonItem *right = [[UIBarButtonItem alloc] initWithCustomView:[CommonTools navigationItemBtnInitWithNormalImageNamed:@"new07.png" andHighlightedImageNamed:@"new07_on.png" target:self action:@selector(gotoAccountInfoView:)]];
    self.navigationItem.rightBarButtonItem = right;
    [right release];
    
    UIImageView *pointImg = [[UIImageView alloc] initWithImage:[UIImage imageNamed:@"video_new_tips.png"]];
    pointImg.frame = CGRectMake(0.0f, 0.0f, 320.0f, 22.0f);
    [self.view addSubview:pointImg];
    [pointImg release];
    
    UILabel *statusLabel = [[UILabel alloc] initWithFrame:CGRectMake(11.0f, 0.0f, 320.0f, 22.0f)];
    statusLabel.backgroundColor = [UIColor clearColor];
    statusLabel.textColor = [UIColor whiteColor];
    statusLabel.font = [UIFont systemFontOfSize:13.0f];
    statusLabel.text = @"已成功登录，请选择一个子账户开始体验";
    statusLabel.contentMode = UIViewContentModeCenter;
    [self.view addSubview:statusLabel];
    [statusLabel release];
    
    UIImageView *iIcon = [[UIImageView alloc] initWithImage:[UIImage imageNamed:@"videoConf32.png"]];
    iIcon.frame = CGRectMake(11.0f, 17.0f+statusLabel.frame.origin.y+statusLabel.frame.size.height, iIcon.frame.size.width, iIcon.frame.size.height);
    [self.view addSubview:iIcon];
    [iIcon release];
    
    UILabel *infoLabel = [[UILabel alloc] initWithFrame:CGRectMake(iIcon.frame.origin.x+iIcon.frame.size.width+11.0f, iIcon.frame.origin.y, 266.0f, 34.0f)];
    infoLabel.font = [UIFont systemFontOfSize:14.0f];
    infoLabel.numberOfLines = 0;
    infoLabel.text = @"子账号类似手机号,用户区分用户或设备.部分功能需在2-3台设备登录不同子账号";
    infoLabel.textColor = [UIColor colorWithRed:204.0f/255.0f green:204.0f/255.0f blue:204.0f/255.0f alpha:1.0f];
    infoLabel.backgroundColor = [UIColor clearColor];
    [self.view addSubview:infoLabel];
    [infoLabel release];
    
    UILabel* accountLabel = [[UILabel alloc] initWithFrame:CGRectMake(11.0f, infoLabel.frame.origin.y+infoLabel.frame.size.height+17.0f, 100.0f, 17.0f)];
    accountLabel.font = [UIFont systemFontOfSize:15.0f];
    accountLabel.textColor = [UIColor whiteColor];
    accountLabel.backgroundColor = [UIColor clearColor];
    accountLabel.text = @"子账户列表：";
    [self.view addSubview:accountLabel];
    [accountLabel release];
    
    account_frame_begin_y = accountLabel.frame.origin.y+accountLabel.frame.size.height+11.0f;
    
    UIButton *logoButton = [[UIButton alloc] initWithFrame:CGRectMake(11.0f, [UIScreen mainScreen].applicationFrame.size.height-88.0f-11.0f, 298.0f, 44.0f)];
    logoBtn = logoButton;
    logoButton.enabled = NO;
    [logoButton setImage:[UIImage imageNamed:@"new163.png"] forState:UIControlStateDisabled];
    [logoButton setImage:[UIImage imageNamed:@"new158.png"] forState:UIControlStateNormal];
    [logoButton setImage:[UIImage imageNamed:@"new158_on.png"] forState:UIControlStateHighlighted];
    [logoButton addTarget:self action:@selector(logoBtnTouch:) forControlEvents:UIControlEventTouchUpInside];
    [self.view addSubview:logoButton];
    
    [self createAccountList];
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

- (void)gotoAccountInfoView:(id)sender
{
    AccountInfoViewController *view = [[AccountInfoViewController alloc] init];
    [self.navigationController pushViewController:view animated:YES];
    [view release];
}

- (void)logoBtnTouch:(id)sender
{
    if (curIndex >=0)
    {
        AccountInfo* account = [accountArr objectAtIndex:curIndex];
        if (self.modelEngineVoip)
        {
            [self displayProgressingView];
            [self.modelEngineVoip connectToCCP:self.modelEngineVoip.serverIP onPort:self.modelEngineVoip.serverPort  withAccount:account.voipId withPsw:account.password withAccountSid:account.subAccount withAuthToken:account.subToken];
        }
    }
}

- (void)viewWillAppear:(BOOL)animated
{
    [super viewWillAppear:animated];
    [self.modelEngineVoip setModalEngineDelegate:self];
}

- (void)viewWillDisappear:(BOOL)animated
{
    [super viewWillDisappear:animated];
}


#pragma mark - 由setModalEngineDelegate设置返回的登录消息
- (void)responseVoipRegister:(ERegisterResult)event data:(NSString *)data
{
    if (event == ERegisterSuccess)
    {
        // 登录成功
        [self dismissProgressingView];
        //把自己从列表中去除
        NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
        NSString* str =  [defaults objectForKey:@"myPhoneNO"];
        if ([str length]>0)
        {
            [self.modelEngineVoip setVoipPhone:str];
        }
        NSMutableArray *rmArray = [[NSMutableArray alloc] init];
        AccountInfo* account = [accountArr objectAtIndex:curIndex];
        [self.modelEngineVoip setVoipName:account.voipId];
        for (AccountInfo* accountinf in self.modelEngineVoip.accountArray)
        {
            if ([accountinf.voipId isEqualToString:account.voipId])
            {
                [rmArray addObject:accountinf];
            }
        }
        [self.modelEngineVoip.accountArray removeObjectsInArray:rmArray];
        [rmArray release];
        
        DemoListViewController *demoListView = [[DemoListViewController alloc] init];
        [self.navigationController setNavigationBarHidden:YES animated:YES];
        [self.navigationController pushViewController:demoListView animated:YES];
        [demoListView release];
        NSString* strVersion = [self.modelEngineVoip getLIBVersion];
        NSLog(@"%@",strVersion);
    }
    if (event == ERegistering)
    {
    }
    else if (event == ERegisterFail)
    {
        [self dismissProgressingView];
        [self  popPromptViewWithMsg:@"登录失败，请稍后重试！" AndFrame:CGRectMake(0, 160, 320, 30)];
    }
    else if (event == ERegisterNot)
    {
        [self dismissProgressingView];
    }
}
- (void)accountBtnTouch:(id)sender
{
    logoBtn.enabled = YES;
    
    for (UIView *view in btn_scrollView.subviews)
    {
        if (view.tag >= BUTTON_SUNACCOUNT_BASE_TAG)
        {
            UIImageView *markImg = (UIImageView *)[view viewWithTag:50];
            markImg.image = [UIImage imageNamed:@"new65.png"];
        }
    }
    
    UIButton *btn = (UIButton*)sender;
    UIImageView *markImg = (UIImageView *)[btn viewWithTag:50];
    markImg.image = [UIImage imageNamed:@"new63.png"];
    curIndex = btn.tag - BUTTON_SUNACCOUNT_BASE_TAG;
}

- (void)createAccountList
{
    UIScrollView *scrollview = [[UIScrollView alloc] initWithFrame:CGRectMake(0.0f, account_frame_begin_y, 320.0f, logoBtn.frame.origin.y - account_frame_begin_y - 11.0f)];
    btn_scrollView = scrollview;
    [self.view addSubview:scrollview];
    CGFloat btn_frame_y = 0.0f;
    accountArr = self.modelEngineVoip.accountArray;
    for (NSUInteger i=0; i<accountArr.count; i++)
    {
        UIButton *accountBtn = [UIButton buttonWithType:UIButtonTypeCustom];
        accountBtn.frame = CGRectMake(11.0f, btn_frame_y, 298.0f, 44.0f);
        [accountBtn setBackgroundImage:[UIImage imageNamed:@"new126.png"] forState:UIControlStateNormal];
        [accountBtn setBackgroundImage:[UIImage imageNamed:@"new126_on.png"] forState:UIControlStateHighlighted];
        [accountBtn addTarget:self action:@selector(accountBtnTouch:) forControlEvents:UIControlEventTouchUpInside];
        AccountInfo* account = [accountArr objectAtIndex:i];
        [accountBtn setTitle:account.voipId forState:UIControlStateNormal];
        [accountBtn setTitleColor:[UIColor whiteColor] forState:UIControlStateNormal];
        accountBtn.contentHorizontalAlignment = UIControlContentHorizontalAlignmentLeft;        
        UIEdgeInsets insets = accountBtn.contentEdgeInsets;
        insets.left += 6.0f;
        accountBtn.contentEdgeInsets = insets;
        accountBtn.tag = BUTTON_SUNACCOUNT_BASE_TAG+i;
        
        UIImageView *markImg = [[UIImageView alloc] initWithImage:[UIImage imageNamed:@"new65.png"]];
        markImg.tag = 50;
        markImg.center = CGPointMake(284.0f, 22.0f);
        [accountBtn addSubview:markImg];
        [markImg release];
        
        [scrollview addSubview:accountBtn];
        
        btn_frame_y += 45.0f;
    }
    scrollview.contentSize = CGSizeMake(320.0f, btn_frame_y);
    [scrollview release];
}
@end
