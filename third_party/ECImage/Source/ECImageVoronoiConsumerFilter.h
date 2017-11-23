#import "ECImageTwoInputFilter.h"

@interface ECImageVoronoiConsumerFilter : ECImageTwoInputFilter
{
    GLint sizeUniform;
}

@property (nonatomic, readwrite) CGSize sizeInPixels;

@end
