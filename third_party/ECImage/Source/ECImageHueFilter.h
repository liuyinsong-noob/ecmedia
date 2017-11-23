
#import "ECImageFilter.h"

@interface ECImageHueFilter : ECImageFilter
{
    GLint hueAdjustUniform;
    
}
@property (nonatomic, readwrite) CGFloat hue;

@end
