#ifndef __VIDEO_BEAUTY_FILTER_H__
#define __VIDEO_BEAUTY_FILTER_H__



#ifdef __cplusplus
extern "C" {
#endif
	
	int Create_Beauty_Filter(void** inst);
	int Init_Beauty_Filter(void* self, unsigned char radius, unsigned char thres);
	int Set_Radius_Thres(void* self, unsigned char radius, unsigned char thres);
	int Proc_Beauty_Filter(void* self, unsigned int w, unsigned int h, unsigned int scan, unsigned char* psrcdst, int maskscan, unsigned char* mask);
	int Free_Beauty_Filter(void* self);

#ifdef __cplusplus
}
#endif // __cplusplus


#endif // !__VIDEOFILTER_H__
