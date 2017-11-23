#import "ECImageFilterGroup.h"
#import "ECImagePicture.h"

@implementation ECImageFilterGroup

@synthesize terminalFilter = _terminalFilter;
@synthesize initialFilters = _initialFilters;
@synthesize inputFilterToIgnoreForUpdates = _inputFilterToIgnoreForUpdates;

- (id)init;
{
    if (!(self = [super init]))
    {
		return nil;
    }
    
    filters = [[NSMutableArray alloc] init];
    
    return self;
}

#pragma mark -
#pragma mark Filter management

- (void)addFilter:(ECImageOutput<ECImageInput> *)newFilter;
{
    [filters addObject:newFilter];
}

- (ECImageOutput<ECImageInput> *)filterAtIndex:(NSUInteger)filterIndex;
{
    return [filters objectAtIndex:filterIndex];
}

- (NSUInteger)filterCount;
{
    return [filters count];
}

#pragma mark -
#pragma mark Still image processing

- (void)useNextFrameForImageCapture;
{
    [self.terminalFilter useNextFrameForImageCapture];
}

- (CGImageRef)newCGImageFromCurrentlyProcessedOutput;
{
    return [self.terminalFilter newCGImageFromCurrentlyProcessedOutput];
}

#pragma mark -
#pragma mark ECImageOutput overrides

- (void)setTargetToIgnoreForUpdates:(id<ECImageInput>)targetToIgnoreForUpdates;
{
    [_terminalFilter setTargetToIgnoreForUpdates:targetToIgnoreForUpdates];
}

- (void)addTarget:(id<ECImageInput>)newTarget atTextureLocation:(NSInteger)textureLocation;
{
    [_terminalFilter addTarget:newTarget atTextureLocation:textureLocation];
}

- (void)removeTarget:(id<ECImageInput>)targetToRemove;
{
    [_terminalFilter removeTarget:targetToRemove];
}

- (void)removeAllTargets;
{
    [_terminalFilter removeAllTargets];
}

- (NSArray *)targets;
{
    return [_terminalFilter targets];
}

- (void)setFrameProcessingCompletionBlock:(void (^)(ECImageOutput *, CMTime))frameProcessingCompletionBlock;
{
    [_terminalFilter setFrameProcessingCompletionBlock:frameProcessingCompletionBlock];
}

- (void (^)(ECImageOutput *, CMTime))frameProcessingCompletionBlock;
{
    return [_terminalFilter frameProcessingCompletionBlock];
}

#pragma mark -
#pragma mark ECImageInput protocol

- (void)newFrameReadyAtTime:(CMTime)frameTime atIndex:(NSInteger)textureIndex;
{
    for (ECImageOutput<ECImageInput> *currentFilter in _initialFilters)
    {
        if (currentFilter != self.inputFilterToIgnoreForUpdates)
        {
            [currentFilter newFrameReadyAtTime:frameTime atIndex:textureIndex];
        }
    }
}

- (void)setInputFramebuffer:(ECImageFramebuffer *)newInputFramebuffer atIndex:(NSInteger)textureIndex;
{
    for (ECImageOutput<ECImageInput> *currentFilter in _initialFilters)
    {
        [currentFilter setInputFramebuffer:newInputFramebuffer atIndex:textureIndex];
    }
}

- (NSInteger)nextAvailableTextureIndex;
{
//    if ([_initialFilters count] > 0)
//    {
//        return [[_initialFilters objectAtIndex:0] nextAvailableTextureIndex];
//    }
    
    return 0;
}

- (void)setInputSize:(CGSize)newSize atIndex:(NSInteger)textureIndex;
{
    for (ECImageOutput<ECImageInput> *currentFilter in _initialFilters)
    {
        [currentFilter setInputSize:newSize atIndex:textureIndex];
    }
}

- (void)setInputRotation:(ECImageRotationMode)newInputRotation atIndex:(NSInteger)textureIndex;
{
    for (ECImageOutput<ECImageInput> *currentFilter in _initialFilters)
    {
        [currentFilter setInputRotation:newInputRotation  atIndex:(NSInteger)textureIndex];
    }
}

- (void)forceProcessingAtSize:(CGSize)frameSize;
{
    for (ECImageOutput<ECImageInput> *currentFilter in filters)
    {
        [currentFilter forceProcessingAtSize:frameSize];
    }
}

- (void)forceProcessingAtSizeRespectingAspectRatio:(CGSize)frameSize;
{
    for (ECImageOutput<ECImageInput> *currentFilter in filters)
    {
        [currentFilter forceProcessingAtSizeRespectingAspectRatio:frameSize];
    }
}

- (CGSize)maximumOutputSize;
{
    // I'm temporarily disabling adjustments for smaller output sizes until I figure out how to make this work better
    return CGSizeZero;

    /*
    if (CGSizeEqualToSize(cachedMaximumOutputSize, CGSizeZero))
    {
        for (id<ECImageInput> currentTarget in _initialFilters)
        {
            if ([currentTarget maximumOutputSize].width > cachedMaximumOutputSize.width)
            {
                cachedMaximumOutputSize = [currentTarget maximumOutputSize];
            }
        }
    }
    
    return cachedMaximumOutputSize;
     */
}

- (void)endProcessing;
{
    if (!isEndProcessing)
    {
        isEndProcessing = YES;
        
        for (id<ECImageInput> currentTarget in _initialFilters)
        {
            [currentTarget endProcessing];
        }
    }
}

- (BOOL)wantsMonochromeInput;
{
    BOOL allInputsWantMonochromeInput = YES;
    for (ECImageOutput<ECImageInput> *currentFilter in _initialFilters)
    {
        allInputsWantMonochromeInput = allInputsWantMonochromeInput && [currentFilter wantsMonochromeInput];
    }
    
    return allInputsWantMonochromeInput;
}

- (void)setCurrentlyReceivingMonochromeInput:(BOOL)newValue;
{
    for (ECImageOutput<ECImageInput> *currentFilter in _initialFilters)
    {
        [currentFilter setCurrentlyReceivingMonochromeInput:newValue];
    }
}

@end
