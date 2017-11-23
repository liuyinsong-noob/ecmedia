#import "ECImageFilter.h"

@interface ECImagePerlinNoiseFilter : ECImageFilter
{
    GLint scaleUniform, colorStartUniform, colorFinishUniform;
}

@property (readwrite, nonatomic) GPUVector4 colorStart;
@property (readwrite, nonatomic) GPUVector4 colorFinish;

@property (readwrite, nonatomic) float scale;

@end
