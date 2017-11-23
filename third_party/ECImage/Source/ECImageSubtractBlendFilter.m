#import "ECImageSubtractBlendFilter.h"

#if TARGET_IPHONE_SIMULATOR || TARGET_OS_IPHONE
NSString *const kECImageSubtractBlendFragmentShaderString = SHADER_STRING
(
 varying highp vec2 textureCoordinate;
 varying highp vec2 textureCoordinate2;

 uniform sampler2D inputImageTexture;
 uniform sampler2D inputImageTexture2;
 
 void main()
 {
	 lowp vec4 textureColor = texture2D(inputImageTexture, textureCoordinate);
	 lowp vec4 textureColor2 = texture2D(inputImageTexture2, textureCoordinate2);
	 
	 gl_FragColor = vec4(textureColor.rgb - textureColor2.rgb, textureColor.a);
 }
);
#else
NSString *const kECImageSubtractBlendFragmentShaderString = SHADER_STRING
(
 varying vec2 textureCoordinate;
 varying vec2 textureCoordinate2;
 
 uniform sampler2D inputImageTexture;
 uniform sampler2D inputImageTexture2;
 
 void main()
 {
	 vec4 textureColor = texture2D(inputImageTexture, textureCoordinate);
	 vec4 textureColor2 = texture2D(inputImageTexture2, textureCoordinate2);
	 
	 gl_FragColor = vec4(textureColor.rgb - textureColor2.rgb, textureColor.a);
 }
);
#endif

@implementation ECImageSubtractBlendFilter

- (id)init;
{
    if (!(self = [super initWithFragmentShaderFromString:kECImageSubtractBlendFragmentShaderString]))
    {
		return nil;
    }
    
    return self;
}

@end

