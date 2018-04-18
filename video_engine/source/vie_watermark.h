/*
Auth chwd 2018.4.10
*/

#ifndef WEBRTC_VIDEO_ENGINE_VIE_WATERMARK_H_
#define WEBRTC_VIDEO_ENGINE_VIE_WATERMARK_H_
#include <string>
#include "../common_types.h"
extern "C"
{
#include "libavfilter/avfiltergraph.h"
#include "libavfilter/buffersink.h"
#include "libavfilter/buffersrc.h"
#include "libavutil/avutil.h"
#include "libavutil/imgutils.h"
};


namespace cloopenwebrtc {

	/*struct WaterMark
	{
		char fontfile[100];
		char fontcolor[6];
		int  fontsize;
		char text[1024];
		int  x;
		int  y;
		char imagepath[1024];
		char startposition[10];
		int  flag;//0 image 1 text
	};*/

	class VIEWaterMark
	{
	public:
		static VIEWaterMark* CreateWatermark(WaterMark watermark, int width, int height);
		~VIEWaterMark();

	protected:
		VIEWaterMark();

	protected:
		bool InitFilter(int width, int height);
		bool CopyFilterData(AVFrame *frame_out, unsigned char* yuvDest);
		bool InitFilterDescr(WaterMark water);

	protected:
		AVFrame *frame_in_;
		AVFrame *frame_out_;

		AVFilterContext *buffersink_ctx_;
		AVFilterContext *buffersrc_ctx_;

		AVFilterGraph *filter_graph_;

		AVPixelFormat pix_fmts_[3];

		int video_stream_index_;

		unsigned char *frame_buffer_in_;
		unsigned char *frame_buffer_out_;

	public:
		bool FilterBufferSinkAndCopyYUV(unsigned char *yuvdata, int width, int height);

	protected:
		char *filter_descr_;
	};

}


#endif  // WEBRTC_VIDEO_ENGINE_VIE_WATERMARK_H_