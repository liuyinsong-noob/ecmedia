#import "ECImageFilter.h"

@interface ECImageBuffer : ECImageFilter
{
    NSMutableArray *bufferedFramebuffers;
}

@property(readwrite, nonatomic) NSUInteger bufferSize;

@end
