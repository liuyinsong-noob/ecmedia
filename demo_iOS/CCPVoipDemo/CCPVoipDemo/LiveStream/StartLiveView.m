

#import "StartLiveView.h"
#import "AVFoundation/AVCaptureDevice.h"
#import "AVFoundation/AVMediaFormat.h"
#import "ModelEngineVoip.h"

#define XJScreenH [UIScreen mainScreen].bounds.size.height
#define XJScreenW [UIScreen mainScreen].bounds.size.width

@interface StartLiveView()

- (UIViewController *)viewController;
//美颜
@property (nonatomic, strong) UIButton *beautyButton;
//切换前后摄像头
@property (nonatomic, strong) UIButton *cameraButton;
//关闭
@property (nonatomic, strong) UIButton *closeButton;
//开始直播
@property (nonatomic, strong) UIButton *startPlayLiveButton;
@property (nonatomic, strong) UIButton *startPushLiveButton;
@property (nonatomic, strong) UIView *containerView;
@property (nonatomic,retain) ModelEngineVoip *modelEngineVoip;


@property (nonatomic) void *session;

@end

static int padding = 30;
@implementation StartLiveView
- (UIViewController *)viewController
{
    //获取当前view的superView对应的控制器
    UIResponder *next = [self nextResponder];
    do {
        if ([next isKindOfClass:[UIViewController class]]) {
            return (UIViewController *)next;
        }
        next = [next nextResponder];
    } while (next != nil);
    return nil;
    
}
- (instancetype)initWithFrame:(CGRect)frame{
    
    if(self = [super initWithFrame:frame]){
        self.backgroundColor = [UIColor clearColor];
        
        //加载视频录制
        [self requestAccessForVideo];
        
        //加载音频录制
        [self requestAccessForAudio];
        
        //创建界面容器
        [self addSubview:self.containerView];
        
        // 添加按钮
        [self.containerView addSubview:self.closeButton];
        [self.containerView addSubview:self.cameraButton];
        [self.containerView addSubview:self.beautyButton];
        [self.containerView addSubview:self.startPlayLiveButton];
        [self.containerView addSubview:self.startPushLiveButton];
        
        self.modelEngineVoip = [ModelEngineVoip getInstance];
    }
    return self;
}

#pragma mark ---- <加载视频录制>
- (void)requestAccessForVideo{

    AVAuthorizationStatus status = [AVCaptureDevice authorizationStatusForMediaType:AVMediaTypeVideo];
    switch (status) {
        case AVAuthorizationStatusNotDetermined:{
            // 许可对话没有出现，发起授权许可
            [AVCaptureDevice requestAccessForMediaType:AVMediaTypeVideo completionHandler:^(BOOL granted) {
                if (granted) {
                    dispatch_async(dispatch_get_main_queue(), ^{
                        //[self.session setRunning:YES];
                    });
                }
            }];
            break;
        }
        case AVAuthorizationStatusAuthorized:{
            // 已经开启授权，可继续
            //[self.session setRunning:YES];
            break;
        }
        case AVAuthorizationStatusDenied:
        case AVAuthorizationStatusRestricted:
            // 用户明确地拒绝授权，或者相机设备无法访问
            
            break;
        default:
            break;
    }
}

#pragma mark ---- <加载音频录制>
- (void)requestAccessForAudio{
    AVAuthorizationStatus status = [AVCaptureDevice authorizationStatusForMediaType:AVMediaTypeAudio];
    switch (status) {
        case AVAuthorizationStatusNotDetermined:{
            [AVCaptureDevice requestAccessForMediaType:AVMediaTypeAudio completionHandler:^(BOOL granted) {
            }];
            break;
        }
        case AVAuthorizationStatusAuthorized:{
            break;
        }
        case AVAuthorizationStatusDenied:
        case AVAuthorizationStatusRestricted:
            break;
        default:
            break;
    }
}




#pragma mark ---- <创建会话>
- (void*)session{
    if(!_session){
        _session = (0);
        _session = [self.modelEngineVoip createLiveStream:0];
    }
    return _session;
}

#pragma mark ---- <界面容器>
- (UIView*)containerView{
    if(!_containerView){
        _containerView = [UIView new];
        _containerView.frame = self.bounds;
        _containerView.backgroundColor = [UIColor clearColor];
        _containerView.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
    }
    return _containerView;
}

#pragma mark ---- <关闭界面>
- (UIButton*)closeButton{
    
    if(!_closeButton){
        _closeButton = [UIButton new];
        _closeButton.frame = CGRectMake(XJScreenW - padding * 2, padding, padding, padding);
        [_closeButton setImage:[UIImage imageNamed:@"close_preview"] forState:UIControlStateNormal];
        _closeButton.exclusiveTouch = YES;
        [_closeButton addTarget:self action:@selector(close:)forControlEvents:UIControlEventTouchUpInside];

    }
    return _closeButton;
}

-(void) close {
    [[self viewController ]dismissViewControllerAnimated:YES completion:nil];
}
-(void) selectCamera {
    
}
#pragma mark ---- <切换摄像头>
- (UIButton*)cameraButton{
    if(!_cameraButton){
        _cameraButton = [UIButton new];
        
         //位置
        _cameraButton.frame = CGRectMake(XJScreenW - padding * 4, padding, padding, padding);
        
        [_cameraButton setImage:[UIImage imageNamed:@"camra_preview"] forState:UIControlStateNormal];
        _cameraButton.exclusiveTouch = YES;
         [_cameraButton addTarget:self action:@selector(selectCamera::)forControlEvents:UIControlEventTouchUpInside];
    }
    return _cameraButton;
}

#pragma mark ---- <美颜功能>
- (UIButton*)beautyButton{
    if(!_beautyButton){
        _beautyButton = [UIButton new];

        //位置
        _beautyButton.frame = CGRectMake(padding, padding, padding, padding);
        
        [_beautyButton setImage:[UIImage imageNamed:@"camra_beauty"] forState:UIControlStateSelected];
        [_beautyButton setImage:[UIImage imageNamed:@"camra_beauty_close"] forState:UIControlStateNormal];
        _beautyButton.exclusiveTouch = YES;
   
    }
    return _beautyButton;
}

#pragma mark ---- <开始录制>
- (void) startPlay:(id) sender {
    self.startPlayLiveButton.selected = !self.startPlayLiveButton.selected;
    if(self.startPlayLiveButton.selected){
        [self.startPushLiveButton removeFromSuperview];
        [self.startPlayLiveButton setTitle:@"结束观看" forState:UIControlStateNormal];
      //  [self.modelEngineVoip playStream:self.session url:@"rtmp://live.yuntongxun.com/live/livestream" view:self.containerView];
        [self.modelEngineVoip playStream:self.session url:@"rtmp://live.yuntongxun.com:1935/live/jiazyjiazy" view:self.containerView];
        
        // [self.modelEngineVoip playStream:self.session url:@"rtmp://live2.fzntv.cn:1935/live/zohi_fztv1" view:self.containerView];
    }else{
        [self.modelEngineVoip stopLiveStream:self.session];
        [self.startPlayLiveButton setTitle:@"观看直播" forState:UIControlStateNormal];
        [self.modelEngineVoip stopLiveStream:self.session];
        
    }
}
- (UIButton*)startPlayLiveButton{
    if(! _startPlayLiveButton){
        _startPlayLiveButton = [UIButton new];
        //位置
        _startPlayLiveButton.frame = CGRectMake((XJScreenW - 200) * 0.5, XJScreenH - 100, 200, 40);
         _startPlayLiveButton.layer.cornerRadius = _startPlayLiveButton.frame.size.height * 0.5;
        [_startPlayLiveButton setTitleColor:[UIColor blackColor] forState:UIControlStateNormal];
        [_startPlayLiveButton.titleLabel setFont:[UIFont systemFontOfSize:16]];
        [_startPlayLiveButton setTitle:@"观看直播" forState:UIControlStateNormal];
        [_startPlayLiveButton setBackgroundColor:[UIColor grayColor]];
        _startPlayLiveButton.exclusiveTouch = YES;
        [_startPlayLiveButton addTarget:self action:@selector(startPlay:)forControlEvents:UIControlEventTouchUpInside];

    }
    return _startPlayLiveButton;
}

- (void) startPush:(id) sender {
    self.startPushLiveButton.selected = !self.startPushLiveButton.selected;
    if(self.startPushLiveButton.selected){
        
        [self.startPushLiveButton setTitle:@"结束直播" forState:UIControlStateNormal];
        [self.startPlayLiveButton removeFromSuperview];
        [self.modelEngineVoip pushStream:self.session url:@"rtmp://live.yuntongxun.com:1935/live/jiazyjiazy" view:self.containerView];
        
    }else{
        [self.modelEngineVoip stopLiveStream:self.session];
        [self.startPushLiveButton setTitle:@"开始直播" forState:UIControlStateNormal];
        [self.modelEngineVoip stopLiveStream:self.session];
        
    }
}

- (UIButton*)startPushLiveButton{
    if(! _startPushLiveButton){
        _startPushLiveButton = [UIButton new];
        //位置
        _startPushLiveButton.frame = CGRectMake((XJScreenW - 200) * 0.5, XJScreenH - 150, 200, 40);
        _startPushLiveButton.layer.cornerRadius = _startPlayLiveButton.frame.size.height * 0.5;
        [_startPushLiveButton setTitleColor:[UIColor blackColor] forState:UIControlStateNormal];
        [_startPushLiveButton.titleLabel setFont:[UIFont systemFontOfSize:16]];
        [_startPushLiveButton setTitle:@"开始直播" forState:UIControlStateNormal];
        [_startPushLiveButton setBackgroundColor:[UIColor grayColor]];
        _startPushLiveButton.exclusiveTouch = YES;
        [_startPushLiveButton addTarget:self action:@selector(startPush:)forControlEvents:UIControlEventTouchUpInside];
        
    }
    return _startPushLiveButton;
}

- (void)dealloc
{
    if( self.session != nil ) {
        [self.modelEngineVoip stopLiveStream:self.session];
        self.session = nil;
    }
    [super dealloc];
}

@end
