/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "vie_file_capturer.h"

#include "texture_video_frame.h"
#include "webrtc_libyuv.h"
#include "module_common_types.h"
#include "process_thread.h"
#include "video_capture_factory.h"
#include "video_processing.h"
#include "video_render_defines.h"
#include "clock.h"
#include "critical_section_wrapper.h"
#include "event_wrapper.h"
#include "logging.h"
#include "thread_wrapper.h"
#include "trace_event.h"
#include "vie_image_process.h"
#include "overuse_frame_detector.h"
#include "vie_defines.h"
#include "vie_encoder.h"

//add by dingxf
#include "libyuv/mjpeg_decoder.h"
#if defined(WIN32)
#include <windows.h>
#endif

#if defined(WEBRTC_IOS) || defined(WEBRTC_MAC)
using namespace cloopenlibyuv;
#else
using namespace libyuv;
#endif

namespace cloopenwebrtc {


ViEFileCapturer::ViEFileCapturer(int capture_id,
                         int engine_id,
                         const Config& config,
                         ProcessThread& module_process_thread)
    : ViECapturer(capture_id, engine_id,config, module_process_thread)
	, captureStarted_(true)
	, bInitialized_(false)
	, video_frame_(nullptr)
	, lastSent_(0)
	, sendFileFrameMaxFps_(30)
	, avgTimePerFrame_(1000000.0 / sendFileFrameMaxFps_)
	, clock_(Clock::GetRealTimeClock())
    , fileCapture_cs_(CriticalSectionWrapper::CreateCriticalSection())
{
}

ViEFileCapturer::~ViEFileCapturer() {
	fileCapture_cs_->Enter();
	if (video_frame_)
	{
		delete video_frame_;
		video_frame_ = nullptr;
	}
	fileCapture_cs_->Leave();
}

ViEFileCapturer* ViEFileCapturer::CreateViEFileCapturer(
	int capture_id,
	int engine_id,
	const Config& config,
	const char* fileUTF8,
	const char* filesSplit,
	ProcessThread& module_process_thread) {
	ViEFileCapturer* capture = new ViEFileCapturer(capture_id, engine_id, config,
		module_process_thread);
	if (!capture ||
		capture->Init(fileUTF8, filesSplit) != 0) {
		delete capture;
		capture = NULL;
	}
	return capture;
}

int32_t ViEFileCapturer::Init(const char* fileUTF8, const char* filesSplit) {
		
	int ret = -1;
	if (fileUTF8)
	{
		FileExtType fet = GetFileExtType(fileUTF8);
		switch (fet)
		{
		case ExtType_Jpg:
		{
			CriticalSectionScoped cs(fileCapture_cs_.get());
			ret = ConvertJPEGToVideoFrame(fileUTF8, &video_frame_);
		}
		break;
#if defined(WIN32)
		case ExtType_Bmp:
		{
			CriticalSectionScoped cs(fileCapture_cs_.get());
			ret = ConvertBMPToVideoFrame(fileUTF8, &video_frame_);
		}
		break;
#endif
		default:
			break;
		} 
	}
	bInitialized_ = (ret == 0);
	lastSent_ = clock_->TimeInMicroseconds();
	return ret;
}

int ViEFileCapturer::FrameCallbackChanged() {
	if (capture_module_)
	{
		return ViECapturer::FrameCallbackChanged();
	}
	return 0;
}

int32_t  ViEFileCapturer::Start(const CaptureCapability& capture_capability) {
	if (capture_module_)
	{
		return ViECapturer::Start(capture_capability);
	}
	return 0;
}

int32_t ViEFileCapturer::Stop() {
	captureStarted_ = false;
	if (capture_module_)
	{
		return ViECapturer::Stop();
	}
	return 0;
}

bool ViEFileCapturer::Started() {
	if (capture_module_)
	{
		return ViECapturer::Started();
	}
	return captureStarted_;
}

const char* ViEFileCapturer::CurrentDeviceName() const {
	if (capture_module_)
	{
		return ViECapturer::CurrentDeviceName();
	}
	return strCurrentDeviceName_.c_str();
}

int32_t ViEFileCapturer::SetCaptureDelay(int32_t delay_ms) {
	if (capture_module_)
	{
		return ViECapturer::SetCaptureDelay(delay_ms);
	}
  return 0;
}
    
WebRtc_Word32 ViEFileCapturer::SetLocalVieoWindow(void* window) {
	if (capture_module_)
	{
		return ViECapturer::SetLocalVieoWindow(window);
	}
	return 0;
}

int32_t ViEFileCapturer::SetRotateCapturedFrames(const RotateCapturedFrame rotation) {
	if (capture_module_)
	{
		return ViECapturer::SetRotateCapturedFrames(rotation);
	}
	return 0;
}

bool ViEFileCapturer::ViECaptureProcess() {
	if (captureStarted_)
	{
		if (bInitialized_)
		{
			CriticalSectionScoped cs(fileCapture_cs_.get());
			if (video_frame_ != NULL && TimeToSendFileFrame())
			{
				video_frame_->set_timestamp(clock_->TimeInMilliseconds());
				video_frame_->set_render_time_ms(clock_->TimeInMilliseconds());

				LOG_F(LS_ERROR) << "ViEFileCapturer::ViECaptureProcess() interval times.";

				lastSent_ = clock_->TimeInMicroseconds();
				DeliverI420Frame(video_frame_);
			}
		}
		return true;
	}
	return false;
}

WebRtc_Word32 ViEFileCapturer::SetCaptureDeviceImage(const I420VideoFrame& capture_device_image) {
	if (capture_module_)
	{
		return ViECapturer::SetCaptureDeviceImage(capture_device_image);
	}
	return 0;
}

int ViEFileCapturer::SetCaptureSettings(VideoCaptureCapability settings)
{
	if (capture_module_)
	{
		return ViECapturer::SetCaptureSettings(settings);
	}
	return 0;
}

int ViEFileCapturer::UpdateLossRate(int lossRate)
{
	if (capture_module_)
	{
		return ViECapturer::UpdateLossRate(lossRate);
	}
    return 0;
}

char* ViEFileCapturer::LoadFilToMemory(const char *pPathname)
{
	char* pBMPBuffer = nullptr;
	FILE* fp = fopen(pPathname, "rb");
	if (fp)
	{
		fseek(fp, 0, SEEK_END);
		long nFileLen = ftell(fp);
		fseek(fp, 0, SEEK_SET);

		pBMPBuffer = new char[nFileLen + 1];
		pBMPBuffer[nFileLen] = 0;
		fread(pBMPBuffer, 1, nFileLen, fp);
		fclose(fp);
	}

	return pBMPBuffer;
}

#if defined(WIN32)
HBITMAP BufferToHBITMAPFromData(char* pBuffer, int* pSrcWidth, int* pSrcHeight)
{
	HBITMAP hShowBMP = nullptr;
	if (pBuffer != nullptr)
	{
		char*            pDIB;
		char* lpBuffer = pBuffer;
		LPVOID           lpDIBBits;
		BITMAPFILEHEADER bmfHeader;
		DWORD            bmfHeaderLen;

		bmfHeaderLen = sizeof(bmfHeader);
		memcpy((char*)&bmfHeader, (char*)lpBuffer, bmfHeaderLen);
		if (bmfHeader.bfType != (*(WORD*)"BM")) return NULL;
		pDIB = lpBuffer + bmfHeaderLen;
		BITMAPINFOHEADER &bmiHeader = *(LPBITMAPINFOHEADER)pDIB;
		BITMAPINFO &bmInfo = *(LPBITMAPINFO)pDIB;
		if (pSrcWidth)
		{
			*pSrcWidth = bmiHeader.biWidth;
		}
		if (pSrcHeight)
		{
			*pSrcHeight = bmiHeader.biHeight;
		}
		lpDIBBits = (lpBuffer)+((BITMAPFILEHEADER *)lpBuffer)->bfOffBits;
		HDC hDC = GetDC(nullptr);
		hShowBMP = CreateDIBitmap(hDC, &bmiHeader, CBM_INIT, lpDIBBits,
			&bmInfo, DIB_RGB_COLORS);
	}
	return hShowBMP;
}

char* ViEFileCapturer::GetBmpFileInfo(const char* pFileName, int & nWidth, int & nHeight, int* pImageDataSize)
{
	char* pRetBuffer = nullptr;
	if (pFileName)
	{
		char* pBuffer = LoadFilToMemory(pFileName);
		HBITMAP hBitmap = BufferToHBITMAPFromData(pBuffer, &nWidth, &nHeight);
		if (hBitmap)
		{
			int imageSize = 0;
			HDC  hDC = CreateCompatibleDC(NULL);
			HBITMAP  hOldBitmap = (HBITMAP)SelectObject(hDC, hBitmap);

			BITMAP bmp;
			GetObject(hBitmap, sizeof(bmp), &bmp);

			BITMAPINFOHEADER bih = { 0 };
			bih.biBitCount = bmp.bmBitsPixel;
			bih.biCompression = BI_RGB;
			bih.biHeight = bmp.bmHeight;
			bih.biPlanes = 1;
			bih.biSize = sizeof(BITMAPINFOHEADER);
			bih.biSizeImage = bmp.bmWidthBytes * bmp.bmHeight;
			bih.biWidth = bmp.bmWidth;

			nWidth = bmp.bmWidth;
			nHeight = bmp.bmHeight;
			imageSize = (((nWidth * 32 + 31) & ~31) / 8) * nHeight;
			pRetBuffer = new  char[imageSize];
			GetDIBits(hDC, hBitmap, 0, bmp.bmHeight, pRetBuffer,(LPBITMAPINFO)&bih, DIB_RGB_COLORS);
			if (pImageDataSize)
			{
				*pImageDataSize = imageSize;
			}

			SelectObject(hDC, hOldBitmap);
			DeleteObject(hBitmap);
			DeleteDC(hDC);

			delete[] pBuffer;
		}
	}

	return  pRetBuffer;
}

int ViEFileCapturer::ConvertBMPToVideoFrame(const char* fileUTF8, I420VideoFrame** video_frame)
{
	int ret = -1;
	if (fileUTF8 && video_frame)
	{
		if (*video_frame != nullptr)
		{
			delete (*video_frame);
		}
		*video_frame = new I420VideoFrame;

		int width = 0, height = 0;
		char* pBuffer = GetBmpFileInfo(fileUTF8, width, height);
		if (pBuffer)
		{
			int half_width = (width + 1) / 2;
			(*video_frame)->CreateEmptyFrame(width, height, width, half_width, half_width);
			ret = ConvertToI420(kARGB, (const uint8_t*)pBuffer, 0, 0, width, -height, 0, kRotateNone, (*video_frame));

			requested_capability_.width = width;
			requested_capability_.height = height;
			requested_capability_.maxFPS = 30;
			(*video_frame)->set_timestamp(clock_->TimeInMilliseconds());
			(*video_frame)->set_render_time_ms(clock_->TimeInMilliseconds());

			delete[] pBuffer;
		}
	}
	return ret;
}
#endif

int ViEFileCapturer::ConvertJPEGToVideoFrame(const char* fileUTF8, I420VideoFrame** video_frame) {
	
	if (!fileUTF8 || !video_frame)
	{
		return -1;
	}
	if (*video_frame != nullptr)
	{
		delete (*video_frame);
	}
	*video_frame = new I420VideoFrame;
	EncodedImage image_buffer;

	FILE* image_file = fopen(fileUTF8, "rb");
	if (!image_file) {
		return -1;
	}
	if (fseek(image_file, 0, SEEK_END) != 0) {
		fclose(image_file);
		return -1;
	}
	int buffer_size = ftell(image_file);
	if (buffer_size == -1) {
		fclose(image_file);
		return -1;
	}
	image_buffer._size = buffer_size;
	if (fseek(image_file, 0, SEEK_SET) != 0) {
		fclose(image_file);
		return -1;
	}
	image_buffer._buffer = new uint8_t[image_buffer._size + 1];
	if (image_buffer._size != fread(image_buffer._buffer, sizeof(uint8_t),
		image_buffer._size, image_file)) {
		fclose(image_file);
		delete[] image_buffer._buffer;
		return -1;
	}
	fclose(image_file);

	int ret = -1;
	MJpegDecoder mjpeg_decoder;
	if (mjpeg_decoder.LoadFrame(image_buffer._buffer, buffer_size))
	{
		int width = mjpeg_decoder.GetWidth();
		int height = mjpeg_decoder.GetHeight();

		int half_width = (width + 1) / 2;
		(*video_frame)->CreateEmptyFrame(width, height, width, half_width, half_width);

		ret = ConvertToI420(kMJPG, image_buffer._buffer, 0, 0, width, height, buffer_size, kRotateNone, (*video_frame));

		requested_capability_.width = width;
		requested_capability_.height = height;
		requested_capability_.maxFPS = sendFileFrameMaxFps_;
		(*video_frame)->set_timestamp(clock_->TimeInMilliseconds());
		(*video_frame)->set_render_time_ms(clock_->TimeInMilliseconds());
	}

	delete[] image_buffer._buffer;
	image_buffer._buffer = NULL;
	return ret;
}

FileExtType ViEFileCapturer::GetFileExtType(const string& fname)
{
	char c = fname.at(fname.length() - 1);
	char c2 = fname.at(fname.length() - 3);

	//jpg
	if ((c == 'g') && (c2 == 'j'))
	{
		return ExtType_Jpg;
	}
	//bmp
	else if ((c == 'p') && (c2 == 'b'))
	{
		return ExtType_Bmp;
	}
	//gif 
	else if ((c == 'f') && (c2 == 'g'))
	{
		return ExtType_Gif;
	}
	//png
	else if ((c == 'g') && (c2 == 'p'))
	{
		return ExtType_Png;
	}
	return ExtType_Unknown;
}

bool ViEFileCapturer::TimeToSendFileFrame() const
{
	bool timeToSend(false);

	WebRtc_Word64 diff = clock_->TimeInMicroseconds() - lastSent_;
	if (diff > avgTimePerFrame_)
	{
		timeToSend = true;
	}
	return timeToSend;
}

}  // namespace webrtc
