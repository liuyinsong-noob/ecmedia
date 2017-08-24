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

#import "RegisterViewController.h"
#import "DemoListViewController.h"
#import "AccountInfo.h"
#define TEXTFILED_BASE_TAG 2000


@interface RegisterViewController ()
{
    
}


- (void)registerVoip:(id)sender;
- (void)keyboardHide;
- (UILabel *)displayText:(NSString*)text andFrame:(CGRect)rect;
@end

@implementation RegisterViewController {
    UITextField *loginIP_textField;
    UITextField *loginPort_textField;
    UITextField *loginPsswd_textField;
}

@synthesize serverip;
@synthesize serverport;


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
        
    }
    return self;
}

-(void)loadView
{
    self.title = @"开发模式登录";
    self.view = [[[UIView alloc] initWithFrame:[UIScreen mainScreen].applicationFrame] autorelease];
    self.view.backgroundColor = VIEW_BACKGROUND_COLOR_FIRSTVIEW;
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
    
    UIBarButtonItem *leftBarItem = [[UIBarButtonItem alloc] initWithCustomView:[CommonTools navigationBackItemBtnInitWithTarget:self action:@selector(popToPreView)]];
    self.navigationItem.leftBarButtonItem = leftBarItem;
    [leftBarItem release];
    
    NSString* fileStr = Nil;
    if (IPHONE5)
    {
        fileStr = @"videoConfBg_1136.png";
    }
    else
    {
        fileStr = @"videoConfBg.png";
    }
    
    UIButton *bgBtn = [[UIButton alloc] initWithFrame:CGRectMake(0, 0, 320.0, self.view.frame.size.height)];
    [bgBtn setImage:[UIImage imageNamed:fileStr] forState:UIControlStateNormal];
    [bgBtn setImage:[UIImage imageNamed:fileStr] forState:UIControlStateHighlighted];
    [bgBtn addTarget:self action:@selector(keyboardHide) forControlEvents:UIControlEventTouchUpInside];
    [self.view addSubview:bgBtn];
    [bgBtn release];
    
    //主账号信息布局
    UILabel *mainLabel = [[UILabel alloc] initWithFrame:CGRectMake(11.0f, 11.0f, 100.0f, 20.0f)];
    mainLabel.text = @"主账号：";
    mainLabel.font = [UIFont systemFontOfSize:16.0f];
    mainLabel.backgroundColor = [UIColor clearColor];
    mainLabel.textColor = [UIColor whiteColor];
    [self.view addSubview:mainLabel];
    [mainLabel release];

    UIImageView *mainImgViewBG1 = [[UIImageView alloc] initWithImage:[UIImage imageNamed:@"new126.png"]];
    mainImgViewBG1.frame = CGRectMake(11.0F, 35.0f, 298, 44);
    [self.view addSubview:mainImgViewBG1];
    [mainImgViewBG1 release];
    
    UIImageView *mainImgViewBG2 = [[UIImageView alloc] initWithImage:[UIImage imageNamed:@"new126.png"]];
    mainImgViewBG2.frame = CGRectMake(11.0F, 35.0f+1+44, 298, 44);
    [self.view addSubview:mainImgViewBG2];
    [mainImgViewBG2 release];
    
    UIImageView *mainImgView1 = [[UIImageView alloc] initWithImage:[UIImage imageNamed:@"new98.png"]];
    mainImgView1.frame = CGRectMake(11.0F, 35.0f, 44, 44);
    [self.view addSubview:mainImgView1];
    [mainImgView1 release];
    
    UIImageView *mainImgView2 = [[UIImageView alloc] initWithImage:[UIImage imageNamed:@"new100.png"]];
    mainImgView2.frame = CGRectMake(11.0F, 35.0f+1+44, 44, 44);
    [self.view addSubview:mainImgView2];
    [mainImgView2 release];
    
    
    UITextField *mainAccountTextField = [[UITextField alloc] initWithFrame:CGRectMake(61.0f, 38.0f, 265.0f, 39.0f)];
    mainAccountTextField.contentVerticalAlignment = UIControlContentVerticalAlignmentCenter;
    [self.view addSubview:mainAccountTextField];
    mainAccountTextField.placeholder = @"请输入主账号";
    mainAccountTextField.delegate = self;
    mainAccountTextField.font = [UIFont systemFontOfSize:14];
    mainAccountTextField.textColor = [UIColor whiteColor];
    mainAccountTextField.tag = TEXTFILED_BASE_TAG;
    [mainAccountTextField release];
    
    UITextField *mainTokenTextField = [[UITextField alloc] initWithFrame:CGRectMake(61.0f, 83, 265.0f, 39.0f)];
    mainTokenTextField.contentVerticalAlignment = UIControlContentVerticalAlignmentCenter;
    [self.view addSubview:mainTokenTextField];
    mainTokenTextField.placeholder = @"请输入主账号令牌";
    mainTokenTextField.delegate = self;
    mainTokenTextField.font = [UIFont systemFontOfSize:14];
    mainTokenTextField.textColor = [UIColor whiteColor];
    mainTokenTextField.tag = TEXTFILED_BASE_TAG + 1;
    [mainTokenTextField release];
    
    
    //子账号信息布局
    UILabel *subLabel = [[UILabel alloc] initWithFrame:CGRectMake(11.0f, 135.0f, 200.0f, 20.0f)];
    subLabel.text = @"请选择子账号：";
    subLabel.font = [UIFont systemFontOfSize:16.0f];
    subLabel.backgroundColor = [UIColor clearColor];
    subLabel.textColor = [UIColor whiteColor];
    [self.view addSubview:subLabel];
    [subLabel release];
    
    UIButton *selectBtn = [UIButton buttonWithType:UIButtonTypeCustom];
    [self.view addSubview:selectBtn];
    selectBtn.tag = TEXTFILED_BASE_TAG + 4;
    selectBtn.frame = CGRectMake(11, 160, 298, 44);
    [selectBtn addTarget:self action:@selector(selectVoip:) forControlEvents:UIControlEventTouchUpInside];
    [selectBtn setBackgroundImage:[UIImage imageNamed:@"new126.png"] forState:UIControlStateNormal];
    [selectBtn setBackgroundImage:[UIImage imageNamed:@"new126_on.png"] forState:UIControlStateSelected];
    [selectBtn setTitle:@"单击选择VoIP账号" forState:UIControlStateNormal];
    [selectBtn setTitleColor:[UIColor whiteColor] forState:UIControlStateNormal];
    selectBtn.titleLabel.font = [UIFont systemFontOfSize:17.0f];
    
    
    UIImageView *mainImgViewBG3 = [[UIImageView alloc] initWithImage:[UIImage imageNamed:@"new126.png"]];
    mainImgViewBG3.frame = CGRectMake(11.0F, 160+1+44, 298, 44);
    [self.view addSubview:mainImgViewBG3];
    [mainImgViewBG3 release];
    
    UIImageView *mainImgViewBG4 = [[UIImageView alloc] initWithImage:[UIImage imageNamed:@"new126.png"]];
    mainImgViewBG4.frame = CGRectMake(11.0F, 160+1+44+44, 298, 44);
    [self.view addSubview:mainImgViewBG4];
    [mainImgViewBG4 release];
    
    
    UILabel *selectLabel = [[UILabel alloc] initWithFrame:CGRectMake(17.0f, 208.0f, 250.0f, 20.0f)];
    selectLabel.text = @"请输入登录IP, 端口, 密码信息:";
    selectLabel.font = [UIFont systemFontOfSize:14.0f];
    selectLabel.backgroundColor = [UIColor clearColor];
    selectLabel.textColor = [UIColor colorWithRed:204./255 green:204./255 blue:204./255 alpha:1.];
    [self.view addSubview:selectLabel];
    [selectLabel release];
 
    // config login IP address
    NSUserDefaults* loginDefault = [NSUserDefaults standardUserDefaults];
    loginIP_textField = [[UITextField alloc]initWithFrame:CGRectMake(20.0f, 235.0f, 120.0f, 30.0f)];
    loginIP_textField.delegate = self;
    loginIP_textField.borderStyle = UITextBorderStyleLine;
    loginIP_textField.layer.borderColor= [UIColor grayColor].CGColor;
    loginIP_textField.layer.borderWidth= 1.0f;
    loginIP_textField.textColor = [UIColor whiteColor];
    loginIP_textField.font = [UIFont systemFontOfSize:13.0f];
    loginIP_textField.backgroundColor = [UIColor clearColor];
    loginIP_textField.textAlignment = NSTextAlignmentCenter;
    NSString *loginIP = [loginDefault objectForKey:@"loginIP"];
    if(loginIP == nil) {
        loginIP_textField.text = @"192.168.177.186";
       // loginIP_textField.text = @"114.215.241.17";
    } else {
        loginIP_textField.text = loginIP;
    }
    [self.view addSubview:loginIP_textField];
    [loginIP_textField release];
    
    // config login port
    loginPort_textField = [[UITextField alloc]initWithFrame:CGRectMake(145.0f, 235.0f, 60.0f, 30.0f)];
    loginPort_textField.delegate = self;
    loginPort_textField.textColor = [UIColor whiteColor];
    loginPort_textField.borderStyle = UITextBorderStyleLine;
    loginPort_textField.layer.borderColor= [UIColor grayColor].CGColor;
    loginPort_textField.layer.borderWidth= 1.0f;
    loginPort_textField.font = [UIFont systemFontOfSize:13.0f];
    loginPort_textField.backgroundColor = [UIColor clearColor];
    loginPort_textField.textAlignment = NSTextAlignmentCenter;
    NSString *loginPort = [loginDefault objectForKey:@"loginPort"];
    if(loginPort == nil) {
        loginPort_textField.text = @"7600";
        //loginPort_textField.text = @"9800";
    } else {
        loginPort_textField.text = loginPort;
    }
    [self.view addSubview:loginPort_textField];
    [loginPort_textField release];
    
    // config login password info
    loginPsswd_textField = [[UITextField alloc]initWithFrame:CGRectMake(210.0f, 235.0f, 90.0f, 30.0f)];
    loginPsswd_textField.delegate = self;
    loginPsswd_textField.textAlignment = NSTextAlignmentCenter;
    loginPsswd_textField.textColor = [UIColor whiteColor];
    loginPsswd_textField.borderStyle = UITextBorderStyleLine;
    loginPsswd_textField.layer.borderColor= [UIColor grayColor].CGColor;
    loginPsswd_textField.layer.borderWidth= 1.0f;
    loginPsswd_textField.font = [UIFont systemFontOfSize:13.0f];
    loginPsswd_textField.backgroundColor = [UIColor clearColor];
    NSString *loginPsswd = [loginDefault objectForKey:@"loginPssWD"];
    if(loginPsswd == nil) {
        loginPsswd_textField.text = @"123456";
    } else {
    //   loginPsswd_textField.text = loginPsswd;
    loginPsswd_textField.text = @"123456";
    }
 
    [self.view addSubview:loginPsswd_textField];
    [loginPsswd_textField release];

    // login button
    UIButton *registBtn = [UIButton buttonWithType:UIButtonTypeCustom];
    [self.view addSubview:registBtn];
    registBtn.frame = CGRectMake(11, self.view.frame.size.height - 170, 298, 44);
    [registBtn addTarget:self action:@selector(registerVoip:) forControlEvents:UIControlEventTouchUpInside];
    [registBtn setBackgroundImage:[UIImage imageNamed:@"new158.png"] forState:UIControlStateNormal];
    [registBtn setBackgroundImage:[UIImage imageNamed:@"new158_on.png"] forState:UIControlStateHighlighted];
    
    UILabel *textLabel = [[UILabel alloc] initWithFrame:CGRectMake(20.0f, self.view.frame.size.height - 105, 280, 40)];
    textLabel.text = @"账号不可直接输入，配置方法请查看开发文档。请选择子账号登陆后即可体验相关能力。";
    textLabel.backgroundColor = [UIColor clearColor];
    textLabel.font = [UIFont systemFontOfSize:12.0f];
    textLabel.textAlignment = NSTextAlignmentLeft;
    [self.view addSubview:textLabel];
    textLabel.numberOfLines = 0;
    textLabel.lineBreakMode = UILineBreakModeCharacterWrap;
    textLabel.textColor = [UIColor colorWithRed:204./255 green:204./255 blue:204./255 alpha:1.];
    [textLabel release];
}

-(void)textFieldDidEndEditing:(UITextField *)textField {
    NSUserDefaults* loginDefault = [NSUserDefaults standardUserDefaults];
    if (textField == loginIP_textField) {
        [loginDefault setObject:textField.text forKey:@"loginIP"];
    } else if(textField == loginPort_textField) {
        [loginDefault setObject:textField.text forKey:@"loginPort"];
    } else if(textField == loginPsswd_textField) {
        [loginDefault setObject:textField.text forKey:@"loginPssWD"];
    } else {
        //todo: something
    }
}

- (void)viewDidLoad
{
    [super viewDidLoad];
    // Do any additional setup after loading the view from its nib.
}
- (void)viewWillAppear:(BOOL)animated
{
    [super viewWillAppear:animated];
    
    //用于设置消息返回的代理
    [self.modelEngineVoip setModalEngineDelegate:self];
    
    NSString *defaultFilePath = [[[NSBundle mainBundle] resourcePath] stringByAppendingPathComponent:CONFIG_FILE_NAME];
    NSString *filePath = defaultFilePath;
    NSString *documentsDirectory = [NSString stringWithFormat:@"%@/Documents/", NSHomeDirectory()];
    NSFileManager *fileManager = [NSFileManager defaultManager];
    NSError *error;
    NSString *writablePath = [documentsDirectory stringByAppendingPathComponent:CONFIG_FILE_NAME];
    BOOL success = [fileManager fileExistsAtPath:writablePath];
    
    if (!success)
    {
        success = [fileManager copyItemAtPath:defaultFilePath toPath:writablePath error:&error];
        if (!success)
        {
            NSAssert1(0, @"Failed to create writable database file with message '%@'.", [error localizedDescription]);
        }
    }
    filePath = writablePath;
    
    //读取配置文件
    NSDictionary *configDic = [[NSDictionary alloc] initWithContentsOfFile:filePath];
    NSArray *keysArr = [configDic allKeys];
    for (NSString *key in keysArr)
    {
        if ([key isEqualToString:CONFIG_KEY_SERVERIP]) {
            self.serverip = [configDic objectForKey:key];
        }
        else if ([key isEqualToString:CONFIG_KEY_SERVERPORT]) {
            self.serverport = [configDic objectForKey:key];
        }
        else if ([key isEqualToString:CONFIG_KEY_APPID])
            self.modelEngineVoip.appID = [configDic objectForKey:key];
        else
        {
            if ([key isEqualToString:CONFIG_KEY_MAINACCOUNT])
            {
                self.modelEngineVoip.mainAccount = [configDic objectForKey:key];
                UITextField *textField = (UITextField*)[self.view viewWithTag:TEXTFILED_BASE_TAG];
                textField.text = [configDic objectForKey:key];
            }
            else if ([key isEqualToString:CONFIG_KEY_MAINTOKEN])
            {
                self.modelEngineVoip.mainToken = [configDic objectForKey:key];
                UITextField *textField = (UITextField*)[self.view viewWithTag:TEXTFILED_BASE_TAG+1];
                textField.text = [configDic objectForKey:key];
            }
        }
    }
    
    //如果在配置文件里没有配置IP则设置默认REST服务器地址
    if ([self.serverip length]<=0)
    {
        self.serverip = VOIP_SERVICEIP;
    }
    //如果在配置文件里没有配置REST端口号则设置默认REST服务器端口号
    if ([self.serverport length]<=0)
    {
        self.serverport = VOIP_SERVICEPORT;
    }
    if (!self.modelEngineVoip.accountArray)
    {
        self.modelEngineVoip.accountArray = [[[NSMutableArray alloc] init] autorelease];
        
    }
    else
        [self.modelEngineVoip.accountArray removeAllObjects];
    NSArray *accountsArray = [configDic objectForKey:CONFIG_KEY_SUBACCOUNTSINFO];
    int i = 0;
    for (NSDictionary *subaccountInfo in accountsArray)
    {
        AccountInfo *info = [[AccountInfo alloc] init];
        info.subAccount = [subaccountInfo objectForKey:CONFIG_KEY_SUBACCOUNT];
        info.subToken = [subaccountInfo objectForKey:CONFIG_KEY_SUBTOKEN];
        info.voipId = [NSString stringWithFormat:@"%d", 1000 + (++i)]; // [subaccountInfo objectForKey:CONFIG_KEY_VOIPACCOUNT];
        info.password = [subaccountInfo objectForKey:CONFIG_KEY_VOIPPASSWORD];
        [self.modelEngineVoip.accountArray addObject:info];
        [info release];
    }
    
    [configDic release];
}

- (void)viewDidAppear:(BOOL)animated
{
//    [super viewDidAppear:animated];
//    NSString *deviceStr = [self.modelEngineVoip.VoipCallService getDeviceVersion];
//    int index = 0;
//    if ([deviceStr isEqualToString:@"iPhone 7"]) {
//        index = 6;
//    } else if ([deviceStr isEqualToString:@"iPhone 6s"])
//    {
//        index = 5;
//    }
////    index = 5;
//    AccountInfo *info = [self.modelEngineVoip.accountArray objectAtIndex:index];
//    NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
//    [defaults setObject:info.voipId forKey:@"myPhoneNO"];
//    [defaults synchronize];
//    [self displayProgressingView];
//    [self.modelEngineVoip connectToCCP:self.serverip onPort:[self.serverport integerValue] withAccount:info.voipId withPsw:info.password withAccountSid:info.subAccount withAuthToken:info.subToken];
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

- (void)dealloc
{
    self.serverip = nil;
    self.serverport = nil;
    [super dealloc];
}

- (void)selectVoip:(id)sender
{
    UIActionSheet *menu;
    menu = [[UIActionSheet alloc]
            initWithTitle: @"选择账号"
            delegate:self
            cancelButtonTitle:nil
            destructiveButtonTitle:nil
            otherButtonTitles:nil];
    for (AccountInfo * info in self.modelEngineVoip.accountArray)
    {
        [menu addButtonWithTitle:info.voipId];
    }
    [menu addButtonWithTitle:@"取消"];
    [menu setCancelButtonIndex:self.modelEngineVoip.accountArray.count];
    [menu showInView:self.view.window];
    [menu release];
}

- (void)registerVoip:(id)sender
{
    NSString *str = loginIP_textField.text;
    [self keyboardHide];
    NSString *mainID = @"";
    NSString *mainToken = @"";
    NSString *subID = @"";
    NSString *subToken = @"";
    NSString *voipID = @"";
    NSString *voipPwd = @"";
    NSString *message = nil;
    for (UIView *view in self.view.subviews)
    {
        message = nil;
        if ([view isKindOfClass:[UITextField class]])
        {
            UITextField *field = (UITextField*)view;
            switch (view.tag)
            {
                case TEXTFILED_BASE_TAG:
                    mainID = field.text;
                    message = @"请输入主账号!";
                    break;
                case TEXTFILED_BASE_TAG+1:
                    mainToken = field.text;
                    message = @"请输入主账号令牌!";
                    break;
                default:
                    break;
            }
            if (field.text.length == 0)
            {
                break;
            }
        }
        
        if ([view isKindOfClass:[UILabel class]])
        {
            UILabel *label = (UILabel*)view;
            NSString * message = @"请确认已输入完整的数据!";
            switch (view.tag)
            {
                case TEXTFILED_BASE_TAG+2:
                    subID = label.text;
                    message = @"请确认子账号不为空!";
                    break;
                case TEXTFILED_BASE_TAG+3:
                    subToken = label.text;
                    message = @"请确认子账号令牌不为空!";
                    break;
                case TEXTFILED_BASE_TAG+5:
                    voipPwd = label.text;
                    message = @"请确认VoIP密码不为空!";
                    break;
                default:
                    break;
            }
            if (label.text.length == 0)
            {
                break;
            }
        }
        
        if (view.tag == TEXTFILED_BASE_TAG+4)
        {
            UIButton *btn = (UIButton*)view;
            voipID = btn.titleLabel.text;
            if ([voipID isEqualToString:@"单击选择VoIP账号"])
            {
                message = @"请选择登录的VoIP账号";
                break;
            }
        }
    }
    
    if (message.length > 0)
    {
        UIAlertView *alertView=nil;
        alertView = [[UIAlertView alloc] initWithTitle:@"提示" message:message delegate:nil cancelButtonTitle:@"确定" otherButtonTitles:nil];
        [alertView show];
        [alertView release];
        return;
    }
    
    if (self.modelEngineVoip)
    {
        [self displayProgressingView];
        [self.modelEngineVoip connectToCCP:loginIP_textField.text onPort:[loginPort_textField.text integerValue] withAccount:voipID withPsw:loginPsswd_textField.text withAccountSid:subID withAuthToken:subToken];
    }
}

-(void)keyboardHide
{
    for (UIView* view in self.view.subviews)
    {
        if ([view isKindOfClass:[UITextField class]])
        {
            [view resignFirstResponder];
        }
    }
}

- (UILabel *)displayText:(NSString*)text andFrame:(CGRect)rect
{
    UILabel *label = [[UILabel alloc] initWithFrame:rect];
    label.font = [UIFont systemFontOfSize:14.0f];
    label.textAlignment = NSTextAlignmentRight;
    label.textColor = [UIColor blackColor];
    label.backgroundColor = [UIColor clearColor];
    label.text = text;
    return [label autorelease];
}

#pragma mark - UITextFieldDelegate
- (BOOL)textFieldShouldBeginEditing:(UITextField *)textField
{
    [UIView beginAnimations:nil context:NULL];
    [UIView setAnimationCurve:UIViewAnimationCurveEaseInOut];
    if (textField.tag == 2002)
    {
        [self.view setFrame:CGRectMake(0, -15, 320, 480)];
    }
    else
    {
        [self.view setFrame:CGRectMake(0, 0, 320, 480)];
    }
    [UIView commitAnimations];
    return YES;
}

#pragma mark - actionSheet delegate
- (void)actionSheet:(UIActionSheet *)actionSheet clickedButtonAtIndex:(NSInteger)buttonIndex
{
    if ( buttonIndex == actionSheet.cancelButtonIndex  || buttonIndex == self.modelEngineVoip.accountArray.count)
    {
        return;
    }
    
    //展示选择的账号信息
    AccountInfo *info = [self.modelEngineVoip.accountArray objectAtIndex:buttonIndex];
    UILabel *textfield_a = (UILabel*)[self.view viewWithTag:TEXTFILED_BASE_TAG+2];
    textfield_a.text = info.subAccount;
    
    UILabel *textfield_b = (UILabel*)[self.view viewWithTag:TEXTFILED_BASE_TAG+3];
    textfield_b.text = info.subToken;

    UIButton *textfield_c = (UIButton*)[self.view viewWithTag:TEXTFILED_BASE_TAG+4];
    [textfield_c setTitle:info.voipId forState:UIControlStateNormal];

    UILabel *textfield_d = (UILabel*)[self.view viewWithTag:TEXTFILED_BASE_TAG+5];
    textfield_d.text = info.password;
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
        UIButton *btn = (UIButton*)[self.view viewWithTag:TEXTFILED_BASE_TAG+4];
        [self.modelEngineVoip setVoipName:btn.titleLabel.text];
        for (AccountInfo* accountinf in self.modelEngineVoip.accountArray)
        {
            if ([accountinf.voipId isEqualToString:str])
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

// 隐藏键盘
- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
    for (UIView* view in self.view.subviews)
    {
        if ([view isKindOfClass:[UITextField class]])
        {
            [view resignFirstResponder];
        }
    }
    
}

@end
