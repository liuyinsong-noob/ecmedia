#include "video_render_gdi.h"
#include "../common_video/source/libyuv/include/webrtc_libyuv.h"
#include "../system_wrappers/include/trace.h"

namespace yuntongxunwebrtc {


	GDIChannel::GDIChannel(HWND hWnd, Trace* trace):
	_hWnd(hWnd),
	_nImgSize(0),
	_pFrameBuffer(nullptr)
	{
		if (_hWnd)
		{
			_hDC = GetDC((HWND)_hWnd);
		}
		else
		{
			_hDC = nullptr;
		}
	}

	GDIChannel::~GDIChannel(){
		if (_hWnd != nullptr && _hDC != nullptr)
		{
			ReleaseDC(_hWnd, _hDC);
		}
		if (_pFrameBuffer)
		{
			delete[] _pFrameBuffer;
		}
	}

	void GDIChannel::SetStreamSettings(uint16_t streamId,
										uint32_t zOrder,
										float startWidth,
										float startHeight,
										float stopWidth,
										float stopHeight){
		_streamId = streamId;
		_zOrder = zOrder;
		_startWidth = startWidth;
		_startHeight = startHeight;
		_stopWidth = stopWidth;
		_stopHeight = stopHeight;
	}

	int GDIChannel::GetStreamSettings(uint16_t streamId,
										uint32_t& zOrder,
										float& startWidth,
										float& startHeight,
										float& stopWidth,
										float& stopHeight){
		streamId = _streamId;
		zOrder = _zOrder;
		startWidth = _startWidth;
		startHeight = _startHeight;
		stopWidth = _stopWidth;
		stopHeight = _stopHeight;
		return 0;
	}

	int32_t GDIChannel::RenderFrame(const uint32_t streamId,
									I420VideoFrame& videoFrame)
	{
		if (_hDC != nullptr)
		{
			int nSrcWidth = videoFrame.width();
			int nSrcHeight = videoFrame.height();
			int nImgSize = (((nSrcWidth * 32 + 31) & ~31) / 8) * nSrcHeight;
			if (nImgSize > 0)
			{
				if (nImgSize != _nImgSize)
				{
					_nImgSize = nImgSize;
					if (_pFrameBuffer != nullptr)
					{
						delete[] _pFrameBuffer; _pFrameBuffer = nullptr;
					}
					_pFrameBuffer = new char[_nImgSize + 1];
					memset(_pFrameBuffer, 0, nImgSize + 1);
				}
				if (_pFrameBuffer)
				{
					ConvertFromI420(videoFrame, kARGB, 4 * nSrcWidth, (LPBYTE)_pFrameBuffer);
					DrawFrame(_pFrameBuffer,nSrcWidth,nSrcHeight);

					return 0;
				}
			}
		}

		return -1;
	}

	void GDIChannel::DrawFrame(char* pBuffer, int nSrcWidth, int nSrcHeight, int nBitCount)
	{
		if (pBuffer != nullptr && _hWnd != nullptr && _hDC != nullptr)
		{
			HBITMAP hBitmap = CreateBitmap(nSrcWidth, nSrcHeight, 1, nBitCount, pBuffer);
			if (hBitmap)
			{
				HDC hMemDC = CreateCompatibleDC(_hDC);
				HBITMAP hOldBitmap = (HBITMAP)SelectObject(hMemDC, hBitmap);

				RECT rectWnd;
				::GetWindowRect(_hWnd, &rectWnd);

				int dstWidth = rectWnd.right - rectWnd.left;
				int dstHeight = rectWnd.bottom - rectWnd.top;

				SetStretchBltMode(_hDC, COLORONCOLOR);
				::StretchBlt(_hDC, 0, 0, dstWidth, dstHeight, hMemDC, 0, 0, nSrcWidth, nSrcHeight, SRCCOPY);

				SelectObject(hMemDC, hOldBitmap);
				DeleteDC(hMemDC);
				DeleteObject(hBitmap);
			}
		}
	}

	VideoRenderGDI::VideoRenderGDI(Trace* trace, HWND hWnd, bool fullScreen):
		_trace(trace),
		_hWnd(hWnd),
		_fullScreen(fullScreen),
		_refD3DCritsect(*CriticalSectionWrapper::CreateCriticalSection()){

	}

	VideoRenderGDI::~VideoRenderGDI(){
	}


	int32_t VideoRenderGDI::Init() {
		if (!_hWnd) {
			return -1;
		}
    	return 0;
	}

	int32_t VideoRenderGDI::ChangeWindow(void* window)
	{
		WEBRTC_TRACE(kTraceError, kTraceVideo, -1, "Not supported.");
		return -1;
	}

	VideoRenderCallback* VideoRenderGDI::CreateChannel(const uint32_t channel,
														const uint32_t zOrder,
														const float left,
														const float top,
														const float right,
														const float bottom)
	{
		CriticalSectionScoped cs(&_refD3DCritsect);

		//FIXME this should be done in VideoAPIWindows? stop the frame deliver first
		//remove the old channel	
		DeleteChannel(channel);

		GDIChannel* gdiChannel = new GDIChannel(_hWnd, nullptr);
		gdiChannel->SetStreamSettings(0, zOrder, left, top, right, bottom);

		// store channel
		_gdiChannels[channel & 0x0000ffff] = gdiChannel;

		// store Z order
		// default streamID is 0
		_gdiZorder.insert(
			std::pair<int, unsigned int>(zOrder, channel & 0x0000ffff));

		return gdiChannel;
	}

	int32_t VideoRenderGDI::DeleteChannel(const uint32_t streamId)
	{
		CriticalSectionScoped cs(&_refD3DCritsect);

		std::multimap<int, unsigned int>::iterator it;
		it = _gdiZorder.begin();
		while (it != _gdiZorder.end())
		{
			if ((streamId & 0x0000ffff) == (it->second & 0x0000ffff))
			{
				it = _gdiZorder.erase(it);
				break;
			}
			it++;
		}

		std::map<int, GDIChannel*>::iterator ddIt;
		ddIt = _gdiChannels.find(streamId & 0x0000ffff);
		if (ddIt != _gdiChannels.end())
		{
			delete ddIt->second;
			_gdiChannels.erase(ddIt);
			return 0;
		}
		return -1;
	}

	int32_t VideoRenderGDI::GetStreamSettings(const uint32_t channel,
		const uint16_t streamId,
		uint32_t& zOrder,
		float& left, float& top,
		float& right, float& bottom)
	{
		std::map<int, GDIChannel*>::iterator ddIt;
		ddIt = _gdiChannels.find(channel & 0x0000ffff);
		GDIChannel* ddobj = NULL;
		if (ddIt != _gdiChannels.end())
		{
			ddobj = ddIt->second;
		}
		if (ddobj == NULL)
		{
			WEBRTC_TRACE(kTraceError, kTraceVideo, -1,
				"Direct3D render failed to find channel");
			return -1;
		}
		// Only allow one stream per channel, demuxing is 
		return ddobj->GetStreamSettings(0, zOrder, left, top, right, bottom);
		//return ddobj->GetStreamSettings(streamId, zOrder, left, top, right, bottom);    
	}


	int32_t VideoRenderGDI::StartRender()
	{
		WEBRTC_TRACE(kTraceError, kTraceVideo, -1, "Not supported.");
		return 0;
	}

	int32_t VideoRenderGDI::StopRender()
	{
		WEBRTC_TRACE(kTraceError, kTraceVideo, -1, "Not supported.");
		return 0;
	}

	bool VideoRenderGDI::IsFullScreen()
	{
		return _fullScreen;
	}

	int32_t VideoRenderGDI::GetGraphicsMemory(uint64_t& totalMemory,
											uint64_t& availableMemory)
	{
		return 0;
	}

	int32_t VideoRenderGDI::SetCropping(const uint32_t channel,
										const uint16_t streamId,
										const float left, const float top,
										const float right, const float bottom)
	{
		WEBRTC_TRACE(kTraceError, kTraceVideo, -1, "Not supported.");
		return 0;
	}

	int32_t VideoRenderGDI::ConfigureRenderer(const uint32_t channel,
											const uint16_t streamId,
											const unsigned int zOrder,
											const float left,
											const float top,
											const float right,
											const float bottom)
	{
		std::map<int, GDIChannel*>::iterator ddIt;
		ddIt = _gdiChannels.find(channel & 0x0000ffff);
		GDIChannel* ddobj = NULL;
		if (ddIt != _gdiChannels.end())
		{
			ddobj = ddIt->second;
		}
		if (ddobj == NULL)
		{
			WEBRTC_TRACE(kTraceError, kTraceVideo, -1,
				"Direct3D render failed to find channel");
			return -1;
		}
		// Only allow one stream per channel, demuxing is 
		ddobj->SetStreamSettings(0, zOrder, left, top, right, bottom);

		return 0;
	}

	int32_t VideoRenderGDI::SetTransparentBackground(const bool enable)
	{
		WEBRTC_TRACE(kTraceError, kTraceVideo, -1, "Not supported.");
		return 0;
	}
	
	int32_t VideoRenderGDI::SetText(const uint8_t textId,
									const uint8_t* text,
									const int32_t textLength,
									const uint32_t colorText,
									const uint32_t colorBg,
									const float left, const float top,
									const float rigth, const float bottom)
	{
		WEBRTC_TRACE(kTraceError, kTraceVideo, -1, "Not supported.");
		return 0;
	}

	int32_t VideoRenderGDI::SetBitmap(const void* bitMap,
											const uint8_t pictureId,
											const void* colorKey,
											const float left, const float top,
											const float right, const float bottom)
	{
		WEBRTC_TRACE(kTraceError, kTraceVideo, -1, "Not supported.");
		return 0;
	}
}