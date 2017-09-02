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

#import <UIKit/UIKit.h>
#import "UIBaseViewController.h"

@interface ViewController : UIBaseViewController<UITextFieldDelegate,UIScrollViewDelegate>
{

}
@property (nonatomic,retain) UITextField *tf_Account;
@property (nonatomic,retain) NSString* voipAccount;
@property (nonatomic,retain) UIScrollView *scrollView;
@property (nonatomic,retain) UITextField *pitch;
@property (nonatomic,retain) UITextField *tempo;
@property (nonatomic,retain) UITextField *rate;
@property (nonatomic,retain) UISwitch *magicSwitch;
@end
