#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "howlingfilter_core.h"
#include <assert.h>


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

int static GetGain(int fs, float fpin, float magn, float* gain)
{
	if( fs != 8000 && fs != 16000 )
		return -1;
	//

	//
	return 0;
}


//Computer the bandwidth of the notchfilter for howling pin
int static GetBandWidth(int fs, float fpin, float* bandwidth)
{
	// for fs = 16000
	//      8     10    15   20   28   33   35   37   38
	//     500   1000  2000 3000 4000 5000 6000 7000 8000
	//==============================================================
	// for fs = 8000
	//      8     10     15     25    30
	//     500   1000   2000   3000  4000
	//
	if( fs != 8000 && fs != 16000 )
		return -1;

	if( fs == 8000 )
	{
		if ( fpin < 500 )
		{
			*bandwidth = 8.0f;
		}
		else if( fpin >= 500 && fpin < 1000 )
		{
			*bandwidth = ((fpin - 500.f)/500.f) * (10.0f - 8.0f) + 8.0f;
		}
		else if( fpin >= 1000 && fpin < 2000 )
		{
			*bandwidth = ((fpin - 1000.0f)/1000.f) * (15.0f - 10.0f) + 10.0f;
		}
		else if( fpin >= 2000 && fpin < 3000 )
		{
			*bandwidth = ((fpin - 2000.0f)/1000.f) * (25.0f - 15.0f) + 15.0f;
		}
		else if( fpin >= 3000 && fpin <= 4000 )
		{
			*bandwidth = ((fpin - 3000.0f)/1000.f) * (30.0f - 25.0f) + 25.0f;
		}

	}

	if( fs == 16000 )
	{
		if ( fpin < 500 )
		{
			*bandwidth = 8.0f;
		}
		else if( fpin >= 500 && fpin < 1000 )
		{
			*bandwidth = ((fpin - 500.f)/500.f) * (10.0f - 8.0f) + 8.0f;
		}
		else if( fpin >= 1000 && fpin < 2000 )
		{
			*bandwidth = ((fpin - 1000.0f)/1000.f) * (15.0f - 10.0f) + 10.0f;
		}
		else if( fpin >= 2000 && fpin < 3000 )
		{
			*bandwidth = ((fpin - 2000.0f)/1000.f) * (20.0f - 15.0f) + 15.0f;
		}
		else if( fpin >= 3000 && fpin < 4000 )
		{
			*bandwidth = ((fpin - 3000.0f)/1000.f) * (28.0f - 20.0f) + 20.0f;
		}
		else if( fpin >= 4000 && fpin < 5000 )
		{
			*bandwidth = ((fpin - 4000.0f)/1000.f) * (33.0f - 28.0f) + 28.0f;
		}
		else if( fpin >= 5000 && fpin < 6000 )
		{
			*bandwidth = ((fpin - 5000.0f)/1000.f) * (35.0f - 33.0f) + 33.0f;
		}
		else if( fpin >= 6000 && fpin < 7000 )
		{
			*bandwidth = ((fpin - 6000.0f)/1000.f) * (37.0f - 35.0f) + 35.0f;
		}
		else if( fpin >= 7000 && fpin <= 8000 )
		{
			*bandwidth = ((fpin - 7000.0f)/1000.f) * (38.0f - 37.0f) + 37.0f;
		}

	}
	return 0;

}

int static GetBandAndGain(int fs, int ibeg, int iend, float* bandwidth, float* gain)
{
	int i;
	float bw;

	if( fs != 8000 && fs != 16000 )
		return -1;

	//bandwidth 
	for( i = ibeg; i <= iend; ++i )
	{
		GetBandWidth(fs, ibeg, &bw);
		*bandwidth += bw;
	}
	//gain
	//
	//    20    25    30   32   34   36   38   40   42   45
	//     1     2     3   4    5     6    7    8    9   10
	i = (iend - ibeg + 1);
	if( i == 1 )
		*gain = 25;
	else if( i == 2 )
		*gain = 25;
	else if( i == 3 )
		*gain = 25;
	else if( i == 4 )
		*gain = 25;
	else if( i == 5 )
		*gain = 25;
	else if( i == 6 )
		*gain = 25;
	else if( i == 7 )
		*gain = 25;
	else if( i == 8 )
		*gain = 25;
	else if( i == 9 )
		*gain = 25;
	else if( i >= 10 )
		*gain = 25;
	
	return 0;
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
	float gain = 20;
	float f0   = pin * fs / 1024.0f;
	float B    = 30.0f;
	float Q, k, v;

	if( self == NULL )
		return -1;

	if( fs != 8000 && fs != 16000 )
		return -1;
	
	GetBandWidth(fs, f0, &B);

	Q = f0 / B;                    //Q = f0 / (f2 - f1) = f0 / B; B is bandwide, think deeply，do not change!

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
		self->head = NULL;
		self->tail = NULL;
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


int HowlingFPoolMakeAndUpdateFilter( HowlingFilterPool* self, int fs, const int* howlingpin, const int howlingcount )
{
	//no problem, has passed testing.
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
	for( i = 0; i < 513; ++i )
	{
		int find = 0;
		Howling_Filter* pos = self->Use->head;

		if( howlingpin[i] == 0 )
			continue;

		for( j = 0; j < use_size; ++j )
		{
			if( pos->origin_pin == i )
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
				if( pos->origin_pin == i)
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
			HowlingFilterInit(filter, fs, i);
			HowlingFilterListAdd2Tail(self->Use, filter);
			self->count++;
		}
	}

	//update all worklist items
	pos = self->Use->head;
	while( pos != NULL )
	{
		pos->idle_times++;
		if( pos->idle_times >= 50 )
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

int HowlingFPoolRemoveAllFilter( HowlingFilterPool* self )
{
	Howling_Filter* pos = self->Use->head;

	while( pos != NULL )
	{
		Howling_Filter* pTmp = pos;
		pos = pTmp->next;
		HowlingFilterListDrop(self->Use, pTmp);
		HowlingFilterListAdd2Tail(self->Idle, pTmp);
	}

	return 0;
}

int HowlingFPoolRemoveFilterPins(  HowlingFilterPool* self,
	                               const float* magn,
	                               const int magnLen,
								   const int* peak_pin,
								   const int peak_count,
	                               const float pav )
{
	/*return 0;*/
	//check every used pins
	Howling_Filter* pos = self->Use->head;
	while( pos != NULL )
	{
		if( pos->idle_times > 10 )
		{
// 			int find = 0;
// 			for( int i = 0; i < peak_count; ++i )
// 			{
// 				if( pos->origin_pin == peak_pin[i]/* || pos->origin_pin == peak_pin[i] - 1 || pos->origin_pin == peak_pin[i] + 1 */)
// 				{
// 					find = 1;
// 					break;
// 				}
// 			}
// 			if( find == 0 && magn[pos->origin_pin] >= 10000 && log10(magn[pos->origin_pin] * magn[pos->origin_pin] / pav) < 0.f )
// 			{
// 				Howling_Filter* pTmp = pos;
// 				pos = pTmp->next;
// 				HowlingFilterListDrop(self->Use, pTmp);
// 				HowlingFilterListAdd2Tail(self->Idle, pTmp);
// 			}
// 			else
// 			{
// 				pos = pos->next;
// 			}
			
			if( magn[pos->origin_pin] >= 5000 && log10(magn[pos->origin_pin] * magn[pos->origin_pin] / pav) < 0.5f )
			{
				Howling_Filter* pTmp = pos;
				pos = pTmp->next;
				HowlingFilterListDrop(self->Use, pTmp);
				HowlingFilterListAdd2Tail(self->Idle, pTmp);
			}
			else
				pos = pos->next;
			
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