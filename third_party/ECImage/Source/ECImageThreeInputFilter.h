#import "ECImageTwoInputFilter.h"

extern NSString *const kECImageThreeInputTextureVertexShaderString;

@interface ECImageThreeInputFilter : ECImageTwoInputFilter
{
    ECImageFramebuffer *thirdInputFramebuffer;

    GLint filterThirdTextureCoordinateAttribute;
    GLint filterInputTextureUniform3;
    ECImageRotationMode inputRotation3;
    GLuint filterSourceTexture3;
    CMTime thirdFrameTime;
    
    BOOL hasSetSecondTexture, hasReceivedThirdFrame, thirdFrameWasVideo;
    BOOL thirdFrameCheckDisabled;
}

- (void)disableThirdFrameCheck;

@end
