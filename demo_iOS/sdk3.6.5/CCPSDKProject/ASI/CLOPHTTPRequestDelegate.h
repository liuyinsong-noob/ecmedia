//
//  CLOPHTTPRequestDelegate.h
//  Part of CLOPHTTPRequest -> http://allseeing-i.com/CLOPHTTPRequest
//
//  Created by Ben Copsey on 13/04/2010.
//  Copyright 2010 All-Seeing Interactive. All rights reserved.
//

@class CLOPHTTPRequest;

@protocol CLOPHTTPRequestDelegate <NSObject>

@optional

// These are the default delegate methods for request status
// You can use different ones by setting didStartSelector / didFinishSelector / didFailSelector
- (void)requestStarted:(CLOPHTTPRequest *)request;
- (void)request:(CLOPHTTPRequest *)request didReceiveResponseHeaders:(NSDictionary *)responseHeaders;
- (void)request:(CLOPHTTPRequest *)request willRedirectToURL:(NSURL *)newURL;
- (void)requestFinished:(CLOPHTTPRequest *)request;
- (void)requestFailed:(CLOPHTTPRequest *)request;
- (void)requestRedirected:(CLOPHTTPRequest *)request;

// When a delegate implements this method, it is expected to process all incoming data itself
// This means that responseData / responseString / downloadDestinationPath etc are ignored
// You can have the request call a different method by setting didReceiveDataSelector
- (void)request:(CLOPHTTPRequest *)request didReceiveData:(NSData *)data;

// If a delegate implements one of these, it will be asked to supply credentials when none are available
// The delegate can then either restart the request ([request retryUsingSuppliedCredentials]) once credentials have been set
// or cancel it ([request cancelAuthentication])
- (void)authenticationNeededForRequest:(CLOPHTTPRequest *)request;
- (void)proxyAuthenticationNeededForRequest:(CLOPHTTPRequest *)request;

@end
