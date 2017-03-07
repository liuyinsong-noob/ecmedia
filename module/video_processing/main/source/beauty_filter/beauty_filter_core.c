#include "video_beauty_filter.h"
#include "beauty_filter_core.h"
#include <malloc.h>
#ifdef __ANDROID__
#include <cmath>
#define ABS std::abs
#else
#include <math.h>
#define ABS abs
#endif
#include <string.h>

//

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
	*self = (Beauty_Filter*)malloc(sizeof(Beauty_Filter));
	if ((*self) == 0)
		return -1;
	
	(*self)->bSharpen = 0;
	(*self)->data = 0;
	(*self)->table = 0;
	(*self)->radius = 20;
	(*self)->thres = 28;
	(*self)->bInitFlag = 0;
	(*self)->w = 0;
	(*self)->h = 0;

	return 0;
}

int init_beauty_filter(Beauty_Filter* self, unsigned char radius, unsigned char thres, int bsharpen)
{
	int i = 0;
	int j = 0;

	if (!self)
		return -1;

	if (radius < 1 || radius > 100)
		return -1;

	if (thres < 1 || thres > 40)
		return -1;

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
			int w = 2500 - 1000 * ABS(j - i) / thres;
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

	if (radius < 1 || radius > 100)
		return -1;

	if (thres < 1 || thres > 40)
		return -1;

	if (self->radius != radius || self->thres != thres)
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
				int w = 2500 - 1000 * ABS(j - i) / thres;
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

		self->data = (unsigned char*)malloc((self->w + 2)* (self->h + 2));

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
		unsigned char* pm = mask + i*maskscan;

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
					pd[j] = f < 16 ? 16 : f > 235 ? 235 : f;
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
					pd[j] = f < 16 ? 16 : f > 235 ? 235 : f;
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

		self->data = (unsigned char*)malloc((self->w + 2)* (self->h + 2));
		
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

		free(self);
		self = 0;
	}

	return 0;
}

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
					float f = 1.f - (ABS(ps0[n] - cc) / (2.5f*thres) );
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
