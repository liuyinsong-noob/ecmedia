#import <Foundation/Foundation.h>
#import "ECImageOutput.h"

@interface ECImageFilterPipeline : NSObject
{
    NSString *stringValue;
}

@property (strong) NSMutableArray *filters;

@property (strong) ECImageOutput *input;
@property (strong) id <ECImageInput> output;

- (id) initWithOrderedFilters:(NSArray*) filters input:(ECImageOutput*)input output:(id <ECImageInput>)output;
- (id) initWithConfiguration:(NSDictionary*) configuration input:(ECImageOutput*)input output:(id <ECImageInput>)output;
- (id) initWithConfigurationFile:(NSURL*) configuration input:(ECImageOutput*)input output:(id <ECImageInput>)output;

- (void) addFilter:(ECImageOutput<ECImageInput> *)filter;
- (void) addFilter:(ECImageOutput<ECImageInput> *)filter atIndex:(NSUInteger)insertIndex;
- (void) replaceFilterAtIndex:(NSUInteger)index withFilter:(ECImageOutput<ECImageInput> *)filter;
- (void) replaceAllFilters:(NSArray *) newFilters;
- (void) removeFilter:(ECImageOutput<ECImageInput> *)filter;
- (void) removeFilterAtIndex:(NSUInteger)index;
- (void) removeAllFilters;

- (UIImage *) currentFilteredFrame;
- (UIImage *) currentFilteredFrameWithOrientation:(UIImageOrientation)imageOrientation;
- (CGImageRef) newCGImageFromCurrentFilteredFrame;

@end
