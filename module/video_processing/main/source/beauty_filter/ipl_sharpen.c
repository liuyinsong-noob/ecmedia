#include "ipl_sharpen.h"
#include "ipl_color.h"
#include "ipl_define.h"
#ifdef _WIN32
#include <malloc.h>
#endif
#ifdef __APPLE__
#include <stdlib.h>
#endif
#include <string.h>
#include <memory.h>

int zipl_sharpen_get_outbufsize(int w, int h)
{
	int size;
	if( w < 0 || h < 0 )
		return 0;

	size = w*h*3*sizeof(float) + w*h*sizeof(int);

	return size;
}

int zipl_sharpen_yuv420p( int w, int h,
	                      unsigned char* psrc, 
	                      unsigned char* pdst,
						  unsigned char* poutbuf
	                     )
{
	int i, j;
	float* phsi;
	float* phs;
	int* pss;
	int* pis;
	int* pid;
	int lessx = w - 1;
	int lessy = h - 1;
	int* pis_c;
	int* pis_u;
	int* pis_d;
	int* pid_d;
	int ind;
	float r, g, b;
	int ir, ig, ib;
	float fh;
	int is, ii, di;
	unsigned char* py;
	unsigned char* pu;
	unsigned char* pv;
	int y, u, v;
	float ftp;

	if( w < 0 || h < 0 )
		return dwIplColorParamError;

	if( psrc == 0 || pdst == 0 )
		return dwIplColorPointerNull;

	py = pdst;
	pu = py + w*h;
	pv = pu + w*h/4;

	if( poutbuf == 0 )
	{
		phsi = (float*)malloc(w*h*3*sizeof(float) + w*h*sizeof(int));
		phs = phsi;
		pss = (int*)(phsi + w*h);
		pis = pss + w*h;
		pid = pis + w*h;
		memset(pid, 0, w*h*sizeof(int));
	}
	else
	{
		phsi = (float*)poutbuf;
		phs = phsi;
		pss = (int*)(phsi + w*h);
		pis = pss + w*h;
		pid = pis + w*h;
		memset(pid, 0, w*h*sizeof(int));
	}
	
	//do yuv420p_hsip conversion
	if( zipl_yuv420p_hsip(w, h, psrc, phsi) != 0 )
	{
		if( poutbuf == 0) free(phsi);
		return dwIplyuv420p2hsipError;
	}

	//do sharpen for intensity
	for( i = 1; i < lessy; ++i )
	{
		pis_c = pis + i * w;
		pis_u = pis_c - w;
		pis_d = pis_c + w;
		pid_d = pid + i * w;
		for( j = 1; j < lessx; ++j )
		{
			pid_d[j] = pis_u[0] + pis_u[1] + pis_u[2] 
			           + pis_d[0] + pis_d[1] + pis_d[2]
					   + pis_c[0] - 8*pis_c[1] + pis_c[2];

			pis_u++;
			pis_c++;
			pis_d++;
		}
	}

	//do hsip_yuvp420 conversion
	for( i = 0; i < h; ++i )
	{
		for( j = 0; j < w; ++j )
		{
			fh = *phs++;
			is = *pss++;
			ii = *pis++;
			di = *pid++;

			ftp = (ii - 0.1f*di);
			ftp = ftp < 0 ? 0 : ftp > 675 ? 675 : ftp;
			
			if( fh >= 0 && fh < 120 )
			{
				ind = fh * 50 + 0.5f;
				b = ftp * is/ii;
				r = ftp * (0.333f + (0.333f - is/(float)ii)*cosfTable_f[ind]);
				g = ftp - (r + b); 
			}
			else if( fh >= 120 && fh < 240 )
			{
				fh -= 120;
				ind = fh * 50 + 0.5f;
				r = ftp * is/ii;
				g = ftp * (0.333f + (0.333f - is/(float)ii)*cosfTable_f[ind]);
				b = ftp - (r + g);
			}
			else
			{
				fh -= 240;
				ind = fh * 50 + 0.5f;
				g = ftp * is/ii;
				b = ftp * (0.333f + (0.333f - is/(float)ii)*cosfTable_f[ind]);
				r = ftp - (g + b);
			}
			
			ib = b;// < 0 ? 0 : b > 255 ? 255 : b;
			ig = g;// < 0 ? 0 : g > 255 ? 255 : g;
			ir = r;// < 0 ? 0 : r > 255 ? 255 : r;

			y     = ((263 * ir + 516 * ig + 100 * ib + 512)>>10) + 16;
			*py++ = y < 16 ? 16 : y > 235 ? 235 : y;
			if( i%2 == 0 && j%2 == 0 )
			{
				u     = ((-152 * ir - 298 * ig + 450 * ib + 512)>>10) + 128;
				*pu++ = u < 16 ? 16 : u > 240 ? 240 : u;

				v     = ((450 * ir - 377 * ig - 73 * ib + 512)>>10) + 128;
				*pv++ = v < 16 ? 16 : v > 240 ? 240 : v;
			}
		}
	}

	if( poutbuf == 0 )
		free(phsi);

	return 0;
}


int zipl_sharpen_yuv420p_r( int w, int h, unsigned char* psrcdst, unsigned char* poutbuf )
{
	int i, j;
	float* phsi;
	float* phs;
	int* pss;
	int* pis;
	int* pid;
	int lessx = w - 1;
	int lessy = h - 1;
	int* pis_c;
	int* pis_u;
	int* pis_d;
	int* pid_d;
	int ind;
	float r, g, b;
	int ir, ig, ib;
	float fh;
	int is, ii, di;
	unsigned char* py;
	unsigned char* pu;
	unsigned char* pv;
	int y, u, v;
	float ftp;

	if( w < 0 || h < 0 )
		return dwIplColorParamError;

	if( psrcdst == 0 )
		return dwIplColorPointerNull;

	py = psrcdst;
	pu = py + w*h;
	pv = pu + w*h/4;

	if( poutbuf == 0 )
	{
		phsi = (float*)malloc(w*h*3*sizeof(float) + w*h*sizeof(int));
		phs = phsi;
		pss = (int*)(phsi + w*h);
		pis = pss + w*h;
		pid = pis + w*h;
		memset(pid, 0, w*h*sizeof(int));
	}
	else
	{
		phsi = (float*)poutbuf;
		phs  = phsi;
		pss = (int*)(phsi + w*h);
		pis = pss + w*h;
		pid = pis + w*h;
		memset(pid, 0, w*h*sizeof(int));
	}
	

	//do yuv420p_hsip conversion
	if( zipl_yuv420p_hsip(w, h, psrcdst, phsi) != 0 )
	{
		if( poutbuf ==0 ) 
			free(phsi);

		return dwIplyuv420p2hsipError;
	}

	//do sharpen for intensity
	for( i = 1; i < lessy; ++i )
	{
		pis_c = pis + i * w;
		pis_u = pis_c - w;
		pis_d = pis_c + w;
		pid_d = pid + i * w;
		for( j = 1; j < lessx; ++j )
		{
			pid_d[j] = pis_u[0] + pis_u[1] + pis_u[2] 
			           + pis_d[0] + pis_d[1] + pis_d[2]
			           + pis_c[0] - 8*pis_c[1] + pis_c[2];

			pis_u++;
			pis_c++;
			pis_d++;
		}
	}

	//do hsip_yuvp420 conversion
	for( i = 0; i < h; ++i )
	{
		for( j = 0; j < w; ++j )
		{
			fh = *phs++;
			is = *pss++;
			ii = *pis++;
			di = *pid++;

			ftp = (ii - 0.1f*di);
			ftp = ftp < 0 ? 0 : ftp > 675 ? 675 : ftp;

			if( fh >= 0 && fh < 120 )
			{
				ind = fh * 50 + 0.5f;
				b = ftp * is/ii;
				r = ftp * (0.333f + (0.333f - is/(float)ii)*cosfTable_f[ind]);
				g = ftp - (r + b); 
			}
			else if( fh >= 120 && fh < 240 )
			{
				fh -= 120;
				ind = fh * 50 + 0.5f;
				r = ftp * is/ii;
				g = ftp * (0.333f + (0.333f - is/(float)ii)*cosfTable_f[ind]);
				b = ftp - (r + g);
			}
			else
			{
				fh -= 240;
				ind = fh * 50 + 0.5f;
				g = ftp * is/ii;
				b = ftp * (0.333f + (0.333f - is/(float)ii)*cosfTable_f[ind]);
				r = ftp - (g + b);
			}

			ib = b;// < 0 ? 0 : b > 255 ? 255 : b;
			ig = g;// < 0 ? 0 : g > 255 ? 255 : g;
			ir = r;// < 0 ? 0 : r > 255 ? 255 : r;

			y     = ((263 * ir + 516 * ig + 100 * ib + 512)>>10) + 16;
			*py++ = y < 16 ? 16 : y > 235 ? 235 : y;
			if( i%2 == 0 && j%2 == 0 )
			{
				u     = ((-152 * ir - 298 * ig + 450 * ib + 512)>>10) + 128;
				*pu++ = u < 16 ? 16 : u > 240 ? 240 : u;

				v     = ((450 * ir - 377 * ig - 73 * ib + 512)>>10) + 128;
				*pv++ = v < 16 ? 16 : v > 240 ? 240 : v;
			}
		}
	}

	if(poutbuf == 0 )
		free(phsi);

	return 0;
}


int zipl_sharpen_yuv420( int w, int h, 
	                     int scany_src, unsigned char* py_src, 
	                     int scanu_src, unsigned char* pu_src, 
	                     int scanv_src, unsigned char* pv_src,
	                     int scany_dst, unsigned char* py_dst, 
	                     int scanu_dst, unsigned char* pu_dst, 
	                     int scanv_dst, unsigned char* pv_dst,
	                     unsigned char* poutbuf
	                    )
{
	int i, j;
	float* phsi;
	float* phs;
	int* pss;
	int* pis;
	int* pid;
	int lessx = w - 1;
	int lessy = h - 1;
	int* pis_c;
	int* pis_u;
	int* pis_d;
	int* pid_d;
	int ind;
	float r, g, b;
	int ir, ig, ib;
	float fh;
	int is, ii, di;
	unsigned char* py;
	unsigned char* pu;
	unsigned char* pv;
	int y, u, v;
	float ftp;

	if( w < 0 || h < 0 || scany_src < 0 || scanu_src < 0 || scanv_src < 0 
		|| scany_dst < 0 || scanu_dst < 0 || scanv_dst < 0 )
		return dwIplColorParamError;

	if( py_src == 0 || pu_src == 0 || pv_src == 0 || py_dst == 0 || pu_dst == 0 || pv_dst == 0 )
		return dwIplColorPointerNull;


	if( poutbuf == 0 )
	{
		phsi = (float*)malloc(w*h*3*sizeof(float) + w*h*sizeof(int));
		phs = phsi;
		pss = (int*)(phsi + w*h);
		pis = pss + w*h;
		pid = pis + w*h;
		memset(pid, 0, w*h*sizeof(int));
	}
	else
	{
		phsi = (float*)poutbuf;
		phs  = phsi;
		pss = (int*)(phsi + w*h);
		pis = pss + w*h;
		pid = pis + w*h;
		memset(pid, 0, w*h*sizeof(int));
	}

	//do yuv420p_hsip conversion
	if( zipl_yuv420_hsip(w, h, scany_src, py_src, scanu_src, pu_src, scanv_src, pv_src, phsi) != 0 )
	{
		if( poutbuf ==0 ) 
			free(phsi);

		return dwIplyuv420_hsipError;
	}

	//do sharpen for intensity
	for( i = 1; i < lessy; ++i )
	{
		pis_c = pis + i * w;
		pis_u = pis_c - w;
		pis_d = pis_c + w;
		pid_d = pid + i * w;
		for( j = 1; j < lessx; ++j )
		{
			pid_d[j] = pis_u[0] + pis_u[1] + pis_u[2] 
			+ pis_d[0] + pis_d[1] + pis_d[2]
			+ pis_c[0] - 8*pis_c[1] + pis_c[2];

			pis_u++;
			pis_c++;
			pis_d++;
		}
	}
	//

	//do hsip_yuv420 conversion
	for( i = 0; i < h; ++i )
	{
		py = py_dst + i*scany_dst;
		pu = pu_dst + (i/2)*scanu_dst;
		pv = pv_dst + (i/2)*scanv_dst;
		for( j = 0; j < w; ++j )
		{
			fh = *phs++;
			is = *pss++;
			ii = *pis++;
			di = *pid++;

			ftp = (ii - 0.1f*di);
			ftp = ftp < 0 ? 0 : ftp > 675 ? 675 : ftp;

			if( fh >= 0 && fh < 120 )
			{
				ind = fh * 50 + 0.5f;
				b = ftp * is/ii;
				r = ftp * (0.333f + (0.333f - is/(float)ii)*cosfTable_f[ind]);
				g = ftp - (r + b); 
			}
			else if( fh >= 120 && fh < 240 )
			{
				fh -= 120;
				ind = fh * 50 + 0.5f;
				r = ftp * is/ii;
				g = ftp * (0.333f + (0.333f - is/(float)ii)*cosfTable_f[ind]);
				b = ftp - (r + g);
			}
			else
			{
				fh -= 240;
				ind = fh * 50 + 0.5f;
				g = ftp * is/ii;
				b = ftp * (0.333f + (0.333f - is/(float)ii)*cosfTable_f[ind]);
				r = ftp - (g + b);
			}

			ib = b;
			ig = g;
			ir = r;

			y     = ((263 * ir + 516 * ig + 100 * ib + 512)>>10) + 16;
			*py++ = y < 16 ? 16 : y > 235 ? 235 : y;
			if( i%2 == 0 && j%2 == 0 )
			{
				u     = ((-152 * ir - 298 * ig + 450 * ib + 512)>>10) + 128;
				*pu++ = u < 16 ? 16 : u > 240 ? 240 : u;

				v     = ((450 * ir - 377 * ig - 73 * ib + 512)>>10) + 128;
				*pv++ = v < 16 ? 16 : v > 240 ? 240 : v;
			}
		}
	}

	if(poutbuf == 0 )
		free(phsi);

	return 0;
}

int zipl_sharpen_yuv420_r( int w, int h, 
	                       int scany, unsigned char* py_srcdst, 
	                       int scanu, unsigned char* pu_srcdst, 
	                       int scanv, unsigned char* pv_srcdst,
	                       unsigned char* poutbuf
	                      )
{
	int i, j;
	float* phsi;
	float* phs;
	int* pss;
	int* pis;
	int* pid;
	int lessx = w - 1;
	int lessy = h - 1;
	int* pis_c;
	int* pis_u;
	int* pis_d;
	int* pid_d;
	int ind;
	float r, g, b;
	int ir, ig, ib;
	float fh;
	int is, ii, di;
	unsigned char* py;
	unsigned char* pu;
	unsigned char* pv;
	int y, u, v;
	float ftp;

	if( w < 0 || h < 0 || scany < 0 || scanu < 0 || scanv < 0 )
		return dwIplColorParamError;

	if( py_srcdst == 0 || pu_srcdst == 0 || pv_srcdst == 0 )
		return dwIplColorPointerNull;


	if( poutbuf == 0 )
	{
		phsi = (float*)malloc(w*h*3*sizeof(float) + w*h*sizeof(int));
		phs = phsi;
		pss = (int*)(phsi + w*h);
		pis = pss + w*h;
		pid = pis + w*h;
		memset(pid, 0, w*h*sizeof(int));
	}
	else
	{
		phsi = (float*)poutbuf;
		phs  = phsi;
		pss = (int*)(phsi + w*h);
		pis = pss + w*h;
		pid = pis + w*h;
		memset(pid, 0, w*h*sizeof(int));
	}

	//do yuv420p_hsip conversion
	if( zipl_yuv420_hsip(w, h, scany, py_srcdst, scanu, pu_srcdst, scanv, pv_srcdst, phsi) != 0 )
	{
		if( poutbuf ==0 ) 
			free(phsi);

		return dwIplyuv420_hsipError;
	}

	//do sharpen for intensity
	for( i = 1; i < lessy; ++i )
	{
		pis_c = pis + i * w;
		pis_u = pis_c - w;
		pis_d = pis_c + w;
		pid_d = pid + i * w;
		for( j = 1; j < lessx; ++j )
		{
			pid_d[j] = pis_u[0] + pis_u[1] + pis_u[2] 
			+ pis_d[0] + pis_d[1] + pis_d[2]
			+ pis_c[0] - 8*pis_c[1] + pis_c[2];

			pis_u++;
			pis_c++;
			pis_d++;
		}
	}
	//

	//do hsip_yuv420 conversion
	for( i = 0; i < h; ++i )
	{
		py = py_srcdst + i*scany;
		pu = pu_srcdst + (i/2)*scanu;
		pv = pv_srcdst + (i/2)*scanv;
		for( j = 0; j < w; ++j )
		{
			fh = *phs++;
			is = *pss++;
			ii = *pis++;
			di = *pid++;

			ftp = (ii - 0.1f*di);
			ftp = ftp < 0 ? 0 : ftp > 675 ? 675 : ftp;

			if( fh >= 0 && fh < 120 )
			{
				ind = fh * 50 + 0.5f;
				b = ftp * is/ii;
				r = ftp * (0.333f + (0.333f - is/(float)ii)*cosfTable_f[ind]);
				g = ftp - (r + b); 
			}
			else if( fh >= 120 && fh < 240 )
			{
				fh -= 120;
				ind = fh * 50 + 0.5f;
				r = ftp * is/ii;
				g = ftp * (0.333f + (0.333f - is/(float)ii)*cosfTable_f[ind]);
				b = ftp - (r + g);
			}
			else
			{
				fh -= 240;
				ind = fh * 50 + 0.5f;
				g = ftp * is/ii;
				b = ftp * (0.333f + (0.333f - is/(float)ii)*cosfTable_f[ind]);
				r = ftp - (g + b);
			}

			ib = b;
			ig = g;
			ir = r;

			y     = ((263 * ir + 516 * ig + 100 * ib + 512)>>10) + 16;
			*py++ = y < 16 ? 16 : y > 235 ? 235 : y;
			if( i%2 == 0 && j%2 == 0 )
			{
				u     = ((-152 * ir - 298 * ig + 450 * ib + 512)>>10) + 128;
				*pu++ = u < 16 ? 16 : u > 240 ? 240 : u;

				v     = ((450 * ir - 377 * ig - 73 * ib + 512)>>10) + 128;
				*pv++ = v < 16 ? 16 : v > 240 ? 240 : v;
			}
		}
	}

	if(poutbuf == 0 )
		free(phsi);

	return 0;
}

int zipl_sharpen_rgb( int w, int h, 
	                  int scansrc, unsigned char* psrc, 
	                  int scandst, unsigned char* pdst,
	                  unsigned char* poutbuf
	                 )
{
	int i, j;
	float* phsi;
	float* phs;
	int* pss;
	int* pis;
	int* pid;
	int lessx = w - 1;
	int lessy = h - 1;
	int* pis_c;
	int* pis_u;
	int* pis_d;
	int* pid_d;
	int ind;
	float r, g, b;
	float fh;
	int is, ii, di;
	float ftp;
	unsigned char* pd;

	if( w < 0 || h < 0 || scansrc < 0 || scandst < 0 )
		return dwIplColorParamError;
	if( psrc == 0 || pdst == 0 )
		return dwIplColorPointerNull;

	if( poutbuf == 0 )
	{
		phsi = (float*)malloc(w*h*3*sizeof(float) + w*h*sizeof(int));
		phs = phsi;
		pss = (int*)(phsi + w*h);
		pis = pss + w*h;
		pid = pis + w*h;
		memset(pid, 0, w*h*sizeof(int));
	}
	else
	{
		phsi = (float*)poutbuf;
		phs  = phsi;
		pss = (int*)(phsi + w*h);
		pis = pss + w*h;
		pid = pis + w*h;
		memset(pid, 0, w*h*sizeof(int));
	}

	//do rgb_hsip conversion
	if( zipl_rgb_hsip_fast(w, h, scansrc, psrc, phsi) != 0 )
	{
		if( poutbuf ==0 ) 
			free(phsi);

		return dwIplrgb2hsipError;
	}

	//do sharpen for intensity
	for( i = 1; i < lessy; ++i )
	{
		pis_c = pis + i * w;
		pis_u = pis_c - w;
		pis_d = pis_c + w;
		pid_d = pid + i * w;
		for( j = 1; j < lessx; ++j )
		{
			pid_d[j] = pis_u[0] + pis_u[1] + pis_u[2] 
			+ pis_d[0] + pis_d[1] + pis_d[2]
			+ pis_c[0] - 8*pis_c[1] + pis_c[2];

			pis_u++;
			pis_c++;
			pis_d++;
		}
	}

	//do hsip_rgb conversion
	for( i = 0; i < h; ++i )
	{
		pd = pdst + i*scandst;
		for( j = 0; j < w; ++j )
		{
			fh = *phs++;
			is = *pss++;
			ii = *pis++;
			di = *pid++;

			ftp = (ii - 0.1f*di);
			ftp = ftp < 0 ? 0 : ftp > 675 ? 675 : ftp;

			if( fh >= 0 && fh < 120 )
			{
				ind = fh * 50 + 0.5f;
				b = ftp * is/ii;
				r = ftp * (0.333f + (0.333f - is/(float)ii)*cosfTable_f[ind]);
				g = ftp - (r + b); 
			}
			else if( fh >= 120 && fh < 240 )
			{
				fh -= 120;
				ind = fh * 50 + 0.5f;
				r = ftp * is/ii;
				g = ftp * (0.333f + (0.333f - is/(float)ii)*cosfTable_f[ind]);
				b = ftp - (r + g);
			}
			else
			{
				fh -= 240;
				ind = fh * 50 + 0.5f;
				g = ftp * is/ii;
				b = ftp * (0.333f + (0.333f - is/(float)ii)*cosfTable_f[ind]);
				r = ftp - (g + b);
			}

			pd[0] = b < 0 ? 0 : b > 255 ? 255 : (unsigned char)b;
			pd[1] = g < 0 ? 0 : g > 255 ? 255 : (unsigned char)g;
			pd[2] = r < 0 ? 0 : r > 255 ? 255 : (unsigned char)r;
			
			pd +=3;
			
		}
	}

	if(poutbuf == 0 )
		free(phsi);

	return 0;
}

int zipl_sharpen_rgb_r( int w, int h, 
	                    int scan, unsigned char* psrcdst, 
	                    unsigned char* poutbuf
					   )
{
	int i, j;
	float* phsi;
	float* phs;
	int* pss;
	int* pis;
	int* pid;
	int lessx = w - 1;
	int lessy = h - 1;
	int* pis_c;
	int* pis_u;
	int* pis_d;
	int* pid_d;
	int ind;
	float r, g, b;
	float fh;
	int is, ii, di;
	float ftp;
	unsigned char* pd;

	if( w < 0 || h < 0 || scan < 0 )
		return dwIplColorParamError;
	if( psrcdst == 0 )
		return dwIplColorPointerNull;

	if( poutbuf == 0 )
	{
		phsi = (float*)malloc(w*h*3*sizeof(float) + w*h*sizeof(int));
		phs = phsi;
		pss = (int*)(phsi + w*h);
		pis = pss + w*h;
		pid = pis + w*h;
		memset(pid, 0, w*h*sizeof(int));
	}
	else
	{
		phsi = (float*)poutbuf;
		phs  = phsi;
		pss = (int*)(phsi + w*h);
		pis = pss + w*h;
		pid = pis + w*h;
		memset(pid, 0, w*h*sizeof(int));
	}

	//do rgb_hsip conversion
	if( zipl_rgb_hsip_fast(w, h, scan, psrcdst, phsi) != 0 )
	{
		if( poutbuf ==0 ) 
			free(phsi);

		return dwIplrgb2hsipError;
	}

	//do sharpen for intensity
	for( i = 1; i < lessy; ++i )
	{
		pis_c = pis + i * w;
		pis_u = pis_c - w;
		pis_d = pis_c + w;
		pid_d = pid + i * w;
		for( j = 1; j < lessx; ++j )
		{
			pid_d[j] = pis_u[0] + pis_u[1] + pis_u[2] 
			+ pis_d[0] + pis_d[1] + pis_d[2]
			+ pis_c[0] - 8*pis_c[1] + pis_c[2];

			pis_u++;
			pis_c++;
			pis_d++;
		}
	}

	//do hsip_rgb conversion
	for( i = 0; i < h; ++i )
	{
		pd = psrcdst + i*scan;
		for( j = 0; j < w; ++j )
		{
			fh = *phs++;
			is = *pss++;
			ii = *pis++;
			di = *pid++;

			ftp = (ii - 0.1f*di);
			ftp = ftp < 0 ? 0 : ftp > 675 ? 675 : ftp;

			if( fh >= 0 && fh < 120 )
			{
				ind = fh * 50 + 0.5f;
				b = ftp * is/ii;
				r = ftp * (0.333f + (0.333f - is/(float)ii)*cosfTable_f[ind]);
				g = ftp - (r + b); 
			}
			else if( fh >= 120 && fh < 240 )
			{
				fh -= 120;
				ind = fh * 50 + 0.5f;
				r = ftp * is/ii;
				g = ftp * (0.333f + (0.333f - is/(float)ii)*cosfTable_f[ind]);
				b = ftp - (r + g);
			}
			else
			{
				fh -= 240;
				ind = fh * 50 + 0.5f;
				g = ftp * is/ii;
				b = ftp * (0.333f + (0.333f - is/(float)ii)*cosfTable_f[ind]);
				r = ftp - (g + b);
			}

			pd[0] = b < 0 ? 0 : b > 255 ? 255 : (unsigned char)b;
			pd[1] = g < 0 ? 0 : g > 255 ? 255 : (unsigned char)g;
			pd[2] = r < 0 ? 0 : r > 255 ? 255 : (unsigned char)r;

			pd +=3;

		}
	}

	if(poutbuf == 0 )
		free(phsi);

	return 0;
}
	
