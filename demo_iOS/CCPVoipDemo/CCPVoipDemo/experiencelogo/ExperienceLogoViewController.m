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

#import "ExperienceLogoViewController.h"
#import "ForgetPwdHelpViewController.h"
#import "ExperienceSelectAccountViewController.h"
#import "AccountInfo.h"
@interface ExperienceLogoViewController ()
{
    UIButton *logoBtn;
    UIButton *registBtn;
    UIView *registView;
    UIView *logoView;
    
    UITextField *pwdTextField;
    UITextField *mailTextField;
}
@end

@implementation ExperienceLogoViewController

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
    
    UIButton * backbtn = [UIButton buttonWithType:UIButtonTypeCustom];
    backbtn.backgroundColor = [UIColor clearColor];
    backbtn.frame = range;
    [backbtn addTarget:self action:@selector(hidekeyboard:) forControlEvents:UIControlEventTouchUpInside];
    [self.view addSubview:backbtn];
    
    
    UIBarButtonItem *leftBarItem = [[UIBarButtonItem alloc] initWithCustomView:[CommonTools navigationBackItemBtnInitWithTarget:self action:@selector(popToPreView)]];
    self.navigationItem.leftBarButtonItem = leftBarItem;
    [leftBarItem release];
    
    logoBtn = [UIButton buttonWithType:UIButtonTypeCustom];
    logoBtn.enabled = NO;
    [logoBtn setTitle:@"云通讯登录" forState:UIControlStateNormal];
    logoBtn.frame = CGRectMake(0.0f, 0.0f, 160.0f, 44.0f);
    [logoBtn setTitleColor:[UIColor whiteColor] forState:UIControlStateNormal];
    logoBtn.backgroundColor = [UIColor clearColor];
    [logoBtn addTarget:self action:@selector(logoBtnTouch:) forControlEvents:UIControlEventTouchUpInside];
    [self.view addSubview:logoBtn];
    
    registBtn = [UIButton buttonWithType:UIButtonTypeCustom];
    [registBtn setTitle:@"注 册" forState:UIControlStateNormal];
    registBtn.frame = CGRectMake(160.0f, 0.0f, 160.0f, 44.0f);
    [registBtn setTitleColor:[UIColor colorWithRed:136.0f/255.0f green:136.0f/255.0f blue:136.0f/255.0f alpha:1.0f] forState:UIControlStateNormal];
    registBtn.backgroundColor = [UIColor colorWithRed:0.0f green:0.0f blue:0.0f alpha:0.5f];
    [registBtn addTarget:self action:@selector(registBtnTouch:) forControlEvents:UIControlEventTouchUpInside];
    [self.view addSubview:registBtn];
    
    UIView *rightView = [[UIView alloc] initWithFrame:CGRectMake(0.0f, registBtn.frame.origin.y+registBtn.frame.size.height, 320.0f, [UIScreen mainScreen].applicationFrame.size.height-44.0f)];
    registView = rightView;
    registView.alpha = 0.0f;
    rightView.backgroundColor = [UIColor clearColor];
    [self.view addSubview:rightView];
    [rightView release];
    
    NSArray *imgArr = [[NSArray alloc] initWithObjects:@"new42.png", @"new46.png", @"new50.png", @"new48.png", nil];
    NSArray *textArr = [[NSArray alloc] initWithObjects:@"登录cloopen.com并注册", @"填写注册信息", @"验证邮箱以及手机号", @"注册激活成功获得8元测试体验费", nil];
    
    CGFloat frame_y = 30.0f;
    for (NSInteger i = 0; i<imgArr.count; i++)
    {
        UIImageView *imgview = [[UIImageView alloc] initWithImage:[UIImage imageNamed:[imgArr objectAtIndex:i]]];
        imgview.center = CGPointMake(160.0f, frame_y+imgview.frame.size.height*0.5);
        [registView addSubview:imgview];
        [imgview release];
        
        UILabel *textLabel = [[UILabel alloc] initWithFrame:CGRectMake(10.0f, imgview.frame.origin.y+imgview.frame.size.height+11.0f, 300.0f, 16.0f)];
        textLabel.font = [UIFont systemFontOfSize:18.0f];
        textLabel.text = [textArr objectAtIndex:i];
        textLabel.textAlignment = NSTextAlignmentCenter;
        textLabel.textColor = [UIColor colorWithRed:204.0f/255.0f green:204.0f/255.0f blue:204.0f/255.0f alpha:1.0f];
        textLabel.backgroundColor = [UIColor clearColor];
        [registView addSubview:textLabel];
        [textLabel release];
        
        if (i == imgArr.count-1)
        {
            break;
        }
        
        UIImageView *nextImg = [[UIImageView alloc] initWithImage:[UIImage imageNamed:@"new44.png"]];
        nextImg.center = CGPointMake(160.0f, textLabel.frame.origin.y+textLabel.frame.size.height+17.0f+nextImg.frame.size.height*0.5);
        [registView addSubview:nextImg];
        [nextImg release];
        
        frame_y = nextImg.frame.origin.y + nextImg.frame.size.height + 17.f;
    }
    [imgArr release];
    [textArr release];
    
    UIView *leftView = [[UIView alloc] initWithFrame:CGRectMake(0.0f, logoBtn.frame.origin.y+logoBtn.frame.size.height, 320.0f, [UIScreen mainScreen].applicationFrame.size.height-44.0f)];
    logoView = leftView;
    leftView.alpha = 1.0f;
    leftView.backgroundColor = [UIColor clearColor];
    [self.view addSubview:leftView];
    [leftView release];
    
    UIImageView *mailImg = [[UIImageView alloc] initWithImage:[UIImage imageNamed:@"new80.png"]];
    mailImg.center = CGPointMake(11.0f+22.0f, 11.0f+22.0f);
    [logoView addSubview:mailImg];
    [mailImg release];
    
    UIView *mailBackview = [[UIView alloc] initWithFrame:CGRectMake(mailImg.frame.origin.x+mailImg.frame.size.width, mailImg.frame.origin.y, 320.0f-mailImg.frame.origin.x-mailImg.frame.size.width-11.0f, mailImg.frame.size.height)];
    mailBackview.backgroundColor = [UIColor colorWithRed:.0f green:.0f blue:0.0f alpha:.3];
    [logoView addSubview:mailBackview];
    [mailBackview release];
    
    UITextField *textfield1 = [[UITextField alloc] initWithFrame:CGRectMake(6.0f, 0.0f, mailBackview.frame.size.width-12.0f, mailBackview.frame.size.height)];
    mailTextField = textfield1;
    textfield1.textColor = [UIColor whiteColor];
    textfield1.keyboardAppearance = UIKeyboardAppearanceAlert;
    textfield1.keyboardType = UIKeyboardTypeEmailAddress;
    textfield1.contentVerticalAlignment = UIControlContentVerticalAlignmentCenter;
    textfield1.placeholder = @"请输入云通讯注册邮箱";
    if ([[[UIDevice currentDevice] systemVersion] floatValue] >= 7)
        [textfield1 setValue:[UIColor grayColor] forKeyPath:@"_placeholderLabel.textColor"];
    [mailBackview addSubview:textfield1];
    [textfield1 release];  
    
    
    UIImageView *pwdImg = [[UIImageView alloc] initWithImage:[UIImage imageNamed:@"new82.png"]];
    pwdImg.center = CGPointMake(11.0f+22.0f, mailImg.frame.origin.y+mailImg.frame.size.height+1.0f+22.0f);
    [logoView addSubview:pwdImg];
    [pwdImg release];
    
    UIView *pwdBackview = [[UIView alloc] initWithFrame:CGRectMake(pwdImg.frame.origin.x+pwdImg.frame.size.width, pwdImg.frame.origin.y, 320.0f-pwdImg.frame.origin.x-pwdImg.frame.size.width-11.0f, pwdImg.frame.size.height)];
    pwdBackview.backgroundColor = [UIColor colorWithRed:0.0f green:0.0f blue:0.0f alpha:0.7f];
    [logoView addSubview:pwdBackview];
    [pwdBackview release];
    
    UITextField *textfield2 = [[UITextField alloc] initWithFrame:CGRectMake(6.0f, 0.0f, pwdBackview.frame.size.width-12.0f, pwdBackview.frame.size.height)];
    pwdTextField = textfield2;
    textfield2.textColor = [UIColor whiteColor];
    textfield2.secureTextEntry = YES;
    textfield2.keyboardAppearance = UIKeyboardAppearanceAlert;
    textfield2.keyboardType = UIKeyboardTypeASCIICapable;
    textfield2.contentVerticalAlignment = UIControlContentVerticalAlignmentCenter;
    textfield2.placeholder = @"请输入云通讯密码";
    if ([[[UIDevice currentDevice] systemVersion] floatValue] >= 7)
        [textfield2 setValue:[UIColor grayColor] forKeyPath:@"_placeholderLabel.textColor"];
    [pwdBackview addSubview:textfield2];
    [textfield2 release];
    
    UIButton *nextBtn = [UIButton buttonWithType:UIButtonTypeCustom];
    nextBtn.frame = CGRectMake(11.0f, pwdImg.frame.origin.y+pwdImg.frame.size.height+11.0f, 298.0f, 44.0f);
    [nextBtn setImage:[UIImage imageNamed:@"new135.png"] forState:UIControlStateNormal];
    [nextBtn setImage:[UIImage imageNamed:@"new135_on.png"] forState:UIControlStateHighlighted];
    [nextBtn addTarget:self action:@selector(nextBtnTouch:) forControlEvents:UIControlEventTouchUpInside];
    [logoView addSubview:nextBtn];
    
    UIButton *forgetPwdBtn = [UIButton buttonWithType:UIButtonTypeCustom];
    CGFloat forgetPwdBtn_w = 73.0f;
    forgetPwdBtn.titleLabel.font = [UIFont systemFontOfSize:15.0f];
    forgetPwdBtn.frame = CGRectMake(320.0f-11.0f-forgetPwdBtn_w, nextBtn.frame.origin.y+nextBtn.frame.size.height+11.0f, forgetPwdBtn_w, 17.0f);
    forgetPwdBtn.backgroundColor = [UIColor clearColor];
    [forgetPwdBtn setTitle:@"忘记密码?" forState:UIControlStateNormal];
    [forgetPwdBtn setTitleColor:[UIColor whiteColor] forState:UIControlStateNormal];
    [forgetPwdBtn addTarget:self action:@selector(forgetPwdBtnTouch:) forControlEvents:UIControlEventTouchUpInside];
    [logoView addSubview:forgetPwdBtn];
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

- (void)logoBtnTouch:(id)sender
{
    logoBtn.enabled = NO;
    logoBtn.backgroundColor = [UIColor clearColor];
    [logoBtn setTitleColor:[UIColor whiteColor] forState:UIControlStateNormal];
    registBtn.enabled = YES;
    registBtn.backgroundColor = [UIColor colorWithRed:0.0f green:0.0f blue:0.0f alpha:0.5f];
    [registBtn setTitleColor:[UIColor colorWithRed:136.0f/255.0f green:136.0f/255.0f blue:136.0f/255.0f alpha:1.0f] forState:UIControlStateNormal];
    
    registView.alpha = 0.0f;
    logoView.alpha = 1.0f;
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


- (void)registBtnTouch:(id)sender
{
    registBtn.enabled = NO;
    registBtn.backgroundColor = [UIColor clearColor];
    [registBtn setTitleColor:[UIColor whiteColor] forState:UIControlStateNormal];
    logoBtn.enabled = YES;
    logoBtn.backgroundColor = [UIColor colorWithRed:0.0f green:0.0f blue:0.0f alpha:0.5f];
    [logoBtn setTitleColor:[UIColor colorWithRed:136.0f/255.0f green:136.0f/255.0f blue:136.0f/255.0f alpha:1.0f] forState:UIControlStateNormal];
    
    registView.alpha = 1.0f;
    logoView.alpha = 0.0f;
    
    [mailTextField resignFirstResponder];
    [pwdTextField resignFirstResponder];
}

- (void)nextBtnTouch:(id)sender
{
    if ( [mailTextField.text length] <=0 ) {
        [self popPromptViewWithMsg:@"请输入用户名！"];
    }
    if ( [pwdTextField.text length] <=0 ) {
        [self popPromptViewWithMsg:@"请输入密码！"];
    }
    [self displayProgressingView];
    self.modelEngineVoip.serverIP = @"sandboxapp.cloopen.com";
    self.modelEngineVoip.serverPort = 8883;
    self.modelEngineVoip.developerUserName = mailTextField.text;
    self.modelEngineVoip.developerUserPasswd = pwdTextField.text;
    [self.modelEngineVoip getDemoAccountsWithUserName:mailTextField.text andUserPwd:pwdTextField.text];
}

- (void)forgetPwdBtnTouch:(id)sender
{
    ForgetPwdHelpViewController *view = [[ForgetPwdHelpViewController alloc] init];
    [self.navigationController pushViewController:view animated:YES];
    [view release];
}

- (void)hidekeyboard:(id)sender
{
    [mailTextField resignFirstResponder];
    [pwdTextField resignFirstResponder];
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
        [self popPromptViewWithMsg:[NSString stringWithFormat:@"错误码：=%d,错误详情：%@",reason.reason,reason.msg]];
    }
}
@end
