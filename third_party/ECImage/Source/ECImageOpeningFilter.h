#import "ECImageFilterGroup.h"

@class ECImageErosionFilter;
@class ECImageDilationFilter;

// A filter that first performs an erosion on the red channel of an image, followed by a dilation of the same radius. 
// This helps to filter out smaller bright elements.

@interface ECImageOpeningFilter : ECImageFilterGroup
{
    ECImageErosionFilter *erosionFilter;
    ECImageDilationFilter *dilationFilter;
}

@property(readwrite, nonatomic) CGFloat verticalTexelSpacing, horizontalTexelSpacing;

- (id)initWithRadius:(NSUInteger)radius;

@end
