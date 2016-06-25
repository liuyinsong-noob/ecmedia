/* @file: howlingfilter_core.h
*  @author: Guoqing Zeng
*  @date: 20160608
*  @description: realized howling pin filter key algorithm.
*                use iir trap filter pool.
*
*  @reference: 《基于DSP的啸叫抑制系统的研究与实现》 邢堃 xx
*  @version: Newly Create! @20160608
*  
*/

#ifndef __HOWLINGFILTER_CORE_H__
#define __HOWLINGFILTER_CORE_H__

//howling filter definition
typedef struct Howling_filterC_
{
	float a[2];
	float b[3];

	float st1;
	float st2;

	float gain;

	//Howling_State hlstate
	int   origin_pin;
	float curr_pin;

	int idle_times; //

	void* prev;
	void* next;

}Howling_Filter;

int HowlingFilterCreate(Howling_Filter** self);
int HowlingFilterInit(Howling_Filter* self, int fs, int pin);
int HowlingFilterFree(Howling_Filter* self);



//howling filter list definition
typedef struct Howling_FilterListC_
{
	Howling_Filter* head;
	Howling_Filter* tail;
	int count;

}Howling_FilterList;

int HowlingFilterListCreate(Howling_FilterList** self);
int HowlingFilterListFree(Howling_FilterList* self);
int HowlingFilterListAdd2Head(Howling_FilterList* self, Howling_Filter* filter);
int HowlingFilterListAdd2Tail(Howling_FilterList* self, Howling_Filter* filter);
int HowlingFilterListDrop(Howling_FilterList* self, Howling_Filter* filter);




//howling filter pool definition
typedef struct HowlingFilerPoolC_
{
	Howling_FilterList* Use;
	Howling_FilterList* Idle;
	int count;
	int initFlag;

}HowlingFilterPool;


int HowlingFPoolCreate(HowlingFilterPool** self);

int HowlingFPoolInit(HowlingFilterPool* self);

int HowlingFPoolMakeAndUpdateFilter( HowlingFilterPool* self, 
	                                 int fs, 
									 int* howlingpin, 
									 int howlingcount );

int HowlingFPoolProcess(HowlingFilterPool* self, 
	                    float* in_data,
						float* out_data,
						int sampels);

int HowlingFPoolFree(HowlingFilterPool* self);

#endif