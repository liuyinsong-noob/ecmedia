#include "video_beauty_filter.h"
#include "beauty_filter_core.h"
#ifndef __APPLE__
#include <malloc.h>
#else
#include <stdlib.h>
#endif
#include <math.h>
#include <string.h>


//
unsigned char* Get_Version_Beauty_Filter()
{
	return "Beauty_Filter_v_20170308_1.0";
}

//
int curve(Beauty_Filter*self, int w, int h, int scan, unsigned char* pbuf, int scanmask, unsigned char* mask)
{
	int i;
	int j;

	if (!self)
		return -1;
	
	
	for (i = 0; i < h; ++i)
	{
		unsigned char* ps = pbuf + i*scan;
		unsigned char* pm = mask + i*scanmask;
		for (int j = 0; j < w; ++j)
		{
			//if (pm[j])
			{
				ps[j] = self->curveTable[ps[j]];
			}
		}
	}

	return 0;
}

//
int static UpdateMask(int w, int h, int scan_curr, unsigned char* mask_curr, int scan_prev, unsigned char* mask_prev)
{
	for (int i = 0; i < h; ++i)
	{
		unsigned char* p0 = mask_curr + i*scan_curr;
		unsigned char* p1 = mask_prev + i*scan_prev;
		for (int j = 0; j < w; ++j)
		{
			//p1[j] = p1[j] * alpha + p0[j] * (1.f-alpha);  alpha = 0.8f
			p1[j] = (p1[j] * 410 + p0[j] * 102) >> 9;
		}
	}
	return 0;
}

//
int zipl_fErosion(Beauty_Filter* self, int ncx, int ncy, int nSrcDstPitch, unsigned char* pSrcDst, int nRow, int nCol)
{
	//边缘外围补255
	if (!self)
		return -1;

	if (nRow <= 0 || nRow >= ncy)
		return -1;

	if (nCol <= 0 || nCol >= ncx)
		return -1;

	//
	int nWindowY = nRow * 2 + 1;
	int nWindowX = nCol * 2 + 1;
	int xx = ncx + 2 * nCol;
	int yy = ncy + 2 * nRow;
	int nxless = nWindowX - 1;
	int nyless = nWindowY - 1;

	if (self->e_buf_len != xx * sizeof(unsigned char*) + (nWindowX + xx + (yy + nWindowY)*xx + xx) || !self->morph_erosion)
	{
		if (self->morph_erosion)
		{
			free(self->morph_erosion);
			self->morph_erosion = 0;
		}
		self->e_buf_len = xx * sizeof(unsigned char*) + (nWindowX + xx + (yy + nWindowY)*xx + xx);
		self->morph_erosion = (unsigned char*)malloc(self->e_buf_len);
		if (self->morph_erosion == 0)
			return -1;
	}

	unsigned char** ppAllScrollWinYs = (unsigned char**)self->morph_erosion;
	unsigned char*  pScrollWinX = self->morph_erosion + xx * sizeof(unsigned char*);
	unsigned char*  pScrollWinX_start = pScrollWinX + nWindowX;
	unsigned char*  pScrollWinY = pScrollWinX_start + xx;
	unsigned char*  pScrollWinY_start = pScrollWinY + nWindowY;
	int        nOffToNextY = nWindowY + yy;

	//清空滑动X窗口前端部分
	memset(pScrollWinX, 0, nWindowX);
	//清空滑动Y窗口前端部分
	for (int i = 0; i < xx; ++i) {
		memset(pScrollWinY + i*nOffToNextY, 0, nWindowY);
	}

	//初始化所有Y窗口
	//特殊y
	for (int i = 0; i < nCol; ++i) {
		unsigned char* pWy_l = pScrollWinY_start + i*nOffToNextY;
		unsigned char* pWy_r = pScrollWinY_start + (ncx + nCol + i)*nOffToNextY;
		memset(pWy_l, 1, yy);
		memset(pWy_r, 1, yy);
	}

	//正常y
	unsigned char* ps = pSrcDst + nRow*nSrcDstPitch;
	for (int j = 0; j < ncx; ++j) {
		unsigned char* pWy = pScrollWinY_start + (j + nCol)*nOffToNextY + nyless;
		pWy[0] = ps[j] > 0 ? 1 : 0;
	}

	for (int i = nWindowY - 2; i >= nRow; --i) {
		unsigned char* ps = pSrcDst + (i - nRow) * nSrcDstPitch;
		for (int j = 0; j < ncx; ++j) {
			unsigned char* pWy = pScrollWinY_start + (j + nCol) * nOffToNextY + i;
			pWy[0] = pWy[1] && ps[j];
		}
	}

	for (int i = nRow - 1; i >= 0; --i) {
		for (int j = 0; j < ncx; ++j) {
			unsigned char* pWy = pScrollWinY_start + (j + nCol)*nOffToNextY + i;
			pWy[0] = pWy[1];
		}
	}

	for (int i = 0; i < xx; ++i)
		ppAllScrollWinYs[i] = pScrollWinY_start + i*nOffToNextY;

	//初始化X窗口
	pScrollWinX_start[nxless] = *(ppAllScrollWinYs[nxless]);
	for (int j = nWindowX - 2; j >= 0; --j) {
		pScrollWinX_start[j] = pScrollWinX_start[j + 1] & (*ppAllScrollWinYs[j]);
	}

	//work...
	unsigned char*  pRealWinX = pScrollWinX_start;
	int        nOffSet = ncy - nRow - 1;
	unsigned char** ppRealAllWinYs = ppAllScrollWinYs + nCol;
	unsigned char** ppRealWinYs2 = ppRealAllWinYs + nCol + 1;
	int        nLessX = ncx - 1;
	int        nLessY = ncy - 1;
	int        nOffSetOfSrc = (nRow + 1)*nSrcDstPitch;
	for (int i = 0; i < ncy; ++i) {
		unsigned char* pd = pSrcDst + i*nSrcDstPitch;
		for (int j = 0; j < ncx; ++j) {
			pd[j] = pRealWinX[0] == 0 ? 0 : 255;
			//更新X窗口
			if (j < nLessX) {
				if (*ppRealWinYs2[j] == 0) {
					pRealWinX = pScrollWinX;
				}
				else {
					pRealWinX[nWindowX] = 1;
					pRealWinX += 1;
				}
			}
		}

		//更新Y窗口
		if (i < nOffSet) {
			unsigned char* ps = pd + nOffSetOfSrc;
			for (int j = 0; j < ncx; ++j) {
				if (ps[j] == 0) {
					ppRealAllWinYs[j] = pScrollWinY + (j + nCol)*nOffToNextY;
				}
				else {
					ppRealAllWinYs[j][nWindowY] = 1;
					ppRealAllWinYs[j] += 1;
				}
			}

			//重新更新X窗口
			pScrollWinX_start[nxless] = *(ppAllScrollWinYs[nxless]);
			for (int j = nWindowX - 2; j >= 0; --j) {
				pScrollWinX_start[j] = pScrollWinX_start[j + 1] & (*ppAllScrollWinYs[j]);
			}
			pRealWinX = pScrollWinX_start;

		}
		else if (i >= nOffSet && i < nLessY) {
			for (int j = 0; j < ncx; ++j) {
				ppRealAllWinYs[j][nWindowY] = 1;
				ppRealAllWinYs[j] += 1;
			}
			//更新X窗口
			pScrollWinX_start[nxless] = *(ppAllScrollWinYs[nxless]);
			for (int j = nWindowX - 2; j >= 0; --j) {
				pScrollWinX_start[j] = pScrollWinX_start[j + 1] & (*ppAllScrollWinYs[j]);
			}
			pRealWinX = pScrollWinX_start;
		}
	}

	return 0;
}

//
int zipl_fDilation(Beauty_Filter* self, int ncx, int ncy, int nSrcDstPitch, unsigned char* pSrcDst, int nRow, int nCol)
{
	//边缘外围补0
	if (!self)
		return -1;

	if (nRow <= 0 || nRow >= ncy)
		return -1;

	if (nCol <= 0 || nCol >= ncx)
		return -1;

	//
	int nWindowY = nRow * 2 + 1;
	int nWindowX = nCol * 2 + 1;
	int xx = ncx + 2 * nCol;
	int yy = ncy + 2 * nRow;
	int nxless = nWindowX - 1;
	int nyless = nWindowY - 1;

	if (self->d_buf_len != xx * sizeof(unsigned char*) + (nWindowX + xx + (yy + nWindowY)*xx + xx) || !self->morph_dilation)
	{
		if (self->morph_dilation)
		{
			free(self->morph_dilation);
			self->morph_dilation = 0;
		}
		self->d_buf_len = xx * sizeof(unsigned char*) + (nWindowX + xx + (yy + nWindowY)*xx + xx);


		self->morph_dilation = (unsigned char*)malloc(self->d_buf_len);
		if (self->morph_dilation == 0)
			return -1;
	}

	unsigned char** ppAllScrollWinYs = (unsigned char**)(self->morph_dilation);
	unsigned char* pScrollWinX = self->morph_dilation + xx * sizeof(unsigned char*);
	unsigned char* pScrollWinX_start = pScrollWinX + nWindowX;
	unsigned char* pScrollWinY = pScrollWinX_start + xx;
	unsigned char* pScrollWinY_start = pScrollWinY + nWindowY;
	int  nOffToNextY = nWindowY + yy;

	//清空滑动X窗口前端部分
	memset(pScrollWinX, 1, nWindowX);               //膨胀，补1
													//清空滑动Y窗口前端部分
	for (int i = 0; i < xx; ++i) {
		memset(pScrollWinY + i*nOffToNextY, 1, nWindowY);
	}

	//初始化所有Y窗口
	//特殊y
	for (int i = 0; i < nCol; ++i) {                                //膨胀，边缘外扩补0
		unsigned char* pWy_l = pScrollWinY_start + i*nOffToNextY;
		unsigned char* pWy_r = pScrollWinY_start + (ncx + nCol + i)*nOffToNextY;
		memset(pWy_l, 0, yy);
		memset(pWy_r, 0, yy);
	}

	//正常y
	unsigned char* ps = pSrcDst + nRow*nSrcDstPitch;
	for (int j = 0; j < ncx; ++j) {
		unsigned char* pWy = pScrollWinY_start + (j + nCol)*nOffToNextY + nyless;
		pWy[0] = ps[j] > 0 ? 1 : 0;
	}

	for (int i = nWindowY - 2; i >= nRow; --i) {
		unsigned char* ps = pSrcDst + (i - nRow) * nSrcDstPitch;
		for (int j = 0; j < ncx; ++j) {
			unsigned char* pWy = pScrollWinY_start + (j + nCol)*nOffToNextY + i;
			pWy[0] = pWy[1] || ps[j];                         //膨胀，或运算                  
		}
	}

	for (int i = nRow - 1; i >= 0; --i) {
		for (int j = 0; j < ncx; ++j) {
			unsigned char* pWy = pScrollWinY_start + (j + nCol)*nOffToNextY + i;
			pWy[0] = pWy[1];
		}
	}

	for (int i = 0; i < xx; ++i)
		ppAllScrollWinYs[i] = pScrollWinY_start + i*nOffToNextY;

	//初始化X窗口
	pScrollWinX_start[nxless] = *(ppAllScrollWinYs[nxless]);
	for (int j = nWindowX - 2; j >= 0; --j) {
		pScrollWinX_start[j] = pScrollWinX_start[j + 1] | (*ppAllScrollWinYs[j]);  //膨胀 ，或运算
	}


	//work...
	unsigned char*  pRealWinX = pScrollWinX_start;
	int        nOffSet = ncy - nRow - 1;
	unsigned char** ppRealAllWinYs = ppAllScrollWinYs + nCol;
	unsigned char** ppRealWinYs2 = ppRealAllWinYs + nCol + 1;
	int        nLessX = ncx - 1;
	int        nLessY = ncy - 1;
	int        nOffSetOfSrc = (nRow + 1)*nSrcDstPitch;
	for (int i = 0; i < ncy; ++i) {
		unsigned char* pd = pSrcDst + i*nSrcDstPitch;
		for (int j = 0; j < ncx; ++j) {
			pd[j] = pRealWinX[0] == 0 ? 0 : 255;
			//更新X窗口
			if (j < nLessX) {
				if (*ppRealWinYs2[j] != 0) {         //膨胀，如果遇到255，就跳转，遇到0，就添加
					pRealWinX = pScrollWinX;
				}
				else {
					pRealWinX[nWindowX] = 0;
					pRealWinX += 1;
				}
			}
		}

		//更新Y窗口
		if (i < nOffSet) {
			unsigned char* ps = pd + nOffSetOfSrc;
			for (int j = 0; j < ncx; ++j) {
				if (ps[j] != 0) {                                                       //膨胀
					ppRealAllWinYs[j] = pScrollWinY + (j + nCol) * nOffToNextY;
				}
				else {
					ppRealAllWinYs[j][nWindowY] = 0;
					ppRealAllWinYs[j] += 1;
				}
			}
			//更新X窗口
			pScrollWinX_start[nxless] = *(ppAllScrollWinYs[nxless]);
			for (int j = nWindowX - 2; j >= 0; --j) {
				pScrollWinX_start[j] = pScrollWinX_start[j + 1] | (*ppAllScrollWinYs[j]);   //膨胀 或运算
			}
			pRealWinX = pScrollWinX_start;

		}
		else if (i >= nOffSet && i < nLessY) {
			for (int j = 0; j < ncx; ++j) {
				ppRealAllWinYs[j][nWindowY] = 0;
				ppRealAllWinYs[j] += 1;
			}
			//更新X窗口
			pScrollWinX_start[nxless] = *(ppAllScrollWinYs[nxless]);
			for (int j = nWindowX - 2; j >= 0; --j) {
				pScrollWinX_start[j] = pScrollWinX_start[j + 1] | (*ppAllScrollWinYs[j]);
			}
			pRealWinX = pScrollWinX_start;
		}
	}

	return 0;
}

int Create_Beauty_Filter(void** inst)
{
	Beauty_Filter* filter = 0;
	if (create_beauty_filter(&filter) == -1)
	{
		*inst = 0;
		return -1;
	}
	else
	{
		*inst = filter;
		return 0;
	}
}
int Init_Beauty_Filter(void* self, unsigned char radius, unsigned char thres)
{
	return init_beauty_filter((Beauty_Filter*)self, radius, thres, 0);
}

int Set_Radius_Thres(void* self, unsigned char radius, unsigned char thres)
{
	return set_radius_thres((Beauty_Filter*)self, radius, thres);
}

int Enable_Whitening(void* self, int enable)
{
	return enable_whitening((Beauty_Filter*)self, enable);
}

int Proc_Beauty_Filter(void* self, unsigned int w, unsigned int h, unsigned int scan, unsigned char* psrcdst, int maskscan, unsigned char* mask)
{
	return beauty_filter((Beauty_Filter*)self, w, h, scan, psrcdst, maskscan, mask);
}

int Free_Beauty_Filter(void* self)
{
	return free_beauty_filter((Beauty_Filter*)self);
}


//////////////////////////////////////////////////////////////////////////


int create_beauty_filter(Beauty_Filter** self)
{
	int i;
	*self = (Beauty_Filter*)malloc(sizeof(Beauty_Filter));
	if ((*self) == 0)
		return -1;
	
	(*self)->bSharpen = 0;
	(*self)->bWhitening = 1;
	(*self)->data = 0;
	(*self)->mask = 0;
	(*self)->table = 0;
	(*self)->radius = 20;
	(*self)->thres = 28;
	(*self)->bInitFlag = 0;
	(*self)->w = 0;
	(*self)->h = 0;
	(*self)->maskw = 0;
	(*self)->maskh = 0;

	(*self)->morph_erosion = 0;
	(*self)->morph_dilation = 0;

	(*self)->e_buf_len = 0;
	(*self)->d_buf_len = 0;
	(*self)->beta = 2;


	for (i = 0; i < 256; ++i)
	{
		float f = log((i / 255.0f) * ((*self)->beta - 1.0f) + 1.f) / log((*self)->beta);
		f *= 255.f;
		(*self)->curveTable[i] = (unsigned char)(f + 0.5f);
	}

	return 0;
}

int init_beauty_filter(Beauty_Filter* self, unsigned char radius, unsigned char thres, int bsharpen)
{
	int i = 0;
	int j = 0;

	if (!self)
		return -1;

	radius = radius < 1 ? 1 : radius > 100 ? 100 : radius;
	thres = thres < 1 ? 1 : thres > 40 ? 40 : thres;

	//if (radius < 1 || radius > 100)
	//	return -1;

	//if (thres < 1 || thres > 40)
	//	return -1;

	self->radius = radius;
	self->thres = thres;
	self->bSharpen = bsharpen;

	self->w = 2048 + 2 * radius;
	self->h = 2048 + 2 * radius;

	//init table
	self->table = (unsigned int*)malloc(256 * 256 * sizeof(unsigned int));
	if (self->table == 0)
		return -1;

	self->data = (unsigned char*)malloc((self->w + 2)* (self->h + 2));
	if (self->data == 0)
	{
		free(self->table);
		self->table = 0;
		return -1;
	}
	memset(self->data, 0, (self->w + 2)* (self->h + 2));

	for (i = 0; i < 256; ++i)
	{
		unsigned int* fi = self->table + i * 256;
		for (j = 0; j < 256; ++j)
		{
			int w = 2500 - 1000 * abs(j - i) / thres;
			if (w < 0)
			{
				w = 0;
				fi[j] = 0;
			}
			else
			{
				fi[j] = (w << 20);
				fi[j] += w*j;
			}
		}
	}

	self->bInitFlag = 1;

	return 0;
}

int set_sharpen(Beauty_Filter* self, int bsharpen)
{
	if (!self)
		return -1;

	self->bSharpen = bsharpen;

	return 0;
}

int set_radius_thres(Beauty_Filter* self, unsigned char radius, unsigned char thres)
{
	int i;
	int j;

	if (!self)
		return -1;

	//if (radius < 1 || radius > 100)
	//	return -1;

	//if (thres < 1 || thres > 40)
	//	return -1;
	radius = radius < 1 ? 1 : radius > 100 ? 100 : radius;
	thres = thres < 1 ? 1 : thres > 40 ? 40 : thres;

	if (self->radius != radius || self->thres != thres || !self->bInitFlag )
	{
		if (self->bInitFlag)
		{
			free(self->table);
			self->table = 0;
			free(self->data);
			self->data = 0;
		}
		self->w = 2048 + 2 * radius;
		self->h = 2048 + 2 * radius;

		//init table
		self->table = (unsigned int*)malloc(256 * 256 * sizeof(unsigned int));
		if (self->table == 0)
			return -1;

		self->data = (unsigned char*)malloc((self->w + 2)* (self->h + 2));
		if (self->data == 0)
		{
			free(self->table);
			self->table = 0;
			return -1;
		}
		memset(self->data, 0, (self->w + 2)* (self->h + 2));

		for (i = 0; i < 256; ++i)
		{
			unsigned int* fi = self->table + i * 256;
			for (j = 0; j < 256; ++j)
			{
				int w = 2500 - 1000 * abs(j - i) / thres;
				if (w < 0)
				{
					w = 0;
					fi[j] = 0;
				}
				else
				{
					fi[j] = (w << 20);
					fi[j] += w*j;
				}
			}
		}

		self->bInitFlag = 1;
		self->radius = radius;
		self->thres = thres;
	}

	return 0;
}

int reset(Beauty_Filter* self)
{
	if (!self)
		return -1;
}

int enable_whitening(Beauty_Filter* self, int enable)
{
	if (!self)
		return -1;

	self->bWhitening = enable;

	return 0;
}

int beauty_filter(Beauty_Filter* self, unsigned int w, unsigned int h, unsigned int scan, unsigned char* psrcdst, int maskscan, unsigned char* mask)
{
	int i = 0;
	int j = 0;
	int r = 0;
	int r2 = 0;
	int thres;
	int Hist[256] = { 0 };
	int xupdate = 0;
	int yupdate = 0;
	unsigned char* ps_u = 0;
	unsigned char* ps_d = 0;
	unsigned char* ps_add = 0;
	unsigned char* ps_sub = 0;

	int xstep = 1;

	int rem = 0;
	unsigned char* data_start = 0;
	unsigned int data_scan = 0;

	if (!self)
		return -1;

	if (!self->bInitFlag)
		return -1;

	if (w == 0 || h == 0 || scan == 0 || psrcdst == 0 || maskscan == 0 || mask == 0)
		return -1;

	r = self->radius;
	thres = self->thres;
	r2 = 2 * r + 1;

	if (w + 2 * r > self->w)
	{
		self->w = w + 2 * r;
		rem = 1;
	}
	if (h + 2 * r > self->h)
	{
		self->h = h + 2 * r;
		rem = 1;
	}
	if (rem == 1)
	{
		if (self->data)
			free(self->data);

		self->data = malloc((self->w + 2)* (self->h + 2));

		if (self->data == 0)
			return -1;

		memset(self->data, 0, (self->w + 2)* (self->h + 2));
	}

	data_scan = w + 2 * r;
	data_start = self->data + r * data_scan + r;

	for (i = 0; i < h; ++i)
	{
		unsigned char* ps = psrcdst + i*scan;
		unsigned char* pd = data_start + i*data_scan;
		memcpy(pd, ps, w);
	}

	//
	if (!self->mask || self->maskw != w || self->maskh != h)
	{
		if (self->mask) {
			free(self->mask);
			self->mask = 0;
		}
		self->mask = (unsigned char*)malloc(w*h);
		if (self->mask == 0)
			return -1;

		memset(self->mask, 0, w*h);
		self->maskw = w;
		self->maskh = h;
	}

	zipl_fDilation(self, w, h, maskscan, mask, 2, 2);
	zipl_fErosion(self, w, h, maskscan, mask, 2, 2);

	UpdateMask(w, h, maskscan, mask, w, self->mask);

	//init hist
	for (i = 0; i < r2; ++i)
	{
		unsigned char* ps = self->data + i*data_scan;
		for (j = 0; j < r2; ++j)
		{
			Hist[ps[j]]++;
		}
	}

	//implement
	xstep = 1;
	for (i = 0; i < h; ++i)
	{
		unsigned char* ps = self->data + i*data_scan;
		unsigned char* pc = data_start + i*data_scan;
		unsigned char* pd = psrcdst + i*scan;
		//unsigned char* pm = mask + i*maskscan;
		unsigned char* pm = self->mask + i*w;

		if (xstep == 1)
		{
			for (j = 0; j < w; ++j)
			{
				int cc = pc[j] * 256;
				int sum = 0;
				int divisor = 0;

				if (xupdate == 1)
				{
					for (int m = 0; m < r2; ++m)
					{
						Hist[*ps_add]++;
						ps_add += data_scan;

						Hist[*ps_sub]--;
						ps_sub += data_scan;
					}
				}

				xupdate = 1;
				ps_sub = ps + j;
				ps_add = ps_sub + r2;

				if (pm[j])
				{
					for (int m = 0; m < 256; ++m)
					{
						if (Hist[m] > 0)
						{
							unsigned int w = *(self->table + cc + m);
							divisor += Hist[m] * (w >> 20);
							sum += Hist[m] * ((w << 12) >> 12);
						}
					}

					int f = (sum + (divisor >> 1)) / (divisor + 1);
					pd[j] = f < 0 ? 0 : f > 255 ? 255 : f;
				}
				else
				{
					pd[j] = pc[j];
				}
			}
		}
		else
		{
			for (j = w - 1; j > -1; --j)
			{
				int cc = pc[j] * 256;
				int sum = 0;
				int divisor = 0;

				if (xupdate == 1)
				{
					for (int m = 0; m < r2; ++m)
					{
						Hist[*ps_add]++;
						ps_add += data_scan;

						Hist[*ps_sub]--;
						ps_sub += data_scan;
					}
				}

				xupdate = 1;
				ps_add = ps + j - 1;
				ps_sub = ps_add + r2;

				if (pm[j])
				{
					for (int m = 0; m < 256; ++m)
					{
						if (Hist[m] > 0)
						{
							unsigned int w = *(self->table + cc + m);
							divisor += Hist[m] * (w >> 20);
							sum += Hist[m] * ((w << 12) >> 12);
						}
					}

					int f = (sum + (divisor >> 1)) / (divisor + 1);
					pd[j] = f < 0 ? 0 : f > 255 ? 255 : f;
				}
				else
				{
					pd[j] = pc[j];
				}
				
			}
		}

		//
		if (xstep == 1)
		{
			ps_u = ps + w - 1;
			ps_d = ps_u + r2*data_scan;
			xstep = -1;
		}
		else
		{
			ps_u = ps;
			ps_d = ps_u + r2*data_scan;
			xstep = 1;
		}
		xupdate = 0;

		for (int m = 0; m < r2; ++m)
		{
			Hist[*ps_u++]--;
			Hist[*ps_d++]++;
		}
	}


	//融合
	for (i = 0; i < h; ++i)
	{
		unsigned char* pss = psrcdst + i*scan;
		unsigned char* pdd = data_start + i*data_scan;
		unsigned char* pm = self->mask + i*w;
		for (int s = 0; s < w; ++s)
		{
			pm[s] = pm[s] > 200 ? 200 : pm[s];
			pss[s] = (pss[s] * pm[s] + pdd[s] * (255 - pm[s])) >> 8;
		}
	}

	//whitening
	if (self->bWhitening)
	{
		curve(self, w, h, scan, psrcdst, maskscan, mask);
	}

	return 0;
}

int beauty_filter2(Beauty_Filter* self, unsigned int w, unsigned int h, unsigned int scan, unsigned char* psrc, unsigned int scan_dst, unsigned char* pdst)
{
	int i = 0;
	int j = 0;
	int r = 0;
	int r2 = 0;
	int thres;
	int Hist[256] = { 0 };
	int xupdate = 0;
	int yupdate = 0;
	unsigned char* ps_u = 0;
	unsigned char* ps_d = 0;
	unsigned char* ps_add = 0;
	unsigned char* ps_sub = 0;
	
	int xstep = 1;
	
	int rem = 0;
	unsigned char* data_start = 0;
	unsigned int data_scan = 0;

	if (!self)
		return -1;

	if (w == 0 || h == 0 || scan == 0 || psrc == 0)
		return -1;

	r       = self->radius;
	thres   = self->thres;
	r2 = 2 * r + 1;
	
	if (w + 2 * r > self->w)
	{
		self->w = w + 2 * r;
		rem = 1;
	}
	if (h + 2 * r > self->h)
	{
		self->h = h + 2 * r;
		rem = 1;
	}
	if (rem == 1)
	{
		if(self->data)
			free(self->data);

		self->data = malloc((self->w + 2)* (self->h + 2));
		
		if (self->data == 0)
			return -1;

		memset(self->data, 0, (self->w + 2)* (self->h + 2));
	}
	
	data_scan = w + 2 * r;
	data_start = self->data + r * data_scan + r;

	for (i = 0; i < h; ++i)
	{
		unsigned char* ps = psrc + i*scan;
		unsigned char* pd = data_start + i*data_scan;
		memcpy(pd, ps, w);
	}

	//init hist
	for (i = 0; i < r2; ++i)
	{
		unsigned char* ps = self->data + i*data_scan;
		for (j = 0; j < r2; ++j)
		{
			Hist[ps[j]]++;
		}
	}

	//implement
	xstep = 1;
	for (i = 0; i < h; ++i)
	{
		unsigned char* ps = self->data + i*data_scan;
		unsigned char* pc = data_start + i*data_scan;
		unsigned char* pd = pdst + i*scan_dst;

		if (xstep == 1)
		{
			for (j = 0; j < w; ++j)
			{
				int cc = pc[j] * 256;
				int sum = 0;
				int divisor = 0;

				if (xupdate == 1)
				{
					for (int m = 0; m < r2; ++m)
					{
						Hist[*ps_add]++;
						ps_add += data_scan;

						Hist[*ps_sub]--;
						ps_sub += data_scan;
					}
				}

				xupdate = 1;
				ps_sub = ps + j;
				ps_add = ps_sub + r2;
				
				for (int m = 0; m < 256; ++m)
				{
					if (Hist[m] > 0)
					{
						unsigned int w = *(self->table + cc + m);
						divisor += Hist[m] * (w >> 20);
						sum += Hist[m] * ((w << 12) >> 12);
					}
				}

				int f = (sum + (divisor >> 1)) / (divisor + 1);
				pd[j] = f < 0 ? 0 : f > 255 ? 255 : f;
			}
		}
		else
		{
			for (j = w-1; j > -1; --j)
			{
				int cc = pc[j] * 256;
				int sum = 0;
				int divisor = 0;

				if (xupdate == 1)
				{
					for (int m = 0; m < r2; ++m)
					{
						Hist[*ps_add]++;
						ps_add += data_scan;

						Hist[*ps_sub]--;
						ps_sub += data_scan;
					}
				}

				xupdate = 1;
				ps_add = ps + j - 1;
				ps_sub = ps_add + r2;

				for (int m = 0; m < 256; ++m)
				{
					if (Hist[m] > 0)
					{
						unsigned int w = *(self->table + cc + m);
						divisor += Hist[m] * (w >> 20);
						sum += Hist[m] * ((w << 12) >> 12);
					}
				}

				int f = (sum + (divisor >> 1)) / (divisor + 1);
				pd[j] = f < 0 ? 0 : f > 255 ? 255 : f;
			}
		}

		//
		if (xstep == 1)
		{
			ps_u = ps + w-1;
			ps_d = ps_u + r2*data_scan;
			xstep = -1;
		}
		else
		{
			ps_u = ps;
			ps_d = ps_u + r2*data_scan;
			xstep = 1;
		}
		xupdate = 0;

		for (int m = 0; m < r2; ++m)
		{
			Hist[*ps_u++]--;
			Hist[*ps_d++]++;
		}
	}

	return 0;
}

int free_beauty_filter(Beauty_Filter* self)
{
	if (self)
	{
		if (self->table)
			free(self->table);

		if (self->data)
			free(self->data);

		if (self->mask)
			free(self->mask);

		if (self->morph_dilation)
			free(self->morph_dilation);

		if (self->morph_erosion)
			free(self->morph_erosion);

		free(self);
		self = 0;
	}

	return 0;
}

/*
//原始算法
void ZFilter_8u_C1R(int width, int height, int scan_src, unsigned char* pbuf_src, int scan_dst, unsigned char* pbuf_dst)
{
	int i = 0;
	int j = 0;
	int r = 20;
	int thres = 28;
	int x_start = r;
	int x_end = width - r;
	int y_start = r;
	int y_end = height - r;
	for (i = y_start; i < y_end; ++i)
	{
		unsigned char* ps_c = pbuf_src + i*scan_src;
		unsigned char* ps = ps_c - r*scan_src;
		unsigned char* pd = pbuf_dst + i*scan_dst;
		for (j = x_start; j < x_end; ++j)
		{
			//processing a block
			unsigned char* pss   = ps + j - r;
			int cc = ps_c[j];
			float numer = 0;
			float denom = 0;
			for (int m = 0; m < 2*r + 1; ++m)
			{
				unsigned char* ps0 = pss + m*scan_src;
				for( int n = 0; n < 2*r + 1; ++n)
				{
					float f = 1.f - ( abs(ps0[n] - cc) / (2.5f*thres) );
					f = f < 0 ? 0 : f;
					denom += f;
					numer += f * ps0[n];
				}
			}
			//
			float f = numer / denom + 0.001f;
			pd[j] = f < 0 ? 0 : f > 255 ? 255 : f;
		}
	}
}

*/
