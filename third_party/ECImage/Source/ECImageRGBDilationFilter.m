#import "ECImageRGBDilationFilter.h"
#import "ECImageDilationFilter.h"

#if TARGET_IPHONE_SIMULATOR || TARGET_OS_IPHONE
NSString *const kECImageRGBDilationRadiusOneFragmentShaderString = SHADER_STRING
(
 precision highp float;
 
 varying vec2 centerTextureCoordinate;
 varying vec2 oneStepPositiveTextureCoordinate;
 varying vec2 oneStepNegativeTextureCoordinate;
 
 uniform sampler2D inputImageTexture;
 
 void main()
 {
     lowp vec4 centerIntensity = texture2D(inputImageTexture, centerTextureCoordinate);
     lowp vec4 oneStepPositiveIntensity = texture2D(inputImageTexture, oneStepPositiveTextureCoordinate);
     lowp vec4 oneStepNegativeIntensity = texture2D(inputImageTexture, oneStepNegativeTextureCoordinate);
     
     lowp vec4 maxValue = max(centerIntensity, oneStepPositiveIntensity);
     
     gl_FragColor = max(maxValue, oneStepNegativeIntensity);
 }
);

NSString *const kECImageRGBDilationRadiusTwoFragmentShaderString = SHADER_STRING
(
 precision highp float;
 
 varying vec2 centerTextureCoordinate;
 varying vec2 oneStepPositiveTextureCoordinate;
 varying vec2 oneStepNegativeTextureCoordinate;
 varying vec2 twoStepsPositiveTextureCoordinate;
 varying vec2 twoStepsNegativeTextureCoordinate;
 
 uniform sampler2D inputImageTexture;
 
 void main()
 {
     lowp vec4 centerIntensity = texture2D(inputImageTexture, centerTextureCoordinate);
     lowp vec4 oneStepPositiveIntensity = texture2D(inputImageTexture, oneStepPositiveTextureCoordinate);
     lowp vec4 oneStepNegativeIntensity = texture2D(inputImageTexture, oneStepNegativeTextureCoordinate);
     lowp vec4 twoStepsPositiveIntensity = texture2D(inputImageTexture, twoStepsPositiveTextureCoordinate);
     lowp vec4 twoStepsNegativeIntensity = texture2D(inputImageTexture, twoStepsNegativeTextureCoordinate);
     
     lowp vec4 maxValue = max(centerIntensity, oneStepPositiveIntensity);
     maxValue = max(maxValue, oneStepNegativeIntensity);
     maxValue = max(maxValue, twoStepsPositiveIntensity);
     maxValue = max(maxValue, twoStepsNegativeIntensity);
     
     gl_FragColor = max(maxValue, twoStepsNegativeIntensity);
 }
);

NSString *const kECImageRGBDilationRadiusThreeFragmentShaderString = SHADER_STRING
(
 precision highp float;
 
 varying vec2 centerTextureCoordinate;
 varying vec2 oneStepPositiveTextureCoordinate;
 varying vec2 oneStepNegativeTextureCoordinate;
 varying vec2 twoStepsPositiveTextureCoordinate;
 varying vec2 twoStepsNegativeTextureCoordinate;
 varying vec2 threeStepsPositiveTextureCoordinate;
 varying vec2 threeStepsNegativeTextureCoordinate;
 
 uniform sampler2D inputImageTexture;
 
 void main()
 {
     lowp vec4 centerIntensity = texture2D(inputImageTexture, centerTextureCoordinate);
     lowp vec4 oneStepPositiveIntensity = texture2D(inputImageTexture, oneStepPositiveTextureCoordinate);
     lowp vec4 oneStepNegativeIntensity = texture2D(inputImageTexture, oneStepNegativeTextureCoordinate);
     lowp vec4 twoStepsPositiveIntensity = texture2D(inputImageTexture, twoStepsPositiveTextureCoordinate);
     lowp vec4 twoStepsNegativeIntensity = texture2D(inputImageTexture, twoStepsNegativeTextureCoordinate);
     lowp vec4 threeStepsPositiveIntensity = texture2D(inputImageTexture, threeStepsPositiveTextureCoordinate);
     lowp vec4 threeStepsNegativeIntensity = texture2D(inputImageTexture, threeStepsNegativeTextureCoordinate);
     
     lowp vec4 maxValue = max(centerIntensity, oneStepPositiveIntensity);
     maxValue = max(maxValue, oneStepNegativeIntensity);
     maxValue = max(maxValue, twoStepsPositiveIntensity);
     maxValue = max(maxValue, twoStepsNegativeIntensity);
     maxValue = max(maxValue, threeStepsPositiveIntensity);
     
     gl_FragColor = max(maxValue, threeStepsNegativeIntensity);
 }
);

NSString *const kECImageRGBDilationRadiusFourFragmentShaderString = SHADER_STRING
(
 precision highp float;
 
 varying vec2 centerTextureCoordinate;
 varying vec2 oneStepPositiveTextureCoordinate;
 varying vec2 oneStepNegativeTextureCoordinate;
 varying vec2 twoStepsPositiveTextureCoordinate;
 varying vec2 twoStepsNegativeTextureCoordinate;
 varying vec2 threeStepsPositiveTextureCoordinate;
 varying vec2 threeStepsNegativeTextureCoordinate;
 varying vec2 fourStepsPositiveTextureCoordinate;
 varying vec2 fourStepsNegativeTextureCoordinate;
 
 uniform sampler2D inputImageTexture;
 
 void main()
 {
     lowp vec4 centerIntensity = texture2D(inputImageTexture, centerTextureCoordinate);
     lowp vec4 oneStepPositiveIntensity = texture2D(inputImageTexture, oneStepPositiveTextureCoordinate);
     lowp vec4 oneStepNegativeIntensity = texture2D(inputImageTexture, oneStepNegativeTextureCoordinate);
     lowp vec4 twoStepsPositiveIntensity = texture2D(inputImageTexture, twoStepsPositiveTextureCoordinate);
     lowp vec4 twoStepsNegativeIntensity = texture2D(inputImageTexture, twoStepsNegativeTextureCoordinate);
     lowp vec4 threeStepsPositiveIntensity = texture2D(inputImageTexture, threeStepsPositiveTextureCoordinate);
     lowp vec4 threeStepsNegativeIntensity = texture2D(inputImageTexture, threeStepsNegativeTextureCoordinate);
     lowp vec4 fourStepsPositiveIntensity = texture2D(inputImageTexture, fourStepsPositiveTextureCoordinate);
     lowp vec4 fourStepsNegativeIntensity = texture2D(inputImageTexture, fourStepsNegativeTextureCoordinate);
     
     lowp vec4 maxValue = max(centerIntensity, oneStepPositiveIntensity);
     maxValue = max(maxValue, oneStepNegativeIntensity);
     maxValue = max(maxValue, twoStepsPositiveIntensity);
     maxValue = max(maxValue, twoStepsNegativeIntensity);
     maxValue = max(maxValue, threeStepsPositiveIntensity);
     maxValue = max(maxValue, threeStepsNegativeIntensity);
     maxValue = max(maxValue, fourStepsPositiveIntensity);
     
     gl_FragColor = max(maxValue, fourStepsNegativeIntensity);
 }
);
#else
NSString *const kECImageRGBDilationRadiusOneFragmentShaderString = SHADER_STRING
(
 varying vec2 centerTextureCoordinate;
 varying vec2 oneStepPositiveTextureCoordinate;
 varying vec2 oneStepNegativeTextureCoordinate;
 
 uniform sampler2D inputImageTexture;
 
 void main()
 {
     vec4 centerIntensity = texture2D(inputImageTexture, centerTextureCoordinate);
     vec4 oneStepPositiveIntensity = texture2D(inputImageTexture, oneStepPositiveTextureCoordinate);
     vec4 oneStepNegativeIntensity = texture2D(inputImageTexture, oneStepNegativeTextureCoordinate);
     
     vec4 maxValue = max(centerIntensity, oneStepPositiveIntensity);
     
     gl_FragColor = max(maxValue, oneStepNegativeIntensity);
 }
 );

NSString *const kECImageRGBDilationRadiusTwoFragmentShaderString = SHADER_STRING
(
 varying vec2 centerTextureCoordinate;
 varying vec2 oneStepPositiveTextureCoordinate;
 varying vec2 oneStepNegativeTextureCoordinate;
 varying vec2 twoStepsPositiveTextureCoordinate;
 varying vec2 twoStepsNegativeTextureCoordinate;
 
 uniform sampler2D inputImageTexture;
 
 void main()
 {
     vec4 centerIntensity = texture2D(inputImageTexture, centerTextureCoordinate);
     vec4 oneStepPositiveIntensity = texture2D(inputImageTexture, oneStepPositiveTextureCoordinate);
     vec4 oneStepNegativeIntensity = texture2D(inputImageTexture, oneStepNegativeTextureCoordinate);
     vec4 twoStepsPositiveIntensity = texture2D(inputImageTexture, twoStepsPositiveTextureCoordinate);
     vec4 twoStepsNegativeIntensity = texture2D(inputImageTexture, twoStepsNegativeTextureCoordinate);
     
     vec4 maxValue = max(centerIntensity, oneStepPositiveIntensity);
     maxValue = max(maxValue, oneStepNegativeIntensity);
     maxValue = max(maxValue, twoStepsPositiveIntensity);
     maxValue = max(maxValue, twoStepsNegativeIntensity);
     
     gl_FragColor = max(maxValue, twoStepsNegativeIntensity);
 }
 );

NSString *const kECImageRGBDilationRadiusThreeFragmentShaderString = SHADER_STRING
(
 varying vec2 centerTextureCoordinate;
 varying vec2 oneStepPositiveTextureCoordinate;
 varying vec2 oneStepNegativeTextureCoordinate;
 varying vec2 twoStepsPositiveTextureCoordinate;
 varying vec2 twoStepsNegativeTextureCoordinate;
 varying vec2 threeStepsPositiveTextureCoordinate;
 varying vec2 threeStepsNegativeTextureCoordinate;
 
 uniform sampler2D inputImageTexture;
 
 void main()
 {
     vec4 centerIntensity = texture2D(inputImageTexture, centerTextureCoordinate);
     vec4 oneStepPositiveIntensity = texture2D(inputImageTexture, oneStepPositiveTextureCoordinate);
     vec4 oneStepNegativeIntensity = texture2D(inputImageTexture, oneStepNegativeTextureCoordinate);
     vec4 twoStepsPositiveIntensity = texture2D(inputImageTexture, twoStepsPositiveTextureCoordinate);
     vec4 twoStepsNegativeIntensity = texture2D(inputImageTexture, twoStepsNegativeTextureCoordinate);
     vec4 threeStepsPositiveIntensity = texture2D(inputImageTexture, threeStepsPositiveTextureCoordinate);
     vec4 threeStepsNegativeIntensity = texture2D(inputImageTexture, threeStepsNegativeTextureCoordinate);
     
     vec4 maxValue = max(centerIntensity, oneStepPositiveIntensity);
     maxValue = max(maxValue, oneStepNegativeIntensity);
     maxValue = max(maxValue, twoStepsPositiveIntensity);
     maxValue = max(maxValue, twoStepsNegativeIntensity);
     maxValue = max(maxValue, threeStepsPositiveIntensity);
     
     gl_FragColor = max(maxValue, threeStepsNegativeIntensity);
 }
);

NSString *const kECImageRGBDilationRadiusFourFragmentShaderString = SHADER_STRING
(
 varying vec2 centerTextureCoordinate;
 varying vec2 oneStepPositiveTextureCoordinate;
 varying vec2 oneStepNegativeTextureCoordinate;
 varying vec2 twoStepsPositiveTextureCoordinate;
 varying vec2 twoStepsNegativeTextureCoordinate;
 varying vec2 threeStepsPositiveTextureCoordinate;
 varying vec2 threeStepsNegativeTextureCoordinate;
 varying vec2 fourStepsPositiveTextureCoordinate;
 varying vec2 fourStepsNegativeTextureCoordinate;
 
 uniform sampler2D inputImageTexture;
 
 void main()
 {
     vec4 centerIntensity = texture2D(inputImageTexture, centerTextureCoordinate);
     vec4 oneStepPositiveIntensity = texture2D(inputImageTexture, oneStepPositiveTextureCoordinate);
     vec4 oneStepNegativeIntensity = texture2D(inputImageTexture, oneStepNegativeTextureCoordinate);
     vec4 twoStepsPositiveIntensity = texture2D(inputImageTexture, twoStepsPositiveTextureCoordinate);
     vec4 twoStepsNegativeIntensity = texture2D(inputImageTexture, twoStepsNegativeTextureCoordinate);
     vec4 threeStepsPositiveIntensity = texture2D(inputImageTexture, threeStepsPositiveTextureCoordinate);
     vec4 threeStepsNegativeIntensity = texture2D(inputImageTexture, threeStepsNegativeTextureCoordinate);
     vec4 fourStepsPositiveIntensity = texture2D(inputImageTexture, fourStepsPositiveTextureCoordinate);
     vec4 fourStepsNegativeIntensity = texture2D(inputImageTexture, fourStepsNegativeTextureCoordinate);
     
     vec4 maxValue = max(centerIntensity, oneStepPositiveIntensity);
     maxValue = max(maxValue, oneStepNegativeIntensity);
     maxValue = max(maxValue, twoStepsPositiveIntensity);
     maxValue = max(maxValue, twoStepsNegativeIntensity);
     maxValue = max(maxValue, threeStepsPositiveIntensity);
     maxValue = max(maxValue, threeStepsNegativeIntensity);
     maxValue = max(maxValue, fourStepsPositiveIntensity);
     
     gl_FragColor = max(maxValue, fourStepsNegativeIntensity);
 }
);
#endif

@implementation ECImageRGBDilationFilter

#pragma mark -
#pragma mark Initialization and teardown

- (id)initWithRadius:(NSUInteger)dilationRadius;
{    
    NSString *fragmentShaderForThisRadius = nil;
    NSString *vertexShaderForThisRadius = nil;
    
    switch (dilationRadius)
    {
        case 0:
        case 1:
        {
            vertexShaderForThisRadius = kECImageDilationRadiusOneVertexShaderString;
            fragmentShaderForThisRadius = kECImageRGBDilationRadiusOneFragmentShaderString;
        }; break;
        case 2:
        {
            vertexShaderForThisRadius = kECImageDilationRadiusTwoVertexShaderString;
            fragmentShaderForThisRadius = kECImageRGBDilationRadiusTwoFragmentShaderString;
        }; break;
        case 3:
        {
            vertexShaderForThisRadius = kECImageDilationRadiusThreeVertexShaderString;
            fragmentShaderForThisRadius = kECImageRGBDilationRadiusThreeFragmentShaderString;
        }; break;
        case 4:
        {
            vertexShaderForThisRadius = kECImageDilationRadiusFourVertexShaderString;
            fragmentShaderForThisRadius = kECImageRGBDilationRadiusFourFragmentShaderString;
        }; break;
        default:
        {
            vertexShaderForThisRadius = kECImageDilationRadiusFourVertexShaderString;
            fragmentShaderForThisRadius = kECImageRGBDilationRadiusFourFragmentShaderString;
        }; break;
    }
    
    if (!(self = [super initWithFirstStageVertexShaderFromString:vertexShaderForThisRadius firstStageFragmentShaderFromString:fragmentShaderForThisRadius secondStageVertexShaderFromString:vertexShaderForThisRadius secondStageFragmentShaderFromString:fragmentShaderForThisRadius]))
    {
        return nil;
    }
    
    return self;
}

- (id)init;
{
    if (!(self = [self initWithRadius:1]))
    {
        return nil;
    }
    
    return self;
}

@end
