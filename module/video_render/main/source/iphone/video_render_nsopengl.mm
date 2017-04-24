/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "engine_configurations.h"
#if defined(IPHONE_GLES_RENDERING)

#include "video_render_nsopengl.h"
#include "critical_section_wrapper.h"
#include "event_wrapper.h"
#include "trace.h"
#include "thread_wrapper.h"
#include "libyuv.h"

#ifdef seanDebugRender
const char *g_render_noopengl = NULL;
#endif

namespace cloopenwebrtc {

VideoChannelNSOpenGL::VideoChannelNSOpenGL(ECIOSDisplay *window, int iId, VideoRenderNSOpenGL* owner) :
_displayWindow( window),
_id( iId),
_owner( owner),
_width( 0),
_height( 0),
_startWidth( 0.0f),
_startHeight( 0.0f),
_stopWidth( 0.0f),
_stopHeight( 0.0f),
_stretchedWidth( 0),
_stretchedHeight( 0),
_oldStretchedHeight( 0),
_oldStretchedWidth( 0),
_xOldWidth( 0),
_yOldHeight( 0),
_buffer( 0),
_bufferSize( 0),
_incommingBufferSize( 0),
_bufferIsUpdated( false),
_numberOfStreams( 0),
_pixelFormat( GL_RGBA),
_pixelDataType( GL_UNSIGNED_BYTE),
_texture( 0),
_bVideoSizeStartedChanging(false)
#ifdef seanDebugRender
    ,_debugFile(NULL)
#endif
{
#ifdef seanDebugRender
    if (g_render_noopengl) {
        _debugFile = fopen(g_render_noopengl, "wb");
    }
#endif

}

VideoChannelNSOpenGL::~VideoChannelNSOpenGL()
{
    if (_buffer)
    {
        delete [] _buffer;
        _buffer = NULL;
    }
#ifdef seanDebugRender
    if (_debugFile) {
        fflush(_debugFile);
        fclose(_debugFile);
    }
#endif
}

int VideoChannelNSOpenGL::ChangeWindow(ECIOSDisplay *window)
{
//    _owner->UnlockAGLCntx();
    _owner->LockAGLCntx();
    _displayWindow  = window;
    _owner->UnlockAGLCntx();
    return 0;

}

WebRtc_Word32 VideoChannelNSOpenGL::GetChannelProperties(float& left,
        float& top,
        float& right,
        float& bottom)
{
    _owner->LockAGLCntx();
    
    left = _startWidth;
    top = _startHeight;
    right = _stopWidth;
    bottom = _stopHeight;

    _owner->UnlockAGLCntx();
    return 0;
}

WebRtc_Word32 VideoChannelNSOpenGL::RenderFrame(const WebRtc_UWord32 /*streamId*/, I420VideoFrame& videoFrame)
{

    _owner->LockAGLCntx();

    WEBRTC_TRACE(kTraceInfo, kTraceVideoRenderer, _id, "RenderFrame _width = %d,videoFrame.Width = %d,_height = %d,videoFrame.Height = %d",_width,(int)videoFrame.width(),_height,(int)videoFrame.height());

    
    if(_width != (int)videoFrame.width() ||
            _height != (int)videoFrame.height())
    {
        if(FrameSizeChange(videoFrame.width(), videoFrame.height(), 1) == -1)
        {
            _owner->UnlockAGLCntx();
            return -1;
        }
    }

    int heightt = videoFrame.height();
    int widtht = videoFrame.width();
    
    int a=0,i;
    for (i=0; i<heightt; i++)
    {
        memcpy(_buffer+a,videoFrame.buffer(kYPlane) + i * videoFrame.stride(kYPlane), widtht);
        a+=widtht;
    }
    for (i=0; i<heightt/2; i++)
    {
        memcpy(_buffer+a,videoFrame.buffer(kUPlane) + i * videoFrame.stride(kUPlane), widtht/2);
        a+=widtht/2;
    }
    for (i=0; i<heightt/2; i++)
    {
        memcpy(_buffer+a,videoFrame.buffer(kVPlane) + i * videoFrame.stride(kVPlane), widtht/2);
        a+=widtht/2;
    }
    
#ifdef seanDebugRender
    if (_debugFile) {
        fwrite(_buffer, a, 1, _debugFile);
        fflush(_debugFile);
    }
#endif
    
    int ret = DeliverFrame(_buffer, a, videoFrame.timestamp());

    _owner->UnlockAGLCntx();
    return ret;
}

int VideoChannelNSOpenGL::UpdateSize(int width, int height)
{
    _owner->LockAGLCntx();
    _width = width;
    _height = height;
    _owner->UnlockAGLCntx();
    return 0;
}

int VideoChannelNSOpenGL::UpdateStretchSize(int stretchHeight, int stretchWidth)
{

    _owner->LockAGLCntx();
    _stretchedHeight = stretchHeight;
    _stretchedWidth = stretchWidth;
    _owner->UnlockAGLCntx();
    return 0;
}

int VideoChannelNSOpenGL::FrameSizeChange(int width, int height, int numberOfStreams)
{
    //  We got a new frame size from VideoAPI, prepare the buffer

    _owner->LockAGLCntx();

    if (width == _width && _height == height)
    {
        // We already have a correct buffer size
        _numberOfStreams = numberOfStreams;
        _owner->UnlockAGLCntx();
        return 0;
    }
    if (_width*_height != width*height) {
        if (_buffer) {
            delete []_buffer;
            _buffer = NULL;
        }
        size_t tmpSize = width*height*1.5;
        _buffer = new unsigned char[tmpSize+1];
        memset(_buffer, 0, tmpSize+1);
    }
    _width = width;
    _height = height;
    
    _owner->UnlockAGLCntx();
    return 0;
}

int VideoChannelNSOpenGL::DeliverFrame(unsigned char* buffer, int bufferSize, unsigned int /*timeStamp90kHz*/)
{
    _owner->LockAGLCntx();
    
//    mblk_t mk;
//    mk.data_ptr = buffer;
//    mk.datalen = bufferSize;
//    mk.h = _height;
//    mk.w = _width;
    
    WEBRTC_TRACE(kTraceInfo, kTraceVideoRenderer, _id, "DeliverFrame h=%d w=%d datalen=%d", _height, _width, bufferSize);
    [_displayWindow renderI420Frame:(void *)buffer width:(NSInteger)_width height:(NSInteger)_height];

    _owner->UnlockAGLCntx();
    return 0;
}

int VideoChannelNSOpenGL::SetStreamSettings(int /*streamId*/, float startWidth, float startHeight, float stopWidth, float stopHeight)
{
    _owner->LockAGLCntx();

    _startWidth = startWidth;
    _stopWidth = stopWidth;
    _startHeight = startHeight;
    _stopHeight = stopHeight;

    int oldNumberOfStreams = _numberOfStreams;

    _width = 0;
    _height = 0;

    int retVal = FrameSizeChange(stopWidth-startWidth, stopHeight-startHeight, oldNumberOfStreams);

    _owner->UnlockAGLCntx();
    return retVal;
}

int VideoChannelNSOpenGL::SetStreamCropSettings(int /*streamId*/, float /*startWidth*/, float /*startHeight*/, float /*stopWidth*/, float /*stopHeight*/)
{
    return -1;
}

    
/*
 *
 *    VideoRenderNSOpenGL
 *
 */

VideoRenderNSOpenGL::VideoRenderNSOpenGL(UIView *windowRef, bool fullScreen, int iId) :
//_windowRef(windowRef),
_id( iId),
_nsglContextCritSec( *CriticalSectionWrapper::CreateCriticalSection()),
_windowRect( ),
_windowWidth( 0),
_windowHeight( 0),
_nsglChannels( ),
_zOrderToChannel( )
{
    UIView *parentView = (UIView *)windowRef;
    if(parentView){
        _windowRef = [[ECIOSDisplay alloc] initWithFrame:CGRectMake(0, 0, parentView.frame.size.width, parentView.frame.size.height)];
        _windowRef.parentView = parentView;
        _windowRef.contentMode = parentView.contentMode;
    }
    
    GetWindowRect(_windowRect);
    _windowWidth = _windowRect.right - _windowRect.left;
    _windowHeight = _windowRect.bottom - _windowRect.top;    
}

int VideoRenderNSOpenGL::ChangeWindow(UIView* newWindowRef)
{
    LockAGLCntx();
    UIView *parentView = newWindowRef;
    
    if (_windowRef) {
        WEBRTC_TRACE(kTraceStream, kTraceVideoRenderer, _id, "OpenGL view parent changed (%p -> %p)", _windowRef.parentView, parentView);
        _windowRef.frame = CGRectMake(0, 0, parentView.frame.size.width, parentView.frame.size.height);
        [_windowRef performSelectorOnMainThread:@selector(stopRendering:) withObject:nil waitUntilDone:NO];
    } else if (parentView == nil) {
        return 0;
    } else {
        _windowRef = [[ECIOSDisplay alloc] initWithFrame:CGRectMake(0, 0, parentView.frame.size.width, parentView.frame.size.height)];
    }
    _windowRef.parentView = parentView;
    _windowRef.contentMode = parentView.contentMode;
    if (parentView)
        [_windowRef performSelectorOnMainThread:@selector(startRendering:) withObject:nil waitUntilDone:NO];
    
    GetWindowRect(_windowRect);
    _windowWidth = _windowRect.right - _windowRect.left;
    _windowHeight = _windowRect.bottom - _windowRect.top;
    
    int error = 0;
    std::map<int, VideoChannelNSOpenGL*>::iterator it = _nsglChannels.begin();
    while (it!= _nsglChannels.end())
    {
        error |= (it->second)->ChangeWindow(_windowRef);
        it++;
    }
    if(error != 0)
    {
        UnlockAGLCntx();
        return -1;
    }

    UnlockAGLCntx();
    return 0;
}

/* Check if the thread and event already exist. 
 * If so then they will simply be restarted
 * If not then create them and continue
 */
WebRtc_Word32 VideoRenderNSOpenGL::StartRender()
{
    LockAGLCntx();
    [_windowRef performSelectorOnMainThread:@selector(startRendering:) withObject:nil waitUntilDone:NO];
//    [_windowRef startRendering:nil];
    UnlockAGLCntx();
    return 0;
}
    
WebRtc_Word32 VideoRenderNSOpenGL::StopRender()
{
    LockAGLCntx();
    [_windowRef performSelectorOnMainThread:@selector(stopRendering:) withObject:nil waitUntilDone:NO];
//    [_windowRef stopRendering:nil];
    UnlockAGLCntx();
    return 0;
}

VideoRenderNSOpenGL::~VideoRenderNSOpenGL()
{
    // Delete all channels
    std::map<int, VideoChannelNSOpenGL*>::iterator it = _nsglChannels.begin();
    while (it!= _nsglChannels.end())
    {
        delete it->second;
        _nsglChannels.erase(it);
        it = _nsglChannels.begin();
    }
    _nsglChannels.clear();

    // Clean the zOrder map
    std::multimap<int, int>::iterator zIt = _zOrderToChannel.begin();
    while(zIt != _zOrderToChannel.end())
    {
        _zOrderToChannel.erase(zIt);
        zIt = _zOrderToChannel.begin();
    }
    _zOrderToChannel.clear();
    
    [_windowRef release];
}

int VideoRenderNSOpenGL::Init()
{
    LockAGLCntx();
    UnlockAGLCntx();
    return 0;
}

VideoChannelNSOpenGL* VideoRenderNSOpenGL::CreateNSGLChannel(int channel, int zOrder, float startWidth, float startHeight, float stopWidth, float stopHeight)
{
    CriticalSectionScoped cs(&_nsglContextCritSec);

    if (HasChannel(channel))
    {
        return NULL;
    }

    if (_zOrderToChannel.find(zOrder) != _zOrderToChannel.end())
    {

    }

    VideoChannelNSOpenGL* newAGLChannel = new VideoChannelNSOpenGL(_windowRef, _id, this);
    if (newAGLChannel->SetStreamSettings(0, startWidth, startHeight, stopWidth, stopHeight) == -1)
    {
        if (newAGLChannel)
        {
            delete newAGLChannel;
            newAGLChannel = NULL;
        }

        return NULL;
    }

    _nsglChannels[channel] = newAGLChannel;
    _zOrderToChannel.insert(std::pair<int, int>(zOrder, channel));

    WEBRTC_TRACE(kTraceInfo, kTraceVideoRenderer, _id, "%s successfully created NSGL channel number %d", __FUNCTION__, channel);

    return newAGLChannel;
}

int VideoRenderNSOpenGL::DeleteAllNSGLChannels()
{
    CriticalSectionScoped cs(&_nsglContextCritSec);

    std::map<int, VideoChannelNSOpenGL*>::iterator it;
    it = _nsglChannels.begin();

    while (it != _nsglChannels.end())
    {
        VideoChannelNSOpenGL* channel = it->second;
        WEBRTC_TRACE(kTraceInfo, kTraceVideoRenderer, _id, "%s Deleting channel %d", __FUNCTION__, channel);
        delete channel;
        it++;
    }
    _nsglChannels.clear();
    return 0;
}

WebRtc_Word32 VideoRenderNSOpenGL::DeleteNSGLChannel(const WebRtc_UWord32 channel)
{
    
    CriticalSectionScoped cs(&_nsglContextCritSec);

    std::map<int, VideoChannelNSOpenGL*>::iterator it;
    it = _nsglChannels.find(channel);
    if (it != _nsglChannels.end())
    {
        delete it->second;
        _nsglChannels.erase(it);
    }
    else
    {
        return -1;
    }

    std::multimap<int, int>::iterator zIt = _zOrderToChannel.begin();
    while( zIt != _zOrderToChannel.end())
    {
        if (zIt->second == (int)channel)
        {
            _zOrderToChannel.erase(zIt);
            break;
        }
        zIt++;
    }

    return 0;
}

WebRtc_Word32 VideoRenderNSOpenGL::GetChannelProperties(const WebRtc_UWord16 streamId,
        WebRtc_UWord32& zOrder,
        float& left,
        float& top,
        float& right,
        float& bottom)
{

    CriticalSectionScoped cs(&_nsglContextCritSec);

    bool channelFound = false;

    // Loop through all channels until we find a match.
    // From that, get zorder.
    // From that, get T, L, R, B
    for (std::multimap<int, int>::reverse_iterator rIt = _zOrderToChannel.rbegin();
            rIt != _zOrderToChannel.rend();
            rIt++)
    {
        if(streamId == rIt->second)
        {
            channelFound = true;

            zOrder = rIt->second;

            std::map<int, VideoChannelNSOpenGL*>::iterator rIt = _nsglChannels.find(streamId);
            VideoChannelNSOpenGL* tempChannel = rIt->second;

            if(-1 == tempChannel->GetChannelProperties(left, top, right, bottom) )
            {
                return -1;
            }
            break;
        }
    }

    if(false == channelFound)
    {

        return -1;
    }

    return 0;
}

bool VideoRenderNSOpenGL::HasChannels()
{
    CriticalSectionScoped cs(&_nsglContextCritSec);

    if (_nsglChannels.begin() != _nsglChannels.end())
    {
        return true;
    }
    return false;
}

bool VideoRenderNSOpenGL::HasChannel(int channel)
{

    CriticalSectionScoped cs(&_nsglContextCritSec);

    std::map<int, VideoChannelNSOpenGL*>::iterator it = _nsglChannels.find(channel);

    if (it != _nsglChannels.end())
    {
        return true;
    }
    return false;
}

int VideoRenderNSOpenGL::GetChannels(std::list<int>& channelList)
{

    CriticalSectionScoped cs(&_nsglContextCritSec);

    std::map<int, VideoChannelNSOpenGL*>::iterator it = _nsglChannels.begin();

    while (it != _nsglChannels.end())
    {
        channelList.push_back(it->first);
        it++;
    }

    return 0;
}

VideoChannelNSOpenGL* VideoRenderNSOpenGL::ConfigureNSGLChannel(int channel, int zOrder, float startWidth, float startHeight, float stopWidth, float stopHeight)
{

    CriticalSectionScoped cs(&_nsglContextCritSec);

    std::map<int, VideoChannelNSOpenGL*>::iterator it = _nsglChannels.find(channel);

    if (it != _nsglChannels.end())
    {
        VideoChannelNSOpenGL* aglChannel = it->second;
        if (aglChannel->SetStreamSettings(0, startWidth, startHeight, stopWidth, stopHeight) == -1)
        {
            WEBRTC_TRACE(kTraceError, kTraceVideoRenderer, _id, "%s failed to set stream settings: channel %d. channel=%d zOrder=%d startWidth=%d startHeight=%d stopWidth=%d stopHeight=%d",
                    __FUNCTION__, channel, zOrder, startWidth, startHeight, stopWidth, stopHeight);
            return NULL;
        }
        WEBRTC_TRACE(kTraceInfo, kTraceVideoRenderer, _id, "%s Configuring channel %d. channel=%d zOrder=%d startWidth=%d startHeight=%d stopWidth=%d stopHeight=%d",
                __FUNCTION__, channel, zOrder, startWidth, startHeight, stopWidth, stopHeight);

        std::multimap<int, int>::iterator it = _zOrderToChannel.begin();
        while(it != _zOrderToChannel.end())
        {
            if (it->second == channel)
            {
                if (it->first != zOrder)
                {
                    _zOrderToChannel.erase(it);
                    _zOrderToChannel.insert(std::pair<int, int>(zOrder, channel));
                }
                break;
            }
            it++;
        }
        return aglChannel;
    }

    return NULL;
}

int VideoRenderNSOpenGL::GetWindowRect(Rect& rect)
{
    CriticalSectionScoped cs(&_nsglContextCritSec);

    if (_windowRef)
    {
        rect.top = [_windowRef frame].origin.y;
        rect.left = [_windowRef frame].origin.x;
        rect.bottom = [_windowRef frame].origin.y + [_windowRef frame].size.height;
        rect.right = [_windowRef frame].origin.x + [_windowRef frame].size.width;
        return 0;
    }
    else
    {
        return -1;
    }
}

WebRtc_Word32 VideoRenderNSOpenGL::ChangeUniqueID(WebRtc_Word32 id)
{
    CriticalSectionScoped cs(&_nsglContextCritSec);
    _id = id;
    return 0;
}

WebRtc_Word32 VideoRenderNSOpenGL::SetText(const WebRtc_UWord8 /*textId*/,
        const WebRtc_UWord8* /*text*/,
        const WebRtc_Word32 /*textLength*/,
        const WebRtc_UWord32 /*textColorRef*/,
        const WebRtc_UWord32 /*backgroundColorRef*/,
        const float /*left*/,
        const float /*top*/,
        const float /*right*/,
        const float /*bottom*/)
{
    return 0;

}

void VideoRenderNSOpenGL::LockAGLCntx()
{
    _nsglContextCritSec.Enter();
}
void VideoRenderNSOpenGL::UnlockAGLCntx()
{
    _nsglContextCritSec.Leave();
}

} //namespace cloopenwebrtc

#endif // IPHONE_GLES_RENDERING
