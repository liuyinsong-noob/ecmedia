#import "ECImageRGBOpeningFilter.h"
#import "ECImageRGBErosionFilter.h"
#import "ECImageRGBDilationFilter.h"

@implementation ECImageRGBOpeningFilter

- (id)init;
{
    if (!(self = [self initWithRadius:1]))
    {
		return nil;
    }
    
    return self;
}

- (id)initWithRadius:(NSUInteger)radius;
{
    if (!(self = [super init]))
    {
		return nil;
    }
    
    // First pass: erosion
    erosionFilter = [[ECImageRGBErosionFilter alloc] initWithRadius:radius];
    [self addFilter:erosionFilter];
    
    // Second pass: dilation
    dilationFilter = [[ECImageRGBDilationFilter alloc] initWithRadius:radius];
    [self addFilter:dilationFilter];
    
    [erosionFilter addTarget:dilationFilter];
    
    self.initialFilters = [NSArray arrayWithObjects:erosionFilter, nil];
    self.terminalFilter = dilationFilter;
    
    return self;
}


@end
