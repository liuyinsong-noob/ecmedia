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

#import "AppDelegate.h"

#import "ModelSelectViewController.h"
#import "ModelEngineVoip.h"
#import "VoipIncomingViewController.h"
#if 0
extern char *globalFilePathcapture;

extern char *filename_path;
extern char *filenameRender_path;
extern char *file_opus;
extern char *neteq_debug;

extern char *g_h264file;

extern char *g_render_noopengl;
extern char *g_opengles_display;
#endif
@implementation AppDelegate
@synthesize modeEngineVoip;

- (void)dealloc
{
    [_window release];
    self.modeEngineVoip = nil;
    [[NSNotificationCenter defaultCenter] removeObserver:self name:@"addressbookChanged" object:nil];
    [super dealloc];
}

//-(void) redirectConsoleLogToDocumentFolder
//{
//    NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
//    NSString *documentsDirectory = [paths objectAtIndex:0];
//    NSString *logPath = [documentsDirectory stringByAppendingPathComponent:@"console_log.txt"];
//    freopen([logPath cStringUsingEncoding:NSASCIIStringEncoding],"a+",stderr);
//}

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions
{
    self.window = [[[UIWindow alloc] initWithFrame:[[UIScreen mainScreen] bounds]] autorelease];
    
    //初始化Voip SDK接口，分配资源
    self.modeEngineVoip = [ModelEngineVoip getInstance];
    
    //首先启动注册页面
    ModelSelectViewController *registerViewController = [[[ModelSelectViewController alloc] init] autorelease];
    UINavigationController *navigationBar = [[UINavigationController alloc] initWithRootViewController:registerViewController];
    CGFloat navBarHeight = 55.0f;
    CGRect frame = CGRectMake(0.0f, 20.0f, 320.0f, navBarHeight);
    [navigationBar.navigationBar setFrame:frame];
    self.window.rootViewController = navigationBar;
    [navigationBar release];
    [self.window makeKeyAndVisible];
    //[self redirectConsoleLogToDocumentFolder];//日志重定向，开启的话，会把nslog输出到文件
    
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(addressbookChangeCallback:)
                                                 name:@"addressbookChanged"
                                               object:nil];
    

    
    NSArray *path = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    NSString *doc_path = [path objectAtIndex:0];
    
//    char c1 = 0x0;
//    char c2 = 0x1;
//    char c3 = 0xf7;
//    char c4 = 0x13;
//    
//    unsigned int  final = (c1<<24)|(c2<<16)| (c3<<8)|c4;
    
    short x = 0xa1a1;
    NSLog(@"%d %u",(x<<1), (x<<1));
    unsigned int  xx = (x<<1);
    NSLog(@"xx %u", xx);
    NSLog(@"haha");
    
    
#if 0
    size_t len = doc_path.length;
    
    filename_path = (char *)malloc(len+1);//new char[len + 1];
    memcpy(filename_path, [doc_path UTF8String], len);
    filename_path[len] = '\0';
    
    filenameRender_path = (char *)malloc(len+1);
    memcpy(filenameRender_path, [doc_path UTF8String], len);
    filenameRender_path[len] = '\0';
    
    
    
    
    
//    NSString* _filename4 = [doc_path stringByAppendingPathComponent:@"encoded_.opus"];
//    size_t len4 = _filename4.length;
//    
//    file_opus = (char *)malloc(len4+1);//new char[len + 1];
//    memcpy(file_opus, [_filename4 UTF8String], len4);
//    file_opus[len4] = '\0';
//    
//    NSString* _filename5 = [doc_path stringByAppendingPathComponent:@"neteq_debug_.opus"];
//    size_t len5 = _filename5.length;
//    
//    neteq_debug = (char *)malloc(len5+1);//new char[len + 1];
//    memcpy(neteq_debug, [_filename5 UTF8String], len5);
//    neteq_debug[len5] = '\0';
    
//    globalFilePathcapture = (char *)malloc(len+1);//new char[len + 1];
//    memcpy(globalFilePathcapture, [_filename UTF8String], len);
//    globalFilePathcapture[len] = '\0';
#endif
    
    
    
//    NSString* _filename5 = [doc_path stringByAppendingPathComponent:@"debug.yuv"];
//    size_t len5 = _filename5.length;
//    
//    g_opengles_display = (char *)malloc(len5+1);//new char[len + 1];
//    memcpy(g_opengles_display, [_filename5 UTF8String], len5);
//    g_opengles_display[len5] = '\0';
    
    return YES;
}


- (void)applicationDidEnterBackground:(UIApplication *)application
{
    contactOptState = -1;//通讯录如果改变则重新建立拼音索引
    /*
     Use this method to release shared resources, save user data, invalidate timers, and store enough application state information to restore your application to its current state in case it is terminated later.
     If your application supports background execution, this method is called instead of applicationWillTerminate: when the user quits.
     */
}

-(void)logout
{
    UINavigationController *navigationBar = (UINavigationController *)self.window.rootViewController;
    [navigationBar setNavigationBarHidden:YES animated:NO];
    [navigationBar popToRootViewControllerAnimated:YES];
    [navigationBar setNavigationBarHidden:YES animated:NO];
}


- (void)applicationWillResignActive:(UIApplication *)application
{
    [self.modeEngineVoip stopUdpTest];
    [self.modeEngineVoip stopCurRecording];
    self.modeEngineVoip.appIsActive = NO;
    // Sent when the application is about to move ·from active to inactive state. This can occur for certain types of temporary interruptions (such as an incoming phone call or SMS message) or when the user quits the application and it begins the transition to the background state.
    // Use this method to pause ongoing tasks, disable timers, and throttle down OpenGL ES frame rates. Games should use this method to pause the game.
}


- (void)applicationWillEnterForeground:(UIApplication *)application
{
    // Called as part of the transition from the background to the inactive state; here you can undo many of the changes made on entering the background.
}

- (void)applicationDidBecomeActive:(UIApplication *)application
{
    self.modeEngineVoip.appIsActive = YES;
    // Restart any tasks that were paused (or not yet started) while the application was inactive. If the application was previously in the background, optionally refresh the user interface.
}

- (void)applicationWillTerminate:(UIApplication *)application
{
    // Called when the application is about to terminate. Save data if appropriate. See also applicationDidEnterBackground:.
}

- (void)application:(UIApplication *)application didReceiveLocalNotification:(UILocalNotification *)notification
{
    NSString *callID = [notification.userInfo objectForKey:KEY_CALLID];
    NSString *type = [notification.userInfo objectForKey:KEY_TYPE];
    NSString *call = [notification.userInfo objectForKey:KEY_CALL_TYPE];
    NSString *caller = [notification.userInfo objectForKey:KEY_CALLNUMBER];
    NSInteger calltype = call.integerValue;
    if ([type isEqualToString:@"comingCall"])
    {
        UIApplication *uiapp = [UIApplication sharedApplication];
        NSArray *localNotiArray = [uiapp scheduledLocalNotifications];
        for (UILocalNotification *notification in localNotiArray)
        {
            NSDictionary *dic = [notification userInfo];
            NSString *value = [dic objectForKey:KEY_TYPE];
            if ([value isEqualToString:@"comingCall"] || [value isEqualToString:@"releaseCall"])
            {
                [uiapp cancelLocalNotification:notification];
            }
        }
        if (self.modeEngineVoip.UIDelegate && [self.modeEngineVoip.UIDelegate respondsToSelector:@selector(incomingCallID:caller:phone:name:callStatus:callType:)])
        {
            [self.modeEngineVoip.UIDelegate incomingCallID:callID caller:caller phone:[notification.userInfo objectForKey:KEY_CALLERPHONE] name:[notification.userInfo objectForKey:KEY_CALLERNAME] callStatus:IncomingCallStatus_accepting callType:calltype];
        }
    }
}

- (void)addressbookChangeCallback:(NSNotification *)_notification
{
    globalcontactsChanged = 1;
    [[NSNotificationCenter defaultCenter] postNotificationName:@"contactsChanged" object:nil userInfo:nil];
}

-(void)printLog:(NSString*)log
{
    NSLog(@"%@",log); //用于xcode日志输出
}
@end
