#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "howlingfilter_core.h"



void static iir_filter(Howling_Filter* filter,
	                   float* in_data,
	                   float* out_data,
	                   int samples)
{

	float tmp = 0;
	int i;
	for( i = 0; i < samples; ++i )
	{
		float st = in_data[i];
		st -= filter->a[0] * filter->st1;
		st -= filter->a[1] * filter->st2;

		//
		tmp = st * filter->b[0];
		tmp += filter->b[1] * filter->st1;
		tmp += filter->b[2] * filter->st2;

		//
		filter->st2 = filter->st1;
		filter->st1 = st;

		out_data[i] = tmp;
	}
}

//
int HowlingFilterCreate(Howling_Filter** self)
{
	*self = (Howling_Filter*)malloc(sizeof(Howling_Filter));
	if( (*self) != NULL )
	{
		(*self)->prev = NULL;
		(*self)->next = NULL;

		(*self)->gain = 0;
		(*self)->st1  = 0;
		(*self)->st2  = 0;
		(*self)->origin_pin = 0;
		(*self)->curr_pin   = 0;
		(*self)->idle_times = 0;
	}

	return -1;
}

int HowlingFilterInit(Howling_Filter* self, int fs, int pin)
{
	float gain = 30;
	float f0   = pin*8000.f/512.0f;
	float B    = 35.0f;
	float Q, k, v;

	if( self == NULL )
		return -1;

	if( fs != 8000 && fs != 16000 )
		return -1;
	
	if( f0 <= 1500 )
		B = 30.f;

	Q = f0 / B;    //Q = f0 / (f2 - f1) = f0 / B; B is bandwide, think deeply，do not change!

	k = tan( 3.14159265f * f0 / fs);
	v = pow(10.f, gain/20.0f);

	self->b[0] = (1.f + k / Q + k*k ) / (1.f + v * k / Q + k*k);
	self->b[1] = 2.0f * ( k*k - 1.f) / (1.f + v * k / Q + k*k);
	self->b[2] = (1.f - k / Q + k*k) / (1.f + v * k / Q + k*k);

	self->a[0] = self->b[1];
	self->a[1] = (1.f - v * k / Q + k*k ) / (1.f + v * k / Q + k*k);

	self->st1  = 0;
	self->st2  = 0;
	self->gain = gain;

	self->origin_pin = pin;
	self->curr_pin   = pin;

	//
	self->prev = NULL;
	self->next = NULL;
	self->idle_times = 0;

	return 0;
}

int HowlingFilterFree(Howling_Filter* self)
{
	if(self != NULL )
	{
		free(self);
		self = NULL;
	}

	return 0;
}

//////////////////////////////////////////////////////////////////////////
int HowlingFilterListCreate(Howling_FilterList** self)
{
	*self = (Howling_FilterList*)malloc( sizeof(Howling_FilterList) );
	
	if( *self != NULL )
	{
		(*self)->head  = NULL;
		(*self)->tail  = NULL;
		(*self)->count = 0;

		return 0;
	}

	return -1;
}

int HowlingFilterListFree(Howling_FilterList* self)
{
	if( self != NULL )
	{
		Howling_Filter* pPos = self->head;
		while( pPos )
		{
			Howling_Filter* pTmp = pPos->next;
			HowlingFilterFree(pPos);
			self->count--;
			pPos = pTmp;
		}
	}

	return 0;
}

int HowlingFilterListAdd2Head(Howling_FilterList* self, Howling_Filter* filter)
{
	if( self == NULL )
		return -1;

	if (filter == NULL )
		return -1;

	if(	self->head == NULL)
	{
		self->head	  =	filter;
		self->tail	  =	filter;
		filter ->prev =	NULL;
		filter ->next =	NULL;
	}
	else
	{
		filter->prev	 = NULL;
		filter->next	 = self->head;
		self->head->prev = filter;
		self->head       = filter;
	}
	self->count++;

	return 0;
}

int HowlingFilterListAdd2Tail(Howling_FilterList* self, Howling_Filter* filter)
{
	if( self == NULL )
		return -1;

	if( filter == NULL )
		return -1;

	if(	self->head == NULL)
	{
		self->head	 = filter;
		self->tail	 = filter;
		filter->prev =	NULL;
		filter->next =	NULL;
	}
	else
	{
		filter->prev     = self->tail;
		filter->next     = NULL;
		self->tail->next = filter;
		self->tail       = filter;
	}
	self->count++;

	return 0;
}

int HowlingFilterListDrop(Howling_FilterList* self, Howling_Filter* filter)
{
	if( self == NULL )
		return -1;

	if( filter == NULL )
		return -1;

	if( self->head == self->tail )
	{
		if(filter != self->head )
		{
			return 0;
		}
		self->head = NULL;
		self->tail = NULL;
	}
	else
	{
		if( filter == self->head )
		{
			Howling_Filter* next = filter->next;
			self->head = next;
			next->prev = NULL;
		}
		else if( filter == self->tail )
		{
			Howling_Filter* prev = filter->prev;
			self->tail = prev;
			prev->next = NULL;
		}
		else
		{
			Howling_Filter* prev = filter->prev;
			Howling_Filter* next = filter->next;
			prev->next = next;
			next->prev = prev;
		}
	}

	self->count--;

	return 0;
}


//////////////////////////////////////////////////////////////////////////
int HowlingFPoolCreate(HowlingFilterPool** self)
{
	(*self) = (HowlingFilterPool*)malloc(sizeof(HowlingFilterPool));
	if( *self != NULL )
	{
		if( HowlingFilterListCreate( &((*self)->Use) ) == -1 )
			return -1;
		if( HowlingFilterListCreate( &((*self)->Idle) ) == -1 )
			return -1;
		(*self)->count    = 0;
		(*self)->initFlag = 1;

		return 0;
	}

	return -1;
}

int HowlingFPoolMakeAndUpdateFilter( HowlingFilterPool* self, int fs, int* howlingpin, int howlingcount )
{
	int use_size = 0;
	int i, j;
	Howling_Filter* pos;

	if( self == NULL || self->initFlag != 1 )
		return -1;

	if( fs != 8000 && fs != 16000 )
		return -1;

	//先从work list中查找，
	//如没有找到则从idle list中查找，如还没有找到则create
	//最后update list
	use_size = self->Use->count;
	for( i = 0; i < howlingcount; ++i )
	{
		int find = 0;
		Howling_Filter* pos = self->Use->head;
		for( j = 0; j < use_size; ++j )
		{
			if( pos->origin_pin == howlingpin[i] )
			{
				pos->idle_times = 0;
				find = 1;
				break;
			}
			pos = pos->next;
		}

		if( find == 0 )
		{
			int size = self->Idle->count;
			Howling_Filter* pos = self->Idle->head;
			for( j = 0; j < size; ++j )
			{
				if( pos->origin_pin == howlingpin[i] )
				{
					pos->idle_times = 0;
					find = 1;
					HowlingFilterListDrop(self->Idle, pos);
					HowlingFilterListAdd2Tail(self->Use, pos);
					break;
				}
				pos = pos->next;
			}
		}

		if( find == 0 )
		{
			//create
			Howling_Filter* filter;
			HowlingFilterCreate(&filter);
			HowlingFilterInit(filter, fs, howlingpin[i]);
			HowlingFilterListAdd2Tail(self->Use, filter);
			self->count++;
		}
	}
	
	//update all worklist items
	pos = self->Use->head;
	while( pos != NULL )
	{
		pos->idle_times++;
		if( pos->idle_times >= 240 )
		{
			Howling_Filter* pTmp = pos;
			pos = pTmp->next;
			HowlingFilterListDrop(self->Use, pTmp);
			HowlingFilterListAdd2Tail(self->Idle, pTmp);
		}
		else
			pos = pos->next;
	}

	return 0;
}

int HowlingFPoolProcess( HowlingFilterPool* self, 
	                     float* in_data,
	                     float* out_data,
	                     int sampels )
{
	float* idata;
	float* odata;
	float* tmp;

	Howling_Filter* item;

	if( self == NULL )
		return -1;

	//
	idata = out_data;
	odata = in_data;
	
	item = self->Use->head;
	while( item != NULL )
	{
		tmp   = idata;
		idata = odata;
		odata = tmp;

		iir_filter(item, idata, odata, sampels);

		item = item->next;
	}

	if( odata != out_data )
	{
		memcpy( out_data, odata, sizeof(float)*sampels );
	}

	return 0;
}

//
int HowlingFPoolFree(HowlingFilterPool* self)
{
	if( self != NULL )
	{
		HowlingFilterListFree(self->Use);
		HowlingFilterListFree(self->Idle);

		free(self);
		self = NULL;
	}

	return 0;
}