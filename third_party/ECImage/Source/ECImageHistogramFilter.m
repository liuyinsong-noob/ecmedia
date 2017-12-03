#import "ECImageHistogramFilter.h"

// Unlike other filters, this one uses a grid of GL_POINTs to sample the incoming image in a grid. A custom vertex shader reads the color in the texture at its position 
// and outputs a bin position in the final histogram as the vertex position. That point is then written into the image of the histogram using translucent pixels.
// The degree of translucency is controlled by the scalingFactor, which lets you adjust the dynamic range of the histogram. The histogram can only be generated for one
// color channel or luminance value at a time.
//
// This is based on this implementation: http://www.shaderwrangler.com/publications/histogram/histogram_cameraready.pdf
//
// Or at least that's how it would work if iOS could read from textures in a vertex shader, which it can't. Therefore, I read the texture data down from the
// incoming frame and process the texture colors as vertices.

NSString *const kECImageRedHistogramSamplingVertexShaderString = SHADER_STRING
(
 attribute vec4 position;
 
 varying vec3 colorFactor;

 void main()
 {
     colorFactor = vec3(1.0, 0.0, 0.0);
     gl_Position = vec4(-1.0 + (position.x * 0.0078125), 0.0, 0.0, 1.0);
     gl_PointSize = 1.0;
 }
);

NSString *const kECImageGreenHistogramSamplingVertexShaderString = SHADER_STRING
(
 attribute vec4 position;
 
 varying vec3 colorFactor;
 
 void main()
 {
     colorFactor = vec3(0.0, 1.0, 0.0);
     gl_Position = vec4(-1.0 + (position.y * 0.0078125), 0.0, 0.0, 1.0);
     gl_PointSize = 1.0;
 }
);

NSString *const kECImageBlueHistogramSamplingVertexShaderString = SHADER_STRING
(
 attribute vec4 position;
 
 varying vec3 colorFactor;
 
 void main()
 {
     colorFactor = vec3(0.0, 0.0, 1.0);
     gl_Position = vec4(-1.0 + (position.z * 0.0078125), 0.0, 0.0, 1.0);
     gl_PointSize = 1.0;
 }
);

NSString *const kECImageLuminanceHistogramSamplingVertexShaderString = SHADER_STRING
(
 attribute vec4 position;
 
 varying vec3 colorFactor;
 
 const vec3 W = vec3(0.2125, 0.7154, 0.0721);
 
 void main()
 {
     float luminance = dot(position.xyz, W);

     colorFactor = vec3(1.0, 1.0, 1.0);
     gl_Position = vec4(-1.0 + (luminance * 0.0078125), 0.0, 0.0, 1.0);
     gl_PointSize = 1.0;
 }
);

#if TARGET_IPHONE_SIMULATOR || TARGET_OS_IPHONE
NSString *const kECImageHistogramAccumulationFragmentShaderString = SHADER_STRING
(
 const lowp float scalingFactor = 1.0 / 256.0;

 varying lowp vec3 colorFactor;

 void main()
 {
     gl_FragColor = vec4(colorFactor * scalingFactor , 1.0);
 }
);
#else
NSString *const kECImageHistogramAccumulationFragmentShaderString = SHADER_STRING
(
 const float scalingFactor = 1.0 / 256.0;
 
 varying vec3 colorFactor;
 
 void main()
 {
     gl_FragColor = vec4(colorFactor * scalingFactor , 1.0);
 }
);
#endif

@implementation ECImageHistogramFilter

@synthesize downsamplingFactor = _downsamplingFactor;

#pragma mark -
#pragma mark Initialization and teardown

- (id)initWithHistogramType:(ECImageHistogramType)newHistogramType;
{
    switch (newHistogramType)
    {
        case kECImageHistogramRed:
        {
            if (!(self = [super initWithVertexShaderFromString:kECImageRedHistogramSamplingVertexShaderString fragmentShaderFromString:kECImageHistogramAccumulationFragmentShaderString]))
            {
                return nil;
            }
        }; break;
        case kECImageHistogramGreen:
        {
            if (!(self = [super initWithVertexShaderFromString:kECImageGreenHistogramSamplingVertexShaderString fragmentShaderFromString:kECImageHistogramAccumulationFragmentShaderString]))
            {
                return nil;
            }
        }; break;
        case kECImageHistogramBlue:
        {
            if (!(self = [super initWithVertexShaderFromString:kECImageBlueHistogramSamplingVertexShaderString fragmentShaderFromString:kECImageHistogramAccumulationFragmentShaderString]))
            {
                return nil;
            }
        }; break;
        case kECImageHistogramLuminance:
        {
            if (!(self = [super initWithVertexShaderFromString:kECImageLuminanceHistogramSamplingVertexShaderString fragmentShaderFromString:kECImageHistogramAccumulationFragmentShaderString]))
            {
                return nil;
            }
        }; break;
        case kECImageHistogramRGB:
        {
            if (!(self = [super initWithVertexShaderFromString:kECImageRedHistogramSamplingVertexShaderString fragmentShaderFromString:kECImageHistogramAccumulationFragmentShaderString]))
            {
                return nil;
            }

            ec_runSynchronouslyOnVideoProcessingQueue(^{
                [ECImageContext useImageProcessingContext];

                secondFilterProgram = [[ECImageContext sharedImageProcessingContext] programForVertexShaderString:kECImageGreenHistogramSamplingVertexShaderString fragmentShaderString:kECImageHistogramAccumulationFragmentShaderString];
                thirdFilterProgram = [[ECImageContext sharedImageProcessingContext] programForVertexShaderString:kECImageBlueHistogramSamplingVertexShaderString fragmentShaderString:kECImageHistogramAccumulationFragmentShaderString];

                if (!secondFilterProgram.initialized) {
                    [self initializeSecondaryAttributes];

                    if (![secondFilterProgram link]) {
                        NSString *progLog = [secondFilterProgram programLog];
                        NSLog(@"Program link log: %@", progLog);
                        NSString *fragLog = [secondFilterProgram fragmentShaderLog];
                        NSLog(@"Fragment shader compile log: %@", fragLog);
                        NSString *vertLog = [secondFilterProgram vertexShaderLog];
                        NSLog(@"Vertex shader compile log: %@", vertLog);
                        filterProgram = nil;
                        NSAssert(NO, @"Filter shader link failed");

                    }

                    [ECImageContext setActiveShaderProgram:secondFilterProgram];

                    glEnableVertexAttribArray(secondFilterPositionAttribute);

                    if (![thirdFilterProgram link]) {
                        NSString *progLog = [secondFilterProgram programLog];
                        NSLog(@"Program link log: %@", progLog);
                        NSString *fragLog = [secondFilterProgram fragmentShaderLog];
                        NSLog(@"Fragment shader compile log: %@", fragLog);
                        NSString *vertLog = [secondFilterProgram vertexShaderLog];
                        NSLog(@"Vertex shader compile log: %@", vertLog);
                        filterProgram = nil;
                        NSAssert(NO, @"Filter shader link failed");
                    }
                }

                secondFilterPositionAttribute = [secondFilterProgram attributeIndex:@"position"];


                thirdFilterPositionAttribute = [thirdFilterProgram attributeIndex:@"position"];
                [ECImageContext setActiveShaderProgram:thirdFilterProgram];

                glEnableVertexAttribArray(thirdFilterPositionAttribute);
            });
        }; break;
    }

    histogramType = newHistogramType;
    
    self.downsamplingFactor = 16;

    return self;
}

- (id)init;
{
    if (!(self = [self initWithHistogramType:kECImageHistogramRGB]))
    {
        return nil;
    }

    return self;
}

- (void)initializeSecondaryAttributes;
{
    [secondFilterProgram addAttribute:@"position"];
	[thirdFilterProgram addAttribute:@"position"];
}

- (void)dealloc;
{
    if (vertexSamplingCoordinates != NULL && ![ECImageContext supportsFastTextureUpload])
    {
        free(vertexSamplingCoordinates);
    }
}

#pragma mark -
#pragma mark Rendering

- (CGSize)sizeOfFBO;
{
    return CGSizeMake(256.0, 3.0);
}

- (void)newFrameReadyAtTime:(CMTime)frameTime atIndex:(NSInteger)textureIndex;
{
    [self renderToTextureWithVertices:NULL textureCoordinates:NULL];
    
    [self informTargetsAboutNewFrameAtTime:frameTime];
}

- (CGSize)outputFrameSize;
{
    return [self sizeOfFBO];
}

- (void)setInputSize:(CGSize)newSize atIndex:(NSInteger)textureIndex;
{
    if (self.preventRendering)
    {
        return;
    }
    
    inputTextureSize = newSize;
}

- (void)setInputRotation:(ECImageRotationMode)newInputRotation atIndex:(NSInteger)textureIndex;
{
    inputRotation = kECImageNoRotation;
}

- (void)renderToTextureWithVertices:(const GLfloat *)vertices textureCoordinates:(const GLfloat *)textureCoordinates;
{
    // we need a normal color texture for this filter
    NSAssert(self.outputTextureOptions.internalFormat == GL_RGBA, @"The output texture format for this filter must be GL_RGBA.");
    NSAssert(self.outputTextureOptions.type == GL_UNSIGNED_BYTE, @"The type of the output texture of this filter must be GL_UNSIGNED_BYTE.");
    
    if (self.preventRendering)
    {
        [firstInputFramebuffer unlock];
        return;
    }
    
    [ECImageContext useImageProcessingContext];

    if ([ECImageContext supportsFastTextureUpload])
    {
        glFinish();
        vertexSamplingCoordinates = [firstInputFramebuffer byteBuffer];
    } else {
        if (vertexSamplingCoordinates == NULL)
        {
            vertexSamplingCoordinates = calloc(inputTextureSize.width * inputTextureSize.height * 4, sizeof(GLubyte));
        }
        glReadPixels(0, 0, inputTextureSize.width, inputTextureSize.height, GL_RGBA, GL_UNSIGNED_BYTE, vertexSamplingCoordinates);
    }
    
    outputFramebuffer = [[ECImageContext sharedFramebufferCache] fetchFramebufferForSize:[self sizeOfFBO] textureOptions:self.outputTextureOptions onlyTexture:NO];
    [outputFramebuffer activateFramebuffer];
    if (usingNextFrameForImageCapture)
    {
        [outputFramebuffer lock];
    }
    
    [ECImageContext setActiveShaderProgram:filterProgram];
    
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);
    
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_ONE, GL_ONE);
    glEnable(GL_BLEND);
    
	glVertexAttribPointer(filterPositionAttribute, 4, GL_UNSIGNED_BYTE, 0, ((unsigned int)_downsamplingFactor - 1) * 4, vertexSamplingCoordinates);
    glDrawArrays(GL_POINTS, 0, inputTextureSize.width * inputTextureSize.height / (CGFloat)_downsamplingFactor);

    if (histogramType == kECImageHistogramRGB)
    {
        [ECImageContext setActiveShaderProgram:secondFilterProgram];
        
        glVertexAttribPointer(secondFilterPositionAttribute, 4, GL_UNSIGNED_BYTE, 0, ((unsigned int)_downsamplingFactor - 1) * 4, vertexSamplingCoordinates);
        glDrawArrays(GL_POINTS, 0, inputTextureSize.width * inputTextureSize.height / (CGFloat)_downsamplingFactor);

        [ECImageContext setActiveShaderProgram:thirdFilterProgram];
        
        glVertexAttribPointer(thirdFilterPositionAttribute, 4, GL_UNSIGNED_BYTE, 0, ((unsigned int)_downsamplingFactor - 1) * 4, vertexSamplingCoordinates);
        glDrawArrays(GL_POINTS, 0, inputTextureSize.width * inputTextureSize.height / (CGFloat)_downsamplingFactor);
    }
    
    glDisable(GL_BLEND);
    [firstInputFramebuffer unlock];

    if (usingNextFrameForImageCapture)
    {
        dispatch_semaphore_signal(imageCaptureSemaphore);
    }
}

#pragma mark -
#pragma mark Accessors

//- (void)setScalingFactor:(CGFloat)newValue;
//{
//    _scalingFactor = newValue;
//    
//    [ECImageContext useImageProcessingContext];
//    [filterProgram use];
//    glUniform1f(scalingFactorUniform, _scalingFactor);
//}

@end
