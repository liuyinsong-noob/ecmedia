#import "ECImageColorInvertFilter.h"

#if TARGET_IPHONE_SIMULATOR || TARGET_OS_IPHONE
NSString *const kECImageInvertFragmentShaderString = SHADER_STRING
(
 varying highp vec2 textureCoordinate;
 
 uniform sampler2D inputImageTexture;
 
 void main()
 {
    lowp vec4 textureColor = texture2D(inputImageTexture, textureCoordinate);
    
    gl_FragColor = vec4((1.0 - textureColor.rgb), textureColor.w);
 }
);                                                                    
#else
NSString *const kECImageInvertFragmentShaderString = SHADER_STRING
(
 varying vec2 textureCoordinate;
 
 uniform sampler2D inputImageTexture;
 
 void main()
 {
     vec4 textureColor = texture2D(inputImageTexture, textureCoordinate);
     
     gl_FragColor = vec4((1.0 - textureColor.rgb), textureColor.w);
 }
 );
#endif

@implementation ECImageColorInvertFilter

- (id)init;
{
    if (!(self = [super initWithFragmentShaderFromString:kECImageInvertFragmentShaderString]))
    {
		return nil;
    }
    
    return self;
}

@end

