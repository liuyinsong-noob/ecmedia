#import <Foundation/Foundation.h>
#import "ECImageContext.h"


struct GPUByteColorVector {
    GLubyte red;
    GLubyte green;
    GLubyte blue;
    GLubyte alpha;
};
typedef struct GPUByteColorVector GPUByteColorVector;

typedef void(^I420FrameAvailableBlock)(const GLubyte* outputBytes,
                                       uint8_t* bytes_y, int stride_y,
                                       uint8_t* bytes_u, int stride_u,
                                       uint8_t* bytes_v, int stride_v,
                                       NSInteger width, int height);

typedef void(^ARGBFrameAvailableBlock)(const GLubyte* outputBytes, int bytesPerRow, int width, int height);

@protocol ECImageRawDataProcessor;

#if TARGET_IPHONE_SIMULATOR || TARGET_OS_IPHONE
@interface ECImageRawDataOutput : NSObject <ECImageInput> {
    CGSize imageSize;
    ECImageRotationMode inputRotation;
    BOOL outputBGRA;
    // add by zhaoyou.
    uint8_t* _i420Bytes;
    I420FrameAvailableBlock i420FrameAvailableBlock;
    ARGBFrameAvailableBlock argbFrameAvailableBlock;
}
#else
@interface ECImageRawDataOutput : NSObject <ECImageInput> {
    CGSize imageSize;
    ECImageRotationMode inputRotation;
    BOOL outputBGRA;
    
    uint8_t* _i420Bytes;
}
#endif

@property(readonly) GLubyte *rawBytesForImage;
@property(nonatomic, copy) void(^newFrameAvailableBlock)(void);
@property(nonatomic) BOOL enabled;




// Initialization and teardown
- (id)initWithImageSize:(CGSize)newImageSize resultsInBGRAFormat:(BOOL)resultsInBGRAFormat;

// Data access
- (GPUByteColorVector)colorAtLocation:(CGPoint)locationInImage;
- (NSUInteger)bytesPerRowInOutput;

- (void)setImageSize:(CGSize)newImageSize;

- (void)lockFramebufferForReading;
- (void)unlockFramebufferAfterReading;

@end
