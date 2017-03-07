#include "ipl_color.h"
#include "ipl_define.h"
#include <math.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

#define  PI  3.14159265f
#define  RAD 57.2957796f
#define  RADInv 0.0174532925f
#define  DENOM  9.58752622e-5
#define  ZMAX(a,b) (a) > (b) ? (a) : (b)
#define  ZMIN(a,b) (a) > (b) ? (b) : (a)
//#define  

//
//yuv420p->rgb  整型实现，效率高， 601色域
//yuv420p 是视频格式，y[16, 235] uv[16, 240]
//test passed
int zipl_yuv420p_rgb( int w,
	                  int h,
	                  unsigned char* pyuv,
	                  int scan_rgb,
	                  unsigned char* prgb
	                )
{
	int i, j;
	unsigned char* py;
	unsigned char* pu;
	unsigned char* pv;
	unsigned char* pd;
	int r, g, b;
	int ww = w/2;
	int hh = h/2;
	unsigned char* yy;
	unsigned char* uu;
	unsigned char* vv;

	if( w <= 0 || h <= 0 || scan_rgb <= 0 )
		return dwIplColorParamError;

	if( prgb == 0 || pyuv == 0 )
		return dwIplColorPointerNull;

	//
	py = pyuv;
	pu = py + w*h;
	pv = pu + w*h/4;

	for( i = 0; i < h; ++i )
	{
		pd = prgb + i*scan_rgb;
		yy = py + i*w;
		uu = pu + (i/2)*ww;
		vv = pv + (i/2)*ww;
		for( j = 0; j < ww; ++j )
		{
			int iy = yy[0] - 16;
			int iu = uu[0] - 128;
			int iv = vv[0] - 128;

			int itpy = 1192 * iy;
			int itp0 = 1634 * iv;
			int itp1 = 401 * iu + 832 * iv;
			int itp2 = 2066 * iu;

			r  = (itpy + itp0 + 512)>>10;
			g  = (itpy - itp1 + 512)>>10;
			b  = (itpy + itp2 + 512)>>10;

			pd[0] = b < 0 ? 0 : (b > 255 ? 255 : b);
			pd[1] = g < 0 ? 0 : (g > 255 ? 255 : g);
			pd[2] = r < 0 ? 0 : (r > 255 ? 255 : r);

			///
			iy   = yy[1] - 16;
			itpy = 1192 * iy;
			r  = (itpy + itp0 + 512)>>10;
			g  = (itpy - itp1 + 512)>>10;
			b  = (itpy + itp2 + 512)>>10;

			pd[3] = b < 0 ? 0 : (b > 255 ? 255 : b);
			pd[4] = g < 0 ? 0 : (g > 255 ? 255 : g);
			pd[5] = r < 0 ? 0 : (r > 255 ? 255 : r);

			pd += 6;
			yy += 2;
			uu++;
			vv++;
		}
	}

	return 0;
}
//test passed
int zipl_yuv420_rgb( int w, int h,
	                 int scany, unsigned char* py,
	                 int scanu, unsigned char* pu,
	                 int scanv, unsigned char* pv,
	                 int scan_rgb,unsigned char* prgb
	                )
{
	int i, j;
	unsigned char* pd;
	int r, g, b;
	int ww = w/2;
	int hh = h/2;
	unsigned char* yy;
	unsigned char* uu;
	unsigned char* vv;

	if( w <= 0 || h <= 0 || scan_rgb <= 0 || scany <= 0 || scanu <= 0 || scanv <= 0 )
		return dwIplColorParamError;

	if( prgb == 0 || py == 0 || pu == 0 || pv == 0 )
		return dwIplColorPointerNull;

	for( i = 0; i < h; ++i )
	{
		pd = prgb + i*scan_rgb;
		yy = py + i*scany;
		uu = pu + (i/2)*scanu;
		vv = pv + (i/2)*scanv;
		for( j = 0; j < ww; ++j )
		{
			int iy = yy[0] - 16;
			int iu = uu[0] - 128;
			int iv = vv[0] - 128;

			int itpy = 1192 * iy;
			int itp0 = 1634 * iv;
			int itp1 = 401 * iu + 832 * iv;
			int itp2 = 2066 * iu;

			r  = (itpy + itp0 + 512)>>10;
			g  = (itpy - itp1 + 512)>>10;
			b  = (itpy + itp2 + 512)>>10;

			pd[0] = b < 0 ? 0 : (b > 255 ? 255 : b);
			pd[1] = g < 0 ? 0 : (g > 255 ? 255 : g);
			pd[2] = r < 0 ? 0 : (r > 255 ? 255 : r);

			///
			iy   = yy[1] - 16;
			itpy = 1192 * iy;
			r  = (itpy + itp0 + 512)>>10;
			g  = (itpy - itp1 + 512)>>10;
			b  = (itpy + itp2 + 512)>>10;

			pd[3] = b < 0 ? 0 : (b > 255 ? 255 : b);
			pd[4] = g < 0 ? 0 : (g > 255 ? 255 : g);
			pd[5] = r < 0 ? 0 : (r > 255 ? 255 : r);

			pd += 6;
			yy += 2;
			uu++;
			vv++;
		}
	}

	return 0;
}

//
//rgb->yuv420p 整型实现，效率高  601色域
//yuv420p 是视频格式， y [16 235] uv [16 240]
//test passed
int zipl_rgb_yuv420p( int w,
	                  int h,
	                  int scan_rgb,
	                  unsigned char* prgb,
	                  unsigned char* pyuv
	                 )
{
	//  
	//   Y         |   0.256788f     0.504129f    0.097906f |  R      16
	//   U     =   |  -0.148223f    -0.290993f    0.439216f |  G   +  128
	//   V         |   0.439216f    -0.367788f   -0.071427f |  B      128
	//
	//                 263           516           100
	//                -152          -298           450
	//                 450          -377           -73
	//

	int i, j;
	unsigned char* ps0, *ps1;
	int r, g, b;
	int y, u, v;
	unsigned char* py;
	unsigned char* pu;
	unsigned char* pv;
	unsigned char* yy0, *yy1;
	unsigned char* uu;
	unsigned char* vv;
	int hh = h / 2;
	int ww = w / 2;

	if( w <= 0 || h <= 0 || scan_rgb <= 0 )
		return dwIplColorParamError;

	if( pyuv == 0 || prgb == 0 )
		return dwIplColorPointerNull;

	//
	py = pyuv;
	pu = py + w * h;
	pv = pu + w * h/4;

	//
	for( i = 0; i < hh; ++i )
	{
		ps0 = prgb + i * 2 * scan_rgb;
		ps1 = ps0 + scan_rgb;
		yy0 = py + i * 2 * w;
		yy1 = yy0 + w;
		uu = pu + i * ww;
		vv = pv + i * ww;
		for( j = 0; j < ww; ++j )
		{
			b     = ps0[0];
			g     = ps0[1];
			r     = ps0[2];
			y      = ((263 * r + 516 * g + 100 * b + 512)>>10) + 16;
			yy0[0] = y < 16 ? 16 : y > 235 ? 235 : y;

			b     = ps1[0];
			g     = ps1[1];
			r     = ps1[2];
			y      = ((263 * r + 516 * g + 100 * b + 512)>>10) + 16;
			yy1[0] = y < 16 ? 16 : y > 235 ? 235 : y;

			u     = ((-152 * r - 298 * g + 450 * b + 512)>>10) + 128;
			uu[0] = u < 16 ? 16 : u > 240 ? 240 : u;

			v     = ((450 * r - 377 * g - 73 * b + 512)>>10) + 128;
			vv[0] = v < 16 ? 16 : v > 240 ? 240 : v;

			b     = ps0[3];
			g     = ps0[4];
			r     = ps0[5];
			y      = ((263 * r + 516 * g + 100 * b + 512)>>10) + 16;
			yy0[1] = y < 16 ? 16 : y > 235 ? 235 : y;

			b     = ps1[3];
			g     = ps1[4];
			r     = ps1[5];
			y      = ((263 * r + 516 * g + 100 * b + 512)>>10) + 16;
			yy1[1] = y < 16 ? 16 : y > 235 ? 235 : y;

			ps0 += 6;
			ps1 += 6;
			yy0 += 2;
			yy1 += 2;
			uu++;
			vv++;
		}
	}

	return 0;
}

//test passed
int zipl_rgb_yuv420( int w, int h,
	                 int scan_rgb, unsigned char* prgb,
	                 int scany, unsigned char* py,
	                 int scanu, unsigned char* pu,
	                 int scanv, unsigned char* pv
	                )
{
	int i, j;
	unsigned char* ps0, *ps1;
	int r, g, b;
	int y, u, v;
	unsigned char* yy0, *yy1;
	unsigned char* uu;
	unsigned char* vv;
	int hh = h / 2;
	int ww = w / 2;

	if( w <= 0 || h <= 0 || scan_rgb <= 0 || scany <= 0 || scanu <= 0 || scanv <= 0 )
		return dwIplColorParamError;

	if( py == 0 || pu == 0 || pv == 0 || prgb == 0 )
		return dwIplColorPointerNull;

	//
	for( i = 0; i < hh; ++i )
	{
		ps0 = prgb + i * 2 * scan_rgb;
		ps1 = ps0 + scan_rgb;
		yy0 = py + i * 2 * scany;
		yy1 = yy0 + scany;
		uu = pu + i * scanu;
		vv = pv + i * scanv;
		for( j = 0; j < ww; ++j )
		{
			b     = ps0[0];
			g     = ps0[1];
			r     = ps0[2];
			y      = ((263 * r + 516 * g + 100 * b + 512)>>10) + 16;
			yy0[0] = y < 16 ? 16 : y > 235 ? 235 : y;

			b     = ps1[0];
			g     = ps1[1];
			r     = ps1[2];
			y      = ((263 * r + 516 * g + 100 * b + 512)>>10) + 16;
			yy1[0] = y < 16 ? 16 : y > 235 ? 235 : y;

			u     = ((-152 * r - 298 * g + 450 * b + 512)>>10) + 128;
			uu[0] = u < 16 ? 16 : u > 240 ? 240 : u;

			v     = ((450 * r - 377 * g - 73 * b + 512)>>10) + 128;
			vv[0] = v < 16 ? 16 : v > 240 ? 240 : v;

			b     = ps0[3];
			g     = ps0[4];
			r     = ps0[5];
			y      = ((263 * r + 516 * g + 100 * b + 512)>>10) + 16;
			yy0[1] = y < 16 ? 16 : y > 235 ? 235 : y;

			b     = ps1[3];
			g     = ps1[4];
			r     = ps1[5];
			y      = ((263 * r + 516 * g + 100 * b + 512)>>10) + 16;
			yy1[1] = y < 16 ? 16 : y > 235 ? 235 : y;

			ps0 += 6;
			ps1 += 6;
			yy0 += 2;
			yy1 += 2;
			uu++;
			vv++;
		}
	}

	return 0;
}

//test passed
int zipl_rgb_hsip( int w, int h,
	               int scan_rgb, 
				   unsigned char* prgb,
	               float* phsi
	              )
{
	int i, j;
	unsigned char* ps;
	int r, g, b;
	int imin;
	int sum;
	float* pfh;
	int* pfs;
	int* pfi;

	if( w <= 0 || h <= 0 || scan_rgb <= 0 )
		return dwIplColorParamError;

	if( prgb == 0 || phsi == 0 )
		return dwIplColorPointerNull;

	pfh = phsi;
	pfs = (int*)(pfh + w*h);
	pfi = pfs + w*h;

	for( i = 0; i < h; ++i )
	{
		ps = prgb + i*scan_rgb;
		for( j = 0; j < w; ++j )
		{
			b = ps[0];
			g = ps[1];
			r = ps[2];

			if( r == g && g == b )
			{//
				*pfh++ = 0.f;
			}
			else
			{
				float theta;
				theta = acosf(0.5f*((r-g) + (r-b)) / sqrtf((r-g)*(r-g) + (r-b)*(g-b)));
				theta *= RAD;
				if( b <= g )
					*pfh++ = theta;
				else
					*pfh++ = 360 - theta;
			}

			imin = r < g ? r : g;
			imin = imin < b ? imin : b;

			sum = r + g + b;
			*pfs++  = imin;

			*pfi++  = sum;

			ps += 3;
		}
	}

	return 0;
}

//test passed
int zipl_rgb_hsip_fast( int w, int h,
	                    int scan_rgb, unsigned char* prgb,
	                    float* phsi
	                  )
{
	int i, j;
	unsigned char* ps;

	float* phh;
	int* pss, *pii;

	int r, g, b;
	int imin;
	int sum;

	if( w <= 0 || h <= 0 || scan_rgb <= 0 )
		return dwIplColorParamError;

	if( prgb == 0 || phsi == 0 )
		return dwIplColorPointerNull;

	phh = phsi;
	pss = (int*)(phh + w*h);
	pii = pss + w*h;

	for( i = 0; i < h; ++i )
	{
		ps= prgb + i*scan_rgb;
		for( j = 0; j < w; ++j )
		{
			b = ps[0];
			g = ps[1];
			r = ps[2];

			if( r == g && g == b )
			{//h
				*phh++ = 0;
			}
			else
			{//h
				//float theta;
				//theta = acosf(0.5f*((r-g) + (r-b)) / sqrtf((r-g)*(r-g) + (r-b)*(g-b)));
				//theta *= RAD;
				int ind;
				int it = ((r-g) + (r-b))/*<<10*/;
				int is = (r-g)*(r-g) + (r-b)*(g-b);
// 				if( it < 0 )
// 					ind = ((it * sqrtfTable[is] - 4096)>>13) + 2048;
// 				else
// 					ind = ((it * sqrtfTable[is] + 4096)>>13) + 2048;
				ind = ((it * sqrtfTable[is] + 4096)>>13) + 2048;
				if( b <= g )
					*phh++ = hsiTable_f[ind];
				else
					*phh++ = 360 - hsiTable_f[ind];
			}

			imin = r < g ? r : g;
			imin = imin < b ? imin : b;

			sum = r + g + b;

			*pss++ = imin;
			*pii++ = sum;

			ps += 3;
		}
	}

	return 0;
}

//test passed
int zipl_hsip_rgb( int w, 
	               int h,
	               float* phsi,
	               int scan_rgb,
	               unsigned char* prgb
	              )
{
	int i, j;
	unsigned char* pd;
	float fr, fg, fb;
	float* ph;
	int *ps, *pi;
	float fh;
	int fs, fi;

	if( w <= 0 || h <= 0 || scan_rgb <= 0 )
		return dwIplColorParamError;

	if( prgb == 0 || phsi == 0 )
		return dwIplColorPointerNull;

	ph = phsi;
	ps = (int*)(ph + w*h);
	pi = ps + w*h;

	for( i = 0; i < h; ++i )
	{
		pd = prgb + i*scan_rgb;
		for( j = 0; j < w; ++j )
		{
			fh = *ph++;
			fs = *ps++;
			fi = *pi++;

			if( fh >= 0 && fh < 120 )
			{
				fr = 0.333f * (fi + (fi - 3*fs) * cosf( fh*RADInv ) / cosf((60 - fh)*RADInv) );
				fg = fi - (fr + fs);
				pd[0] = fs;
				pd[1] = fg < 0 ? 0 : fg > 255 ? 255 : (unsigned char)fg;
				pd[2] = fr < 0 ? 0 : fr > 255 ? 255 : (unsigned char)fr;
			}
			else if( fh >= 120 && fh < 240 )
			{
				fh -= 120;
				fg = 0.333f * (fi + (fi - 3*fs) * cosf( fh*RADInv ) / cosf((60 - fh)*RADInv) );
				fb = fi - (fs + fg);
				pd[0] = fb < 0 ? 0 : fb > 255 ? 255 : (unsigned char)fb;
				pd[1] = fg < 0 ? 0 : fg > 255 ? 255 : (unsigned char)fg;
				pd[2] = fs;
			}
			else
			{
				fh -= 240;
				fb = 0.333f * (fi + (fi - 3*fs) * cosf( fh*RADInv ) / cosf((60 - fh)*RADInv) );
				fr = fi - (fs + fb);
				pd[0] = fb < 0 ? 0 : fb > 255 ? 255 : (unsigned char)fb;
				pd[1] = fs;
				pd[2] = fr < 0 ? 0 : fr > 255 ? 255 : (unsigned char)fr;

			}
			pd += 3;
		}
	}

	return 0;
}

//test passed
int zipl_hsip_rgb_fast( int w, int h,
	                    float* phsi,
	                    int scan_rgb, unsigned char* prgb
	                   )
{
	int i, j;
	unsigned char* pd;
	float* phh;
	int* pss, *pii;
	int r, g, b;
	float fh;
	int fs, fi;
	int ind;

	if( w <= 0 || h <= 0 || scan_rgb <= 0 )
		return dwIplColorParamError;

	if( prgb == 0 || phsi == 0 )
		return dwIplColorPointerNull;

	phh = phsi;
	pss = (int*)(phh + w*h);
	pii = pss + w*h;

	for( i = 0; i < h; ++i )
	{
		pd = prgb + (h-1-i)*scan_rgb;
		for( j = 0; j < w; ++j )
		{
			fh = *phh++;
			fs = *pss++;
			fi = *pii++;

			if( fh >= 0 && fh < 120 )
			{
				ind = fh * 50 + 0.5f;
				r = ( (fi<<12) + (fi - 3*fs)*cosfTable[ind] + 6144) / 12288;
				g = fi - (r + fs); 

				pd[0] = fs;
				pd[1] = g < 0 ? 0 : g > 255 ? 255 : g;
				pd[2] = r < 0 ? 0 : r > 255 ? 255 : r;
			}
			else if( fh >= 120 && fh < 240 )
			{
				fh -= 120;
				ind = fh * 50 + 0.5f;
				g = ( (fi<<12) + (fi - 3*fs)*cosfTable[ind] + 6144 ) / 12288;
				b = fi - (fs + g);
				pd[0] = b < 0 ? 0 : b > 255 ? 255 : b;
				pd[1] = g < 0 ? 0 : g > 255 ? 255 : g;
				pd[2] = fs;
			}
			else
			{
				fh -= 240;
				ind = fh * 50 + 0.5f;
				b = ( (fi<<12) + (fi - 3*fs)*cosfTable[ind] + 6144 ) / 12288;
				r = fi - (fs + b);
				pd[0] = b < 0 ? 0 : b > 255 ? 255 : b;
				pd[1] = fs;
				pd[2] = r < 0 ? 0 : r > 255 ? 255 : r;
			}
			pd += 3;
		}
	}

	return 0;
}


//////////////////////////////////////////////////////////////////////////
//test passed
int zipl_yuv420p_hsip( int w, int h,
	                   unsigned char* pyuv,
	                   float* phsi
	                  )
{
	int i, j;
	unsigned char* py0;
	unsigned char* py1;
	unsigned char* pu;
	unsigned char* pv;
	float* phh0, *phh1;
	int* pss0, *pss1;
	int* pii0, *pii1;
	int hh = h>>1;
	int ww = w>>1;
	int r, g, b;
	int imin;
	int sum;

	if( w < 0 || h < 0 )
		return dwIplColorParamError;
	if( pyuv == 0 || phsi == 0 )
		return dwIplColorPointerNull;

	py0 = pyuv;
	py1 = py0 + w;
	pu  = py0 + w*h;
	pv  = pu + w*h/4;

	phh0 = phsi;
	phh1 = phh0 + w;
	pss0 = (int*)(phh0 + w*h);
	pss1 = pss0 + w;
	pii0 = pss0 + w*h;
	pii1 = pii0 + w;
	

	for( i = 0; i < hh; ++i )
	{
		for( j = 0; j < ww; ++j )
		{
			int iy = py0[0] - 16;
			int iu = *pu++ - 128;
			int iv = *pv++ - 128;

			int itpy = 1192 * iy;
			int itp0 = 1634 * iv;
			int itp1 = 401 * iu + 832 * iv;
			int itp2 = 2066 * iu;

			r  = (itpy + itp0 + 512)>>10;
			g  = (itpy - itp1 + 512)>>10;
			b  = (itpy + itp2 + 512)>>10;
			r  = r < 0 ? 0 : r > 255 ? 255 : r;
			g  = g < 0 ? 0 : g > 255 ? 255 : g;
			b  = b < 0 ? 0 : b > 255 ? 255 : b;

			//rgb转hsi
			if( r == g && g == b )
			{//h
				*phh0++ = 0;
			}
			else
			{//h
				int ind;
				int it = ((r-g) + (r-b))/*<<10*/;
				int is = (r-g)*(r-g) + (r-b)*(g-b);
// 				if( it < 0 )
// 					ind = ((it * sqrtfTable[is] - 4096)>>13) + 2048;
// 				else
// 					ind = ((it * sqrtfTable[is] + 4096)>>13) + 2048;
				ind = ((it * sqrtfTable[is] + 4096)>>13) + 2048;
				if( b <= g )
					*phh0++ = hsiTable_f[ind];
				else
					*phh0++ = 360 - hsiTable_f[ind];
			}

			imin = r < g ? r : g;
			imin = imin < b ? imin : b;

			sum = r + g + b;

			*pss0++ = imin;
			*pii0++ = sum;

			//////////////////////////////////////////////////////////////////////////
			iy   = py0[1] - 16;
			itpy = 1192 * iy;
			r    = (itpy + itp0 + 512)>>10;
			g    = (itpy - itp1 + 512)>>10;
			b    = (itpy + itp2 + 512)>>10;
			r    = r < 0 ? 0 : r > 255 ? 255 : r;
			g    = g < 0 ? 0 : g > 255 ? 255 : g;
			b    = b < 0 ? 0 : b > 255 ? 255 : b;
			
			//rgb转hsi
			if( r == g && g == b )
			{//h
				*phh0++ = 0;
			}
			else
			{//h
				int ind;
				int it = ((r-g) + (r-b))/*<<10*/;
				int is = (r-g)*(r-g) + (r-b)*(g-b);
// 				if( it < 0 )
// 					ind = ((it * sqrtfTable[is] - 4096)>>13) + 2048;
// 				else
// 					ind = ((it * sqrtfTable[is] + 4096)>>13) + 2048;
				ind = ((it * sqrtfTable[is] + 4096)>>13) + 2048;
				if( b <= g )
					*phh0++ = hsiTable_f[ind];
				else
					*phh0++ = 360 - hsiTable_f[ind];
			}

			imin = r < g ? r : g;
			imin = imin < b ? imin : b;

			sum = r + g + b;

			*pss0++ = imin;
			*pii0++ = sum;

			//next line
			iy   = py1[0] - 16;
			itpy = 1192 * iy;
			
			r  = (itpy + itp0 + 512)>>10;
			g  = (itpy - itp1 + 512)>>10;
			b  = (itpy + itp2 + 512)>>10;
			r  = r < 0 ? 0 : r > 255 ? 255 : r;
			g  = g < 0 ? 0 : g > 255 ? 255 : g;
			b  = b < 0 ? 0 : b > 255 ? 255 : b;

			//rgb转hsi
			if( r == g && g == b )
			{//h
				*phh1++ = 0;
			}
			else
			{//h
				int ind;
				int it = ((r-g) + (r-b))/*<<10*/;
				int is = (r-g)*(r-g) + (r-b)*(g-b);
// 				if( it < 0 )
// 					ind = ((it * sqrtfTable[is] - 4096)>>13) + 2048;
// 				else
// 					ind = ((it * sqrtfTable[is] + 4096)>>13) + 2048;
				ind = ((it * sqrtfTable[is] + 4096)>>13) + 2048;
				if( b <= g )
					*phh1++ = hsiTable_f[ind];
				else
					*phh1++ = 360 - hsiTable_f[ind];
			}

			imin = r < g ? r : g;
			imin = imin < b ? imin : b;

			sum = r + g + b;

			*pss1++ = imin;
			*pii1++ = sum;

			//////////////////////////////////////////////////////////////////////////
			iy   = py1[1] - 16;
			itpy = 1192 * iy;
			r    = (itpy + itp0 + 512)>>10;
			g    = (itpy - itp1 + 512)>>10;
			b    = (itpy + itp2 + 512)>>10;
			r    = r < 0 ? 0 : r > 255 ? 255 : r;
			g    = g < 0 ? 0 : g > 255 ? 255 : g;
			b    = b < 0 ? 0 : b > 255 ? 255 : b;

			//rgb转hsi
			if( r == g && g == b )
			{//h
				*phh1++ = 0;
			}
			else
			{//h
				int ind;
				int it = ((r-g) + (r-b))/*<<10*/;
				int is = (r-g)*(r-g) + (r-b)*(g-b);
// 				if( it < 0 )
// 					ind = ((it * sqrtfTable[is] - 4096)>>13) + 2048;
// 				else
// 					ind = ((it * sqrtfTable[is] + 4096)>>13) + 2048;

				ind = ((it * sqrtfTable[is] + 4096)>>13) + 2048;
				if( b <= g )
					*phh1++ = hsiTable_f[ind];
				else
					*phh1++ = 360 - hsiTable_f[ind];
			}

			imin = r < g ? r : g;
			imin = imin < b ? imin : b;

			sum = r + g + b;

			*pss1++ = imin;
			*pii1++ = sum;

			py0 += 2;
			py1 += 2;
		}
		phh0 += w;
		phh1 += w;
		pss0 += w;
		pss1 += w;
		pii0 += w;
		pii1 += w;
		py0 += w;
		py1 += w;
	}

	return 0;
}

//test passed
int zipl_yuv420_hsip( int w, int h,
	                  int scany, unsigned char* py,
	                  int scanu, unsigned char* pu,
	                  int scanv, unsigned char* pv,
	                  float* phsi
	                 )
{
	int i, j;
	unsigned char* py0;
	unsigned char* py1;
	float* phh0, *phh1;
	int* pss0, *pss1;
	int* pii0, *pii1;
	int hh = h>>1;
	int ww = w>>1;
	int r, g, b;
	int imin;
	int sum;

	if( w < 0 || h < 0 || scany < 0 || scanu < 0 || scanv < 0 )
		return dwIplColorParamError;
	if( py == 0 || pu == 0 || pv == 0 || phsi == 0 )
		return dwIplColorPointerNull;
	
	phh0 = phsi;
	phh1 = phh0 + w;
	pss0 = (int*)(phh0 + w*h);
	pss1 = pss0 + w;
	pii0 = pss0 + w*h;
	pii1 = pii0 + w;

	//
	for( i = 0; i < hh; ++i )
	{
		py0 = py + 2*i*scany;
		py1 = py0 + scany;
		for( j = 0; j < ww; ++j )
		{
			int iy = py0[0] - 16;
			int iu = pu[j] - 128;
			int iv = pv[j] - 128;

			int itpy = 1192 * iy;
			int itp0 = 1634 * iv;
			int itp1 = 401 * iu + 832 * iv;
			int itp2 = 2066 * iu;

			r  = (itpy + itp0 + 512)>>10;
			g  = (itpy - itp1 + 512)>>10;
			b  = (itpy + itp2 + 512)>>10;
			r  = r < 0 ? 0 : r > 255 ? 255 : r;
			g  = g < 0 ? 0 : g > 255 ? 255 : g;
			b  = b < 0 ? 0 : b > 255 ? 255 : b;

			//rgb转hsi
			if( r == g && g == b )
			{//h
				*phh0++ = 0;
			}
			else
			{//h
				int ind;
				int it = ((r-g) + (r-b));
				int is = (r-g)*(r-g) + (r-b)*(g-b);
				//if( it < 0 )
				//	ind = ((it * sqrtfTable[is] - 4096)>>13) + 2048;
				//else
				//	ind = ((it * sqrtfTable[is] + 4096)>>13) + 2048;
				ind = ((it * sqrtfTable[is] + 4096)>>13) + 2048;
				if( b <= g )
					*phh0++ = hsiTable_f[ind];
				else
					*phh0++ = 360 - hsiTable_f[ind];
			}

			imin = r < g ? r : g;
			imin = imin < b ? imin : b;

			sum = r + g + b;

			*pss0++ = imin;
			*pii0++ = sum;

			//////////////////////////////////////////////////////////////////////////
			iy   = py0[1] - 16;
			itpy = 1192 * iy;
			r    = (itpy + itp0 + 512)>>10;
			g    = (itpy - itp1 + 512)>>10;
			b    = (itpy + itp2 + 512)>>10;
			r    = r < 0 ? 0 : r > 255 ? 255 : r;
			g    = g < 0 ? 0 : g > 255 ? 255 : g;
			b    = b < 0 ? 0 : b > 255 ? 255 : b;

			//rgb转hsi
			if( r == g && g == b )
			{//h
				*phh0++ = 0;
			}
			else
			{//h
				int ind;
				int it = (r-g) + (r-b);
				int is = (r-g)*(r-g) + (r-b)*(g-b);
				//if( it < 0 )
					//ind = ((it * sqrtfTable[is] - 4096)>>13) + 2048;
				//else
					//ind = ((it * sqrtfTable[is] + 4096)>>13) + 2048;
				ind = ((it * sqrtfTable[is] + 4096)>>13) + 2048;
				if( b <= g )
					*phh0++ = hsiTable_f[ind];
				else
					*phh0++ = 360 - hsiTable_f[ind];
			}

			imin = r < g ? r : g;
			imin = imin < b ? imin : b;

			sum = r + g + b;

			*pss0++ = imin;
			*pii0++ = sum;

			//next line
			iy   = py1[0] - 16;
			itpy = 1192 * iy;

			r  = (itpy + itp0 + 512)>>10;
			g  = (itpy - itp1 + 512)>>10;
			b  = (itpy + itp2 + 512)>>10;
			r  = r < 0 ? 0 : r > 255 ? 255 : r;
			g  = g < 0 ? 0 : g > 255 ? 255 : g;
			b  = b < 0 ? 0 : b > 255 ? 255 : b;

			//rgb转hsi
			if( r == g && g == b )
			{//h
				*phh1++ = 0;
			}
			else
			{//h
				int ind;
				int it = ((r-g) + (r-b));
				int is = (r-g)*(r-g) + (r-b)*(g-b);
// 				if( it < 0 )
// 					ind = ((it * sqrtfTable[is] - 4096)>>13) + 2048;
// 				else
// 					ind = ((it * sqrtfTable[is] + 4096)>>13) + 2048;
				ind = ((it * sqrtfTable[is] + 4096)>>13) + 2048;
				if( b <= g )
					*phh1++ = hsiTable_f[ind];
				else
					*phh1++ = 360 - hsiTable_f[ind];
			}

			imin = r < g ? r : g;
			imin = imin < b ? imin : b;

			sum = r + g + b;

			*pss1++ = imin;
			*pii1++ = sum;

			//////////////////////////////////////////////////////////////////////////
			iy   = py1[1] - 16;
			itpy = 1192 * iy;
			r    = (itpy + itp0 + 512)>>10;
			g    = (itpy - itp1 + 512)>>10;
			b    = (itpy + itp2 + 512)>>10;
			r    = r < 0 ? 0 : r > 255 ? 255 : r;
			g    = g < 0 ? 0 : g > 255 ? 255 : g;
			b    = b < 0 ? 0 : b > 255 ? 255 : b;

			//rgb转hsi
			if( r == g && g == b )
			{//h
				*phh1++ = 0;
			}
			else
			{//h
				int ind;
				int it = ((r-g) + (r-b));
				int is = (r-g)*(r-g) + (r-b)*(g-b);
// 				if( it < 0 )
// 					ind = ((it * sqrtfTable[is] - 4096)>>13) + 2048;
// 				else
// 					ind = ((it * sqrtfTable[is] + 4096)>>13) + 2048;
				ind = ((it * sqrtfTable[is] + 4096)>>13) + 2048;
				if( b <= g )
					*phh1++ = hsiTable_f[ind];
				else
					*phh1++ = 360 - hsiTable_f[ind];
			}

			imin = r < g ? r : g;
			imin = imin < b ? imin : b;

			sum = r + g + b;

			*pss1++ = imin;
			*pii1++ = sum;

			py0 += 2;
			py1 += 2;
		}
		phh0 += w;
		phh1 += w;
		pss0 += w;
		pss1 += w;
		pii0 += w;
		pii1 += w;
		pu += scanu;
		pv += scanv;
	}

	return 0;
}

//test passed 
int zipl_hsip_yuv420p( int w, int h,
	                   float* phsi,
	                   unsigned char* pyuv
	                  )
{
	int i, j;
	int hh = h>>1;
	int ww = w>>1;
	float* phh0, *phh1;
	int* pss0, *pss1;
	int* pii0, *pii1;
	int r, g, b;
	int ind;
	float fh;
	int fs, fi;
	int y, u, v;
	unsigned char* py0, *py1;
	unsigned char* pu, *pv;

	if( w < 0 || h < 0 )
		return dwIplColorParamError;
	if( phsi == 0 || pyuv == 0 )
		return dwIplColorPointerNull;

	phh0 = phsi;
	phh1 = phh0 + w;
	pss0 = (int*)(phh0 + w*h);
	pss1 = pss0 + w;
	pii0 = pss0 + w*h;
	pii1 = pii0 + w;

	py0 = pyuv;
	py1 = py0 + w;
	pu  = py0 + w*h;
	pv  = pu + ww*hh;
	
	for( i = 0; i < hh; ++i )
	{
		for( j = 0; j < ww; ++j )
		{
			fh = phh0[0];
			fs = pss0[0];
			fi = pii0[0];

			//y0 u0 v0
			if( fh >= 0 && fh < 120 )
			{
				ind = fh * 50 + 0.5f;
				b = fs;
				r = ( (fi<<12) + (fi - 3*fs)*cosfTable[ind] + 6144) / 12288;
				g = fi - (r + fs); 
			}
			else if( fh >= 120 && fh < 240 )
			{
				fh -= 120;
				ind = fh * 50 + 0.5f;
				r = fs;
				g = ( (fi<<12) + (fi - 3*fs)*cosfTable[ind] + 6144 ) / 12288;
				b = fi - (fs + g);
			}
			else
			{
				fh -= 240;
				ind = fh * 50 + 0.5f;
				g = fs;
				b = ( (fi<<12) + (fi - 3*fs)*cosfTable[ind] + 6144 ) / 12288;
				r = fi - (fs + b);
			}

			y      = ((263 * r + 516 * g + 100 * b + 512)>>10) + 16;
			py0[0] = y < 16 ? 16 : y > 235 ? 235 : y;

			u      = ((-152 * r - 298 * g + 450 * b + 512)>>10) + 128;
			*pu++  = u < 16 ? 16 : u > 240 ? 240 : u;

			v      = ((450 * r - 377 * g - 73 * b + 512)>>10) + 128;
			*pv++  = v < 16 ? 16 : v > 240 ? 240 : v;

			//////////////////////////////////////////////////////////////////////////
			fh = phh0[1];
			fs = pss0[1];
			fi = pii0[1];

			//y0 u0 v0
			if( fh >= 0 && fh < 120 )
			{
				ind = fh * 50 + 0.5f;
				b = fs;
				r = ( (fi<<12) + (fi - 3*fs)*cosfTable[ind] + 6144) / 12288;
				g = fi - (r + fs); 
			}
			else if( fh >= 120 && fh < 240 )
			{
				fh -= 120;
				ind = fh * 50 + 0.5f;
				r = fs;
				g = ( (fi<<12) + (fi - 3*fs)*cosfTable[ind] + 6144 ) / 12288;
				b = fi - (fs + g);
			}
			else
			{
				fh -= 240;
				ind = fh * 50 + 0.5f;
				g = fs;
				b = ( (fi<<12) + (fi - 3*fs)*cosfTable[ind] + 6144 ) / 12288;
				r = fi - (fs + b);
			}
			y      = ((263 * r + 516 * g + 100 * b + 512)>>10) + 16;
			py0[1] = y < 16 ? 16 : y > 235 ? 235 : y;

			py0  += 2;
			phh0 += 2;
			pss0 += 2;
			pii0 += 2;

			////////////////////////////next line///////////////////////////////////////////
			fh = phh1[0];
			fs = pss1[0];
			fi = pii1[0];

			//y1
			if( fh >= 0 && fh < 120 )
			{
				ind = fh * 50 + 0.5f;
				b = fs;
				r = ( (fi<<12) + (fi - 3*fs)*cosfTable[ind] + 6144) / 12288;
				g = fi - (r + fs); 
			}
			else if( fh >= 120 && fh < 240 )
			{
				fh -= 120;
				ind = fh * 50 + 0.5f;
				r = fs;
				g = ( (fi<<12) + (fi - 3*fs)*cosfTable[ind] + 6144 ) / 12288;
				b = fi - (fs + g);
			}
			else
			{
				fh -= 240;
				ind = fh * 50 + 0.5f;
				g = fs;
				b = ( (fi<<12) + (fi - 3*fs)*cosfTable[ind] + 6144 ) / 12288;
				r = fi - (fs + b);
			}
			y      = ((263 * r + 516 * g + 100 * b + 512)>>10) + 16;
			py1[0] = y < 16 ? 16 : y > 235 ? 235 : y;
			//////////////////////////////////////////////////////////////////////////

			fh = phh1[1];
			fs = pss1[1];
			fi = pii1[1];

			//y1
			if( fh >= 0 && fh < 120 )
			{
				ind = fh * 50 + 0.5f;
				b = fs;
				r = ( (fi<<12) + (fi - 3*fs)*cosfTable[ind] + 6144) / 12288;
				g = fi - (r + fs); 
			}
			else if( fh >= 120 && fh < 240 )
			{
				fh -= 120;
				ind = fh * 50 + 0.5f;
				r = fs;
				g = ( (fi<<12) + (fi - 3*fs)*cosfTable[ind] + 6144 ) / 12288;
				b = fi - (fs + g);
			}
			else
			{
				fh -= 240;
				ind = fh * 50 + 0.5f;
				g = fs;
				b = ( (fi<<12) + (fi - 3*fs)*cosfTable[ind] + 6144 ) / 12288;
				r = fi - (fs + b);
			}
			y      = ((263 * r + 516 * g + 100 * b + 512)>>10) + 16;
			py1[1] = y < 16 ? 16 : y > 235 ? 235 : y;

			py1  += 2;
			phh1 += 2;
			pss1 += 2;
			pii1 += 2;
		}
		phh0 += w;
		phh1 += w;
		pss0 += w;
		pss1 += w;
		pii0 += w;
		pii1 += w;
		py0 += w;
		py1 += w;
	}

	return 0;
}

int zipl_hsip_yuv420( int w, int h,
	                  float* phsi,
	                  int scany, unsigned char* py,
	                  int scanu, unsigned char* pu,
	                  int scanv, unsigned char* pv
	                 )
{
	int i, j;
	int hh = h>>1;
	int ww = w>>1;
	float* phh0, *phh1;
	int* pss0, *pss1;
	int* pii0, *pii1;
	int r, g, b;
	int ind;
	float fh;
	int fs, fi;
	int y, u, v;
	unsigned char* py0, *py1;

	if( w < 0 || h < 0 || scany < 0 || scanu < 0 || scanv < 0 )
		return dwIplColorParamError;
	if( phsi == 0 || pu == 0 || pv ==0 )
		return dwIplColorPointerNull;

	phh0 = phsi;
	phh1 = phh0 + w;
	pss0 = (int*)(phh0 + w*h);
	pss1 = pss0 + w;
	pii0 = pss0 + w*h;
	pii1 = pii0 + w;

	for( i = 0; i < hh; ++i )
	{
		py0 = py + 2*i*scany;
		py1 = py0 + scany;
		for( j = 0; j < ww; ++j )
		{
			fh = phh0[0];
			fs = pss0[0];
			fi = pii0[0];

			//y0 u0 v0
			if( fh >= 0 && fh < 120 )
			{
				ind = fh * 50 + 0.5f;
				b = fs;
				r = ( (fi<<12) + (fi - 3*fs)*cosfTable[ind] + 6144) / 12288;
				g = fi - (r + fs); 
			}
			else if( fh >= 120 && fh < 240 )
			{
				fh -= 120;
				ind = fh * 50 + 0.5f;
				r = fs;
				g = ( (fi<<12) + (fi - 3*fs)*cosfTable[ind] + 6144 ) / 12288;
				b = fi - (fs + g);
			}
			else
			{
				fh -= 240;
				ind = fh * 50 + 0.5f;
				g = fs;
				b = ( (fi<<12) + (fi - 3*fs)*cosfTable[ind] + 6144 ) / 12288;
				r = fi - (fs + b);
			}

			y      = ((263 * r + 516 * g + 100 * b + 512)>>10) + 16;
			py0[0] = y < 16 ? 16 : y > 235 ? 235 : y;

			u      = ((-152 * r - 298 * g + 450 * b + 512)>>10) + 128;
			pu[j]  = u < 16 ? 16 : u > 240 ? 240 : u;

			v      = ((450 * r - 377 * g - 73 * b + 512)>>10) + 128;
			pv[j]  = v < 16 ? 16 : v > 240 ? 240 : v;

			//////////////////////////////////////////////////////////////////////////
			fh = phh0[1];
			fs = pss0[1];
			fi = pii0[1];

			//y0 u0 v0
			if( fh >= 0 && fh < 120 )
			{
				ind = fh * 50 + 0.5f;
				b = fs;
				r = ( (fi<<12) + (fi - 3*fs)*cosfTable[ind] + 6144) / 12288;
				g = fi - (r + fs); 
			}
			else if( fh >= 120 && fh < 240 )
			{
				fh -= 120;
				ind = fh * 50 + 0.5f;
				r = fs;
				g = ( (fi<<12) + (fi - 3*fs)*cosfTable[ind] + 6144 ) / 12288;
				b = fi - (fs + g);
			}
			else
			{
				fh -= 240;
				ind = fh * 50 + 0.5f;
				g = fs;
				b = ( (fi<<12) + (fi - 3*fs)*cosfTable[ind] + 6144 ) / 12288;
				r = fi - (fs + b);
			}
			y      = ((263 * r + 516 * g + 100 * b + 512)>>10) + 16;
			py0[1] = y < 16 ? 16 : y > 235 ? 235 : y;

			py0  += 2;
			phh0 += 2;
			pss0 += 2;
			pii0 += 2;

			////////////////////////////next line///////////////////////////////////////////
			fh = phh1[0];
			fs = pss1[0];
			fi = pii1[0];

			//y1
			if( fh >= 0 && fh < 120 )
			{
				ind = fh * 50 + 0.5f;
				b = fs;
				r = ( (fi<<12) + (fi - 3*fs)*cosfTable[ind] + 6144) / 12288;
				g = fi - (r + fs); 
			}
			else if( fh >= 120 && fh < 240 )
			{
				fh -= 120;
				ind = fh * 50 + 0.5f;
				r = fs;
				g = ( (fi<<12) + (fi - 3*fs)*cosfTable[ind] + 6144 ) / 12288;
				b = fi - (fs + g);
			}
			else
			{
				fh -= 240;
				ind = fh * 50 + 0.5f;
				g = fs;
				b = ( (fi<<12) + (fi - 3*fs)*cosfTable[ind] + 6144 ) / 12288;
				r = fi - (fs + b);
			}
			y      = ((263 * r + 516 * g + 100 * b + 512)>>10) + 16;
			py1[0] = y < 16 ? 16 : y > 235 ? 235 : y;
			//////////////////////////////////////////////////////////////////////////

			fh = phh1[1];
			fs = pss1[1];
			fi = pii1[1];

			//y1
			if( fh >= 0 && fh < 120 )
			{
				ind = fh * 50 + 0.5f;
				b = fs;
				r = ( (fi<<12) + (fi - 3*fs)*cosfTable[ind] + 6144) / 12288;
				g = fi - (r + fs); 
			}
			else if( fh >= 120 && fh < 240 )
			{
				fh -= 120;
				ind = fh * 50 + 0.5f;
				r = fs;
				g = ( (fi<<12) + (fi - 3*fs)*cosfTable[ind] + 6144 ) / 12288;
				b = fi - (fs + g);
			}
			else
			{
				fh -= 240;
				ind = fh * 50 + 0.5f;
				g = fs;
				b = ( (fi<<12) + (fi - 3*fs)*cosfTable[ind] + 6144 ) / 12288;
				r = fi - (fs + b);
			}
			y      = ((263 * r + 516 * g + 100 * b + 512)>>10) + 16;
			py1[1] = y < 16 ? 16 : y > 235 ? 235 : y;

			py1  += 2;
			phh1 += 2;
			pss1 += 2;
			pii1 += 2;
		}
		phh0 += w;
		phh1 += w;
		pss0 += w;
		pss1 += w;
		pii0 += w;
		pii1 += w;
		pu += scanu;
		pv += scanv;
	}

	return 0;
}

int get_skin_mask_rgb(int w, int h, int scanrgb, unsigned char* prgb, int scanmask, unsigned char* mask)
{
	int i;
	int j;
	for (i = 0; i < h; ++i)
	{
		unsigned char* ps = prgb + i*scanrgb;
		unsigned char* pm = mask + i*scanmask;
		for (j = 0; j < w; ++j)
		{
			int b = ps[0];
			int g = ps[1];
			int r = ps[2];
			if (r > 95 && g > 40 && b > 20 && r > g && r > b)
			{
				int mx = ZMAX(r, g);
				mx = ZMAX(mx, b);
				int mn = ZMIN(r, g);
				mn = ZMIN(mn, b);
				if (mx - mn > 15 && abs(r - g) > 15)
					pm[j] = 255;
				else
					pm[j] = 0;
			}
			else
				pm[j] = 0;

			ps += 3;
		
		}
	}

	return 0;
}

int zipl_fDilation(int ncx, int ncy, int nSrcDstPitch, unsigned char* pSrcDst, int nRow, int nCol)
{
	//边缘外围补0
	assert(nRow > 0 && nRow < ncy);
	assert(nCol > 0 && nCol < ncx);
	//
	int nWindowY = nRow * 2 + 1;
	int nWindowX = nCol * 2 + 1;
	int xx = ncx + 2 * nCol;
	int yy = ncy + 2 * nRow;
	int nxless = nWindowX - 1;
	int nyless = nWindowY - 1;

	//CDYRIPLMemoryAuto MemAuto(xx * sizeof(DY_LPUINT8) + (nWindowX + xx + (yy + nWindowY)*xx + xx));
	void* MemAuto = malloc(xx * sizeof(unsigned char*) + (nWindowX + xx + (yy + nWindowY)*xx + xx));
	unsigned char** ppAllScrollWinYs = (unsigned char**)MemAuto;
	unsigned char* pScrollWinX = ((unsigned char*)MemAuto + xx * sizeof(unsigned char*));
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

	free(MemAuto);

	return 0;
}