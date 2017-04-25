

#import "CameraViewController.h"
#import "StartLiveView.h"

@interface CameraViewController ()
@property (strong, nonatomic) IBOutlet UIImageView *backgroundView;
@property (strong, nonatomic) IBOutlet UITextField *myTitle;
@property (strong, nonatomic) IBOutlet UIView *middleView;
@property (strong, nonatomic) IBOutlet UIButton *backBtn;
@property (strong, nonatomic) IBOutlet UIView *liveView;
@end

@implementation CameraViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    
    //设置背景图片高斯模糊
    [self gaussianImage];
    
    //隐藏状态栏
    [[UIApplication sharedApplication] setStatusBarHidden:TRUE];

    //设置键盘TextField
    [self setupTextField];
    self.navigationItem.leftBarButtonItem = [[UIBarButtonItem alloc] initWithTitle:@"返回" style:UIBarButtonItemStylePlain target:self action:@selector(backMain)];
}

#pragma mark ---- <设置键盘TextField>
- (void)setupTextField {
    
    [_myTitle becomeFirstResponder];
    
    //设置键盘颜色
    _myTitle.tintColor = [UIColor whiteColor];
    
    //设置占位文字颜色
    [_myTitle setValue:[UIColor whiteColor] forKeyPath:@"_placeholderLabel.textColor"];

}

#pragma mark ---- <设置背景图片高斯模糊>
- (void)gaussianImage {
    
    //GPUImageGaussianBlurFilter * blurFilter = [[GPUImageGaussianBlurFilter alloc] init];
    //blurFilter.blurRadiusInPixels = 2.0;
    UIImage * image = [UIImage imageNamed:@"bg_zbfx"];
   
    //UIImage *blurredImage = [blurFilter imageByFilteringImage:image];
    
    //self.backgroundView.image = blurredImage;
    self.backgroundView.image  = image;
}

//返回主界面
- (IBAction)backMain {
    [self.liveView removeFromSuperview];
    self.liveView = nil;
    [self.navigationController popViewControllerAnimated:YES];
}

//开始直播采集
- (IBAction)startLiveStream {

    StartLiveView* wwwliveView = [[StartLiveView alloc] initWithFrame:self.view.bounds];
    self.liveView = wwwliveView;
    [wwwliveView release];
     [self.view addSubview:self.liveView];
    
    _backBtn.hidden = YES;
    _middleView.hidden = YES;
     [_myTitle resignFirstResponder];
    
}

@end
