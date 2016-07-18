//#include "stdafx.h"
#include "bilteral_filter.h"
#include <stdlib.h>
#include <math.h>
#include <memory.h>


int CreateBilterFilter(BilteralFilterCore** self)
{
	*self = (BilteralFilterCore*)malloc(sizeof(BilteralFilterCore));
	if( *self != NULL )
	{
		(*self)->width      = 0;
		(*self)->height     = 0;
		(*self)->tmpBuf     = 0;
		
		(*self)->gaussTable = 0;
		(*self)->sigmad     = 0;
		(*self)->sigmar     = 0;

		(*self)->initFlag   = 0;

		return 0;
	}

	return -1;
}


int BilterFilterInitCore(BilteralFilterCore* self, 
	                     int width,
						 int height,
						 int sigmad, 
						 int sigmar)
{
	if( self == NULL )
		return -1;

	if( sigmad <= 0 || sigmar <= 0 )
		return -1;

	self->sigmad = sigmad;
	self->sigmar = sigmar;

	self->width = width;
	self->height= height;

	if( self->tmpBuf != NULL )
		free(self->tmpBuf);

	self->tmpBuf = (int*)malloc(width*height*sizeof(int));
	memset(self->tmpBuf, 0, width*height*sizeof(int));

	if( self->gaussTable != NULL )
		free(self->gaussTable);
	
	/*
	int nsize = sigmad*2 + 1;
	self->gaussTable = (int*)malloc(nsize * sizeof(int));
	int ndv = (sigmad*sigmad) << 1;

	for (unsigned int i = 0; i < nsize; i++)
	{
		//int ntp =(int) (expf((float)((nsize - i) * (nsize - i)) / ndv ));
		float ftp = (expf((float)((nsize - i) * (nsize - i)) / ndv ));
		int ntp = ftp;
		(self->gaussTable)[i] = ntp;
	}
	*/
	
	
	self->gaussTable = (int*)malloc((sigmad + 1)*sizeof(int));
	float ndv = -sigmad*sigmad*2.0f;
	for( int i = 0; i <= sigmad; ++i )
	{
		float ftp = expf( i*i / ndv ) * 16 + 0.5f;
		int ntp = ftp;
		(self->gaussTable)[i] = ntp;
	}

	int square = sigmar*sigmar*2;
	for(int j = 0; j < 256; j++)
	{	
		(self->rangeTable)[j] = (int)(1024*exp(-(j*j) / (float)square) + 0.5f);
	}
	
	self->initFlag = 1;

	return 0;
}

int BilterFilterProcessCore(BilteralFilterCore* self,
	                        int stepsrc,
	                        unsigned char* psrc,
	                        int stepdst,
	                        unsigned char* pdst)
{
	if( self == NULL || self->initFlag != 1 )
		return -1;

	int nRadius = self->sigmad;
	int width   = self->width;
	int height  = self->height;

	int lessy   = height - nRadius;
	int lessx   = width - nRadius;

	int vaule_pos = self->rangeTable[0] * self->gaussTable[0];

	for( int i = 0; i < height; ++i )
	{
		unsigned char* ps = psrc + i * stepsrc;
		int*  pd          = self->tmpBuf + i*width;
		for( int j = nRadius; j < lessx; ++j )
		{
			int c = ps[j];
			int nSum           = 0;//像素亮度和
			int nNormalFactor  = 0;//像素归一化因子

			int nPhotometric  = 0; //像素亮度权值临时变量
			int nSimilarity   = 0; //像素联合权值临时变量

			for( int k = 1; k <= nRadius; ++k )
			{
				nPhotometric      = self->rangeTable[abs(c - ps[j-k])];//查表计算亮度差权值
				nSimilarity       = nPhotometric * self->gaussTable[k];//查表计算空间域高斯模板权值				

				nNormalFactor     += nSimilarity;
				nSum              += nSimilarity * ps[j-k];

				nPhotometric      = self->rangeTable[abs(c - ps[j+k])];//查表计算亮度差权值
				nSimilarity       = nPhotometric * self->gaussTable[k];//查表计算空间域高斯模板权值				

				nNormalFactor     += nSimilarity;
				nSum              += nSimilarity * ps[j+k];
			}

			//nSimilarity        = self->rangeTable[0] * self->gaussTable[0];
			nNormalFactor     += vaule_pos/*nSimilarity*/;
			nSum              += vaule_pos/*nSimilarity*/ * c;

			pd[j]         = (nSum + (nNormalFactor >>1)) / (nNormalFactor ) ;
		}
	}
	// end of x direction filter


	for( int i = nRadius; i < lessy; ++i )
	{
		int* ps_c  = self->tmpBuf + i * width;
		unsigned char* pd = pdst + i * stepdst;

		for( int j = nRadius; j < lessx; ++j )
		{
			int c  = ps_c[j];

			int nSum          = 0;
			int nNormalFactor = 0;
			int nPhotometric  = 0;
			int nSimilarity   = 0;

			for( int k = 1; k <= nRadius; ++k )
			{
				int offset = width*k;

				int ps_uk = *(ps_c - offset + j);

				nPhotometric      = self->rangeTable[abs(ps_uk - c)];
				nSimilarity       = nPhotometric * self->gaussTable[k];

				nNormalFactor    += nSimilarity;
				nSum             += nSimilarity * ps_uk;

				///
				int ps_dk = *(ps_c + offset + j);

				nPhotometric      = self->rangeTable[abs(ps_dk - c)];
				nSimilarity       = nPhotometric * self->gaussTable[k];

				nNormalFactor    += nSimilarity;
				nSum             += nSimilarity * ps_dk;
			}

			//nSimilarity        = self->rangeTable[0] * self->gaussTable[0];
			nNormalFactor     += vaule_pos/*nSimilarity*/;
			nSum              += vaule_pos/*nSimilarity*/ * c;

			int ntp           = (nSum + (nNormalFactor >>1)) / (nNormalFactor) ;
			pd[j]             = ntp < 0 ? 0 : (ntp > 255 ? 255 : ntp);
		}
	}
	//end of y direction filter

	//
	for( int i = 0; i < nRadius; ++i)
	{
		memcpy(pdst + i*stepdst, psrc + i*stepsrc, width);
		memcpy(pdst + (height - 1 - i)*stepdst, psrc + (height - 1 - i)*stepsrc, width);
	}

	for( int i = nRadius; i < lessy; ++i )
	{
		unsigned char* ps = psrc + i*stepsrc;
		unsigned char* pd = pdst + i*stepdst;
		memcpy(pd, ps, nRadius);
		memcpy(pd+lessx-1, ps + lessx-1, nRadius);
	}

	return 0;
}

int BilterFilterProcessCore(BilteralFilterCore* self,
	                        int step,
	                        unsigned char* pbuf)
{
	if( self == NULL || self->initFlag != 1 )
		return -1;

	int nRadius = self->sigmad;
	int width   = self->width;
	int height  = self->height;
	
	int lessy   = height - nRadius;
	int lessx   = width - nRadius;

	int vaule_pos = self->rangeTable[0] * self->gaussTable[0];

	for( int i = 0; i < height; ++i )
	{
		unsigned char* ps = pbuf + i * step;
		int*  pd          = self->tmpBuf + i*width;
		for( int j = nRadius; j < lessx; ++j )
		{
			int c = ps[j];
			int nSum           = 0;//像素亮度和
			int nNormalFactor  = 0;//像素归一化因子

			int nPhotometric  = 0; //像素亮度权值临时变量
			int nSimilarity   = 0; //像素联合权值临时变量

			for( int k = 1; k <= nRadius; ++k )
			{
				nPhotometric      = self->rangeTable[abs(c - ps[j-k])];//查表计算亮度差权值
				nSimilarity       = nPhotometric * self->gaussTable[k];//查表计算空间域高斯模板权值				

				nNormalFactor     += nSimilarity;
				nSum              += nSimilarity * ps[j-k];

				nPhotometric      = self->rangeTable[abs(c - ps[j+k])];//查表计算亮度差权值
				nSimilarity       = nPhotometric * self->gaussTable[k];//查表计算空间域高斯模板权值				

				nNormalFactor     += nSimilarity;
				nSum              += nSimilarity * ps[j+k];
			}

			//nSimilarity        = self->rangeTable[0] * self->gaussTable[0];
			nNormalFactor     += vaule_pos/*nSimilarity*/;
			nSum              += vaule_pos/*nSimilarity*/ * c;

			pd[j]         = (nSum + (nNormalFactor >>1)) / (nNormalFactor ) ;
		}
	}
	// end of x direction filter

	
	for( int i = nRadius; i < lessy; ++i )
	{
		int* ps_c  = self->tmpBuf + i * width;
		unsigned char* pd = pbuf + i * step;
		
		for( int j = nRadius; j < lessx; ++j )
		{
			int c  = ps_c[j];
			
			int nSum          = 0;
			int nNormalFactor = 0;
			int nPhotometric  = 0;
			int nSimilarity   = 0;
			
			for( int k = 1; k <= nRadius; ++k )
			{
				int offset = width*k;

				int ps_uk = *(ps_c - offset + j);
				
				nPhotometric      = self->rangeTable[abs(ps_uk - c)];
				nSimilarity       = nPhotometric * self->gaussTable[k];

				nNormalFactor    += nSimilarity;
				nSum             += nSimilarity * ps_uk;

				///
				int ps_dk = *(ps_c + offset + j);

				nPhotometric      = self->rangeTable[abs(ps_dk - c)];
				nSimilarity       = nPhotometric * self->gaussTable[k];

				nNormalFactor    += nSimilarity;
				nSum             += nSimilarity * ps_dk;
			}
			
			//nSimilarity        = self->rangeTable[0] * self->gaussTable[0];
			nNormalFactor     += vaule_pos/*nSimilarity*/;
			nSum              += vaule_pos/*nSimilarity*/ * c;

			int ntp           = (nSum + (nNormalFactor >>1)) / (nNormalFactor) ;
			pd[j]             = ntp < 0 ? 0 : (ntp > 255 ? 255 : ntp);
		}
	}
	//end of y direction filter
	
	/*
	for (int x = 0; x < self->width; x++)
	{
		int nBegin         = (x - nRadius > 0) ? (-nRadius) :  -x;//边缘特殊处理
		int nEnd           = ((x + nRadius) < width) ? nRadius : (nRadius + x - width - 1);//边缘特殊处理

		for (int y = 0; y < self->height; y++) 
		{
			unsigned char *pCenter  = (pbuf + x + y*step);	//滤波的中心像素		
			int nSum           = 0;//像素亮度和
			int nNormalFactor  = 0;//像素归一化因子
			
			for (int i = nBegin; i <= nEnd ; i++) 
			{				
				int nPhotometric  = 0; //像素亮度权值临时变量
				int nSimilarity   = 0; //像素联合权值临时变量
				int dL            = 0; //像素亮度差临时变量

				unsigned char *pNeighbor = pbuf+ (x+i) + y*step;//从滤波的中心点，向正负nRadius滤波				
				dL                  = *pCenter - *pNeighbor;//计算亮度差

				nPhotometric        = self->rangeTable[abs(dL)];//查表计算亮度差权值
				nSimilarity         = nPhotometric * self->gaussTable[abs(i)];//查表计算空间域高斯模板权值				

				nNormalFactor       += nSimilarity;
				nSum                += nSimilarity * (*pNeighbor);

			}			
			int *target   = self->tmpBuf + x  + y* width;             //写入目标像素			
			nNormalFactor = (nNormalFactor > 0) ? nNormalFactor : 1;  //防止被零除
			*target       = (nSum + (nNormalFactor >>1)) / (nNormalFactor ) ;
		}
	}
	*/
	

	//////////////////////////////////////////////////////////////////////////
	/*
	int ntp = 0;
	for (int x = 0; x < width ; x++) 
	{
		for (int y = 0; y < height ; y++) 
		{
			int *pCenter      = self->tmpBuf + x + y * width;
			int nSum          = 0;
			int nNormalFactor = 0;

			int nBegin  = y - nRadius > 0 ? -nRadius :  -y;//边缘特殊处理
			int nEnd    = y + nRadius < height ? nRadius :  height - y - 1;

			for (int j = nBegin; j <= nEnd; j++) 
			{				
				int nPhotometric  = 0;
				int nSimilarity   = 0;
				int  dL           = 0;
				int *pNeighbor    = self->tmpBuf + x  + (y+j)* width ;

				dL                = *pCenter - *pNeighbor;	
				nPhotometric      = self->rangeTable[int(abs(dL))];
				nSimilarity       = nPhotometric * self->gaussTable[abs(j)];

				nNormalFactor    += nSimilarity;
				nSum             += nSimilarity * (*pNeighbor);
			}			
			nNormalFactor         = nNormalFactor > 0 ? nNormalFactor : 1;//防止被零除
			ntp                   = (nSum + (nNormalFactor >>1)) / (nNormalFactor) ;
			*(pbuf + x + y*step)  = ntp < 0 ? 0 : (ntp > 255 ? 255 : ntp);
		}
	}
	*/

	return 0;
}

int BilterFilterFree(BilteralFilterCore* self)
{
	if( self != NULL )
	{
		if( self->tmpBuf )
			free(self->tmpBuf);

		if( self->gaussTable )
			free(self->gaussTable);

		free(self);
	}

	return 0;
}