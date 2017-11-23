#import "ECImageFilterGroup.h"

@class ECImageRGBErosionFilter;
@class ECImageRGBDilationFilter;

// A filter that first performs a dilation on each color channel of an image, followed by an erosion of the same radius. 
// This helps to filter out smaller dark elements.

@interface ECImageRGBClosingFilter : ECImageFilterGroup
{
    ECImageRGBErosionFilter *erosionFilter;
    ECImageRGBDilationFilter *dilationFilter;
}

- (id)initWithRadius:(NSUInteger)radius;


@end
