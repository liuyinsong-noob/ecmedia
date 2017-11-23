#import "ECImageFilter.h"

@interface ECImageExposureFilter : ECImageFilter
{
    GLint exposureUniform;
}

// Exposure ranges from -10.0 to 10.0, with 0.0 as the normal level
@property(readwrite, nonatomic) CGFloat exposure; 

@end
