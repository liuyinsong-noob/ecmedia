#import "ECImageErosionFilter.h"
#import "ECImageDilationFilter.h"

@implementation ECImageErosionFilter

#if TARGET_IPHONE_SIMULATOR || TARGET_OS_IPHONE
NSString *const kECImageErosionRadiusOneFragmentShaderString = SHADER_STRING
(
 precision lowp float;
 
 varying vec2 centerTextureCoordinate;
 varying vec2 oneStepPositiveTextureCoordinate;
 varying vec2 oneStepNegativeTextureCoordinate;
 
 uniform sampler2D inputImageTexture;
 
 void main()
 {
     float centerIntensity = texture2D(inputImageTexture, centerTextureCoordinate).r;
     float oneStepPositiveIntensity = texture2D(inputImageTexture, oneStepPositiveTextureCoordinate).r;
     float oneStepNegativeIntensity = texture2D(inputImageTexture, oneStepNegativeTextureCoordinate).r;
     
     lowp float minValue = min(centerIntensity, oneStepPositiveIntensity);
     minValue = min(minValue, oneStepNegativeIntensity);
     
     gl_FragColor = vec4(vec3(minValue), 1.0);
 }
);

NSString *const kECImageErosionRadiusTwoFragmentShaderString = SHADER_STRING
(
 precision lowp float;
 
 varying vec2 centerTextureCoordinate;
 varying vec2 oneStepPositiveTextureCoordinate;
 varying vec2 oneStepNegativeTextureCoordinate;
 varying vec2 twoStepsPositiveTextureCoordinate;
 varying vec2 twoStepsNegativeTextureCoordinate;
 
 uniform sampler2D inputImageTexture;
 
 void main()
 {
     float centerIntensity = texture2D(inputImageTexture, centerTextureCoordinate).r;
     float oneStepPositiveIntensity = texture2D(inputImageTexture, oneStepPositiveTextureCoordinate).r;
     float oneStepNegativeIntensity = texture2D(inputImageTexture, oneStepNegativeTextureCoordinate).r;
     float twoStepsPositiveIntensity = texture2D(inputImageTexture, twoStepsPositiveTextureCoordinate).r;
     float twoStepsNegativeIntensity = texture2D(inputImageTexture, twoStepsNegativeTextureCoordinate).r;
     
     lowp float minValue = min(centerIntensity, oneStepPositiveIntensity);
     minValue = min(minValue, oneStepNegativeIntensity);
     minValue = min(minValue, twoStepsPositiveIntensity);
     minValue = min(minValue, twoStepsNegativeIntensity);
     
     gl_FragColor = vec4(vec3(minValue), 1.0);
 }
);

NSString *const kECImageErosionRadiusThreeFragmentShaderString = SHADER_STRING
(
 precision lowp float;
 
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
     float centerIntensity = texture2D(inputImageTexture, centerTextureCoordinate).r;
     float oneStepPositiveIntensity = texture2D(inputImageTexture, oneStepPositiveTextureCoordinate).r;
     float oneStepNegativeIntensity = texture2D(inputImageTexture, oneStepNegativeTextureCoordinate).r;
     float twoStepsPositiveIntensity = texture2D(inputImageTexture, twoStepsPositiveTextureCoordinate).r;
     float twoStepsNegativeIntensity = texture2D(inputImageTexture, twoStepsNegativeTextureCoordinate).r;
     float threeStepsPositiveIntensity = texture2D(inputImageTexture, threeStepsPositiveTextureCoordinate).r;
     float threeStepsNegativeIntensity = texture2D(inputImageTexture, threeStepsNegativeTextureCoordinate).r;
     
     lowp float minValue = min(centerIntensity, oneStepPositiveIntensity);
     minValue = min(minValue, oneStepNegativeIntensity);
     minValue = min(minValue, twoStepsPositiveIntensity);
     minValue = min(minValue, twoStepsNegativeIntensity);
     minValue = min(minValue, threeStepsPositiveIntensity);
     minValue = min(minValue, threeStepsNegativeIntensity);
     
     gl_FragColor = vec4(vec3(minValue), 1.0);
 }
);

NSString *const kECImageErosionRadiusFourFragmentShaderString = SHADER_STRING
(
 precision lowp float;
 
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
     float centerIntensity = texture2D(inputImageTexture, centerTextureCoordinate).r;
     float oneStepPositiveIntensity = texture2D(inputImageTexture, oneStepPositiveTextureCoordinate).r;
     float oneStepNegativeIntensity = texture2D(inputImageTexture, oneStepNegativeTextureCoordinate).r;
     float twoStepsPositiveIntensity = texture2D(inputImageTexture, twoStepsPositiveTextureCoordinate).r;
     float twoStepsNegativeIntensity = texture2D(inputImageTexture, twoStepsNegativeTextureCoordinate).r;
     float threeStepsPositiveIntensity = texture2D(inputImageTexture, threeStepsPositiveTextureCoordinate).r;
     float threeStepsNegativeIntensity = texture2D(inputImageTexture, threeStepsNegativeTextureCoordinate).r;
     float fourStepsPositiveIntensity = texture2D(inputImageTexture, fourStepsPositiveTextureCoordinate).r;
     float fourStepsNegativeIntensity = texture2D(inputImageTexture, fourStepsNegativeTextureCoordinate).r;
     
     lowp float minValue = min(centerIntensity, oneStepPositiveIntensity);
     minValue = min(minValue, oneStepNegativeIntensity);
     minValue = min(minValue, twoStepsPositiveIntensity);
     minValue = min(minValue, twoStepsNegativeIntensity);
     minValue = min(minValue, threeStepsPositiveIntensity);
     minValue = min(minValue, threeStepsNegativeIntensity);
     minValue = min(minValue, fourStepsPositiveIntensity);
     minValue = min(minValue, fourStepsNegativeIntensity);
     
     gl_FragColor = vec4(vec3(minValue), 1.0);
 }
);
#else
NSString *const kECImageErosionRadiusOneFragmentShaderString = SHADER_STRING
(
 varying vec2 centerTextureCoordinate;
 varying vec2 oneStepPositiveTextureCoordinate;
 varying vec2 oneStepNegativeTextureCoordinate;
 
 uniform sampler2D inputImageTexture;
 
 void main()
 {
     float centerIntensity = texture2D(inputImageTexture, centerTextureCoordinate).r;
     float oneStepPositiveIntensity = texture2D(inputImageTexture, oneStepPositiveTextureCoordinate).r;
     float oneStepNegativeIntensity = texture2D(inputImageTexture, oneStepNegativeTextureCoordinate).r;
     
     float minValue = min(centerIntensity, oneStepPositiveIntensity);
     minValue = min(minValue, oneStepNegativeIntensity);
     
     gl_FragColor = vec4(vec3(minValue), 1.0);
 }
);

NSString *const kECImageErosionRadiusTwoFragmentShaderString = SHADER_STRING
(
 varying vec2 centerTextureCoordinate;
 varying vec2 oneStepPositiveTextureCoordinate;
 varying vec2 oneStepNegativeTextureCoordinate;
 varying vec2 twoStepsPositiveTextureCoordinate;
 varying vec2 twoStepsNegativeTextureCoordinate;
 
 uniform sampler2D inputImageTexture;
 
 void main()
 {
     float centerIntensity = texture2D(inputImageTexture, centerTextureCoordinate).r;
     float oneStepPositiveIntensity = texture2D(inputImageTexture, oneStepPositiveTextureCoordinate).r;
     float oneStepNegativeIntensity = texture2D(inputImageTexture, oneStepNegativeTextureCoordinate).r;
     float twoStepsPositiveIntensity = texture2D(inputImageTexture, twoStepsPositiveTextureCoordinate).r;
     float twoStepsNegativeIntensity = texture2D(inputImageTexture, twoStepsNegativeTextureCoordinate).r;
     
     float minValue = min(centerIntensity, oneStepPositiveIntensity);
     minValue = min(minValue, oneStepNegativeIntensity);
     minValue = min(minValue, twoStepsPositiveIntensity);
     minValue = min(minValue, twoStepsNegativeIntensity);
     
     gl_FragColor = vec4(vec3(minValue), 1.0);
 }
);

NSString *const kECImageErosionRadiusThreeFragmentShaderString = SHADER_STRING
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
     float centerIntensity = texture2D(inputImageTexture, centerTextureCoordinate).r;
     float oneStepPositiveIntensity = texture2D(inputImageTexture, oneStepPositiveTextureCoordinate).r;
     float oneStepNegativeIntensity = texture2D(inputImageTexture, oneStepNegativeTextureCoordinate).r;
     float twoStepsPositiveIntensity = texture2D(inputImageTexture, twoStepsPositiveTextureCoordinate).r;
     float twoStepsNegativeIntensity = texture2D(inputImageTexture, twoStepsNegativeTextureCoordinate).r;
     float threeStepsPositiveIntensity = texture2D(inputImageTexture, threeStepsPositiveTextureCoordinate).r;
     float threeStepsNegativeIntensity = texture2D(inputImageTexture, threeStepsNegativeTextureCoordinate).r;
     
     float minValue = min(centerIntensity, oneStepPositiveIntensity);
     minValue = min(minValue, oneStepNegativeIntensity);
     minValue = min(minValue, twoStepsPositiveIntensity);
     minValue = min(minValue, twoStepsNegativeIntensity);
     minValue = min(minValue, threeStepsPositiveIntensity);
     minValue = min(minValue, threeStepsNegativeIntensity);
     
     gl_FragColor = vec4(vec3(minValue), 1.0);
 }
);

NSString *const kECImageErosionRadiusFourFragmentShaderString = SHADER_STRING
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
     float centerIntensity = texture2D(inputImageTexture, centerTextureCoordinate).r;
     float oneStepPositiveIntensity = texture2D(inputImageTexture, oneStepPositiveTextureCoordinate).r;
     float oneStepNegativeIntensity = texture2D(inputImageTexture, oneStepNegativeTextureCoordinate).r;
     float twoStepsPositiveIntensity = texture2D(inputImageTexture, twoStepsPositiveTextureCoordinate).r;
     float twoStepsNegativeIntensity = texture2D(inputImageTexture, twoStepsNegativeTextureCoordinate).r;
     float threeStepsPositiveIntensity = texture2D(inputImageTexture, threeStepsPositiveTextureCoordinate).r;
     float threeStepsNegativeIntensity = texture2D(inputImageTexture, threeStepsNegativeTextureCoordinate).r;
     float fourStepsPositiveIntensity = texture2D(inputImageTexture, fourStepsPositiveTextureCoordinate).r;
     float fourStepsNegativeIntensity = texture2D(inputImageTexture, fourStepsNegativeTextureCoordinate).r;
     
     float minValue = min(centerIntensity, oneStepPositiveIntensity);
     minValue = min(minValue, oneStepNegativeIntensity);
     minValue = min(minValue, twoStepsPositiveIntensity);
     minValue = min(minValue, twoStepsNegativeIntensity);
     minValue = min(minValue, threeStepsPositiveIntensity);
     minValue = min(minValue, threeStepsNegativeIntensity);
     minValue = min(minValue, fourStepsPositiveIntensity);
     minValue = min(minValue, fourStepsNegativeIntensity);
     
     gl_FragColor = vec4(vec3(minValue), 1.0);
 }
);
#endif

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
            fragmentShaderForThisRadius = kECImageErosionRadiusOneFragmentShaderString;
        }; break;
        case 2:
        {
            vertexShaderForThisRadius = kECImageDilationRadiusTwoVertexShaderString;
            fragmentShaderForThisRadius = kECImageErosionRadiusTwoFragmentShaderString;
        }; break;
        case 3:
        {
            vertexShaderForThisRadius = kECImageDilationRadiusThreeVertexShaderString;
            fragmentShaderForThisRadius = kECImageErosionRadiusThreeFragmentShaderString;
        }; break;
        case 4:
        {
            vertexShaderForThisRadius = kECImageDilationRadiusFourVertexShaderString;
            fragmentShaderForThisRadius = kECImageErosionRadiusFourFragmentShaderString;
        }; break;
        default:
        {
            vertexShaderForThisRadius = kECImageDilationRadiusFourVertexShaderString;
            fragmentShaderForThisRadius = kECImageErosionRadiusFourFragmentShaderString;
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
