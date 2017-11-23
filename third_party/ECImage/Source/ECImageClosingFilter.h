#import "ECImageFilterGroup.h"

@class ECImageErosionFilter;
@class ECImageDilationFilter;

// A filter that first performs a dilation on the red channel of an image, followed by an erosion of the same radius. 
// This helps to filter out smaller dark elements.

@interface ECImageClosingFilter : ECImageFilterGroup
{
    ECImageErosionFilter *erosionFilter;
    ECImageDilationFilter *dilationFilter;
}

@property(readwrite, nonatomic) CGFloat verticalTexelSpacing, horizontalTexelSpacing;

- (id)initWithRadius:(NSUInteger)radius;

@end
