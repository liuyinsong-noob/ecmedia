/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_VIDEO_ENGINE_VIE_FILE_CAPTURER_H_
#define WEBRTC_VIDEO_ENGINE_VIE_FILE_CAPTURER_H_

#include "vie_capturer.h"
#include <string>

using namespace std;

namespace yuntongxunwebrtc {


	typedef enum
	{
		ExtType_Unknown = 0,
		ExtType_Jpg = 1,
		ExtType_Bmp = 2,
		ExtType_Png = 3,
		ExtType_Gif = 4
	}FileExtType;

	class ViEFileCapturer : public ViECapturer
	{
	public:

		static ViEFileCapturer* CreateViEFileCapturer(
			int capture_id,
			int engine_id,
			const Config& config,
			const char* fileUTF8,
			const char* filesSplit,
			ProcessThread& module_process_thread);

		~ViEFileCapturer();

		virtual int FrameCallbackChanged();
		virtual int32_t SetCaptureDelay(int32_t delay_ms);
		virtual WebRtc_Word32 SetLocalVieoWindow(void* window);
		virtual int32_t SetRotateCapturedFrames(const RotateCapturedFrame rotation);
		virtual WebRtc_Word32 SetCaptureDeviceImage(const I420VideoFrame& capture_device_image);
		virtual int SetCaptureSettings(VideoCaptureCapability settings);
		virtual int UpdateLossRate(int lossRate);
		virtual bool ViECaptureProcess();

		// Start/Stop.
		virtual int32_t Start(const CaptureCapability& capture_capability = CaptureCapability());
		virtual int32_t Stop();
		virtual bool Started();

		// Information.
		virtual const char* CurrentDeviceName() const;

	protected:
		ViEFileCapturer(int capture_id,
			int engine_id,
			const Config& config,
			ProcessThread& module_process_thread);

		int32_t Init(const char* fileUTF8, const char* filesSplit = nullptr);

		char* LoadFilToMemory(const char *pPathname);
		int ConvertJPEGToVideoFrame(const char* fileUTF8, I420VideoFrame** video_frame);
		FileExtType GetFileExtType(const string& fname);
#if defined(WIN32)
		int ConvertBMPToVideoFrame(const char* fileUTF8, I420VideoFrame** video_frame);
		char* GetBmpFileInfo(const char* pFileName, int & nWidth, int & nHeight, int* nImageDataSize = nullptr);
#endif
		bool TimeToSendFileFrame() const;

	protected:
		Clock* clock_;
		I420VideoFrame* video_frame_;
		string strCurrentDeviceName_;
		bool captureStarted_;
		bool bInitialized_;
		ThreadWrapper* fileCapture_thread_;
		scoped_ptr<CriticalSectionWrapper> fileCapture_cs_;

		const int sendFileFrameMaxFps_;
		const int64_t avgTimePerFrame_;
		int64_t lastSent_;
	};

}  // namespace webrtc

#endif  // WEBRTC_VIDEO_ENGINE_VIE_FILE_CAPTURER_H_
