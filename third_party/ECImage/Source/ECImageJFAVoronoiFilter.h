#import "ECImageFilter.h"

@interface ECImageJFAVoronoiFilter : ECImageFilter
{
    GLuint secondFilterOutputTexture;
    GLuint secondFilterFramebuffer;
    
    
    GLint sampleStepUniform;
    GLint sizeUniform;
    NSUInteger numPasses;
    
}

@property (nonatomic, readwrite) CGSize sizeInPixels;

@end