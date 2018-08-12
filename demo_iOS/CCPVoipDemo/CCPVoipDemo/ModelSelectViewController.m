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

#import "ModelSelectViewController.h"
#import "RegisterViewController.h"
#import "ExperienceLogoViewController.h"
#import "ExperienceSelectAccountViewController.h"
#import "AccountInfo.h"
@interface ModelSelectViewController ()

@end

@implementation ModelSelectViewController

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
    self.title = @"选择登录模式";
    self.view = [[[UIView alloc] initWithFrame:[UIScreen mainScreen].applicationFrame] autorelease];
    int value = 0;
    if ([[[UIDevice currentDevice] systemVersion] floatValue] >= 7) {
        value = 20;
    }

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
    
    [self.navigationController setNavigationBarHidden:YES animated:NO];
    if ([[[UIDevice currentDevice] systemVersion] doubleValue] >= 5.0)
    {
//        if ([[[UIDevice currentDevice] systemVersion] doubleValue] >= 7.0)
//        {
//            [self.navigationController.navigationBar setBackgroundImage:[UIImage imageNamed:@"navigation_iOS7.png"] forBarMetrics:0];
//        }
//        else
            [self.navigationController.navigationBar setBackgroundImage:[UIImage imageNamed:@"navigation.png"] forBarMetrics:0];
        NSDictionary *dict = [NSDictionary dictionaryWithObject:[UIColor whiteColor] forKey:UITextAttributeTextColor];
        self.navigationController.navigationBar.titleTextAttributes = dict;
    }
    self.navigationController.navigationBar.tintColor = VIEW_BACKGROUND_COLOR_FIRSTVIEW;
    [UIApplication sharedApplication].statusBarStyle =UIStatusBarStyleBlackOpaque;
    
    UIView *topView = [[UIView alloc] initWithFrame:CGRectMake(0.0f, 0.0f, 320.0f, 92.0f+value)];
    topView.backgroundColor = [UIColor colorWithRed:34.0f/255.0f green:34.0f/255.0f blue:34.0f/255.0f alpha:1.0f];
    
//    UIImageView *logoImg = [[UIImageView alloc] initWithImage:[UIImage imageNamed:@"logoDemo.png"]];
//    logoImg.center = CGPointMake(77.0f, 45.0f+value/2);
//    [topView addSubview:logoImg];
//    [logoImg release];
    
    UILabel *topLabel = [[UILabel alloc] initWithFrame:CGRectMake(200.0f, 50.0f+value, 109.0f, 20.0f)];
    topLabel.backgroundColor = [UIColor colorWithRed:34.0f/255.0f green:34.0f/255.0f blue:34.0f/255.0f alpha:1.0f];
    topLabel.text = @"能力演示";
    topLabel.textColor = [UIColor whiteColor];
    topLabel.textAlignment = NSTextAlignmentRight;
    [topView addSubview:topLabel];
    [topLabel release];
    
    [self.view addSubview:topView];
    [topView release];
    
    UIButton *leftView = [UIButton buttonWithType:UIButtonTypeCustom];
    leftView.frame = CGRectMake(0, topView.frame.size.height+topView.frame.origin.y, 160.0f, [UIScreen mainScreen].applicationFrame.size.height - topView.frame.size.height+value);
    [leftView setBackgroundImage:[self createImageWithColor:[UIColor colorWithRed:182.0f/255.0f green:213.0f/255.0f blue:86.0f/255.0f alpha:1.0f]] forState:UIControlStateNormal];
    [leftView setBackgroundImage:[self createImageWithColor:[UIColor colorWithRed:136.0f/255.0f green:160.0f/255.0f blue:64.0f/255.0f alpha:1.0f]] forState:UIControlStateHighlighted];
    [leftView addTarget:self action:@selector(leftBtn:) forControlEvents:UIControlEventTouchUpInside];
    
    UIImageView *leftimage = [[UIImageView alloc] initWithImage:[UIImage imageNamed:@"new59.png"]];
    leftimage.center = CGPointMake(80.0f, leftView.frame.size.height*0.5-30.0f);
    [leftView addSubview:leftimage];
    [leftimage release];
    
    UILabel* leftLabel1 = [[UILabel alloc] initWithFrame:CGRectMake(10, leftimage.frame.origin.y+leftimage.frame.size.height+8.0f, 140.0f, 15.0f)];
    leftLabel1.text = @"开发模式";
    leftLabel1.backgroundColor = [UIColor clearColor];
    leftLabel1.textColor = [UIColor whiteColor];
    leftLabel1.textAlignment = NSTextAlignmentCenter;
    [leftView addSubview:leftLabel1];
    [leftLabel1 release];
    
    UILabel* leftLabel2 = [[UILabel alloc] initWithFrame:CGRectMake(10, leftLabel1.frame.origin.y+leftLabel1.frame.size.height+6.0f, 140.0f, 12.0f)];
    leftLabel2.text = @"预先导入配置文件";
    leftLabel2.font = [UIFont systemFontOfSize:13.0f];
    leftLabel2.backgroundColor = [UIColor clearColor];
    leftLabel2.textColor = [UIColor whiteColor];
    leftLabel2.textAlignment = NSTextAlignmentCenter;
    [leftView addSubview:leftLabel2];
    [leftLabel2 release];
    
    UILabel* leftLabel3 = [[UILabel alloc] init];
    leftLabel3.numberOfLines = 0;
    leftLabel3.lineBreakMode = NSLineBreakByWordWrapping;
    leftLabel3.backgroundColor = [UIColor clearColor];
    leftLabel3.font = [UIFont systemFontOfSize:13.0f];
    leftLabel3.text = @"请按yuntongxun.com开发文档说明完善配置文件，并导入SD卡或描述文件来使用";
    leftLabel3.textColor = [UIColor whiteColor];
    CGSize size = [leftLabel3.text sizeWithFont:leftLabel3.font constrainedToSize:CGSizeMake(138.0f, 200.0f) lineBreakMode:NSLineBreakByWordWrapping];
    leftLabel3.frame = CGRectMake(11, leftView.frame.size.height - size.height - 11, size.width, size.height);
    [leftView addSubview:leftLabel3];
    [leftLabel3 release];
    
    [self.view addSubview:leftView];
    
    
    UIButton *rightView = [UIButton buttonWithType:UIButtonTypeCustom];
    rightView.frame = CGRectMake(160, topView.frame.size.height+topView.frame.origin.y, 160.0f, [UIScreen mainScreen].applicationFrame.size.height - topView.frame.size.height+value);
    [rightView setBackgroundImage:[self createImageWithColor:[UIColor colorWithRed:85.0f/255.0f green:188.0f/255.0f blue:117.0f/255.0f alpha:1.0f]] forState:UIControlStateNormal];
    [rightView setBackgroundImage:[self createImageWithColor:[UIColor colorWithRed:64.0f/255.0f green:141.0f/255.0f blue:88.0f/255.0f alpha:1.0f]] forState:UIControlStateHighlighted];
    [rightView addTarget:self action:@selector(rightBtn:) forControlEvents:UIControlEventTouchUpInside];
    
    UIImageView *rightimage = [[UIImageView alloc] initWithImage:[UIImage imageNamed:@"new61.png"]];
    rightimage.center = CGPointMake(80.0f, rightView.frame.size.height*0.5-30.0f);
    [rightView addSubview:rightimage];
    [rightimage release];
    
    UILabel* rightLabel1 = [[UILabel alloc] initWithFrame:CGRectMake(10, rightimage.frame.origin.y+rightimage.frame.size.height+8.0f, 140.0f, 15.0f)];
    rightLabel1.text = @"体验模式";
    rightLabel1.backgroundColor = [UIColor clearColor];
    rightLabel1.textColor = [UIColor whiteColor];
    rightLabel1.textAlignment = NSTextAlignmentCenter;
    [rightView addSubview:rightLabel1];
    [rightLabel1 release];
    
    UILabel* rightLabel2 = [[UILabel alloc] initWithFrame:CGRectMake(10, rightLabel1.frame.origin.y+rightLabel1.frame.size.height+6.0f, 140.0f, 12.0f)];
    rightLabel2.text = @"用云通讯账号登录使用";
    rightLabel2.font = [UIFont systemFontOfSize:13.0f];
    rightLabel2.backgroundColor = [UIColor clearColor];
    rightLabel2.textColor = [UIColor whiteColor];
    rightLabel2.textAlignment = NSTextAlignmentCenter;
    [rightView addSubview:rightLabel2];
    [rightLabel2 release];
    
    UILabel* rightLabel3 = [[UILabel alloc] init];
    rightLabel3.numberOfLines = 0;
    rightLabel3.lineBreakMode = NSLineBreakByWordWrapping;
    rightLabel3.backgroundColor = [UIColor clearColor];
    rightLabel3.font = [UIFont systemFontOfSize:13.0f];
    rightLabel3.text = @"通过云通讯账户登录即可体验全部功能，但部分功能会因应用未上线而受到限制";
    rightLabel3.textColor = [UIColor whiteColor];
    CGSize rightsize = [rightLabel3.text sizeWithFont:rightLabel3.font constrainedToSize:CGSizeMake(138.0f, 200.0f) lineBreakMode:NSLineBreakByWordWrapping];
    rightLabel3.frame = CGRectMake(11, rightView.frame.size.height - rightsize.height - 11, rightsize.width, rightsize.height);
    [rightView addSubview:rightLabel3];
    [rightLabel3 release];
    
    [self.view addSubview:rightView];
    
    NSUserDefaults *userDefaults = [NSUserDefaults standardUserDefaults];
    BOOL noFirst = [userDefaults boolForKey:@"noFirst"];
    if (!noFirst)
    {        
        [userDefaults setBool:YES forKey:@"noFirst"];
        [userDefaults synchronize];
        UIButton *surfaceView = [UIButton buttonWithType:UIButtonTypeCustom];
        surfaceView.frame = CGRectMake(0.0f, 0.0, 320.0f, [UIScreen mainScreen].bounds.size.height);
        surfaceView.backgroundColor = [UIColor colorWithRed:0.0f green:0.0f blue:0.0f alpha:0.7f];
        [surfaceView addTarget:self action:@selector(surfaceBtn:) forControlEvents:UIControlEventTouchUpInside];
        [self.view addSubview:surfaceView];
        
        UILabel *infoLabel1 = [[UILabel alloc] init];
        infoLabel1.numberOfLines = 0;
        infoLabel1.textColor = [UIColor whiteColor];
        infoLabel1.backgroundColor = [UIColor clearColor];
        infoLabel1.lineBreakMode = NSLineBreakByWordWrapping;
        infoLabel1.text = @"如果您是开发者\n并已查阅yuntongxun.com中的\n相关文档，\n左侧属于您";
        infoLabel1.font = [UIFont systemFontOfSize:15.0f];
        size = [infoLabel1.text sizeWithFont:infoLabel1.font constrainedToSize:CGSizeMake(200.0f, 100.0f) lineBreakMode:NSLineBreakByWordWrapping];
        infoLabel1.frame = CGRectMake(24.0f, leftView.frame.origin.y+(leftimage.frame.origin.y-17.0f-11.0f-9.0f-size.height), size.width, size.height);
        [surfaceView addSubview:infoLabel1];
        [infoLabel1 release];
        
        UIImageView *downArrow = [[UIImageView alloc] initWithImage:[UIImage imageNamed:@"new000.png"]];
        downArrow.center = CGPointMake(80.0f, leftView.frame.origin.y+(leftimage.frame.origin.y-17.0f-4.5f));
        [surfaceView addSubview:downArrow];
        [downArrow release];
        
        UILabel *infoLabel2 = [[UILabel alloc] init];
        infoLabel2.textAlignment = NSTextAlignmentRight;
        infoLabel2.numberOfLines = 0;
        infoLabel2.textColor = [UIColor whiteColor];
        infoLabel2.backgroundColor = [UIColor clearColor];
        infoLabel2.lineBreakMode = NSLineBreakByWordWrapping;
        infoLabel2.text = @"如果您只想体验下云通讯的能力\n不搞代码，请点右侧";
        infoLabel2.font = [UIFont systemFontOfSize:15.0f];
        size = [infoLabel2.text sizeWithFont:infoLabel2.font constrainedToSize:CGSizeMake(240.0f, 100.0f) lineBreakMode:NSLineBreakByWordWrapping];
        infoLabel2.frame = CGRectMake(320.0f-24.0f-size.width, rightView.frame.origin.y+rightLabel2.frame.origin.y+rightLabel2.frame.size.height+17.0f+9.0f+11.0f, size.width, size.height);
        [surfaceView addSubview:infoLabel2];
        [infoLabel2 release];
        
        UIImageView *upArrow = [[UIImageView alloc] initWithImage:[UIImage imageNamed:@"new001.png"]];
        upArrow.center = CGPointMake(240.0f, rightView.frame.origin.y+rightLabel2.frame.origin.y+rightLabel2.frame.size.height+17.0f+4.5f);
        [surfaceView addSubview:upArrow];
        [upArrow release];
    }
}
- (void)viewDidLoad
{
    [super viewDidLoad];
	// Do any additional setup after loading the view.
}

- (void)viewWillAppear:(BOOL)animated
{
    [super viewWillAppear:animated];
    [self.navigationController setNavigationBarHidden:YES animated:YES];
    [self.modelEngineVoip setModalEngineDelegate:self];
}


- (void)viewDidAppear:(BOOL)animated
{
    [super viewDidAppear:animated];
    RegisterViewController *view = [[RegisterViewController alloc] init];
    [self.navigationController pushViewController:view animated:YES];
    [view release];
}

- (void)viewWillDisappear:(BOOL)animated
{
    [self.navigationController setNavigationBarHidden:NO animated:YES];
    [self.modelEngineVoip setModalEngineDelegate:nil];
    [super viewWillDisappear:animated];
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

- (void)leftBtn:(id)sender
{
    RegisterViewController *view = [[RegisterViewController alloc] init];
    [self.navigationController pushViewController:view animated:YES];
    [view release];
}

- (void)rightBtn:(id)sender
{
    NSUserDefaults *userDefaults = [NSUserDefaults standardUserDefaults];
    NSString *userName = [userDefaults objectForKey:@"developerUserName"];
    NSString *userPwd = [userDefaults objectForKey:@"developerUserPasswd"];
    self.modelEngineVoip.developerUserName = userName.length>0?__TEXT(userName):nil;
    self.modelEngineVoip.developerUserPasswd = userPwd.length>0?__TEXT(userPwd):nil;
    if ([self.modelEngineVoip.developerUserName length]>0 && [self.modelEngineVoip.developerUserPasswd length]> 0)
    {
        self.modelEngineVoip.serverIP = @"sandboxapp.yuntongxun.com";
        self.modelEngineVoip.serverPort = 8883;
        [self.modelEngineVoip getDemoAccountsWithUserName:self.modelEngineVoip.developerUserName andUserPwd:self.modelEngineVoip.developerUserPasswd];
    }
    else
    {
        ExperienceLogoViewController *view = [[ExperienceLogoViewController alloc] init];
        [self.navigationController pushViewController:view animated:YES];
        [view release];
    }
}


-(void)onGetDemoAccountsWithReason:(CloopenReason *)reason
{
    [self dismissProgressingView];
    if (reason.reason == 0)
    {
        ExperienceSelectAccountViewController *view = [[ExperienceSelectAccountViewController alloc] init];
        [self.navigationController pushViewController:view animated:YES];
        [view release];
    }
    else
    {
        ExperienceLogoViewController *view = [[ExperienceLogoViewController alloc] init];
        [self.navigationController pushViewController:view animated:YES];
        [view popPromptViewWithMsg:[NSString stringWithFormat:@"错误码：%d,错误详情：%@",reason.reason,reason.msg]];
        [view release];
    }
}
- (void)surfaceBtn:(id)sender
{
    UIView * surfaceview = (UIView*)sender;
    [surfaceview removeFromSuperview];
}
@end
