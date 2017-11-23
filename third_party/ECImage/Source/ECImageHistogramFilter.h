#import "ECImageFilter.h"

typedef enum { kECImageHistogramRed, kECImageHistogramGreen, kECImageHistogramBlue, kECImageHistogramRGB, kECImageHistogramLuminance} ECImageHistogramType;

@interface ECImageHistogramFilter : ECImageFilter
{
    ECImageHistogramType histogramType;
    
    GLubyte *vertexSamplingCoordinates;
    
    ECGLProgram *secondFilterProgram, *thirdFilterProgram;
    GLint secondFilterPositionAttribute, thirdFilterPositionAttribute;
}

// Rather than sampling every pixel, this dictates what fraction of the image is sampled. By default, this is 16 with a minimum of 1.
@property(readwrite, nonatomic) NSUInteger downsamplingFactor;

// Initialization and teardown
- (id)initWithHistogramType:(ECImageHistogramType)newHistogramType;
- (void)initializeSecondaryAttributes;

@end
