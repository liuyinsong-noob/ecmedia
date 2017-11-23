#import <Foundation/Foundation.h>
#import <QuartzCore/QuartzCore.h>
#import "ECImageFramebuffer.h"

@interface ECImageFramebufferCache : NSObject

// Framebuffer management
- (ECImageFramebuffer *)fetchFramebufferForSize:(CGSize)framebufferSize textureOptions:(GPUTextureOptions)textureOptions onlyTexture:(BOOL)onlyTexture;
- (ECImageFramebuffer *)fetchFramebufferForSize:(CGSize)framebufferSize onlyTexture:(BOOL)onlyTexture;
- (void)returnFramebufferToCache:(ECImageFramebuffer *)framebuffer;
- (void)purgeAllUnassignedFramebuffers;
- (void)addFramebufferToActiveImageCaptureList:(ECImageFramebuffer *)framebuffer;
- (void)removeFramebufferFromActiveImageCaptureList:(ECImageFramebuffer *)framebuffer;

@end
