/*@file beauty_filter_core.h
  @author Guoqing Zeng
  @date 20170302
  @description
     -基于表面模糊算法实现
	 -采用建表加速
	 -采用肤色掩模加速
  @detail

*/
#ifndef __BEAUTYFILTERCORE_H__
#define __BEAUTYFILTERCORE_H__


typedef struct Beauty_FilterC
{
	unsigned char radius;  // must be [1 100]
	unsigned char thres;   // must be [1 40]
	unsigned char* data;
	unsigned char* mask;
	unsigned int* table;

	int bSharpen;
	int bWhitening;
	int bInitFlag;

	unsigned int w;
	unsigned int h;
	unsigned int maskw;
	unsigned int maskh;

	//
	unsigned char* morph_dilation;
	unsigned char* morph_erosion;
	unsigned int d_buf_len;
	unsigned int e_buf_len;

	//
	unsigned char curveTable[256];
	unsigned char beta;

}Beauty_Filter;


int create_beauty_filter(Beauty_Filter** self);
int init_beauty_filter(Beauty_Filter* self, unsigned char radius, unsigned char thres, int bsharpen);
int set_sharpen(Beauty_Filter* self, int bsharpen);
int set_radius_thres(Beauty_Filter* self, unsigned char radius, unsigned char thres);
int enable_whitening(Beauty_Filter* self, int enable);
int reset(Beauty_Filter* self);
int beauty_filter(Beauty_Filter* self, unsigned int w, unsigned int h, unsigned int scan, unsigned char* psrcdst, int maskscan, unsigned char* mask);
int beauty_filter2(Beauty_Filter* self, unsigned int w, unsigned int h, unsigned int scan, unsigned char* psrc, unsigned int scan_dst, unsigned char* pdst);
int free_beauty_filter(Beauty_Filter* self);

int zipl_fErosion(Beauty_Filter* self, int ncx, int ncy, int nSrcDstPitch, unsigned char* pSrcDst, int nRow, int nCol);
int zipl_fDilation(Beauty_Filter* self, int ncx, int ncy, int nSrcDstPitch, unsigned char* pSrcDst, int nRow, int nCol);


#endif // !__VIDEOFILTER_H__
