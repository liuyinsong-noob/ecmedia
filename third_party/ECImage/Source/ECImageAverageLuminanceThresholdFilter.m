#import "ECImageAverageLuminanceThresholdFilter.h"
#import "ECImageLuminosity.h"
#import "ECImageLuminanceThresholdFilter.h"

@interface ECImageAverageLuminanceThresholdFilter()
{
    ECImageLuminosity *luminosityFilter;
    ECImageLuminanceThresholdFilter *luminanceThresholdFilter;
}
@end

@implementation ECImageAverageLuminanceThresholdFilter

@synthesize thresholdMultiplier = _thresholdMultiplier;

#pragma mark -
#pragma mark Initialization and teardown

- (id)init;
{
    if (!(self = [super init]))
    {
		return nil;
    }
    
    self.thresholdMultiplier = 1.0;
    
    luminosityFilter = [[ECImageLuminosity alloc] init];
    [self addFilter:luminosityFilter];
    
    luminanceThresholdFilter = [[ECImageLuminanceThresholdFilter alloc] init];
    [self addFilter:luminanceThresholdFilter];
    
    __unsafe_unretained ECImageAverageLuminanceThresholdFilter *weakSelf = self;
    __unsafe_unretained ECImageLuminanceThresholdFilter *weakThreshold = luminanceThresholdFilter;
    
    [luminosityFilter setLuminosityProcessingFinishedBlock:^(CGFloat luminosity, CMTime frameTime) {
        weakThreshold.threshold = luminosity * weakSelf.thresholdMultiplier;
    }];
    
    self.initialFilters = [NSArray arrayWithObjects:luminosityFilter, luminanceThresholdFilter, nil];
    self.terminalFilter = luminanceThresholdFilter;
    
    return self;
}

@end
