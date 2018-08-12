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

#import "AccountInfoViewController.h"
#import "TestNumberListViewController.h"
@interface AccountInfoViewController ()
@end

@implementation AccountInfoViewController

- (void)loadView
{
    self.title = @"账户信息";
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
        
    NSArray *imageArr = [NSArray arrayWithObjects:@"new88.png",@"new90.png",@"new96.png",@"new92.png",@"new94.png", nil];
    CGFloat frame_y = 11.0f;
    for (NSUInteger i = 0; i<5 ; i++)
    {
        UIImageView *mailImg = [[UIImageView alloc] initWithImage:[UIImage imageNamed:[imageArr objectAtIndex:i]]];
        mailImg.center = CGPointMake(11.0f+22.0f, frame_y+22.0f);
        [self.view addSubview:mailImg];
        [mailImg release];
        
        UIView *mailBackview = [[UIView alloc] initWithFrame:CGRectMake(mailImg.frame.origin.x+mailImg.frame.size.width, mailImg.frame.origin.y, 320.0f-mailImg.frame.origin.x-mailImg.frame.size.width-11.0f, mailImg.frame.size.height)];
        mailBackview.backgroundColor = [UIColor blackColor];
        [self.view addSubview:mailBackview];
        [mailBackview release];
        
        UILabel *label = [[UILabel alloc] initWithFrame:CGRectMake(6.0f, 0.0f, mailBackview.frame.size.width-12.0f, mailBackview.frame.size.height)];
        label.textColor = [UIColor whiteColor];
        label.backgroundColor = [UIColor blackColor];
        [mailBackview addSubview:label];
        [label release];
        if (i==1 || i==4)
        {
            frame_y = mailImg.frame.origin.y + mailImg.frame.size.height + 11.0f;
        }
        else
        {
            frame_y = mailImg.frame.origin.y + mailImg.frame.size.height + 1.0f;
        }
        
        switch (i) {
            case 0:
            {
                label.text = self.modelEngineVoip.developerUserName;
            }
                break;
            case 1:
            {
                label.text = self.modelEngineVoip.developerNickName;
            }
                break;
            case 2:
            {
                label.text = self.modelEngineVoip.mainAccount;
            }
                break;
            case 3:
            {
                label.text = self.modelEngineVoip.appID;
            }
                break;
            case 4:
            {
                label.text = self.modelEngineVoip.appName;
            }
                break;
            default:
                break;
        }
    }
    
    UIButton *bindNumBtn = [UIButton buttonWithType:UIButtonTypeCustom];
    bindNumBtn.frame = CGRectMake(11.0f, frame_y, 298.0f, 44.0f);
    [bindNumBtn setBackgroundImage:[UIImage imageNamed:@"new126.png"] forState:UIControlStateNormal];
    [bindNumBtn setBackgroundImage:[UIImage imageNamed:@"new126_on.png"] forState:UIControlStateHighlighted];
    [bindNumBtn addTarget:self action:@selector(bindNumBtnTouch:) forControlEvents:UIControlEventTouchUpInside];
    [bindNumBtn setTitle:@"测试号码绑定" forState:UIControlStateNormal];
    [bindNumBtn setTitleColor:[UIColor whiteColor] forState:UIControlStateNormal];
    bindNumBtn.contentHorizontalAlignment = UIControlContentHorizontalAlignmentLeft;
    UIEdgeInsets insets = bindNumBtn.contentEdgeInsets;
    insets.left += 6.0f;
    bindNumBtn.contentEdgeInsets = insets;
    
    UIImageView *markImg = [[UIImageView alloc] initWithImage:[UIImage imageNamed:@"new67.png"]];
    markImg.tag = 50;
    markImg.center = CGPointMake(282.0f, 22.0f);
    [bindNumBtn addSubview:markImg];
    [markImg release];
    
    [self.view addSubview:bindNumBtn];
    
    UIButton *unLogoBtn = [UIButton buttonWithType:UIButtonTypeCustom];
    unLogoBtn.frame = CGRectMake(11.0f, bindNumBtn.frame.origin.y+bindNumBtn.frame.size.height+11.0f, 298.0f, 44.0f);
    [unLogoBtn setImage:[UIImage imageNamed:@"new152.png"] forState:UIControlStateNormal];
    [unLogoBtn setImage:[UIImage imageNamed:@"new152_on.png"] forState:UIControlStateHighlighted];
    [unLogoBtn addTarget:self action:@selector(unLogoBtnTouch:) forControlEvents:UIControlEventTouchUpInside];
    [self.view addSubview:unLogoBtn];
    
    UILabel *infoLabel = [[UILabel alloc] initWithFrame:CGRectMake(11.0f, unLogoBtn.frame.origin.y+unLogoBtn.frame.size.height+11.0f, 298.0f, 30.0f)];
    infoLabel.numberOfLines = 0;
    infoLabel.lineBreakMode = NSLineBreakByWordWrapping;
    infoLabel.font = [UIFont systemFontOfSize:11.0f];
    infoLabel.text = @"您可以登录yuntongxun.com查询该账户基本信息、余额、应用计费等相关信息及创建新应用、充值等相关操作.";
    infoLabel.textColor = [UIColor colorWithRed:204.0f/255.0f green:204.0f/255.0f blue:204.0f/255.0f alpha:1.0f];
    infoLabel.backgroundColor = [UIColor clearColor];
    [self.view addSubview:infoLabel];
    [infoLabel release];
    
    UIBarButtonItem *leftBarItem = [[UIBarButtonItem alloc] initWithCustomView:[CommonTools navigationBackItemBtnInitWithTarget:self action:@selector(popToPreView)]];
    self.navigationItem.leftBarButtonItem = leftBarItem;
    [leftBarItem release];
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

- (void)bindNumBtnTouch:(id)sender
{
    TestNumberListViewController *view = [[TestNumberListViewController alloc] init];
    [self.navigationController pushViewController:view animated:YES];
    [view release];
}

- (void)unLogoBtnTouch:(id)sender
{
    [self.modelEngineVoip logoutDemoExperience];
    [self.navigationController popToRootViewControllerAnimated:YES];
}
-(void)dealloc
{
    [super dealloc];
}
@end
