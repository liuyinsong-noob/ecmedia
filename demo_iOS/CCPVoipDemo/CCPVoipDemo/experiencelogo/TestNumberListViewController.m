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

#import "TestNumberListViewController.h"
#import "BindTestNumberViewController.h"
@interface TestNumberListViewController ()

@end

@implementation TestNumberListViewController

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
    self.title = @"绑定测试号码";
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
    
    UIImageView *pointImg = [[UIImageView alloc] initWithImage:[UIImage imageNamed:@"video_new_tips.png"]];
    pointImg.frame = CGRectMake(0.0f, 0.0f, 320.0f, 22.0f);
    [self.view addSubview:pointImg];
    [pointImg release];
    
    UILabel *statusLabel = [[UILabel alloc] initWithFrame:CGRectMake(11.0f, 0.0f, 320.0f, 22.0f)];
    statusLabel.backgroundColor = [UIColor clearColor];
    statusLabel.textColor = [UIColor whiteColor];
    statusLabel.font = [UIFont systemFontOfSize:13.0f];
    statusLabel.text = @"Demo及未上线应用建议绑定1-2个落地测试号码";
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
    infoLabel.text = @"未上线应用只能向测试号码打电话,发短信.落地电话,语音验证码,外呼通知受此限制.";
    infoLabel.textColor = [UIColor colorWithRed:204.0f/255.0f green:204.0f/255.0f blue:204.0f/255.0f alpha:1.0f];
    infoLabel.backgroundColor = [UIColor clearColor];
    [self.view addSubview:infoLabel];
    [infoLabel release];
    
    UILabel* accountLabel = [[UILabel alloc] initWithFrame:CGRectMake(11.0f, infoLabel.frame.origin.y+infoLabel.frame.size.height+17.0f, 100.0f, 17.0f)];
    accountLabel.font = [UIFont systemFontOfSize:15.0f];
    accountLabel.textColor = [UIColor whiteColor];
    accountLabel.backgroundColor = [UIColor clearColor];
    accountLabel.text = @"测试号码：";
    [self.view addSubview:accountLabel];
    [accountLabel release];
    
    
    CGFloat frame_y = accountLabel.frame.origin.y+accountLabel.frame.size.height+11.0f;
    for (NSUInteger i = 0; i<3 ; i++)
    {
        NSString *btnTitle = @"按钮";
        
        UIImageView *mailImg = [[UIImageView alloc] initWithImage:[UIImage imageNamed:@"new84.png"]];
        mailImg.center = CGPointMake(11.0f+22.0f, frame_y+22.0f);
        [self.view addSubview:mailImg];
        [mailImg release];
        
        UIButton *bindBtn = [UIButton buttonWithType:UIButtonTypeCustom];
        bindBtn.frame = CGRectMake(mailImg.frame.origin.x+mailImg.frame.size.width, mailImg.frame.origin.y, 320.0f-mailImg.frame.origin.x-mailImg.frame.size.width-11.0f, mailImg.frame.size.height);
        [bindBtn setBackgroundImage:[UIImage imageNamed:@"new126.png"] forState:UIControlStateNormal];
        [bindBtn setBackgroundImage:[UIImage imageNamed:@"new126.png"] forState:UIControlStateDisabled];
        [bindBtn setBackgroundImage:[UIImage imageNamed:@"new126_on.png"] forState:UIControlStateHighlighted];
        [bindBtn addTarget:self action:@selector(bindBtnTouch:) forControlEvents:UIControlEventTouchUpInside];
        [bindBtn setTitle:btnTitle forState:UIControlStateNormal];
        [bindBtn setTitleColor:[UIColor whiteColor] forState:UIControlStateNormal];
        bindBtn.contentHorizontalAlignment = UIControlContentHorizontalAlignmentLeft;
        bindBtn.tag = 100+i;
        UIEdgeInsets insets = bindBtn.contentEdgeInsets;
        insets.left += 6.0f;
        bindBtn.contentEdgeInsets = insets;
        [self.view addSubview:bindBtn];
        
        if (i == 0)
        {
            bindBtn.enabled = NO;
            frame_y = mailImg.frame.origin.y + mailImg.frame.size.height + 1.0f;
        }
        else
        {
            UIImageView *accessImg = [[UIImageView alloc] initWithImage:[UIImage imageNamed:@"new67.png"]];
            accessImg.tag = 50;
            accessImg.center = CGPointMake(238.0f, 22.0f);
            [bindBtn addSubview:accessImg];
            [accessImg release];
            frame_y = mailImg.frame.origin.y + mailImg.frame.size.height + (i==1?1.0f:11.0f);
        }
    }
    
    UIButton *finishButton = [[UIButton alloc] initWithFrame:CGRectMake(11.0f, frame_y, 298.0f, 44.0f)];
    [finishButton setImage:[UIImage imageNamed:@"new140.png"] forState:UIControlStateNormal];
    [finishButton setImage:[UIImage imageNamed:@"new140_on.png"] forState:UIControlStateHighlighted];
    [finishButton addTarget:self action:@selector(popToPreView) forControlEvents:UIControlEventTouchUpInside];
    [self.view addSubview:finishButton];
    [finishButton release];
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
    [self updateNumberList];
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

- (void)updateNumberList
{
    for (NSUInteger i = 0; i<3 ; i++)
    {
        NSString *btnTitle = self.modelEngineVoip.test_numberArray.count>i?[self.modelEngineVoip.test_numberArray objectAtIndex:i]:@"待绑定";
        UIButton *bindBtn = (UIButton*)[self.view viewWithTag:100+i];
        [bindBtn setTitle:btnTitle forState:UIControlStateNormal];
        if (i == 0)
        {
            [bindBtn setTitleColor:[UIColor whiteColor] forState:UIControlStateNormal];
        }
        else if ([btnTitle isEqualToString:@"待绑定"])
        {
            UIView * label = [bindBtn viewWithTag:100];
            if (label)
            {
                [label removeFromSuperview];
            }
            [bindBtn setTitleColor:[UIColor colorWithRed:136.0f/255.0f green:136.0f/255.0f blue:136.0f/255.0f alpha:1.0f] forState:UIControlStateNormal];
        }
        else
        {
            [bindBtn setTitleColor:[UIColor whiteColor] forState:UIControlStateNormal];
            CGSize size = [bindBtn.titleLabel.text sizeWithFont:bindBtn.titleLabel.font forWidth:200.0f lineBreakMode:NSLineBreakByWordWrapping];
            UILabel * label = (UILabel*)[bindBtn viewWithTag:100];
            if (label == nil)
            {
                label = [[UILabel alloc] init];
                label.tag = 100;
                label.text = @"已验证";
                label.font = [UIFont systemFontOfSize:13.0f];
                label.textColor = [UIColor colorWithRed:136.0f/255.0f green:136.0f/255.0f blue:136.0f/255.0f alpha:1.0f];
                label.backgroundColor = [UIColor clearColor];
                [bindBtn addSubview:label];
                [label release];
            }
            label.frame = CGRectMake(6.0f+size.width+11.0f, 0.0f, 40.0f, 44.0f);
        }
    }
}

- (void)bindBtnTouch:(id)sender
{
    int value = [(UIButton*)sender tag] - 100;
    NSString* str = nil;
    if (value < [self.modelEngineVoip.test_numberArray count])
    {
        str = [self.modelEngineVoip.test_numberArray objectAtIndex:value];
    }
    BindTestNumberViewController *view = [[BindTestNumberViewController alloc] initWithTextNumber:str];
    [self.navigationController pushViewController:view animated:YES];
    [view release];
}
@end
