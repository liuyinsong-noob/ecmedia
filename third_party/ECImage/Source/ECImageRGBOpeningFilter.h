#import "ECImageFilterGroup.h"

@class ECImageRGBErosionFilter;
@class ECImageRGBDilationFilter;

// A filter that first performs an erosion on each color channel of an image, followed by a dilation of the same radius. 
// This helps to filter out smaller bright elements.

@interface ECImageRGBOpeningFilter : ECImageFilterGroup
{
    ECImageRGBErosionFilter *erosionFilter;
    ECImageRGBDilationFilter *dilationFilter;
}

- (id)initWithRadius:(NSUInteger)radius;

@end
