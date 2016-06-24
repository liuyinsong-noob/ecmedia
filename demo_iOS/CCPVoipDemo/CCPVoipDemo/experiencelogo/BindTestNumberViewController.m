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

#import "BindTestNumberViewController.h"

@interface BindTestNumberViewController ()
{
    UITextField *phoneTextField;
}
@end

@implementation BindTestNumberViewController

@synthesize oldNumber;

- (id)initWithTextNumber:(NSString*) textNumber
{
    self = [super init];
    if (self)
    {
        if ([textNumber length] > 0)
        {
            self.oldNumber = textNumber;
            phoneTextField.text = textNumber;
        }
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
    statusLabel.text = @"请输入要绑定的手机或固话号码";
    statusLabel.contentMode = UIViewContentModeCenter;
    [self.view addSubview:statusLabel];
    [statusLabel release];
    
    CGFloat frame_y = statusLabel.frame.origin.y+statusLabel.frame.size.height+11.0f;
    UIImageView *phoneImg = [[UIImageView alloc] initWithImage:[UIImage imageNamed:@"new84.png"]];
    phoneImg.center = CGPointMake(11.0f+22.0f, frame_y+22.0f);
    [self.view addSubview:phoneImg];
    [phoneImg release];
    
    UIView *phoneBackview = [[UIView alloc] initWithFrame:CGRectMake(phoneImg.frame.origin.x+phoneImg.frame.size.width, phoneImg.frame.origin.y, 320.0f-phoneImg.frame.origin.x-phoneImg.frame.size.width-11.0f, phoneImg.frame.size.height)];
    phoneBackview.backgroundColor = [UIColor colorWithRed:0.0f green:0.0f blue:0.0f alpha:0.7f];
    [self.view addSubview:phoneBackview];
    [phoneBackview release];
    
    UITextField *textfield1 = [[UITextField alloc] initWithFrame:CGRectMake(6.0f, 0.0f, phoneBackview.frame.size.width-12.0f, phoneBackview.frame.size.height)];
    phoneTextField = textfield1;
    textfield1.textColor = [UIColor whiteColor];
    textfield1.keyboardAppearance = UIKeyboardAppearanceAlert;
    textfield1.keyboardType = UIKeyboardTypePhonePad;
    textfield1.contentVerticalAlignment = UIControlContentVerticalAlignmentCenter;
    textfield1.placeholder = @"固话需加区号";
    if ([[[UIDevice currentDevice] systemVersion] floatValue] >= 7)
        [textfield1 setValue:[UIColor grayColor] forKeyPath:@"_placeholderLabel.textColor"];
    [phoneBackview addSubview:textfield1];
    [textfield1 release];
    
    
    frame_y = textfield1.frame.origin.y+textfield1.frame.size.height+60.0f;
    UIButton *startButton = [[UIButton alloc] initWithFrame:CGRectMake(11.0f, frame_y, 298.0f, 44.0f)];
    [startButton setImage:[UIImage imageNamed:@"new151.png"] forState:UIControlStateNormal];
    [startButton setImage:[UIImage imageNamed:@"new151_on.png"] forState:UIControlStateHighlighted];
    [startButton addTarget:self action:@selector(VerifyMyCode:) forControlEvents:UIControlEventTouchUpInside];
    [self.view addSubview:startButton];
    [startButton release];
}

-(void)VerifyMyCode:(id)sender
{
    if ([phoneTextField.text length] <= 0)
    {
        [self popPromptViewWithMsg:@"请输入绑定的号码！"];
        return;
    }
    [phoneTextField resignFirstResponder];
    [self displayProgressingView];
    [self.modelEngineVoip editTestNumWithOldPhoneNumber:self.oldNumber andNewPhoneNumber:phoneTextField.text];
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


- (void)updateBtnDisplay:(NSTimer*)timer
{
    UIButton* btn = (UIButton*)[timer userInfo];
    btn.enabled = YES;
}

- (void) onEditTestNumWithReason:(CloopenReason *) reason
{
    [self dismissProgressingView];
    if (reason.reason == 0)
    {
        if (self.oldNumber.length == 0)
        {
            [self.modelEngineVoip.test_numberArray addObject:phoneTextField.text];
        }
        else if(phoneTextField.text.length > 0)
        {
            NSInteger index = [self.modelEngineVoip.test_numberArray indexOfObject:self.oldNumber];
            [self.modelEngineVoip.test_numberArray replaceObjectAtIndex:index withObject:phoneTextField.text];
        }
        
        [self.navigationController popViewControllerAnimated:YES];
    }
    else
    {
        [self popPromptViewWithMsg:@"绑定号码失败"];
    }
}

-(void)dealloc
{
    self.oldNumber = nil;
    [super dealloc];
}
@end
