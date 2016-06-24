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
#import "VideoView.h"
#import <QuartzCore/QuartzCore.h>
@interface VideoView()


@end

@implementation VideoView

@synthesize voipLabel;
@synthesize footView;
@synthesize videoLabel;
@synthesize icon;
@synthesize ivChoose;
@synthesize bgView;
@synthesize strVoip;
@synthesize imagePath;
- (id)initWithFrame:(CGRect)frame
{
    self = [super initWithFrame:frame];
    if (self) {
        // Initialization code
        
        UIView* transparentView = [[UIView alloc] initWithFrame:CGRectMake(0, 0, frame.size.width, frame.size.height)];
        transparentView.backgroundColor = [UIColor blackColor];
        transparentView.alpha = 0.3;
        [self addSubview: transparentView];
        [transparentView release];
        
        UIView* viewbg = [[UIView alloc] initWithFrame:CGRectMake(0, 0, frame.size.width, (frame.size.width / 3)*4)];
        viewbg.backgroundColor = [UIColor clearColor];
        self.bgView = viewbg;
        [self addSubview: self.bgView];
        [viewbg release];
        
        UILabel* lb2 = [[UILabel alloc] initWithFrame:CGRectMake(0 , frame.size.height/2-6, frame.size.width, 30)];
        lb2.backgroundColor = [UIColor clearColor];
        lb2.font = [UIFont systemFontOfSize:18];
        lb2.textAlignment = NSTextAlignmentCenter;
        lb2.textColor = [UIColor whiteColor];
        lb2.text = @"";
        self.videoLabel = lb2;
        [self addSubview:self.videoLabel];
        [lb2 release];
        
        UIView* view = [[UIView alloc] initWithFrame:CGRectMake(0, frame.size.height-22, frame.size.width, 22)];
        view.backgroundColor = [UIColor blackColor];
        view.alpha = 0.3;
        self.footView = view;
        [self addSubview:self.footView];
        [view release];
        
        UILabel* lb1 = [[UILabel alloc] initWithFrame:CGRectMake(6, frame.size.height-22, frame.size.width-12, 22)];
        lb1.backgroundColor = [UIColor clearColor];
        lb1.textAlignment = NSTextAlignmentLeft;
        lb1.font = [UIFont systemFontOfSize:16];
        lb1.textColor = [UIColor whiteColor];
        self.voipLabel = lb1;
        [self addSubview:self.voipLabel];
        [lb1 release];
        
        UIImage* image = [UIImage imageNamed:@"videoConf43.png"];
        UIImageView* iv = [[UIImageView alloc] initWithFrame:CGRectMake(frame.size.width - 6 - 2, frame.size.height - 6 - 10, 2, 10)];
        iv.image = image;
        [self addSubview:iv];
        self.icon = iv;
        [iv release];
        
        UIImage* imgChoose = [UIImage imageNamed:@"videoConf27.png"];
        UIImageView* imgViewChoose = [[UIImageView alloc] initWithFrame:CGRectMake(0, 0 ,frame.size.width, frame.size.height)];
        imgViewChoose.image = imgChoose;
        [self addSubview:imgViewChoose];
        self.ivChoose = imgViewChoose;
        [imgViewChoose release];
        self.ivChoose.hidden = YES;
        
        self.backgroundColor = [UIColor clearColor];
        self.clipsToBounds = YES;
    }
    return self;
}

/*
// Only override drawRect: if you perform custom drawing.
// An empty implementation adversely affects performance during animation.
- (void)drawRect:(CGRect)rect
{
    // Drawing code
}
*/

-(void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
    if (self.strVoip)
    {
        if (self.myDelegate && [self.myDelegate respondsToSelector:@selector(onChooseIndex:andVoipAccount:)]) {
            [self.myDelegate onChooseIndex:self.tag andVoipAccount:self.strVoip];
        }
    }
}

-(void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{
    
}

- (void)setBgViewImagePath:(NSString*)imgPath
{
    if (imgPath.length > 0)
    {
        if (self.imagePath.length > 0)
        {
            if ([imgPath isEqualToString:self.imagePath])
            {
                return;
            }
            else
            {
                [[NSFileManager defaultManager] removeItemAtPath:self.imagePath error:nil];
            }
        }        
        
        UIImage *image = [[UIImage alloc] initWithContentsOfFile:imgPath];
        if (image)
        {
            self.bgView.layer.contents = (id) image.CGImage;
            self.bgView.layer.backgroundColor = [UIColor clearColor].CGColor;
        }
        [image release];
    }
    self.imagePath = imgPath;
}

-(void)dealloc
{
    self.imagePath = nil;
    self.strVoip = nil;
    self.footView = nil;
    self.voipLabel = nil;
    self.videoLabel = nil;
    self.icon = nil;
    self.bgView = nil;
    self.ivChoose = Nil;
    [super dealloc];
}
@end

@implementation MultiVideoView
@synthesize isDisplayVideo;
@synthesize originFrame;
@synthesize remoteRatioHehgth;
@synthesize remoteRatioWidth;
- (id)initWithFrame:(CGRect)frame
{
    self = [super initWithFrame:frame];
    if (self)
    {
        self.originFrame = frame;
        self.remoteRatioHehgth = 4.0f;
        self.remoteRatioWidth = 3.0f;
    }
    return self;
}

-(void)setFrame:(CGRect)frame
{
    [super setFrame:frame];
    
    CGRect bgframe = CGRectMake(0, 0, frame.size.width, (frame.size.width / 3)*4);
    if(remoteRatioHehgth/remoteRatioWidth > bgframe.size.height/bgframe.size.width)
    {
        bgframe.size.width = (remoteRatioWidth/remoteRatioHehgth)*bgframe.size.height;
    }
    else
    {
        bgframe.size.height = (remoteRatioHehgth/remoteRatioWidth)*bgframe.size.width;
    }
    self.bgView.frame = bgframe;
    
    self.videoLabel.frame = CGRectMake(0 , frame.size.height/2-6, frame.size.width, 30);
    self.footView.frame = CGRectMake(0, frame.size.height-22, frame.size.width, 22);
    self.voipLabel.frame = CGRectMake(6, frame.size.height-22, frame.size.width-12, 22);
    self.icon.frame = CGRectMake(frame.size.width - 6 - 2, frame.size.height - 6 - 10, 2, 10);
}

-(void)setVideoRatioChangedWithHeight:(CGFloat)height withWidth:(CGFloat)width
{
    self.remoteRatioWidth = width;
    self.remoteRatioHehgth = height;
    CGRect frame = self.bgView.frame;
    if(height/width > frame.size.height/frame.size.width)
    {
        frame.size.width = (width/height)*frame.size.height;
    }
    else
    {
        frame.size.height = (height/width)*frame.size.width;
    }
    self.bgView.frame = frame;
}

-(void)dealloc
{
    [super dealloc];
}
@end
