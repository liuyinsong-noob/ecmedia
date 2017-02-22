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

#import "VoIPCallViewController.h"
#import "ViewController.h"
#import "AppDelegate.h"
#import "CallViewController.h"
@interface VoIPCallViewController ()

@end

@implementation VoIPCallViewController

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
{
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self) {
        // Custom initialization
    }
    return self;
}
-(id)init
{
    if (self = [super init])
    {
        isRecordingMP4 = false;
    }
    return self;
}

-(void)loadView
{
    self.title = @"网络电话能力演示";
    self.view = [[[UIView alloc] initWithFrame:[UIScreen mainScreen].applicationFrame] autorelease];
    self.view.backgroundColor = VIEW_BACKGROUND_COLOR_WHITE;
    
    UIBarButtonItem *leftBarItem = [[UIBarButtonItem alloc] initWithCustomView:[CommonTools navigationBackItemBtnInitWithTarget:self action:@selector(popToPreView)]];
    self.navigationItem.leftBarButtonItem = leftBarItem;
    [leftBarItem release];
    
    UIImageView *pointImg = [[UIImageView alloc] initWithImage:[UIImage imageNamed:@"point_bg.png"]];
    pointImg.frame = CGRectMake(0.0f, 0.0f, 320.0f, 29.0f);
    [self.view addSubview:pointImg];
    [pointImg release];
    
    UILabel *lbhead = [[UILabel alloc] initWithFrame:CGRectMake(0.0f, 0.0f, 320.0f, 29.0f)] ;
    lbhead.backgroundColor = [UIColor clearColor];
    lbhead.textColor = [UIColor whiteColor];
    lbhead.textAlignment = UITextAlignmentLeft;
    lbhead.font = [UIFont systemFontOfSize:13.0f];
    lbhead.text =  @"    请选择通话方式";
    [self.view addSubview:lbhead];
    [lbhead release];
    
    UIButton* btnVoip = [UIButton buttonWithType:(UIButtonTypeCustom)];
    btnVoip.frame = CGRectMake(35, 61, 250, 125);
    [btnVoip setBackgroundImage:[UIImage imageNamed:@"demonstrate_voip_button1_off.png"] forState:(UIControlStateNormal)];
    [btnVoip setBackgroundImage:[UIImage imageNamed:@"demonstrate_voip_button1_on.png"] forState:(UIControlStateSelected)];
    [btnVoip addTarget:self action:@selector(goToVoipView:) forControlEvents:(UIControlEventTouchUpInside)];
    [self.view addSubview:btnVoip];
    
    UIButton* btnCall = [UIButton buttonWithType:(UIButtonTypeCustom)];
    btnCall.frame = CGRectMake(35, 207, 250, 125);
    [btnCall setBackgroundImage:[UIImage imageNamed:@"demonstrate_voip_button2_off.png"] forState:(UIControlStateNormal)];
    [btnCall setBackgroundImage:[UIImage imageNamed:@"demonstrate_voip_button2_on.png"] forState:(UIControlStateSelected)];
    [btnCall addTarget:self action:@selector(goCallView) forControlEvents:(UIControlEventTouchUpInside)];
    [self.view addSubview:btnCall];
    
    // added by zhaoyou 2017-01-16, 录制 mp4 小视频
    UIButton* btnRecordMp4 = [UIButton buttonWithType:(UIButtonTypeCustom)];
    btnRecordMp4.frame = CGRectMake(35, 350, 250, 125);
    [btnRecordMp4 setTitle:@"开始录制视频" forState:UIControlStateNormal];
    [btnRecordMp4 setTitleColor: [UIColor blackColor] forState:UIControlStateNormal];
    [btnRecordMp4.layer setBorderWidth:1.0];
    [btnRecordMp4.layer setBorderColor:[[UIColor blackColor] CGColor]];
    [btnRecordMp4 addTarget:self action:@selector(goRecordPreview) forControlEvents:(UIControlEventTouchUpInside)];
    [self.view addSubview:btnRecordMp4];
}

- (void)viewDidLoad
{
    [super viewDidLoad];
    // Do any additional setup after loading the view from its nib.
}
- (void)viewWillAppear:(BOOL)animated
{
    [super viewWillAppear:animated];
    [self.modelEngineVoip setModalEngineDelegate:self];
}
- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

- (void)dealloc {
    [super dealloc];
}

-(void)goCallView
{
    UIBarButtonItem *leftBarItem = [[UIBarButtonItem alloc] initWithCustomView:[CommonTools navigationBackItemBtnInitWithTarget:self action:@selector(popToPreView)]];
    self.navigationItem.leftBarButtonItem = leftBarItem;
    [leftBarItem release];    
    CallViewController *view = [[CallViewController alloc] init];
    [self.navigationController pushViewController:view animated:YES];
    [view release];
}
- (void)goToVoipView:(id)sender
{    
    UIBarButtonItem *leftBarItem = [[UIBarButtonItem alloc] initWithCustomView:[CommonTools navigationBackItemBtnInitWithTarget:self action:@selector(popToPreView)]];
    self.navigationItem.leftBarButtonItem = leftBarItem;
    [leftBarItem release];
    
    ViewController *voipView = [[ViewController alloc] init];
    [self.navigationController pushViewController:voipView animated:YES];
    [voipView release];
}

-(void)goRecordPreview {
    NSLog(@"开始录制视频！！");
//    UIButton *btn = (UIButton*)sender;
//    [btn setTitle:@"停止录制视频" forState:UIControlStateNormal];
    NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    NSString *docDir = [paths objectAtIndex:0];
    docDir = [docDir stringByAppendingString:@"/output.mp4"];
    if(!isRecordingMP4) {
        [self.modelEngineVoip startRecordLocalMedia:docDir withView:self.view];
        isRecordingMP4 = true;
    } else {
        [self.modelEngineVoip stopRecordLocalMedia];
        isRecordingMP4 = false;
    }
}
@end
