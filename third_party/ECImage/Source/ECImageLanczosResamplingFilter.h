#import "ECImageTwoPassTextureSamplingFilter.h"

@interface ECImageLanczosResamplingFilter : ECImageTwoPassTextureSamplingFilter

@property(readwrite, nonatomic) CGSize originalImageSize;

@end
