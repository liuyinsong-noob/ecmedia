#import "ECImageThreeInputFilter.h"

extern NSString *const kGPUImageFourInputTextureVertexShaderString;

@interface ECImageFourInputFilter : ECImageThreeInputFilter
{
    ECImageFramebuffer *fourthInputFramebuffer;

    GLint filterFourthTextureCoordinateAttribute;
    GLint filterInputTextureUniform4;
    ECImageRotationMode inputRotation4;
    GLuint filterSourceTexture4;
    CMTime fourthFrameTime;
    
    BOOL hasSetThirdTexture, hasReceivedFourthFrame, fourthFrameWasVideo;
    BOOL fourthFrameCheckDisabled;
}

- (void)disableFourthFrameCheck;

@end
