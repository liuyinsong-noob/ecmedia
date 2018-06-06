/*
Auth chwd 2018.4.10
*/

#include "vie_watermark.h"
#include "../system_wrappers/include/logging.h"
using namespace cloopenwebrtc;

//create and init filter obj
VIEWaterMark* cloopenwebrtc::VIEWaterMark::CreateWatermark(WaterMark watermark, int width, int height)
{
	VIEWaterMark* water_mark = new VIEWaterMark();
	if (!water_mark || !water_mark->InitFilterDescr(watermark) || !water_mark->InitFilter(width, height)) {
		delete water_mark;
		water_mark = NULL;
	}
	return water_mark;
}

cloopenwebrtc::VIEWaterMark::~VIEWaterMark()
{
	if (filter_descr_) {
		delete filter_descr_;
	}
	if (frame_buffer_in_) {
		delete frame_buffer_in_;
	}


	if (frame_buffer_out_) {
		delete frame_buffer_out_;
	}
	if (frame_in_) {
		av_frame_free(&frame_in_);
	}
	if (frame_out_) {
		av_frame_free(&frame_out_);
	}
	if (filter_graph_) {
		avfilter_graph_free(&filter_graph_);
	}
	
}

cloopenwebrtc::VIEWaterMark::VIEWaterMark()
{
	video_stream_index_ = -1;
	frame_buffer_in_ = NULL;
	frame_buffer_out_ = NULL;
	pix_fmts_[0] = AV_PIX_FMT_YUV420P;
	pix_fmts_[1] = AV_PIX_FMT_ARGB;
	pix_fmts_[2] = AV_PIX_FMT_NONE;
	frame_in_ = NULL;
	frame_out_ = NULL;
	filter_graph_ = NULL;
	filter_descr_ = new char[1024];
}


bool cloopenwebrtc::VIEWaterMark::InitFilter(int width, int height)
{
	avfilter_register_all();
	
	char args[512];
	AVFilter *buffersrc = avfilter_get_by_name("buffer");
	AVFilter *buffersink = avfilter_get_by_name("buffersink");
	if (!buffersink)
		return false;

	AVFilterInOut *outputs = avfilter_inout_alloc();
	AVFilterInOut *inputs = avfilter_inout_alloc();
	AVBufferSinkParams *buffersink_params;

	filter_graph_ = avfilter_graph_alloc();

	/* buffer video source: the decoded frames from the decoder will be inserted here. */

	snprintf(args, sizeof(args),
		"video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
		width, height, AV_PIX_FMT_YUV420P,
		1, 25, 1, 1);

	int ret = avfilter_graph_create_filter(&buffersrc_ctx_, buffersrc, "in",
		args, NULL, filter_graph_);
	if (ret < 0)
		return false;

	/* buffer video sink: to terminate the filter chain. */

	buffersink_params = av_buffersink_params_alloc();
	buffersink_params->pixel_fmts = pix_fmts_;
	ret = avfilter_graph_create_filter(&buffersink_ctx_, buffersink, "out", NULL, buffersink_params, filter_graph_);
	av_free(buffersink_params);
	if (ret < 0)
		return false;

	/* Endpoints for the filter graph. */

	outputs->name = av_strdup("in");
	outputs->filter_ctx = buffersrc_ctx_;
	outputs->pad_idx = 0;
	outputs->next = NULL;

	inputs->name = av_strdup("out");
	inputs->filter_ctx = buffersink_ctx_;
	inputs->pad_idx = 0;
	inputs->next = NULL;
	if ((ret = avfilter_graph_parse_ptr(filter_graph_,(const char*)filter_descr_,
		&inputs, &outputs, NULL)) < 0) {
		char buf[100];
		av_strerror(ret, buf, 100);
		LOG(LS_ERROR) << "avfilter_graph_parse_ptr err:" << buf << " descr:"<< filter_descr_ <<"ret:"<< ret;
		return false;
	}

	if ((ret = avfilter_graph_config(filter_graph_, NULL)) < 0)
		return false;

	frame_in_ = av_frame_alloc();
	frame_buffer_in_ = (unsigned char *)av_malloc(av_image_get_buffer_size(AV_PIX_FMT_YUV420P, width, height, 1));
	av_image_fill_arrays(frame_in_->data, frame_in_->linesize, frame_buffer_in_, AV_PIX_FMT_YUV420P, width, height, 1);

	frame_out_ = av_frame_alloc();
	frame_buffer_out_ = (unsigned char *)av_malloc(av_image_get_buffer_size(AV_PIX_FMT_YUV420P, width, height, 1));
	av_image_fill_arrays(frame_out_->data, frame_out_->linesize, frame_buffer_out_, AV_PIX_FMT_YUV420P, width, height, 1);

	frame_in_->width = width;
	frame_in_->height = height;
	frame_in_->format = AV_PIX_FMT_YUV420P;


	return true;
}

bool cloopenwebrtc::VIEWaterMark::CopyFilterData(AVFrame *frame_out, unsigned char * yuvDest)
{
	int width = frame_out->width;
	int height = frame_out->height;
	for (int i = 0; i < frame_out->height; i++) {
		memcpy(yuvDest + width*i, frame_out->data[0] + frame_out->linesize[0] * i, width);
	}
	for (int i = 0; i < frame_out->height / 2; i++) {
		memcpy(yuvDest + width * height + i*width / 2, frame_out->data[1] + frame_out->linesize[1] * i, width / 2);
	}
	for (int i = 0; i < frame_out->height / 2; i++) {
		memcpy(yuvDest + width * height * 5 / 4 + i*width / 2, frame_out->data[2] + frame_out->linesize[2] * i, width / 2);
	}

	return true;
}

bool cloopenwebrtc::VIEWaterMark::InitFilterDescr(WaterMark watermark)
{	

	if (0 == watermark.flag) {

		if (strcmp("topleft", watermark.startposition)==0) {
			sprintf(filter_descr_, "movie=%s[wm];[in][wm]overlay=%d:%d[out]", watermark.imagepath, watermark.x, watermark.y);
		}
		if (strcmp("bottomleft", watermark.startposition) == 0) {
			sprintf(filter_descr_, "movie=%s[wm];[in][wm]overlay=%d:main_h-overlay_h-%d[out]", watermark.imagepath, watermark.x, watermark.y);
		}
		if (strcmp("bottomright", watermark.startposition)==0) {
			sprintf(filter_descr_, "movie=%s[wm];[in][wm]overlay=main_w-overlay_w-%d:%d[out]", watermark.imagepath, watermark.x, watermark.y);
		}
		if (strcmp("topright", watermark.startposition)==0) {
			sprintf(filter_descr_, "movie=%s[wm];[in][wm]overlay=main_w-overlay_w-%d:main_h-overlay_h-%d[out]", watermark.imagepath, watermark.x, watermark.y);
		}
		return true;
	}
	else if(1 == watermark.flag){

		sprintf(filter_descr_, "drawtext=fontfile=%s:fontcolor_expr=%s:fontsize=%d:text='%s':x=%d:y=%d",
			watermark.fontfile, watermark.fontcolor, watermark.fontsize, watermark.text, watermark.x,watermark.y);

		return true;
	}
}

bool cloopenwebrtc::VIEWaterMark::FilterBufferSinkAndCopyYUV(unsigned char *yuvdata, int width, int height)
{
	//copy yuv data
	memcpy(frame_buffer_in_, yuvdata, width*height * 3 / 2);

	//input Y,U,V
	frame_in_->data[0] = frame_buffer_in_;
	frame_in_->data[1] = frame_buffer_in_ + width*height;
	frame_in_->data[2] = frame_buffer_in_ + width*height * 5 / 4;

	if (av_buffersrc_add_frame(buffersrc_ctx_, frame_in_) < 0) {
		return false;
	}

	/* pull filtered pictures from the filtergraph */
	if (av_buffersink_get_frame(buffersink_ctx_, frame_out_) < 0)
		return false;

	if ((frame_out_->linesize[0] == frame_out_->width) && (frame_out_->linesize[1] == width/2) && (frame_out_->linesize[2] == width/2)) {
		memcpy(yuvdata, frame_out_->data[0], width*height);
        memcpy(yuvdata + width * height, frame_out_->data[1], width*height/ 4);
        memcpy(yuvdata + width * height * 5 / 4, frame_out_->data[2], width*height / 4);
	}
	else{
		CopyFilterData(frame_out_, yuvdata);
	}

	av_frame_unref(frame_out_);

	return true;
}
