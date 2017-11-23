#import <Foundation/Foundation.h>
#import "ECImageContext.h"

@protocol ECImageTextureOutputDelegate;

@interface ECImageTextureOutput : NSObject <ECImageInput>
{
    ECImageFramebuffer *firstInputFramebuffer;
}

@property(readwrite, unsafe_unretained, nonatomic) id<ECImageTextureOutputDelegate> delegate;
@property(readonly) GLuint texture;
@property(nonatomic) BOOL enabled;

- (void)doneWithTexture;

@end

@protocol ECImageTextureOutputDelegate
- (void)newFrameReadyFromTextureOutput:(ECImageTextureOutput *)callbackTextureOutput;
@end
