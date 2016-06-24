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

#import "ForgetPwdHelpViewController.h"

@interface ForgetPwdHelpViewController ()

@end

@implementation ForgetPwdHelpViewController

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
    self.title = @"忘记密码帮助";
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
    
    NSArray *imgArr = [[NSArray alloc] initWithObjects:@"new42.png", @"new46.png", @"new52.png", @"new48.png", nil];
    NSArray *textArr = [[NSArray alloc] initWithObjects:@"登录cloopen.com点击登录", @"点击右侧\"忘记密码\"", @"输入注册邮箱并按提示操作", @"重置密码成，使用新密码登录", nil];
    
    CGFloat frame_y = 50.0f;
    for (NSInteger i = 0; i<imgArr.count; i++)
    {
        UIImageView *imgview = [[UIImageView alloc] initWithImage:[UIImage imageNamed:[imgArr objectAtIndex:i]]];
        imgview.center = CGPointMake(160.0f, frame_y+imgview.frame.size.height*0.5);
        [self.view addSubview:imgview];
        [imgview release];
        
        UILabel *textLabel = [[UILabel alloc] initWithFrame:CGRectMake(10.0f, imgview.frame.origin.y+imgview.frame.size.height+11.0f, 300.0f, 16.0f)];
        textLabel.font = [UIFont systemFontOfSize:18.0f];
        textLabel.text = [textArr objectAtIndex:i];
        textLabel.textAlignment = NSTextAlignmentCenter;
        textLabel.textColor = [UIColor colorWithRed:204.0f/255.0f green:204.0f/255.0f blue:204.0f/255.0f alpha:1.0f];
        textLabel.backgroundColor = [UIColor clearColor];
        [self.view addSubview:textLabel];
        [textLabel release];
        
        if (i == imgArr.count-1)
        {
            break;
        }
        
        UIImageView *nextImg = [[UIImageView alloc] initWithImage:[UIImage imageNamed:@"new44.png"]];
        nextImg.center = CGPointMake(160.0f, textLabel.frame.origin.y+textLabel.frame.size.height+17.0f+nextImg.frame.size.height*0.5);
        [self.view addSubview:nextImg];
        [nextImg release];
        
        frame_y = nextImg.frame.origin.y + nextImg.frame.size.height + 17.f;
    }
    [imgArr release];
    [textArr release];
}
- (void)viewDidLoad
{
    [super viewDidLoad];
	// Do any additional setup after loading the view.
}

- (void)viewWillAppear:(BOOL)animated
{
    [super viewWillAppear:animated];
    self.modelEngineVoip.UIDelegate = self;
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

@end
