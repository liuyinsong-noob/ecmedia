//
//  CLOPInputStream.h
//  Part of CLOPHTTPRequest -> http://allseeing-i.com/CLOPHTTPRequest
//
//  Created by Ben Copsey on 10/08/2009.
//  Copyright 2009 All-Seeing Interactive. All rights reserved.
//

#import <Foundation/Foundation.h>

@class CLOPHTTPRequest;

// This is a wrapper for NSInputStream that pretends to be an NSInputStream itself
// Subclassing NSInputStream seems to be tricky, and may involve overriding undocumented methods, so we'll cheat instead.
// It is used by CLOPHTTPRequest whenever we have a request body, and handles measuring and throttling the bandwidth used for uploading

@interface CLOPInputStream : NSObject {
	NSInputStream *stream;
	CLOPHTTPRequest *request;
}
+ (id)inputStreamWithFileAtPath:(NSString *)path request:(CLOPHTTPRequest *)request;
+ (id)inputStreamWithData:(NSData *)data request:(CLOPHTTPRequest *)request;

@property (retain, nonatomic) NSInputStream *stream;
@property (assign, nonatomic) CLOPHTTPRequest *request;
@end
