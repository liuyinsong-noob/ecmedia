#import "ECImageFilter.h"

@interface ECImageColorPackingFilter : ECImageFilter
{
    GLint texelWidthUniform, texelHeightUniform;
    
    CGFloat texelWidth, texelHeight;
}

@end
