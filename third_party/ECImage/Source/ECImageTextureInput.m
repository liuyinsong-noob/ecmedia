#import "ECImageTextureInput.h"

@implementation ECImageTextureInput

#pragma mark -
#pragma mark Initialization and teardown

- (id)initWithTexture:(GLuint)newInputTexture size:(CGSize)newTextureSize;
{
    if (!(self = [super init]))
    {
        return nil;
    }

    runSynchronouslyOnVideoProcessingQueue(^{
        [ECImageContext useImageProcessingContext];
    });
    
    textureSize = newTextureSize;

    runSynchronouslyOnVideoProcessingQueue(^{
        outputFramebuffer = [[ECImageFramebuffer alloc] initWithSize:newTextureSize overriddenTexture:newInputTexture];
    });
    
    return self;
}

#pragma mark -
#pragma mark Image rendering

- (void)processTextureWithFrameTime:(CMTime)frameTime;
{
    runAsynchronouslyOnVideoProcessingQueue(^{
        for (id<ECImageInput> currentTarget in targets)
        {
            NSInteger indexOfObject = [targets indexOfObject:currentTarget];
            NSInteger targetTextureIndex = [[targetTextureIndices objectAtIndex:indexOfObject] integerValue];
            
            [currentTarget setInputSize:textureSize atIndex:targetTextureIndex];
            [currentTarget setInputFramebuffer:outputFramebuffer atIndex:targetTextureIndex];
            [currentTarget newFrameReadyAtTime:frameTime atIndex:targetTextureIndex];
        }
    });
}

@end
