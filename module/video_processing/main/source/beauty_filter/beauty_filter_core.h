#ifndef __BEAUTYFILTERCORE_H__
#define __BEAUTYFILTERCORE_H__


#ifdef __cplusplus
extern "C" {
#endif

typedef struct Beauty_FilterC
{
	unsigned char radius;  // must be [1 100]
	unsigned char thres;   // must be [1 40]
	unsigned char* data;
	unsigned int* table;
	int bSharpen;
	int bInitFlag;

	unsigned int w;
	unsigned int h;
}Beauty_Filter;



int create_beauty_filter(Beauty_Filter** self);
int init_beauty_filter(Beauty_Filter* self, unsigned char radius, unsigned char thres, int bsharpen);
int set_sharpen(Beauty_Filter* self, int bsharpen);
int set_radius_thres(Beauty_Filter* self, unsigned char radius, unsigned char thres);
int beauty_filter(Beauty_Filter* self, unsigned int w, unsigned int h, unsigned int scan, unsigned char* psrcdst, int maskscan, unsigned char* mask);
int beauty_filter2(Beauty_Filter* self, unsigned int w, unsigned int h, unsigned int scan, unsigned char* psrc, unsigned int scan_dst, unsigned char* pdst);
int free_beauty_filter(Beauty_Filter* self);



	void ZFilter_8u_C1R(int width, int height, int scan_src, unsigned char* pbuf_src, int scan_dst, unsigned char* pbuf_dst);

#ifdef __cplusplus
}
#endif // __cplusplus


#endif // !__VIDEOFILTER_H__
