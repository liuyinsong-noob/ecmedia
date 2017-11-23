#import "ECImageFilter.h"

extern NSString *const kECImageLuminanceFragmentShaderString;

/** Converts an image to grayscale (a slightly faster implementation of the saturation filter, without the ability to vary the color contribution)
 */
@interface ECImageGrayscaleFilter : ECImageFilter

@end
