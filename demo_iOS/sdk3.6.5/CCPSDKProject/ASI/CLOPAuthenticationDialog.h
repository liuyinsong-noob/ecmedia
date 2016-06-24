//
//  CLOPAuthenticationDialog.h
//  Part of CLOPHTTPRequest -> http://allseeing-i.com/CLOPHTTPRequest
//
//  Created by Ben Copsey on 21/08/2009.
//  Copyright 2009 All-Seeing Interactive. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
@class CLOPHTTPRequest;

typedef enum _CLOPAuthenticationType {
	CLOPStandardAuthenticationType = 0,
    CLOPProxyAuthenticationType = 1
} CLOPAuthenticationType;

@interface CLOPAutorotatingViewController : UIViewController
@end

@interface CLOPAuthenticationDialog : CLOPAutorotatingViewController <UIActionSheetDelegate, UITableViewDelegate, UITableViewDataSource> {
	CLOPHTTPRequest *request;
	CLOPAuthenticationType type;
	UITableView *tableView;
	UIViewController *presentingController;
	BOOL didEnableRotationNotifications;
}
+ (void)presentAuthenticationDialogForRequest:(CLOPHTTPRequest *)request;
+ (void)dismiss;

@property (retain) CLOPHTTPRequest *request;
@property (assign) CLOPAuthenticationType type;
@property (assign) BOOL didEnableRotationNotifications;
@property (retain, nonatomic) UIViewController *presentingController;
@end
