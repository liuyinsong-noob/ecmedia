#ifndef WEBRTC_MODULES_VIDEO_RENDER_MAIN_SOURCE_WINDOWS_VIDEO_RENDER_GDI_H_
#define WEBRTC_MODULES_VIDEO_RENDER_MAIN_SOURCE_WINDOWS_VIDEO_RENDER_GDI_H_

#include "video_render_defines.h"
#include "i_video_render_win.h"

#include <windows.h>
#include <Map>

namespace yuntongxunwebrtc {
	class Trace;

	class GDIChannel : public VideoRenderCallback
	{
	public:
		GDIChannel(HWND hwnd, Trace* trace);

		virtual ~GDIChannel();

		virtual int32_t RenderFrame(const uint32_t streamId,
									I420VideoFrame& videoFrame);

		void SetStreamSettings(uint16_t streamId,
								uint32_t zOrder,
								float startWidth,
								float startHeight,
								float stopWidth,
								float stopHeight);
		int GetStreamSettings(uint16_t streamId,
								uint32_t& zOrder,
								float& startWidth,
								float& startHeight,
								float& stopWidth,
								float& stopHeight);
	private:
		void DrawFrame(char* pBuffer, int nSrcWidth, int nSrcHeight, int nBitCount = 32);

	private:
		uint16_t _streamId;
		uint32_t _zOrder;
		float _startWidth;
		float _startHeight;
		float _stopWidth;
		float _stopHeight;

		HWND _hWnd;
		HDC _hDC;
		uint32_t _nImgSize;
		char* _pFrameBuffer;
	};


	class VideoRenderGDI : IVideoRenderWin
	{
	public:
		VideoRenderGDI(Trace* trace, HWND hWnd, bool fullScreen);
		~VideoRenderGDI();
	public:
		//IVideoRenderWin

		/**************************************************************************
		*
		*   Init
		*
		***************************************************************************/
		virtual int32_t Init();

		/**************************************************************************
		*
		*   Incoming Streams
		*
		***************************************************************************/
		virtual VideoRenderCallback* CreateChannel(const uint32_t streamId,
			const uint32_t zOrder,
			const float left,
			const float top,
			const float right,
			const float bottom);

		virtual int32_t DeleteChannel(const uint32_t streamId);

		virtual int32_t GetStreamSettings(const uint32_t channel,
			const uint16_t streamId,
			uint32_t& zOrder,
			float& left, float& top,
			float& right, float& bottom);

		/**************************************************************************
		*
		*   Start/Stop
		*
		***************************************************************************/

		virtual int32_t StartRender();
		virtual int32_t StopRender();

		/**************************************************************************
		*
		*   Properties
		*
		***************************************************************************/

		virtual bool IsFullScreen();

		virtual int32_t SetCropping(const uint32_t channel,
			const uint16_t streamId,
			const float left, const float top,
			const float right, const float bottom);

		virtual int32_t ConfigureRenderer(const uint32_t channel,
			const uint16_t streamId,
			const unsigned int zOrder,
			const float left, const float top,
			const float right, const float bottom);

		virtual int32_t SetTransparentBackground(const bool enable);

		virtual int32_t ChangeWindow(void* window);

		virtual int32_t GetGraphicsMemory(uint64_t& totalMemory,
			uint64_t& availableMemory);

		virtual int32_t SetText(const uint8_t textId,
			const uint8_t* text,
			const int32_t textLength,
			const uint32_t colorText,
			const uint32_t colorBg,
			const float left, const float top,
			const float rigth, const float bottom);

		virtual int32_t SetBitmap(const void* bitMap,
			const uint8_t pictureId,
			const void* colorKey,
			const float left, const float top,
			const float right, const float bottom);

	private:
		Trace* _trace;
		HWND _hWnd;
		bool _fullScreen;

		CriticalSectionWrapper& _refD3DCritsect;
		std::map<int, GDIChannel*> _gdiChannels;
		std::multimap<int, unsigned int> _gdiZorder;

	};


} //namespace yuntongxunwebrtc
#endif