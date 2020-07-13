//#include <d3dx9.h>

#include "linux_render.h"
#include <map>

#include "rtc_base/logging.h"
#include "third_party/libyuv/include/libyuv.h"
#include "third_party/libyuv/include/libyuv/convert_argb.h"
#include "system_wrappers/include/sleep.h"
#include "api/video/i420_buffer.h"
#include "api/video/video_frame.h"
#include "api/video/video_frame_buffer.h"
#include "common_video/libyuv/include/webrtc_libyuv.h"
static int dispCount = 0;
#define DISP_MAX 128
static Display *dispArray[DISP_MAX];


LinuxRender::LinuxRender(void* window,const int render_mode,bool mirror)
    : _renderMode(render_mode),      
      _screenUpdateThread(new rtc::PlatformThread(ScreenUpdateThreadProc,
                                                  this,
                                                  "win_thread_render_update",
                                                  rtc::kNormalPriority)),
      _running(false),
      _mirrorRender(mirror),
      _display(NULL), _shminfo(), _image(NULL), _gc(NULL),
      _width(DEFAULT_RENDER_FRAME_WIDTH), 
      _height(DEFAULT_RENDER_FRAME_HEIGHT), _outWidth(0), _outHeight(0),
      _xPos(0), _yPos(0), _dispCount(0),_buffer(NULL),
      _top(0.0), _left(0.0), _right(1.0), _bottom(1.0)
      {
  // _screenUpdateEvent = EventTimerWrapper::Create();
        _window = reinterpret_cast<Window>(window);
	InitWindow(_window, _left,_top,_right,_bottom);
}

LinuxRender::~LinuxRender() {
  if (_running) {
    _running = false;
    _screenUpdateThread->Stop();
  
    RemoveRenderer();
    if (_gc) {
      XFreeGC(_display, _gc);
      _gc = NULL;
    }
    if (_display)
    {
        XCloseDisplay(_display);
        _display = NULL;
    }
 } 
}

int LinuxRender::Init()
{
	return 0;
}

int LinuxRender::InitWindow(Window window, float left, float top,
                              float right, float bottom) {
  
    rtc::CritScope lock(&render_buffer_lock_);

    _window = window;
    _left = left;
    _right = right;
    _top = top;
    _bottom = bottom;

    _display = XOpenDisplay(NULL); // Use default display
    if (!_window || !_display)
    {
        return -1;
    }

    if (dispCount < DISP_MAX)
    {
        dispArray[dispCount] = _display;
        _dispCount = dispCount;
        dispCount++;
    }
    else
    {
        return -1;
    }

    if ((1 < left || left < 0) || (1 < top || top < 0) || (1 < right || right
            < 0) || (1 < bottom || bottom < 0))
    {
        return -1;
    }


    int x, y;
    unsigned int winWidth, winHeight, borderwidth, depth;
    Window rootret;
    if (XGetGeometry(_display, _window, &rootret, &x, &y, &winWidth,
                     &winHeight, &borderwidth, &depth) == 0)
    {
        return -1;
    }


    _xPos = (int32_t) (winWidth * left);
    _yPos = (int32_t) (winHeight * top);
    _outWidth = (int32_t) (winWidth * (right - left));
    _outHeight = (int32_t) (winHeight * (bottom - top));
    if (_outWidth % 2)
        _outWidth++;  
    if (_outHeight % 2)
        _outHeight++;

    
    _gc = XCreateGC(_display, _window, 0, 0);
    if (!_gc) {
      assert(false);
      return -1;
    }

    if (CreateLocalRenderer(_outWidth, _outHeight) == -1)
    {
        return -1;
    }
    return 0;



  if (!_screenUpdateThread) {
    RTC_LOG(LS_ERROR) << "LinuxRender Thread not created";
    return -1;
  }

  return 0;
}



/*
 *
 *    Rendering process
 *
 */
void LinuxRender::ScreenUpdateThreadProc(void* obj) {
  static_cast<LinuxRender*>(obj)->ScreenUpdateProcess();
}

bool LinuxRender::ScreenUpdateProcess() {

   //add for test
  //while (_running) {
 while (0) {
    rtc::CritScope lock(&render_buffer_lock_);
    webrtc::VideoFrame* frame_render = render_buffer_.FrameToRender();
    if (frame_render) {
      RTC_LOG(LS_INFO) << "render a frame, width:" << _width << " _height:" << _height<<" frame_render->width():"<<frame_render->width()<<"  frame_render->height():"<< frame_render->height();

      if (_width != frame_render->width() ||
          _height != frame_render->height()) {
        if (FrameSizeChange(frame_render->width(), frame_render->height()) ==
            -1)
          return false;
      }

      DeliverFrame(frame_render);
    } else {
      RTC_LOG(LS_INFO) << "no frame this:" << this
                       << " theadid:" << &_screenUpdateThread;
      webrtc::SleepMs(1);      
      continue;
    }
    render_buffer_.ReturnFrame(frame_render);
  }

  return true;
}





VideoRenderType LinuxRender::RenderType() {
  return kRenderX11;
}

int LinuxRender::RenderFrame(const webrtc::VideoFrame& videoFrame) {
  rtc::CritScope lock(&render_buffer_lock_);
  if (_width != videoFrame.width() || _height
      != videoFrame.height()) {
      //add for test
      if (FrameSizeChange(videoFrame.width(), videoFrame.height()) == -1) {
        return -1;
     }
  }
  return DeliverFrame(&videoFrame);

  //rtc::CritScope lock(&render_buffer_lock_);
  //return render_buffer_.AddFrame(&videoFrame);
  
}


int LinuxRender::CreateLocalRenderer(int width, int height)
{
    RTC_LOG(LS_INFO) << "CreateLocalRenderer, id: " << _Id; 
    rtc::CritScope lock(&render_buffer_lock_);

    if (!_window || !_display)
    {
        return -1;
    }

    if (_running)
    {
    	RTC_LOG(LS_INFO) << "Renderer already running, exits.";
        return -1;
    }

    _width = width;
    _height = height;

    _image = XShmCreateImage(_display, CopyFromParent, 24, ZPixmap, NULL,
                             &_shminfo, _outWidth, _outHeight); 
    _shminfo.shmid = shmget(IPC_PRIVATE, (_image->bytes_per_line
            * _image->height), IPC_CREAT | 0777);


    _shminfo.shmaddr = _image->data = (char*) shmat(_shminfo.shmid, 0, 0);
    if (_image->data == reinterpret_cast<char*>(-1))
    {
        return -1;
    }
    _buffer = (unsigned char*) _image->data;
    _shminfo.readOnly = False;

    if (!XShmAttach(_display, &_shminfo))
    {
        return -1;
    }
    XSync(_display, False);

    _running = true;
    return 0;
}


int LinuxRender::RemoveRenderer()
{
    RTC_LOG(LS_INFO) << "RemoveRenderer, id: " << _Id; 
    if (!_running)
    {
        return 0;
    }
    _running = false;

    XShmDetach(_display, &_shminfo);
    XDestroyImage( _image );
    _image = NULL;
    shmdt(_shminfo.shmaddr);
    _shminfo.shmaddr = NULL;
    _buffer = NULL;
    shmctl(_shminfo.shmid, IPC_RMID, 0);
    _shminfo.shmid = 0;
    return 0;
}

int LinuxRender::FrameSizeChange(int width, int height) {
  RTC_LOG(LS_INFO) << "FrameSizeChange, width: " << width
                   << " height: " << height;
  
    /* 
    rtc::CritScope lock(&render_buffer_lock_);
    if (_running)
    {
        RemoveRenderer();
    }
    if (CreateLocalRenderer(width, height) == -1)
    {
        return -1;
    }
   */
    return 0;
}

int LinuxRender::DeliverFrame(const webrtc::VideoFrame* video_frame) {
    rtc::CritScope lock(&render_buffer_lock_);


  if (!_running) {
    RTC_LOG(LS_INFO) << "sky _running is false " ;
    return 0;
  }

  if (!dispArray[_dispCount]) {
    RTC_LOG(LS_INFO) << "sky dispArray[_dispCount] is 0 " ;
    return -1;
  }
	
   rtc::scoped_refptr<webrtc::VideoFrameBuffer> tmp_frame_buf= ScaleVideoFrameBuffer(*video_frame->video_frame_buffer()->ToI420(),_outWidth, _outHeight);
   rtc::scoped_refptr<webrtc::I420BufferInterface> i420_buffer =tmp_frame_buf->GetI420();
   libyuv::ConvertFromI420(
      i420_buffer->DataY(), i420_buffer->StrideY(), i420_buffer->DataU(),
      i420_buffer->StrideU(), i420_buffer->DataV(), i420_buffer->StrideV(),
      _buffer, 0, _outWidth, _outHeight,
      ConvertVideoType( webrtc::VideoType::kARGB));

  XShmPutImage(_display, _window, _gc, _image, 0, 0, _xPos, _yPos, _outWidth,
              _outHeight, True);

  XSync(_display, False);

  return 0;
}

 int LinuxRender::StartRenderInternal() 
 {
  if (_running) {
    RTC_LOG(LS_WARNING) << "render thread already running.";
    return -1;
  }

  _running = true;
  //_screenUpdateThread->Start();
 
   return 0;
 }

 int LinuxRender::StopRenderInternal()
 {
   if (!_running)
     RTC_LOG(LS_WARNING) << "no render thread is running.";

   _running = false;
   //_screenUpdateThread->Stop();
   return 0;
 }
