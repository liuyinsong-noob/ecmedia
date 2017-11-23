#import "ECImageOutput.h"
#import "ECImageFilter.h"

@interface ECImageFilterGroup : ECImageOutput <ECImageInput>
{
    NSMutableArray *filters;
    BOOL isEndProcessing;
}

@property(readwrite, nonatomic, strong) ECImageOutput<ECImageInput> *terminalFilter;
@property(readwrite, nonatomic, strong) NSArray *initialFilters;
@property(readwrite, nonatomic, strong) ECImageOutput<ECImageInput> *inputFilterToIgnoreForUpdates;

// Filter management
- (void)addFilter:(ECImageOutput<ECImageInput> *)newFilter;
- (ECImageOutput<ECImageInput> *)filterAtIndex:(NSUInteger)filterIndex;
- (NSUInteger)filterCount;

@end
