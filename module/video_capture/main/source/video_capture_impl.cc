/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "video_capture_impl.h"

#include <stdlib.h>

#include "../common_video/source/libyuv/include/webrtc_libyuv.h"
#include "module_common_types.h"
#include "video_capture_config.h"
#include "../system_wrappers/include/clock.h"
#include "../system_wrappers/include/critical_section_wrapper.h"
#include "../system_wrappers/include/logging.h"
#include "../system_wrappers/include/ref_count.h"
#include "../system_wrappers/include/tick_util.h"
#include "../system_wrappers/include/trace_event.h"

#include "../system_wrappers/include/trace.h"
#include "../base/timeutils.h"

namespace yuntongxunwebrtc
{
namespace videocapturemodule
{
VideoCaptureModule* VideoCaptureImpl::Create(
    const int32_t id,
    VideoCaptureExternal*& externalCapture)
{
    RefCountImpl<VideoCaptureImpl>* implementation = new RefCountImpl<VideoCaptureImpl>(id);
    externalCapture = implementation;
    return implementation;
}

const char* VideoCaptureImpl::CurrentDeviceName() const
{
    return _deviceUniqueId;
}

// static
int32_t VideoCaptureImpl::RotationFromDegrees(int degrees,
                                              VideoCaptureRotation* rotation) {
  switch (degrees) {
    case 0:
      *rotation = kCameraRotate0;
      return 0;
    case 90:
      *rotation = kCameraRotate90;
      return 0;
    case 180:
      *rotation = kCameraRotate180;
      return 0;
    case 270:
      *rotation = kCameraRotate270;
      return 0;
    default:
      return -1;;
  }
}

// static
int32_t VideoCaptureImpl::RotationInDegrees(VideoCaptureRotation rotation,
                                            int* degrees) {
  switch (rotation) {
    case kCameraRotate0:
      *degrees = 0;
      return 0;
    case kCameraRotate90:
      *degrees = 90;
      return 0;
    case kCameraRotate180:
      *degrees = 180;
      return 0;
    case kCameraRotate270:
      *degrees = 270;
      return 0;
  }
  return -1;
}

int32_t VideoCaptureImpl::ChangeUniqueId(const int32_t id)
{
    _id = id;
    return 0;
}

// returns the number of milliseconds until the module want a worker thread to call Process
int64_t VideoCaptureImpl::TimeUntilNextProcess()
{
    CriticalSectionScoped cs(&_callBackCs);
    const int64_t kProcessIntervalMs = 300;
    return kProcessIntervalMs -
        (TickTime::Now() - _lastProcessTime).Milliseconds();
}

// Process any pending tasks such as timeouts
int32_t VideoCaptureImpl::Process()
{
    CriticalSectionScoped cs(&_callBackCs);

    const TickTime now = TickTime::Now();
    _lastProcessTime = TickTime::Now();

    // Handle No picture alarm

    if (_lastProcessFrameCount.Ticks() == _incomingFrameTimes[0].Ticks() &&
        _captureAlarm != Raised)
    {
        if (_noPictureAlarmCallBack && _captureCallBack)
        {
            _captureAlarm = Raised;
            _captureCallBack->OnNoPictureAlarm(_id, _captureAlarm);
        }
    }
    else if (_lastProcessFrameCount.Ticks() != _incomingFrameTimes[0].Ticks() &&
             _captureAlarm != Cleared)
    {
        if (_noPictureAlarmCallBack && _captureCallBack)
        {
            _captureAlarm = Cleared;
            _captureCallBack->OnNoPictureAlarm(_id, _captureAlarm);

        }
    }

    // Handle frame rate callback
    if ((now - _lastFrameRateCallbackTime).Milliseconds()
        > kFrameRateCallbackInterval)
    {
        if (_frameRateCallBack && _captureCallBack)
        {
            const uint32_t frameRate = CalculateFrameRate(now);
            _captureCallBack->OnCaptureFrameRate(_id, frameRate);
        }
        _lastFrameRateCallbackTime = now; // Can be set by EnableFrameRateCallback

    }

    _lastProcessFrameCount = _incomingFrameTimes[0];

	//add
	// Handle start image frame rates.
	if (_startImageFrameIntervall
		&& (now - _lastSentStartImageTime).Milliseconds() >= _startImageFrameIntervall)
	{
		_lastSentStartImageTime = now;
		if (_dataCallBack)
		{
			_captureFrame.CopyFrame(_startImage);
			_captureFrame.set_render_time_ms(TickTime::MillisecondTimestamp());
			_dataCallBack->OnIncomingCapturedFrame(_id, _captureFrame);
		}
	}

    return 0;
}

VideoCaptureImpl::VideoCaptureImpl(const int32_t id)
    : _id(id),
      _deviceUniqueId(NULL),
      _apiCs(*CriticalSectionWrapper::CreateCriticalSection()),
      _captureDelay(0),
      _requestedCapability(),
      _callBackCs(*CriticalSectionWrapper::CreateCriticalSection()),
      _lastProcessTime(TickTime::Now()),
      _lastFrameRateCallbackTime(TickTime::Now()),
      _frameRateCallBack(false),
      _noPictureAlarmCallBack(false),
      _captureAlarm(Cleared),
      _setCaptureDelay(0),
      _dataCallBack(NULL),
      _captureCallBack(NULL),
      _lastProcessFrameCount(TickTime::Now()),
      _rotateFrame(kVideoRotation_0),
      last_capture_time_(0),
      delta_ntp_internal_ms_(
          Clock::GetRealTimeClock()->CurrentNtpInMilliseconds() -
          TickTime::MillisecondTimestamp()),
		_startImage(), _startImageFrameIntervall(0),
_lastSentStartImageTime(TickTime::Now()){
    _requestedCapability.width = kDefaultWidth;
    _requestedCapability.height = kDefaultHeight;
    _requestedCapability.maxFPS = 30;
    _requestedCapability.rawType = kVideoI420;
    _requestedCapability.codecType = kVideoCodecUnknown;
    memset(_incomingFrameTimes, 0, sizeof(_incomingFrameTimes));
}

VideoCaptureImpl::~VideoCaptureImpl()
{
    DeRegisterCaptureDataCallback();
    DeRegisterCaptureCallback();
    delete &_callBackCs;
    delete &_apiCs;

    if (_deviceUniqueId)
        delete[] _deviceUniqueId;
}

void VideoCaptureImpl::RegisterCaptureDataCallback(
    VideoCaptureDataCallback& dataCallBack) {
    CriticalSectionScoped cs(&_apiCs);
    CriticalSectionScoped cs2(&_callBackCs);
    _dataCallBack = &dataCallBack;
}

void VideoCaptureImpl::DeRegisterCaptureDataCallback() {
    CriticalSectionScoped cs(&_apiCs);
    CriticalSectionScoped cs2(&_callBackCs);
    _dataCallBack = NULL;
}
void VideoCaptureImpl::RegisterCaptureCallback(VideoCaptureFeedBack& callBack) {

    CriticalSectionScoped cs(&_apiCs);
    CriticalSectionScoped cs2(&_callBackCs);
    _captureCallBack = &callBack;
}
void VideoCaptureImpl::DeRegisterCaptureCallback() {

    CriticalSectionScoped cs(&_apiCs);
    CriticalSectionScoped cs2(&_callBackCs);
    _captureCallBack = NULL;
}
void VideoCaptureImpl::SetCaptureDelay(int32_t delayMS) {
    CriticalSectionScoped cs(&_apiCs);
    _captureDelay = delayMS;
}
int32_t VideoCaptureImpl::CaptureDelay()
{
    CriticalSectionScoped cs(&_apiCs);
    return _setCaptureDelay;
}

int32_t VideoCaptureImpl::DeliverCapturedFrame(I420VideoFrame& captureFrame,
                                               int64_t capture_time) {

  UpdateFrameCount();  // frame count used for local frame rate callback.

  const bool callOnCaptureDelayChanged = _setCaptureDelay != _captureDelay;
  // Capture delay changed
  if (_setCaptureDelay != _captureDelay) {
      _setCaptureDelay = _captureDelay;
  }

  // Set the capture time
  if (capture_time != 0) {
    captureFrame.set_render_time_ms(capture_time - delta_ntp_internal_ms_);
  } else {
    captureFrame.set_render_time_ms(TickTime::MillisecondTimestamp());
  }

  if (captureFrame.render_time_ms() == last_capture_time_) {
    // We don't allow the same capture time for two frames, drop this one.
    return -1;
  }
  last_capture_time_ = captureFrame.render_time_ms();

  if (_dataCallBack) {
    if (callOnCaptureDelayChanged) {
      _dataCallBack->OnCaptureDelayChanged(_id, _captureDelay);
    }
    _dataCallBack->OnIncomingCapturedFrame(_id, captureFrame);
  }

  return 0;
}

int32_t VideoCaptureImpl::IncomingFrame(
    uint8_t* videoFrame,
    size_t videoFrameLength,
    const VideoCaptureCapability& frameInfo,
    int64_t captureTime/*=0*/)
{
    CriticalSectionScoped cs(&_apiCs);
    CriticalSectionScoped cs2(&_callBackCs);

    const int32_t width = frameInfo.width;
    const int32_t height = frameInfo.height;
    RawVideoType rawFrameType = frameInfo.rawType;
#ifdef __ANDROID__
    //use video frame fist bit for indicating android raw video type.
    // rawFrameType = ((videoFrame[0] & 0x01) == 0x00) ? kVideoNV21 : kVideoRGBA;
    // todo: now force to close android video filter, needing support it someday.
    rawFrameType = kVideoNV21;
#endif

    if (frameInfo.codecType == kVideoCodecUnknown)
    {
        // Not encoded, convert to I420.
        const VideoType commonVideoType =
                  RawVideoTypeToCommonVideoVideoType(rawFrameType);

        if (rawFrameType != kVideoMJPEG &&
            CalcBufferSize(commonVideoType, width,
                           abs(height)) != videoFrameLength)
        {
            LOG(LS_ERROR) << "Wrong incoming frame length.";
            return -1;
        }

        int stride_y = width;
        int stride_uv = (width + 1) / 2;
        int target_width = width;
        int target_height = height;
        // Rotating resolution when for 90/270 degree rotations.
        if (_rotateFrame == kVideoRotation_90 || _rotateFrame == kVideoRotation_270)  {
            target_width = abs(height);
            target_height = width;
        }
        // TODO(mikhal): Update correct aligned stride values.
        //Calc16ByteAlignedStride(target_width, &stride_y, &stride_uv);
        // Setting absolute height (in case it was negative).
        // In Windows, the image starts bottom left, instead of top left.
        // Setting a negative source height, inverts the image (within LibYuv).
        int ret = _captureFrame.CreateEmptyFrame(target_width,
                                                 abs(target_height),
                                                 stride_y,
                                                 stride_uv, stride_uv);
        if (ret < 0)
        {
            LOG(LS_ERROR) << "Failed to create empty frame, this should only "
                             "happen due to bad parameters.";
            return -1;
        }
        
        const int conversionResult = ConvertToI420(commonVideoType,
                                                   videoFrame,
                                                   0, 0,  // No cropping
                                                   width, height,
                                                   videoFrameLength,
                                                   _rotateFrame,
                                                   &_captureFrame);
        if (conversionResult < 0)
        {
          LOG(LS_ERROR) << "Failed to convert capture frame from type "
                        << rawFrameType << "to I420.";
            return -1;
        }
        DeliverCapturedFrame(_captureFrame, captureTime);
    }
    else // Encoded format
    {
        assert(false);
        return -1;
    }

    return 0;
}

int32_t VideoCaptureImpl::IncomingI420VideoFrame(I420VideoFrame* video_frame,
                                                 int64_t captureTime) {

  CriticalSectionScoped cs(&_apiCs);
  CriticalSectionScoped cs2(&_callBackCs);
  DeliverCapturedFrame(*video_frame, captureTime);

  return 0;
}

int32_t VideoCaptureImpl::SetCaptureRotation(VideoCaptureRotation rotation) {
  CriticalSectionScoped cs(&_apiCs);
  CriticalSectionScoped cs2(&_callBackCs);
  switch (rotation){
    case kCameraRotate0:
      _rotateFrame = kVideoRotation_0;
      break;
    case kCameraRotate90:
      _rotateFrame = kVideoRotation_90;
      break;
    case kCameraRotate180:
      _rotateFrame = kVideoRotation_180;
      break;
    case kCameraRotate270:
      _rotateFrame = kVideoRotation_270;
      break;
    default:
      return -1;
  }
  return 0;
}

void VideoCaptureImpl::EnableFrameRateCallback(const bool enable) {
    CriticalSectionScoped cs(&_apiCs);
    CriticalSectionScoped cs2(&_callBackCs);
    _frameRateCallBack = enable;
    if (enable)
    {
        _lastFrameRateCallbackTime = TickTime::Now();
    }
}

void VideoCaptureImpl::EnableNoPictureAlarm(const bool enable) {
    CriticalSectionScoped cs(&_apiCs);
    CriticalSectionScoped cs2(&_callBackCs);
    _noPictureAlarmCallBack = enable;
}

void VideoCaptureImpl::UpdateFrameCount()
{
    if (_incomingFrameTimes[0].MicrosecondTimestamp() == 0)
    {
        // first no shift
    }
    else
    {
        // shift
        for (int i = (kFrameRateCountHistorySize - 2); i >= 0; i--)
        {
            _incomingFrameTimes[i + 1] = _incomingFrameTimes[i];
        }
    }
    _incomingFrameTimes[0] = TickTime::Now();
}

uint32_t VideoCaptureImpl::CalculateFrameRate(const TickTime& now)
{
    int32_t num = 0;
    int32_t nrOfFrames = 0;
    for (num = 1; num < (kFrameRateCountHistorySize - 1); num++)
    {
        if (_incomingFrameTimes[num].Ticks() <= 0
            || (now - _incomingFrameTimes[num]).Milliseconds() > kFrameRateHistoryWindowMs) // don't use data older than 2sec
        {
            break;
        }
        else
        {
            nrOfFrames++;
        }
    }
    if (num > 1)
    {
        int64_t diff = (now - _incomingFrameTimes[num - 1]).Milliseconds();
        if (diff > 0)
        {
            return uint32_t((nrOfFrames * 1000.0f / diff) + 0.5f);
        }
    }

    return nrOfFrames;
}

WebRtc_Word32 VideoCaptureImpl::StartSendImage(const I420VideoFrame& videoFrame,
	WebRtc_Word32 frameRate)
{
	CriticalSectionScoped cs(&_apiCs);
	CriticalSectionScoped cs2(&_callBackCs);
	if (frameRate < 1 || frameRate > kMaxFrameRate)
	{
		WEBRTC_TRACE(yuntongxunwebrtc::kTraceError, yuntongxunwebrtc::kTraceVideoCapture, _id,
			"StartSendImage Invalid parameter. frameRate %d", (int) frameRate);
		return -1;;
	}
	_startImage.CopyFrame(videoFrame);
	_startImageFrameIntervall = 1000 / frameRate;
	_lastSentStartImageTime = TickTime::Now();
	return 0;

}
WebRtc_Word32 VideoCaptureImpl::StopSendImage()
{
	CriticalSectionScoped cs(&_apiCs);
	CriticalSectionScoped cs2(&_callBackCs);
	_startImageFrameIntervall = 0;
	return 0;
}

}  // namespace videocapturemodule
}  // namespace webrtc
