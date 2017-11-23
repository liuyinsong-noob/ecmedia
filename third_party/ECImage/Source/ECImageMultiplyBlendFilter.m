#import "ECImageMultiplyBlendFilter.h"

#if TARGET_IPHONE_SIMULATOR || TARGET_OS_IPHONE
NSString *const kECImageMultiplyBlendFragmentShaderString = SHADER_STRING
(
 varying highp vec2 textureCoordinate;
 varying highp vec2 textureCoordinate2;

 uniform sampler2D inputImageTexture;
 uniform sampler2D inputImageTexture2;
 
 void main()
 {
     lowp vec4 base = texture2D(inputImageTexture, textureCoordinate);
     lowp vec4 overlayer = texture2D(inputImageTexture2, textureCoordinate2);
          
     gl_FragColor = overlayer * base + overlayer * (1.0 - base.a) + base * (1.0 - overlayer.a);
 }
);
#else
NSString *const kECImageMultiplyBlendFragmentShaderString = SHADER_STRING
(
 varying vec2 textureCoordinate;
 varying vec2 textureCoordinate2;
 
 uniform sampler2D inputImageTexture;
 uniform sampler2D inputImageTexture2;
 
 void main()
 {
     vec4 base = texture2D(inputImageTexture, textureCoordinate);
     vec4 overlayer = texture2D(inputImageTexture2, textureCoordinate2);
     
     gl_FragColor = overlayer * base + overlayer * (1.0 - base.a) + base * (1.0 - overlayer.a);
 }
);
#endif

@implementation ECImageMultiplyBlendFilter

- (id)init;
{
    if (!(self = [super initWithFragmentShaderFromString:kECImageMultiplyBlendFragmentShaderString]))
    {
		return nil;
    }
    
    return self;
}

@end

