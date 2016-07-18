/* @file  : bilteral_filter.h
*  @author: GuoqingZeng
*  @date  : 20160525
*  @description: realize bilateral filter, which can get much more better result than
*                other normal filter methods such as gauss filter. 
*                1. denoise and smooth image
*                2. keep edge
*  @version: Create!
*/
#ifndef __BILTERAL_FILTER_H__
#define __BILTERAL_FILTER_H__


typedef struct  
{
	int width;
	int height;
	int* tmpBuf;

	int  rangeTable[256];
	int* gaussTable;

	int  sigmad;
	int  sigmar;

	int initFlag;
}BilteralFilterCore;


int CreateBilterFilter(BilteralFilterCore** self);

int BilterFilterInitCore(BilteralFilterCore* self, 
	                     int width,
						 int height,
						 int sigmad, 
						 int sigmar);

int BilterFilterProcessCore(BilteralFilterCore* self,
							int stepsrc,
							unsigned char* psrc,
							int stepdst,
							unsigned char* pdst);

int BilterFilterProcessCore(BilteralFilterCore* self,
	                        int step,
	                        unsigned char* pbuf);

int BilterFilterFree(BilteralFilterCore* self);

#endif