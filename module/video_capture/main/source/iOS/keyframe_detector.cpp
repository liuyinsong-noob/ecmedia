//#include "stdafx.h"
#include "keyframe_detector.h"
#include <math.h>
#include <assert.h>
#include <string>
#include <stdio.h>
#define  EPSION 1e-6

float static Compute_definition(int width, int height, int step, unsigned char* pBuf, int threshold = 7)
{
	int ws = width>>3;
	int hs = height>>3;
	int w  = width*3 / 4;
	int h  = height*3 / 4;

	int sum1 = 0;
	int sum2 = 0;
	int tmp  = 0;

	for( int i = 0; i < h; ++i )
	{
		unsigned char* ps = pBuf + (hs + i)*step + ws;
		unsigned char* ps1 = ps + step;

		for( int j = 0; j < w; ++j )
		{
			tmp = abs(ps[j] - ps[j + 1]) + abs(ps[j] - ps1[j]);
			sum1 += tmp;
			if( tmp > threshold )
				sum2 += tmp;
		}
	}

	return (sum2 / (sum1 + 0.000001f));
}

unsigned char static get_aver_value(int w, int h, int step, unsigned char* pbuf)
{
	unsigned int uSum = 0;
	for(int i = 0; i < h; ++i)
	{
		unsigned char* ps = pbuf + i*step;
		for(int j = 0; j < w; ++j)
		{
			uSum += ps[j];
		}
	}

	int s = w*h;
	unsigned char uValue = (uSum + s/2)/s;

	return uValue;
}

int CreateKeyFrameDetect(KeyFrameDetectCore** self)
{
	*self = (KeyFrameDetectCore*)malloc(sizeof(KeyFrameDetectCore));

	if( *self != NULL )
	{
		(*self)->width      = 0;
		(*self)->height     = 0;

		(*self)->firstframe = 0;

		(*self)->tmpBuf     = 0;
		(*self)->currBuf    = 0;
		(*self)->prevBuf    = 0;

		(*self)->initFlag   = 0;

		return 0;
	}

	return -1;
}


int KeyFrameDetectInitCore(KeyFrameDetectCore* self, 
	                       int width,
	                       int height)
{
	if( self != NULL )
	{
		self->width      = width;
		self->height     = height;
		
		self->firstframe = 1;

		if( self->tmpBuf != NULL )
			free(self->tmpBuf);

		self->tmpBuf = (unsigned char*)malloc(width*height*4);
		memset(self->tmpBuf, 0, width*height*4);
		self->prevBuf = self->tmpBuf;
		self->currBuf = self->tmpBuf + width*height*2;

		self->initFlag = 1;

		return 0;
	}

	return -1;
}

int _i = 1;
int KeyFrameDetectProcess(KeyFrameDetectCore*self,
	                      int stepy,
	                      unsigned char* pyBuf,
	                      int stepu,
	                      unsigned char* puBuf,
	                      int stepv,
	                      unsigned char* pvBuf, 
						  void* flog)
{
	if( self == NULL )
		return -1;

	if( self->initFlag != 1 )
		return -1;

	int xstep    = 8;
	int xbegin   = 24;

	int ystep    = 8;
	int ybegin   = 24;

	int nlessy = self->height - ybegin;
	int nlessx = self->width  - xbegin;

	unsigned char* py = self->currBuf;
	unsigned char* pu = py + self->width*self->height;
	unsigned char* pv = pu + self->width*self->height/2;

	if( self->firstframe == 1 )
	{
		for(int i = 0; i < nlessy; i += ystep)
		{
			int offset = i + ybegin;
			unsigned char* ps_y = pyBuf + offset*stepy + xbegin;
			unsigned char* ps_u = puBuf + (offset*stepu + xbegin) / 2;
			unsigned char* ps_v = pvBuf + (offset*stepv + xbegin) / 2;
			for(int j = 0; j < nlessx; j += xstep)
			{
				*py++ = get_aver_value(2, 2, stepy, ps_y + j);
				*pu++ = get_aver_value(1, 1, stepu, ps_u + j/2);
				*pv++ = get_aver_value(1, 1, stepv, ps_v + j/2);
			}
		}

		unsigned char* ptmp = self->currBuf;
		self->currBuf = self->prevBuf;
		self->prevBuf = ptmp;

		self->firstframe = 0;

		return 1;
	}
	else
	{
		int col = 0;
		int row = 0;

		for(int i = 0; i < nlessy; i += ystep)
		{
			row++;
			int offset = i + ybegin;
			unsigned char* ps_y = pyBuf + offset*stepy + xbegin;
			unsigned char* ps_u = puBuf + (offset*stepu + xbegin)/2;
			unsigned char* ps_v = pvBuf + (offset*stepv + xbegin)/2;
			col = 0;
			for(int j = 0; j < nlessx; j += xstep)
			{
				col++;
				*py++ = get_aver_value(2, 2, stepy, ps_y + j);
				*pu++ = get_aver_value(1, 1, stepu, ps_u + j/2);
				*pv++ = get_aver_value(1, 1, stepv, ps_v + j/2);
			}
		}


		int total  = col*row;
		float total_div = 1.f / total;


		unsigned char* pf_y  = self->prevBuf;
		unsigned char* pf_u  = pf_y + self->width*self->height;
		unsigned char* pf_v  = pf_u + self->width*self->height/2;

		unsigned char* pg_y  = self->currBuf;
		unsigned char* pg_u  = pg_y + self->width*self->height;
		unsigned char* pg_v  = pg_u + self->width*self->height/2;

		float yhist[8];
		float uhist[8];
		float vhist[8];

		memset( yhist, 0, 8*sizeof(float) );
		memset( uhist, 0, 8*sizeof(float) );
		memset( vhist, 0, 8*sizeof(float) );

		for(int i = 0; i < row; ++i)
		{
			for(int j = 0; j < col; ++j)
			{	
				int m = (*pf_y++)>>5;
				int n = (*pg_y++)>>5;
				int Index = abs(m-n);
				yhist[Index] += total_div;

				m = (*pf_u++)>>5;
				n = (*pg_u++)>>5;
				Index = abs(m-n);
				uhist[Index] += total_div;

				m = (*pf_v++)>>5;
				n = (*pg_v++)>>5;
				Index = abs(m-n);
				vhist[Index] += total_div;
				
			}
		}

		//
		unsigned char* ptmp = self->currBuf;
		self->currBuf = self->prevBuf;
		self->prevBuf = ptmp;


		float SumY = yhist[0];
		for ( int i = 1; i < 8; ++i )
		{
			SumY += i*i*i*yhist[i];
		}

		float fS_y = 0.0f;
		if ( SumY >= EPSION )
			fS_y = 1.0f / SumY;
		else
			fS_y = 1.0f;

		float SumU = uhist[0];
		for ( int i = 1; i < 8; ++i )
		{
			SumU += i*i*i*uhist[i];
		}

		float fS_u = 0.0f;
		if ( SumU >= EPSION )
			fS_u = 1.0f / SumU;
		else
			fS_u = 1.0f;

		float SumV = vhist[0];
		for ( int i = 1; i < 8; ++i )
		{
			SumV += i*i*i*vhist[i];
		}

		float fS_v = 0.0f;
		if ( SumV >= EPSION )
			fS_v = 1.0f / SumV;
		else
			fS_v = 1.0f;

		float fSimilary  = (fS_y + fS_u + fS_v)/3;
		float fclear = Compute_definition(self->width, self->height, stepy, pyBuf);
//		printf("-------frame %d similary = %.2f\n", _i++, fSimilary);
        if (flog) {
            fprintf((FILE*)flog, "-------frame %d similary = %.2f, definition = %.2f\r\n", _i++, fSimilary, fclear);
        }
		

		if( fSimilary > 0.94f && fclear > 0.08f )
			return 1;
		else
			return 0;
	}
}


int KeyFrameDetectFree(KeyFrameDetectCore*self)
{
	if( self != NULL )
	{
		if( self->tmpBuf != NULL )
			free(self->tmpBuf);

		free(self);
	}

	return 0;
}
