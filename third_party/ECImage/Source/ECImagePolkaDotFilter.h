#import "ECImagePixellateFilter.h"

@interface ECImagePolkaDotFilter : ECImagePixellateFilter
{
    GLint dotScalingUniform;
}

@property(readwrite, nonatomic) CGFloat dotScaling;

@end
