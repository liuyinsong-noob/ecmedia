//
//  ECImageBeautyFaceFilter.m
//
//  Created by jianqiangzhang on 16/5/27.
//  Copyright © 2016年 jianqiangzhang. All rights reserved.
//
/*
#import "ECImageBrightnessFilter.h"                //亮度
#import "ECImageExposureFilter.h"                  //曝光
#import "ECImageContrastFilter.h"                  //对比度
#import "ECImageSaturationFilter.h"                //饱和度
#import "ECImageGammaFilter.h"                     //伽马线
#import "ECImageColorInvertFilter.h"               //反色
#import "ECImageSepiaFilter.h"                     //褐色（怀旧）
#import "ECImageLevelsFilter.h"                    //色阶
#import "ECImageGrayscaleFilter.h"                 //灰度
#import "ECImageHistogramFilter.h"                 //色彩直方图，显示在图片上
#import "ECImageHistogramGenerator.h"              //色彩直方图
#import "ECImageRGBFilter.h"                       //RGB
#import "ECImageToneCurveFilter.h"                 //色调曲线
#import "ECImageMonochromeFilter.h"                //单色
#import "ECImageOpacityFilter.h"                   //不透明度
#import "ECImageHighlightShadowFilter.h"           //提亮阴影
#import "ECImageFalseColorFilter.h"                //色彩替换（替换亮部和暗部色彩）
#import "ECImageHueFilter.h"                       //色度
#import "ECImageChromaKeyFilter.h"                 //色度键
#import "ECImageWhiteBalanceFilter.h"              //白平横
#import "ECImageAverageColor.h"                    //像素平均色值
#import "ECImageSolidColorGenerator.h"             //纯色
#import "ECImageLuminosity.h"                      //亮度平均
#import "ECImageAverageLuminanceThresholdFilter.h" //像素色值亮度平均，图像黑白（有类似漫画效果）

#import "ECImageLookupFilter.h"                    //lookup 色彩调整
#import "ECImageAmatorkaFilter.h"                  //Amatorka lookup
#import "ECImageMissEtikateFilter.h"               //MissEtikate lookup
#import "ECImageSoftEleganceFilter.h"              //SoftElegance lookup

#pragma mark - 图像处理 Handle Image

#import "ECImageCrosshairGenerator.h"              //十字
#import "ECImageLineGenerator.h"                   //线条

#import "ECImageTransformFilter.h"                 //形状变化
#import "ECImageCropFilter.h"                      //剪裁
#import "ECImageSharpenFilter.h"                   //锐化
#import "ECImageUnsharpMaskFilter.h"               //反遮罩锐化

#import "ECImageFastBlurFilter.h"                  //模糊
#import "ECImageGaussianBlurFilter.h"              //高斯模糊
#import "ECImageGaussianSelectiveBlurFilter.h"     //高斯模糊，选择部分清晰
#import "ECImageBoxBlurFilter.h"                   //盒状模糊
#import "ECImageTiltShiftFilter.h"                 //条纹模糊，中间清晰，上下两端模糊
#import "ECImageMedianFilter.h"                    //中间值，有种稍微模糊边缘的效果
#import "ECImageBilateralFilter.h"                 //双边模糊
#import "ECImageErosionFilter.h"                   //侵蚀边缘模糊，变黑白
#import "ECImageRGBErosionFilter.h"                //RGB侵蚀边缘模糊，有色彩
#import "ECImageDilationFilter.h"                  //扩展边缘模糊，变黑白
#import "ECImageRGBDilationFilter.h"               //RGB扩展边缘模糊，有色彩
#import "ECImageOpeningFilter.h"                   //黑白色调模糊
#import "ECImageRGBOpeningFilter.h"                //彩色模糊
#import "ECImageClosingFilter.h"                   //黑白色调模糊，暗色会被提亮
#import "ECImageRGBClosingFilter.h"                //彩色模糊，暗色会被提亮
#import "ECImageLanczosResamplingFilter.h"         //Lanczos重取样，模糊效果
#import "ECImageNonMaximumSuppressionFilter.h"     //非最大抑制，只显示亮度最高的像素，其他为黑
#import "ECImageThresholdedNonMaximumSuppressionFilter.h" //与上相比，像素丢失更多

#import "ECImageSobelEdgeDetectionFilter.h"        //Sobel边缘检测算法(白边，黑内容，有点漫画的反色效果)
#import "ECImageCannyEdgeDetectionFilter.h"        //Canny边缘检测算法（比上更强烈的黑白对比度）
#import "ECImageThresholdEdgeDetectionFilter.h"    //阈值边缘检测（效果与上差别不大）
#import "ECImagePrewittEdgeDetectionFilter.h"      //普瑞维特(Prewitt)边缘检测(效果与Sobel差不多，貌似更平滑)
#import "ECImageXYDerivativeFilter.h"              //XYDerivative边缘检测，画面以蓝色为主，绿色为边缘，带彩色
#import "ECImageHarrisCornerDetectionFilter.h"     //Harris角点检测，会有绿色小十字显示在图片角点处
#import "ECImageNobleCornerDetectionFilter.h"      //Noble角点检测，检测点更多
#import "ECImageShiTomasiFeatureDetectionFilter.h" //ShiTomasi角点检测，与上差别不大
#import "ECImageMotionDetector.h"                  //动作检测
#import "ECImageHoughTransformLineDetector.h"      //线条检测
#import "ECImageParallelCoordinateLineTransformFilter.h" //平行线检测

#import "ECImageLocalBinaryPatternFilter.h"        //图像黑白化，并有大量噪点

#import "ECImageLowPassFilter.h"                   //用于图像加亮
#import "ECImageHighPassFilter.h"                  //图像低于某值时显示为黑

#pragma mark - 视觉效果 Visual Effect

#import "ECImageSketchFilter.h"                    //素描
#import "ECImageThresholdSketchFilter.h"           //阀值素描，形成有噪点的素描
#import "ECImageToonFilter.h"                      //卡通效果（黑色粗线描边）
#import "ECImageSmoothToonFilter.h"                //相比上面的效果更细腻，上面是粗旷的画风
#import "ECImageKuwaharaFilter.h"                  //桑原(Kuwahara)滤波,水粉画的模糊效果；处理时间比较长，慎用

#import "ECImageMosaicFilter.h"                    //黑白马赛克
#import "ECImagePixellateFilter.h"                 //像素化
#import "ECImagePolarPixellateFilter.h"            //同心圆像素化
#import "ECImageCrosshatchFilter.h"                //交叉线阴影，形成黑白网状画面
#import "ECImageColorPackingFilter.h"              //色彩丢失，模糊（类似监控摄像效果）

#import "ECImageVignetteFilter.h"                  //晕影，形成黑色圆形边缘，突出中间图像的效果
#import "ECImageSwirlFilter.h"                     //漩涡，中间形成卷曲的画面
#import "ECImageBulgeDistortionFilter.h"           //凸起失真，鱼眼效果
#import "ECImagePinchDistortionFilter.h"           //收缩失真，凹面镜
#import "ECImageStretchDistortionFilter.h"         //伸展失真，哈哈镜
#import "ECImageGlassSphereFilter.h"               //水晶球效果
#import "ECImageSphereRefractionFilter.h"          //球形折射，图形倒立

#import "ECImagePosterizeFilter.h"                 //色调分离，形成噪点效果
#import "ECImageCGAColorspaceFilter.h"             //CGA色彩滤镜，形成黑、浅蓝、紫色块的画面
#import "ECImagePerlinNoiseFilter.h"               //柏林噪点，花边噪点
#import "ECImage3x3ConvolutionFilter.h"            //3x3卷积，高亮大色块变黑，加亮边缘、线条等
#import "ECImageEmbossFilter.h"                    //浮雕效果，带有点3d的感觉
#import "ECImagePolkaDotFilter.h"                  //像素圆点花样
#import "ECImageHalftoneFilter.h"                  //点染,图像黑白化，由黑点构成原图的大致图形

#pragma mark - 混合模式 Blend
#import "ECImageMultiplyBlendFilter.h"             //通常用于创建阴影和深度效果
#import "ECImageNormalBlendFilter.h"               //正常
#import "ECImageAlphaBlendFilter.h"                //透明混合,通常用于在背景上应用前景的透明度
#import "ECImageDissolveBlendFilter.h"             //溶解
#import "ECImageOverlayBlendFilter.h"              //叠加,通常用于创建阴影效果
#import "ECImageDarkenBlendFilter.h"               //加深混合,通常用于重叠类型
#import "ECImageLightenBlendFilter.h"              //减淡混合,通常用于重叠类型
#import "ECImageSourceOverBlendFilter.h"           //源混合
#import "ECImageColorBurnBlendFilter.h"            //色彩加深混合
#import "ECImageColorDodgeBlendFilter.h"           //色彩减淡混合
#import "ECImageScreenBlendFilter.h"               //屏幕包裹,通常用于创建亮点和镜头眩光
#import "ECImageExclusionBlendFilter.h"            //排除混合
#import "ECImageDifferenceBlendFilter.h"           //差异混合,通常用于创建更多变动的颜色
#import "ECImageSubtractBlendFilter.h"             //差值混合,通常用于创建两个图像之间的动画变暗模糊效果
#import "ECImageHardLightBlendFilter.h"            //强光混合,通常用于创建阴影效果
#import "ECImageSoftLightBlendFilter.h"            //柔光混合
#import "ECImageChromaKeyBlendFilter.h"            //色度键混合
#import "ECImageMaskFilter.h"                      //遮罩混合
#import "ECImageHazeFilter.h"                      //朦胧加暗
#import "ECImageLuminanceThresholdFilter.h"        //亮度阈
#import "ECImageAdaptiveThresholdFilter.h"         //自适应阈值
#import "ECImageAddBlendFilter.h"                  //通常用于创建两个图像之间的动画变亮模糊效果
#import "ECImageDivideBlendFilter.h"               //通常用于创建两个图像之间的动画变暗模糊效果
*/

#import "ECImageBeautyFaceFilter.h"
#import "ECImageBrightnessFilter.h"
#import "ECImageContrastFilter.h"
#import "ECImageSaturationFilter.h"
#import "ECImageBilateralFilter.h"
#import "ECImageHSBFilter.h"
#import "ECImageThreeInputFilter.h"
#import "ECImageCannyEdgeDetectionFilter.h"

// Internal CombinationFilter(It should not be used outside)
@interface ECImageCombinationFilter : ECImageThreeInputFilter
{
    GLint smoothDegreeUniform;
}

@property (nonatomic, assign) CGFloat intensity;

@end

NSString *const kECImageBeautifyFragmentShaderString = SHADER_STRING
(
 varying highp vec2 textureCoordinate;
 varying highp vec2 textureCoordinate2;
 varying highp vec2 textureCoordinate3;
 
 uniform sampler2D inputImageTexture;
 uniform sampler2D inputImageTexture2;
 uniform sampler2D inputImageTexture3;
 uniform mediump float smoothDegree;
 
 void main()
 {
     highp vec4 bilateral = texture2D(inputImageTexture, textureCoordinate);
     highp vec4 canny = texture2D(inputImageTexture2, textureCoordinate2);
     highp vec4 origin = texture2D(inputImageTexture3,textureCoordinate3);
     highp vec4 smooth;
     lowp float r = origin.r;
     lowp float g = origin.g;
     lowp float b = origin.b;
     if (canny.r < 0.2 && r > 0.3725 && g > 0.1568 && b > 0.0784 && r > b && (max(max(r, g), b) - min(min(r, g), b)) > 0.0588 && abs(r-g) > 0.0588) {
         smooth = (1.0 - smoothDegree) * (origin - bilateral) + bilateral;
     }
     else {
         smooth = origin;
     }
     smooth.r = log(1.0 + 0.2 * smooth.r)/log(1.2);
     smooth.g = log(1.0 + 0.2 * smooth.g)/log(1.2);
     smooth.b = log(1.0 + 0.2 * smooth.b)/log(1.2);
     gl_FragColor = smooth;
 }
 );

@implementation ECImageCombinationFilter

- (id)init {
    if (self = [super initWithFragmentShaderFromString:kECImageBeautifyFragmentShaderString]) {
        smoothDegreeUniform = [filterProgram uniformIndex:@"smoothDegree"];
    }
    self.intensity = 0.5;
    return self;
}

- (void)setIntensity:(CGFloat)intensity {
    _intensity = intensity;
    [self setFloat:intensity forUniform:smoothDegreeUniform program:filterProgram];
}

@end

@implementation ECImageBeautyFaceFilter
- (id)init;
{
    if (!(self = [super init]))
    {
        return nil;
    }
    /*
    brightnessFilter = [[ECImageBrightnessFilter alloc] init];
    brightnessFilter.brightness = 10.f;
    [self addFilter:brightnessFilter];
    
    contrastFilter = [[ECImageContrastFilter alloc] init];
    contrastFilter.contrast = 20.f;
    [self addFilter:contrastFilter];
    
    saturationFilter = [[ECImageSaturationFilter alloc] init];
    saturationFilter.saturation = 20.f;
    [self addFilter:saturationFilter];
    */
    // First pass: face smoothing filter
    bilateralFilter = [[ECImageBilateralFilter alloc] init];
    bilateralFilter.distanceNormalizationFactor = 4.0;
    [self addFilter:bilateralFilter];
    
    // Second pass: edge detection
    cannyEdgeFilter = [[ECImageCannyEdgeDetectionFilter alloc] init];
    [self addFilter:cannyEdgeFilter];
    
    // Third pass: combination bilateral, edge detection and origin
    combinationFilter = [[ECImageCombinationFilter alloc] init];
    [self addFilter:combinationFilter];
    
    // Adjust HSB
    hsbFilter = [[ECImageHSBFilter alloc] init];
    [hsbFilter adjustBrightness:1.1];
    [hsbFilter adjustSaturation:1.1];
    
    [bilateralFilter addTarget:combinationFilter];
    [cannyEdgeFilter addTarget:combinationFilter];
    
    [combinationFilter addTarget:hsbFilter];
    
    //self.initialFilters = [NSArray arrayWithObjects:brightnessFilter, contrastFilter, saturationFilter, bilateralFilter,cannyEdgeFilter,combinationFilter,nil];
    self.initialFilters = [NSArray arrayWithObjects:bilateralFilter,cannyEdgeFilter,combinationFilter,nil];
    
    self.terminalFilter = hsbFilter;
    
    return self;
}

#pragma mark -
#pragma mark ECImageInput protocol

- (void)newFrameReadyAtTime:(CMTime)frameTime atIndex:(NSInteger)textureIndex;
{
    for (ECImageOutput<ECImageInput> *currentFilter in self.initialFilters)
    {
        if (currentFilter != self.inputFilterToIgnoreForUpdates)
        {
            if (currentFilter == combinationFilter) {
                textureIndex = 2;
            }
            [currentFilter newFrameReadyAtTime:frameTime atIndex:textureIndex];
        }
    }
}

- (void)setInputFramebuffer:(ECImageFramebuffer *)newInputFramebuffer atIndex:(NSInteger)textureIndex;
{
    for (ECImageOutput<ECImageInput> *currentFilter in self.initialFilters)
    {
        if (currentFilter == combinationFilter) {
            textureIndex = 2;
        }
        [currentFilter setInputFramebuffer:newInputFramebuffer atIndex:textureIndex];
    }
}

- (void)setDistanceNormalizationFactor:(CGFloat)value{
    bilateralFilter.distanceNormalizationFactor = value;
}

- (void)setBrightness:(CGFloat)brightness saturation:(CGFloat)saturation{
    [hsbFilter adjustBrightness:brightness];
    [hsbFilter adjustSaturation:saturation];
}

@end
