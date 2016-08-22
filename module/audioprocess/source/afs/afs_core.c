#include "afs_core.h"
#include "fft4g.h"
#include <math.h>
#include <assert.h>
#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include "howlingfilter_core.h"


//
static void UpdateBuffer(const float* frame,
	                     int frame_length,
						 int buffer_length,
						 float* buffer)
{
	//
	memcpy(buffer,
		   buffer + frame_length,
		   sizeof(float) * (buffer_length - frame_length) );

	//
	if( frame )
	{
		memcpy(buffer + buffer_length - frame_length,
			   frame,
			   sizeof(float) * frame_length );
	}
	else
	{
		memset(buffer + buffer_length - frame_length,
			   0,
			   sizeof(float) * frame_length );
	}

}

//
static void Windowing(const float* window,
	                  const float* data,
					  int length,
					  float* data_windowed)
{
	int i = 0;
	for( i = 0; i < length; ++i )
	{
		data_windowed[i] = window[i] * data[i];
	}
}

//transforms the signal from time to frequency domain.
//
// Inputs:
//    *|time_data| is the signal in time domain.
//    *|time_data_length| is the length of analysis buffer
//    *|magintude_length| is the length of spectrum magnitude, which
//     equals the length of both |real| and |image| (time_data_length / 2 + 1)
// Outputs:
//    *|time_data| is the signal in the frequency domain.
//    *|real| is the real part of the frequency domain.
//    *|imag| is the imaginary part of the frequency domain.
//    *|magn| is the calculated magnitude of the frequency domain.

static void FFT(AcousticfeedbackSupressionC* self, 
	            float* time_data,
	            int time_data_length,
	            int magnitude_length,
	            float* real,
	            float* imag,
	            float* magn)
{
	//
	int i;
	assert(magnitude_length == time_data_length / 2 +1);
	//fwd fft
	WebRtc_rdft(self->anaLen, 1, time_data, self->ip, self->wfft);

	//
	real[0] = time_data[0];
	imag[0] = 0;
	magn[0] = fabs(real[0]) + 1.0f;

	real[magnitude_length - 1]  = time_data[1];
	imag[magnitude_length - 1]  = 0;
	magn[magnitude_length - 1]  = fabs(time_data[1]) + 1.0f;

	for( i = 1;  i < magnitude_length - 1; ++i )
	{
		real[i]  = time_data[2 * i];
		imag[i]  = time_data[2 * i + 1];
		//
		magn[i]  = sqrtf(real[i]*real[i] + imag[i]*imag[i]) + 1.0f;
	}
}


//
static int Silence(const float* frame, int frame_length)
{
	//-45 dBFS = 185,  20log|a/max|
	//-40 dBFS = 328
	int i;
	for( i = 0; i < frame_length; ++i )
	{
		if( frame[i] > 325 || frame[i] < -325 )
			return 0;
	}

	return 1;
}

//////////////////////////语音帧类型判断////////////////////////////////
static int FrameType( AcousticfeedbackSupressionC* self )
{
	if( self == NULL || self->initFlag != 1 )
		return -1;

	//
	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
// analyze spectrum magnitude information and compute peaks in frequency domain.
//
// Input: 
//       *|magn| is the magnitude of the frequency domain, analyzes these data and 
//               gives the peak statistic in frequency domain.
//
// Output:
//       *|peak_magn| is the magnitude value of the peak.
//       *|peak_pin| is the frequency pin of the peak.
//       *|peak_inter_l| is the left edge of the peak interval.
//       *|peak_inter_r| is the right edge of the peak interval.
//       *|peak_count| is the count of peaks.
int static SpectrumPeakEx( AcousticfeedbackSupressionC* self,
	                       const float* magn,
						   float* peak_magn,
						   int* peak_pin )
{
	int i, j, k;
	int find;
	float max;
	int pin;

	if( self == NULL || self->initFlag != 1 )
		return -1;

	//statics spectrum info
	for( i = 0; i < self->peak_maxcount; ++i )
	{
		max = -1.f;
		for( j = self->start_pin; j < self->end_pin; ++j )
		{
			if( max < magn[j] )
			{
				find = 0;
				for( k = 0; k < i; ++k )
				{
					if( j >= peak_pin[k]-1 && j <= peak_pin[k]+1 )
					{
						find = 1;
						break;
					}
				}
				if( find == 0 )
				{
					max = magn[j];
					pin = j;
				}
			}
		}
		peak_magn[i] = max;
		peak_pin[i]  = pin;
	}

	return 0;
}

int static SpectrumPeak( AcousticfeedbackSupressionC* self,
	                      const float* magn,
						  float* peak_magn,
						  int* peak_pin,
						  int* peak_inter_l,
						  int* peak_inter_r,
						  int* peak_count)
{
	int i, j, k, t;
	int left, right;
	int ind = 0;

	float energy = 0.f;
	float energys_ex[5] = {0};
	int ibeg = 0;
	int iend = 0;
	float sc = 0;
	float bw = 0;

	float peak = 0;
	int   peak_cnt = 0;

	if( self == NULL || self->initFlag != 1 )
		return -1;

	/////////////////////////////分析频段能量//////////////////////////////////////////
	for( i = 0; i < self->magnLen; ++i )
	{
		energy += magn[i] * magn[i];
	}
	
	ibeg = 0;
	for( i = 0; i < 5; ++i )
	{
		iend =self->bands_ex[i];
		for( j = ibeg; j <= iend; ++j )
		{
			energys_ex[i] += magn[j]*magn[j];
		}
		ibeg = iend + 1;
	}

	
	//////////////////////////////////////////////////////////////////////////
	if( self->reset == 1 )
	{
		self->energy = energy;
		memcpy(self->energys_ex, energys_ex, sizeof(float)*5);
		self->reset = 0;
	}
	else
	{
		self->energy = energy + self->rc_alpha * (self->energy - energy);
		//memcpy(self->energys_ex, energys_ex, sizeof(float)*5);
		for( i = 0; i < 5; ++i )
			self->energys_ex[i] = 0.1 * self->energys_ex[i] + 0.9 * energys_ex[i]; 

		//
		memcpy(energys_ex, self->energys_ex, sizeof(float)*5);
	}

	///////////////////////////////计算频率质心和带宽/////////////////////////
	for( i = 0; i < 513; ++i )
	{
		sc += i*magn[i]*magn[i];
	}
	sc = sc / energy;
	self->sc = sc;
	for( i = 0; i < 513; ++i )
	{
		bw += (i-sc)*(i-sc)*magn[i]*magn[i];
	}
	bw = bw / energy;
	self->bw = bw;

	/////////////////////////////新的语音帧判断算法////////////////////////////
	if( (sc > 20 && sc < 100) && (bw > 100 && bw < 5000) )
	{//语音帧可能性很大，只要energys_ex分布合理
		if( energys_ex[0] >= LOWFREQENCYENERGY_THRES )
		{
			float max = 0;
			for( i = 1; i < 5; ++i )
			{
				if( max < energys_ex[i] / energys_ex[0] )
					max = energys_ex[i] / energys_ex[0];
			}
			if( max <= 1.f )
			{
				if( energy > 1e11 )
					self->frametype = 0;
				else
					self->frametype = 5;
			}
			else if( max > 1.f && max < 20 )
				self->frametype = 0;
			else if( max >= 20 && max <= 50 )
				self->frametype = 2;
			else 
				self->frametype = 1;
		}
		else
		{//应该好好考虑，可能存在啸叫
			if( energy < NOISEENERGY_THRES )
				self->frametype = 3;
			else if( energys_ex[0] >= NOISEENERGY_THRES )
				self->frametype = 5;
			else
				self->frametype = 4;
		}
	}
	else if( (bw >= 3000 && bw <= 10000) && sc >= 80 )
	{//噪声的可能性大，也可能是啸叫+语音
		if( energy < NOISEENERGY_THRES )
			self->frametype = 3;
		else
		{
			float max = 0;
			for( i = 1; i < 5; ++i )
			{
				if( max < energys_ex[i] / energys_ex[0] )
					max = energys_ex[i] / energys_ex[0];
			}

			if( energys_ex[0] >= 5e11 && energys_ex[0] < 1e14 )
			{
				if( max < 3.f )
					self->frametype = 0;
				else if( max >= 5.f )
					self->frametype = 2;
				else
					self->frametype = 4;
			}
			else if( energys_ex[0] > 1e9 && max > 50.f )
				self->frametype = 1;
			else if( max > 300 )
				self->frametype = 1;
			else
				self->frametype = 4;
		}
	}
	else if( bw > 10000 && sc > 80 )
	{//噪声可能性大
		float max = 0;
		for( i = 1; i < 5; ++i )
		{
			if( max < energys_ex[i] / energys_ex[0] )
				max = energys_ex[i] / energys_ex[0];
		}
		if( max < 20 && energy < 5e10 )
			self->frametype = 3;
		else
			self->frametype = 4;
	}
	else if( bw < 3000 && sc >= 120 )
	{//极可能是啸叫点
		float max = 0;
		for( i = 1; i < 5; ++i )
		{
			if( max < energys_ex[i] / energys_ex[0] )
				max = energys_ex[i] / energys_ex[0];
		}
		if( max > 50 )
			self->frametype = 1;
		else
			self->frametype = 4;
	}
	else if( (sc > 20 && sc < 50) && (bw > 50 && bw <= 100) )
	{//极可能是低频语音
		if( energys_ex[0] + energys_ex[1] > 1e11 && energy <= 1e13 )
			self->frametype = 0;
		else
			self->frametype = 4;
	}
	else
	{//可能是啸叫点或噪声
		float max = 0;
		for( i = 1; i < 5; ++i )
		{
			if( max < energys_ex[i] / energys_ex[0] )
				max = energys_ex[i] / energys_ex[0];
		}
		if( max > 200 )
			self->frametype = 1;
		else
			self->frametype = 4;
	}

	//////////////////////////////////////////////////////////////////////////
	if( self->frametype == 1 || self->frametype == 2 )
	{//初始化相关啸叫计数器
		memset( self->bands_howl_count, 0, sizeof(int)*5 );
		if( energys_ex[1]/energys_ex[0] > 40.f )
			self->bands_howl_count[1] = 2;
		if( energys_ex[2]/energys_ex[0] > 40.f )
			self->bands_howl_count[2] = 2;
		if( energys_ex[3]/energys_ex[0] > 40.f )
			self->bands_howl_count[3] = 3;
		if( energys_ex[4]/energys_ex[0] > 40.f )
			self->bands_howl_count[4] = 4;
	}
	else if( self->frametype == 4 )
	{//初始化相关啸叫计数器
		memset( self->bands_howl_count, 0, sizeof(int)*5 );
		if( energys_ex[0] < 5e7 )
		{
			if( energys_ex[1]/energys_ex[0] > 200.f )
				self->bands_howl_count[1] = 2;
			if( energys_ex[2]/energys_ex[0] > 200.f )
				self->bands_howl_count[2] = 2;
			if( energys_ex[3]/energys_ex[0] > 200.f )
				self->bands_howl_count[3] = 2;
			if( energys_ex[4]/energys_ex[0] > 200.f )
				self->bands_howl_count[4] = 3;
		}
		else if( energys_ex[0] < 5e9 )
		{
			if( energys_ex[1]/energys_ex[0] > 100.f )
				self->bands_howl_count[1] = 2;
			if( energys_ex[2]/energys_ex[0] > 100.f )
				self->bands_howl_count[2] = 2;
			if( energys_ex[3]/energys_ex[0] > 100.f )
				self->bands_howl_count[3] = 2;
			if( energys_ex[4]/energys_ex[0] > 100.f )
				self->bands_howl_count[4] = 3;
		}
		else
		{
			if( energys_ex[1]/energys_ex[0] > 50.f )
				self->bands_howl_count[1] = 2;
			if( energys_ex[2]/energys_ex[0] > 50.f )
				self->bands_howl_count[2] = 2;
			if( energys_ex[3]/energys_ex[0] > 50.f )
				self->bands_howl_count[3] = 2;
			if( energys_ex[4]/energys_ex[0] > 50.f )
				self->bands_howl_count[4] = 3;
		}
	}
	else if( self->frametype == 0 )
	{
		memset(self->bands_howl_count, 0, sizeof(int)*5);
		self->reset = 1;
	}

	for( i =  1; i < 5; ++i )
	{
		for( j = 0; j < self->bands_howl_count[i]; ++j )
		{//
			ind  = -1;
			peak = 0.f;
			ibeg = self->bands_ex[i-1];
			iend = self->bands_ex[i];
			for( k = ibeg; k < iend; ++k )
			{
				if( peak < magn[k] && magn[k] > SPECTRUM_PEAK_MAGNITUDE )
				{
					for( t = 0; t < peak_cnt; ++t )
					{
						if( k >= peak_inter_l[t] && k <= peak_inter_r[t] )
						{
							break;
						}
					}
					if( t == peak_cnt )
					{
						peak = magn[k];
						ind  = k;
					}
				}
			}
			if( ind == -1 )
			{
				break;
			}
			else
			{
				peak_magn[peak_cnt] = peak;
				peak_pin[peak_cnt]  = ind;

				//计算峰值区间
				peak_inter_l[peak_cnt] = ind;
				peak_inter_r[peak_cnt] = ind;

				//确定左边界
				left = ibeg;
				for( k = 0; k < peak_cnt; ++k )
				{
					if( ind > peak_inter_r[k] && left < peak_inter_r[k] )
						left = peak_inter_r[k];
				}

				//搜索确定左边界点
				for( k = ind; k > left; --k)
				{
					if( magn[k] * 1.5f < magn[k-1] )
						break;
					else
						peak_inter_l[peak_cnt] = k-1;
				}

				//确定右边界
				right = iend;
				for( k = 0; k < peak_cnt; ++k )
				{
					if( ind < peak_inter_l[k] && right > peak_inter_l[k] )
						right = peak_inter_l[k];
				}

				//搜索确定右边界
				for( k = ind; k < right; ++k )
				{
					if( magn[k] * 1.5f < magn[k+1] )
						break;
					else
						peak_inter_r[peak_cnt] = k+1;
				}

				peak_cnt++;
			}
		}
	}

	for( i = 0; i < peak_cnt; ++i )
	{
		for( k = peak_inter_l[i]; k < peak_pin[i]; ++k )
		{
			if( magn[k] >= peak_magn[i]*0.5f )
			{
				peak_inter_l[i] = k;
				break;
			}
		}
		for( k = peak_inter_r[i]; k > peak_pin[i]; --k )
		{
			if( magn[k] >= peak_magn[i] * 0.5f )
			{
				peak_inter_r[i] = k;
				break;
			}
		}
	}

	*peak_count = peak_cnt;
	
	return 0;
}

//
int static RemoveRepeatHowlingPins( AcousticfeedbackSupressionC* self )
{
	int i, j;

	if( self == NULL || self->initFlag != 1 )
		return -1;

	for( i = 0; i < self->howling_count_curr; ++i )
	{
		int pin = self->howling_pin_curr[i];
		int num = self->howling_num_curr[i];
		int freeze = self->howling_frezee_curr[i];
		int offset = self->howling_offset_curr[i];
		for( j = i + 1; j < self->howling_count_curr; ++j )
		{
			/*
			if( pin == self->howling_pin_curr[j]
			    && num == self->howling_num_curr[j]
				&& freeze == self->howling_frezee_curr[j]
			    && offset == self->howling_offset_curr[j] )
				{
					self->howling_count_curr    -= 1;
					self->howling_pin_curr[j]    = self->howling_pin_curr[self->howling_count_curr];
					self->howling_num_curr[j]    = self->howling_num_curr[self->howling_count_curr];
					self->howling_frezee_curr[j] = self->howling_frezee_curr[self->howling_count_curr];
					self->howling_offset_curr[j] = self->howling_offset_curr[self->howling_count_curr];
					self->howling_magn_curr[j]   = self->howling_magn_curr[self->howling_count_curr];
					
					j -= 1;
				}
			*/
			if( pin == self->howling_pin_curr[j] 
			    && freeze == self->howling_frezee_curr[j]
				&& offset == self->howling_offset_curr[j] )
				{
					if( num >= self->howling_num_curr[j] )
					{
						self->howling_count_curr    -= 1;
						self->howling_pin_curr[j]    = self->howling_pin_curr[self->howling_count_curr];
						self->howling_num_curr[j]    = self->howling_num_curr[self->howling_count_curr];
						self->howling_frezee_curr[j] = self->howling_frezee_curr[self->howling_count_curr];
						self->howling_offset_curr[j] = self->howling_offset_curr[self->howling_count_curr];
						self->howling_magn_curr[j]   = self->howling_magn_curr[self->howling_count_curr];

						j -= 1;
					}
					else
					{
						num = self->howling_num_curr[j];
						self->howling_magn_curr[i] = self->howling_magn_curr[j];

						self->howling_count_curr    -= 1;
						self->howling_pin_curr[j]    = self->howling_pin_curr[self->howling_count_curr];
						self->howling_num_curr[j]    = self->howling_num_curr[self->howling_count_curr];
						self->howling_frezee_curr[j] = self->howling_frezee_curr[self->howling_count_curr];
						self->howling_offset_curr[j] = self->howling_offset_curr[self->howling_count_curr];
						self->howling_magn_curr[j]   = self->howling_magn_curr[self->howling_count_curr];

						j -= 1;
					}
				}
		}
	}

	return 0;
}


int static ComputePapr( AcousticfeedbackSupressionC* self,
	                    const float* magn,
						const float* peak_magn,
						const int* peak_pin,
						const int* peak_inter_l,
						const int* peak_inter_r,
						int peak_count,
						float* papr,
						int* real_peak_count
						)
{
	int i, k, m;
	int leak_count = 0;
	float fpav = 0.0f;
	float e = 0.f;
	float v = 0.f;
	float energy_peak = 0.f;
	float energy_avg  = 0.f;
	
	if( self == NULL || self->initFlag != 1 )
		return -1;

	//计算峰值功率比（Peak-to-Average Power Ratio）
	for( i = self->start_pin; i < self->magnLen; ++i )
		fpav += magn[i]*magn[i];

	if( peak_count == 0 )
	{
		*real_peak_count = 0;
		self->pav        = fpav;
		return 0;
	}

	for( i = 0; i < peak_count; ++i )
	{
		energy_peak += peak_magn[i]*peak_magn[i];
		for( k = peak_inter_l[i]; k <= peak_inter_r[i]; ++k )
		{
			e += magn[k]*magn[k];
		}
		leak_count += (peak_inter_r[i] - peak_inter_l[i] + 1);
	}
	
	fpav -= e;

	//////////////////////////////////////////////////////////////////////////
	for( i = 0; i < peak_count; ++i )
	{
		k = peak_inter_l[i];
		for( m = 0; m < peak_count; ++m )
		{
			if( k == peak_inter_r[m] )
			{ 
				leak_count--;
				fpav += magn[k]*magn[k];
				break;
			}
		}
	}

	for( i = 0; i < peak_count; ++i )
	{
		k = peak_inter_l[i];
		for( m = k; m < self->start_pin; ++m )
		{
			leak_count--;
			fpav += magn[m]*magn[m];
		}
	}


	fpav /= (self->magnLen - leak_count - self->start_pin);
	fpav += 0.00000001f;

	*real_peak_count = 0;
	for( i = 0; i < peak_count; ++i )
	{
		papr[i] = 10.f*log10(peak_magn[i]*peak_magn[i] / fpav);
		//if( ( papr[i] > 0.f || (papr[i] > -5.0f && peak_magn[i] > 5000.f) ) )
		//	(*real_peak_count)++;
	}
	*real_peak_count = peak_count;
	self->pav = fpav;
	
	return 0;
}

//
//
//
int static GetPinTypeEx( int pin,
	                     float magn_prev,
	                     const float* magn,
						 float pav,
						 int* real_pin )
{
	//0 for freeze pin.
	//1 for noise or error.
	//2 for true peak

	// freeze judge firstly

	float ff, ff1, ff3;
	if( magn[pin] <= 3000 )
		return 0;

	ff = 10.f*log10(magn[pin]*magn[pin] / pav);
	if( ff <= -10.f )
		return 0;

	if( ff > 10.f )
	{
		*real_pin = pin;
		return 2;
	}
	
	if( magn[pin-1] >= magn[pin+1] )
	{
		ff1 = 10.f*log10(magn[pin-1]*magn[pin-1] / pav);
		if( (magn_prev <= magn[pin-1] || magn_prev - magn[pin-1] < 0.5f * magn_prev) &&  ff1 > 10.f )
		{
			*real_pin = pin-1;
			return 2;
		}
	}
	else
	{
		ff3 = 10.f*log10(magn[pin+1]*magn[pin+1] / pav);
		if( (magn_prev <= magn[pin+1] || magn_prev - magn[pin+1] < 0.5f * magn_prev) &&  ff3 > 10.f )
		{
			*real_pin = pin+1;
			return 2;
		}
	}

	return 1;
}


//////////////////////////////////////////////////////////////////////////
//decide the frequency pin type
//
// Inputs:
//        *|pin| is the frequency pin in frequency domain.
//        *|magn| is the magnitude in the frequency domain. 
//        *|peak_pin| is the peak frequency pin.
//        *|peak_count| is the num of peak in frequency domain.
//        *|pav| is the left energy equals to total energy substract num of peak_count peak's energy. 
// Outputs:
//        *|real_pin| is the real true pin.
//
// return value
//       -1 for not key pin, noise
//        0 for freeze pin, lowest magn or par very low
//        1 for pseduo peak pin
//        2 for true peak pin
int static GetPinType(int pin, 
	                  const float* magn, 
					  const int* peak_pin,
					  int peak_count,
					  float pav,
					  int* real_pin)
{
	//-1 for not key pin, noise
	// 0 for freeze pin, lowest magn or par very low
	// 1 for pseduo peak pin
	// 2 for true peak pin	
	int i;
	float ff, ff1, ff3;
	for( i = 0; i < peak_count; ++i )
	{
		if( peak_pin[i] == pin )
			return 1;
	}
	
	if( magn[pin] <= 1000 )
		return 0;

	ff = 10.f*log10(magn[pin]*magn[pin] / pav);
	if( ff <= -15.f )
		return 0;

	//
	ff1 = 10.f*log10(magn[pin-1]*magn[pin-1] / pav);
	ff3 = 10.f*log10(magn[pin+1]*magn[pin+1] / pav);

	if( ff1 >= ff3 && ff1 >= 10.f && magn[pin-1] > LEAST_PEAK_MAGNITUDE )
	{
		*real_pin = pin - 1;
		return 2;
	}

	if( ff3 >= ff1 && ff3 >= 10.f && magn[pin+1] > LEAST_PEAK_MAGNITUDE )
	{
		*real_pin = pin + 1;
		return 2;
	}

	return -1;
}

int static UpdateHowlingDataEx( AcousticfeedbackSupressionC* self,
	                            const float* magn,
	                            const float* peak_magn,
	                            const int* peak_pin,
								const int* peak_inter_l,
								const int* peak_inter_r,
	                            const int peak_count,
	                            const int real_peak_count )
{
	int i, j;
	float pav;
	int find0, find1, find2;
	float ff, ff1, ff3;

	if( self == NULL || self->initFlag != 1 )
		return -1;

	if( self->frametype == 0 )
		self->howling_testcount = 0;

	pav = self->pav;
	if( self->howling_testcount == 0 )
	{//没有测试啸叫点，启动状态，赋值及初始化
		for( i = 0; i < real_peak_count; ++i )
		{
			if( magn[peak_pin[i] - 1] >= SPECTRUM_PEAK_MAGNITUDE )
			{
				self->howling_magn_curr[self->howling_count_curr]   = magn[peak_pin[i] - 1];
				self->howling_pin_curr[self->howling_count_curr]    = peak_pin[i] - 1;
				self->howling_num_curr[self->howling_count_curr]    = 1;
				self->howling_frezee_curr[self->howling_count_curr] = 0;
				self->howling_offset_curr[self->howling_count_curr] = 0;
				self->howling_count_curr++;
				self->howling_testcount++;
			}

			self->howling_magn_curr[self->howling_count_curr]   = peak_magn[i];
			self->howling_pin_curr[self->howling_count_curr]    = peak_pin[i];
			self->howling_num_curr[self->howling_count_curr]    = 1;
			self->howling_frezee_curr[self->howling_count_curr] = 0;
			self->howling_offset_curr[self->howling_count_curr] = 0;
			self->howling_count_curr++;
			self->howling_testcount++;

			if ( magn[peak_pin[i] + 1] >= SPECTRUM_PEAK_MAGNITUDE )
			{
				self->howling_magn_curr[self->howling_count_curr]   = magn[peak_pin[i] + 1];
				self->howling_pin_curr[self->howling_count_curr]    = peak_pin[i] + 1;
				self->howling_num_curr[self->howling_count_curr]    = 1;
				self->howling_frezee_curr[self->howling_count_curr] = 0;
				self->howling_offset_curr[self->howling_count_curr] = 0;
				self->howling_count_curr++;
				self->howling_testcount++;
			}
		}
	}
	else
	{
		if( real_peak_count > 0 )
		{//验证新的啸叫点
			for( i = 0; i < real_peak_count; ++i )
			{
				//for peak_pin[i]-1
				find1 = 0;
				for( j = 0; j < self->howling_count_prev; ++j )
				{
					if( peak_pin[i] - 1 == self->howling_pin_prev[j] )
					{
						//self->howling_magn_curr[self->howling_count_curr]   = magn[peak_pin[i] - 1];
						self->howling_magn_curr[self->howling_count_curr]   = 0.9f*self->howling_magn_prev[self->howling_count_curr] + 0.1f*magn[peak_pin[i] - 1];
						self->howling_pin_curr[self->howling_count_curr]    = peak_pin[i] - 1;
						if( self->howling_frezee_prev[j] >= 5 )
							self->howling_num_curr[self->howling_count_curr]    = self->howling_num_prev[j] + 2;
						else
							self->howling_num_curr[self->howling_count_curr]    = self->howling_num_prev[j] + 1;
						self->howling_frezee_curr[self->howling_count_curr] = 0;
						self->howling_offset_curr[self->howling_count_curr] = self->howling_offset_prev[j];
						self->howling_frezee_prev[j] = -1;

						self->howling_count_curr++;
						find1++;
					}
				}


				//for peak_pin[i]
				find0 = 0;
				for( j = 0; j < self->howling_count_prev; ++j )
				{
					if( peak_pin[i] == self->howling_pin_prev[j] )
					{
						//self->howling_magn_curr[self->howling_count_curr]   = peak_magn[i];
						self->howling_magn_curr[self->howling_count_curr]   = 0.9f*self->howling_magn_prev[self->howling_count_curr] + 0.1f*peak_magn[i];
						self->howling_pin_curr[self->howling_count_curr]    = peak_pin[i];
						if( self->howling_frezee_prev[j] >= 5 )
							self->howling_num_curr[self->howling_count_curr]    = self->howling_num_prev[j] + 2;
						else
							self->howling_num_curr[self->howling_count_curr]    = self->howling_num_prev[j] + 1;
						self->howling_frezee_curr[self->howling_count_curr] = 0;
						self->howling_offset_curr[self->howling_count_curr] = self->howling_offset_prev[j];
						self->howling_frezee_prev[j] = -1;

						self->howling_count_curr++;
						find0++;
					}
				}

				//for peak_pin[i]+1
				find2 = 0;
				for( j = 0; j < self->howling_count_prev; ++j )
				{
					if( peak_pin[i] + 1 == self->howling_pin_prev[j] )
					{
						//self->howling_magn_curr[self->howling_count_curr]   = magn[peak_pin[i] + 1];
						self->howling_magn_curr[self->howling_count_curr]   = 0.9f*self->howling_magn_prev[self->howling_count_curr] + 0.1f*magn[peak_pin[i] + 1];
						self->howling_pin_curr[self->howling_count_curr]    = peak_pin[i] + 1;
						if( self->howling_frezee_prev[j] >= 5 )
							self->howling_num_curr[self->howling_count_curr]    = self->howling_num_prev[j] + 2;
						else
							self->howling_num_curr[self->howling_count_curr]    = self->howling_num_prev[j] + 1;
						self->howling_frezee_curr[self->howling_count_curr] = 0;
						self->howling_offset_curr[self->howling_count_curr] = self->howling_offset_prev[j];
						self->howling_frezee_prev[j] = -1;

						self->howling_count_curr++;
						find2++;
					}
				}

				if( find1 == 0 /*&& magn[peak_pin[i] - 1] >= SPECTRUM_PEAK_MAGNITUDE*/ )
				{
					self->howling_magn_curr[self->howling_count_curr]   = magn[peak_pin[i] - 1];
					self->howling_pin_curr[self->howling_count_curr]    = peak_pin[i] - 1;
					self->howling_num_curr[self->howling_count_curr]    = 1;
					self->howling_frezee_curr[self->howling_count_curr] = 0;
					self->howling_offset_curr[self->howling_count_curr] = 0;
					self->howling_count_curr++;
				}

				if( find0 == 0 )
				{
					self->howling_magn_curr[self->howling_count_curr]   = peak_magn[i];
					self->howling_pin_curr[self->howling_count_curr]    = peak_pin[i];
					self->howling_num_curr[self->howling_count_curr]    = 1;
					self->howling_frezee_curr[self->howling_count_curr] = 0;
					self->howling_offset_curr[self->howling_count_curr] = 0;
					self->howling_count_curr++;
				}

				if( find2 == 0 /*&& magn[peak_pin[i] + 1] >= SPECTRUM_PEAK_MAGNITUDE*/ )
				{
					self->howling_magn_curr[self->howling_count_curr]   = magn[peak_pin[i] + 1];
					self->howling_pin_curr[self->howling_count_curr]    = peak_pin[i] + 1;
					self->howling_num_curr[self->howling_count_curr]    = 1;
					self->howling_frezee_curr[self->howling_count_curr] = 0;
					self->howling_offset_curr[self->howling_count_curr] = 0;
					self->howling_count_curr++;
				}
			}

			//对剩下的所有频点进行甄别，主要是静音幅度、噪声甄别
			for( j = 0; j < self->howling_count_prev; ++j )
			{
				if( self->howling_frezee_prev[j] != -1 )
				{
					// for freeze pin, process specially
					// freeze judge firstly
					int pin      = self->howling_pin_prev[j];
					float magn_p = self->howling_magn_prev[j];
					ff = 10.f*log10(magn[pin]*magn[pin] / pav);

					if( magn[pin] < magn_p*0.25 || ff < -5.f )
					{
						self->howling_magn_curr[self->howling_count_curr]   = 0.9f * magn_p + 0.1f * magn[pin];
						self->howling_pin_curr[self->howling_count_curr]    = pin;
						self->howling_num_curr[self->howling_count_curr]    = self->howling_num_prev[j];
						self->howling_frezee_curr[self->howling_count_curr] = self->howling_frezee_prev[j] + 1;
						self->howling_offset_curr[self->howling_count_curr] = self->howling_offset_prev[j];
						self->howling_count_curr++;
						continue;
					}

					
					if( ff > 10.f || (magn_p < magn[pin] && self->howling_frezee_prev[j] >= 5) )
					{
						self->howling_magn_curr[self->howling_count_curr]   = 0.9f * magn_p + 0.1f * magn[pin];
						self->howling_pin_curr[self->howling_count_curr]    = pin;
						self->howling_num_curr[self->howling_count_curr]    = self->howling_num_prev[j] + 1;
						self->howling_frezee_curr[self->howling_count_curr] = 0;
						self->howling_offset_curr[self->howling_count_curr] = self->howling_offset_prev[j];
						self->howling_count_curr++;
					}

					ff1 = 10.f*log10(magn[pin-1]*magn[pin-1] / pav);
					if( /*(magn_p <= magn[pin-1] || magn_p - magn[pin-1] < 0.5f * magn_p) &&*/  ff1 > 5.f )
					{
						self->howling_magn_curr[self->howling_count_curr]   = 0.9f * magn_p + 0.1f * magn[pin-1];
						self->howling_pin_curr[self->howling_count_curr]    = pin-1;
						self->howling_num_curr[self->howling_count_curr]    = self->howling_num_prev[j] + 1;
						self->howling_frezee_curr[self->howling_count_curr] = 0;
						self->howling_offset_curr[self->howling_count_curr] = self->howling_offset_prev[j] - 1;
						self->howling_count_curr++;
					}

					ff3 = 10.f*log10(magn[pin+1]*magn[pin+1] / pav);
					if( /*(magn_p <= magn[pin+1] || magn_p - magn[pin+1] < 0.5f * magn_p) &&*/  ff3 > 5.f )
					{
						self->howling_magn_curr[self->howling_count_curr]   = 0.9f * magn_p + 0.1f * magn[pin+1];
						self->howling_pin_curr[self->howling_count_curr]    = pin+1;
						self->howling_num_curr[self->howling_count_curr]    = self->howling_num_prev[j] + 1;
						self->howling_frezee_curr[self->howling_count_curr] = 0;
						self->howling_offset_curr[self->howling_count_curr] = self->howling_offset_prev[j] + 1;
						self->howling_count_curr++;
					}
				}
			}
		}
		else
		{
			//没有新的啸叫点，需要对之前的啸叫点进行静音幅度判断
			if( self->frametype == 3 || self->frametype == 5 )
			{
				for( i = 0; i < self->howling_count_prev; ++i )
				{	
					int pin      = self->howling_pin_prev[i];
				    float magn_p = self->howling_magn_prev[i];
					self->howling_magn_curr[self->howling_count_curr]   = 0.9f * magn_p + 0.1f * magn[pin];
					self->howling_pin_curr[self->howling_count_curr]    = pin;
					self->howling_num_curr[self->howling_count_curr]    = self->howling_num_prev[i];
					self->howling_frezee_curr[self->howling_count_curr] = self->howling_frezee_prev[i] + 1;
					self->howling_offset_curr[self->howling_count_curr] = self->howling_offset_prev[i];
					self->howling_count_curr++;
				}

			}
			else
			{
				for( i = 0; i < self->howling_count_prev; ++i )
				{	
					int pin      = self->howling_pin_prev[i];
					float magn_p = self->howling_magn_prev[i];
					ff = 10.f*log10(magn[pin]*magn[pin] / pav);

					if( magn[pin] < magn_p*0.25 || ff < -15.f )
					{
						self->howling_magn_curr[self->howling_count_curr]   = 0.9f * magn_p + 0.1f * magn[pin];
						self->howling_pin_curr[self->howling_count_curr]    = pin;
						self->howling_num_curr[self->howling_count_curr]    = self->howling_num_prev[i];
						self->howling_frezee_curr[self->howling_count_curr] = self->howling_frezee_prev[i] + 1;
						self->howling_offset_curr[self->howling_count_curr] = self->howling_offset_prev[i];
						self->howling_count_curr++;
					}
				}
			}
		}
	}

	return 0;
}



/////////////////////////////////////////////////////////////////////////////////////////////
// update the howling data, using previous howling data and current peak statistic magnitude information. 
//
// Inputs:
//        *|magn| is the magnitude in the frequency domain.
//        *|peak_magn| is the magnitude of the peak (in magnitude) in the frequency domain.
//        *|peak_pin| is the frequency pin of the peaks.
//        *|peak_count| is the num of peak.
//        *|real_peak_count| is the num of real peak which satisfied some conditions.
//        *|pav| is the left energy equals to total energy substract num of peak_count peak's energy.
// Outputs:
//        update the self's current howling data.
//
int static UpdateHowlingData( AcousticfeedbackSupressionC* self,
	                          const float* magn,
	                          const float* peak_magn,
							  const int* peak_pin,
							  const int peak_count,
							  const int real_peak_count,
							  const float pav
							  )
{
	int i, j;
	int find;
	int ty;
	int real_pin;

	if( self == NULL || self->initFlag != 1 )
		return -1;

	if( self->howling_testcount == 0 )
	{//没有测试啸叫点，启动状态，赋值及初始化
		for( i = 0; i < real_peak_count; ++i )
		{
			self->howling_magn_curr[i]   = peak_magn[i];
			self->howling_pin_curr[i]    = peak_pin[i];
			self->howling_num_curr[i]    = 1;
			self->howling_frezee_curr[i] = 0;
			self->howling_offset_curr[i] = 0;
			self->howling_count_curr++;
			self->howling_testcount++;
		}
	}
	else
	{
		if( real_peak_count > 0 )
		{//验证新的啸叫点
			for( i = 0; i < real_peak_count; ++i )
			{
				find = 0;
				//for peak_pin[i]
				for( j = 0; j < self->howling_count_prev; ++j )
				{
					if( peak_pin[i] == self->howling_pin_prev[j] )
					{
						self->howling_magn_curr[self->howling_count_curr]   = peak_magn[i];
						self->howling_pin_curr[self->howling_count_curr]    = peak_pin[i];
						self->howling_num_curr[self->howling_count_curr]    = self->howling_num_prev[j] + 1;
						self->howling_frezee_curr[self->howling_count_curr] = 0;
						self->howling_offset_curr[self->howling_count_curr] = self->howling_offset_prev[j];
						self->howling_frezee_prev[j] = -1;

						self->howling_count_curr++;
						find++;
					}
				}

				//for peak_pin[i]-1
				for( j = 0; j < self->howling_count_prev; ++j )
				{
					if( peak_pin[i] - 1 == self->howling_pin_prev[j] )
					{
						self->howling_magn_curr[self->howling_count_curr]   = peak_magn[i];
						self->howling_pin_curr[self->howling_count_curr]    = peak_pin[i];
						self->howling_num_curr[self->howling_count_curr]    = self->howling_num_prev[j] + 1;
						self->howling_frezee_curr[self->howling_count_curr] = 0;
						self->howling_offset_curr[self->howling_count_curr] = self->howling_offset_prev[j] + 1;
						self->howling_frezee_prev[j] = -1;

						self->howling_count_curr++;
						find++;
					}
				}

				//for peak_pin[i]+1
				for( j = 0; j < self->howling_count_prev; ++j )
				{
					if( peak_pin[i] + 1 == self->howling_pin_prev[j] )
					{
						self->howling_magn_curr[self->howling_count_curr]   = peak_magn[i];
						self->howling_pin_curr[self->howling_count_curr]    = peak_pin[i];
						self->howling_num_curr[self->howling_count_curr]    = self->howling_num_prev[j] + 1;
						self->howling_frezee_curr[self->howling_count_curr] = 0;
						self->howling_offset_curr[self->howling_count_curr] = self->howling_offset_prev[j] - 1;
						self->howling_frezee_prev[j] = -1;

						self->howling_count_curr++;
						find++;
					}
				}


				//if( 1 ) 所有点都加入到啸叫测试序列中去
				if( find == 0 )
				{
					self->howling_magn_curr[self->howling_count_curr]   = peak_magn[i];
					self->howling_pin_curr[self->howling_count_curr]    = peak_pin[i];
					self->howling_num_curr[self->howling_count_curr]    = 1;
					self->howling_frezee_curr[self->howling_count_curr] = 0;
					self->howling_offset_curr[self->howling_count_curr] = 0;
					self->howling_count_curr++;
				}
			}

			//对剩下的所有频点进行甄别，主要是伪峰值、静音幅度、噪声甄别
			for( j = 0; j < self->howling_count_prev; ++j )
			{
				if( self->howling_frezee_prev[j] != -1 )
				{
					//int real_pin;
					ty = GetPinType(self->howling_pin_prev[j], magn, peak_pin, peak_count, pav, &real_pin);
					if( ty == 1 )
					{//当前频点是伪峰值, 不执行num+1，应该对伪峰值的阈值进一步讨论！！！！！！现在的算法太简单
						self->howling_magn_curr[self->howling_count_curr]   = magn[self->howling_pin_prev[j]];
						self->howling_pin_curr[self->howling_count_curr]    = self->howling_pin_prev[j];
						self->howling_num_curr[self->howling_count_curr]    = self->howling_num_prev[j];
						self->howling_frezee_curr[self->howling_count_curr] = 0;
						self->howling_offset_curr[self->howling_count_curr] = self->howling_offset_prev[j];
						self->howling_count_curr++;
					}
					else if( ty == 0 )
					{//当前频点是静音频点
						self->howling_magn_curr[self->howling_count_curr]   = magn[self->howling_pin_prev[j]];
						self->howling_pin_curr[self->howling_count_curr]    = self->howling_pin_prev[j];
						self->howling_num_curr[self->howling_count_curr]    = self->howling_num_prev[j];
						self->howling_frezee_curr[self->howling_count_curr] = self->howling_frezee_prev[j] + 1;
						self->howling_offset_curr[self->howling_count_curr] = self->howling_offset_prev[j];
						self->howling_count_curr++;
					}
					else if( ty == 2 )
					{//当前频点是真峰值
						self->howling_magn_curr[self->howling_count_curr]   = magn[real_pin];
						self->howling_pin_curr[self->howling_count_curr]    = real_pin;
						self->howling_num_curr[self->howling_count_curr]    = self->howling_num_prev[j]+1;
						self->howling_frezee_curr[self->howling_count_curr] = 0;
						self->howling_offset_curr[self->howling_count_curr] = self->howling_offset_prev[j] + (real_pin - self->howling_pin_prev[j]);
						self->howling_count_curr++;
					}
				}
			}
		}
		else
		{
			//没有新的啸叫点，需要对之前的啸叫点进行静音幅度判断
			for( i = 0; i < self->howling_count_prev; ++i )
			{
				//int real_pin;
				ty = GetPinType(self->howling_pin_prev[i], magn, peak_pin, peak_count, pav, &real_pin);
				if( ty == 1 )
				{//当前频点是伪峰值, 不再执行num + 1
					self->howling_magn_curr[self->howling_count_curr]   = magn[self->howling_pin_prev[i]];
					self->howling_pin_curr[self->howling_count_curr]    = self->howling_pin_prev[i];
					self->howling_num_curr[self->howling_count_curr]    = self->howling_num_prev[i];
					self->howling_frezee_curr[self->howling_count_curr] = 0;
					self->howling_offset_curr[self->howling_count_curr] = self->howling_offset_prev[i];
					self->howling_count_curr++;
				}
				else if( ty == 0 )
				{//当前频点是静音频点
					self->howling_magn_curr[self->howling_count_curr]   = magn[self->howling_pin_prev[i]];
					self->howling_pin_curr[self->howling_count_curr]    = self->howling_pin_prev[i];
					self->howling_num_curr[self->howling_count_curr]    = self->howling_num_prev[i];
					self->howling_frezee_curr[self->howling_count_curr] = self->howling_frezee_prev[i] + 1;
					self->howling_offset_curr[self->howling_count_curr] = self->howling_offset_prev[i];
					self->howling_count_curr++;
				}
			}
		}
	}

	return 0;
}

//////////////////////////////////////////////////////////////////////////
// judge the howling pin using the howling data.
// 
// Inputs:
//       *|peak_magn| is the magnitude of the peak (in magnitude) in the frequency domain.
//       *|peak_pin| is the frequency pin of the peaks.
//       *|real_peak_count|is the num of real peak which satisfied some conditions.
// Outputs:
//       update self's howlingpin and howlingcount.
//
int static HowlingPinJudge( AcousticfeedbackSupressionC* self,
	                        const float* peak_magn,
							const int* peak_pin,
							const int real_peak_count
						   )
{
	int i;

	self->howlingpincount = 0;
	memset(self->howlingpin, 0, sizeof(int)*513);

	if( self == NULL || self->initFlag != 1 )
		return -1;

	for( i = 0; i < self->howling_count_curr; ++i )
	{
		if( self->howling_offset_curr[i] > 2 || self->howling_offset_curr[i] < -2 )
		{//首先移除偏移量过大的频点(暂时定为+2\-2)
			if( i == self->howling_count_curr - 1 )
			{
				self->howling_count_curr -= 1;
				break;
			}
			else
			{
				self->howling_count_curr -= 1;
				self->howling_magn_curr[i]   = self->howling_magn_curr[self->howling_count_curr];
				self->howling_pin_curr[i]    = self->howling_pin_curr[self->howling_count_curr];
				self->howling_num_curr[i]    = self->howling_num_curr[self->howling_count_curr];
				self->howling_frezee_curr[i] = self->howling_frezee_curr[self->howling_count_curr];
				self->howling_offset_curr[i] = self->howling_offset_curr[self->howling_count_curr];
				
				i -= 1;
				continue;
			}
		}

		
		if( self->howling_num_curr[i] >= 3 && self->howling_frezee_curr[i] < 50 )
		{//确认啸叫点
			self->howlingpin[self->howling_pin_curr[i]] += self->howling_num_curr[i];
		}

		
		if( self->howling_num_curr[i] >= 2 && self->howling_frezee_curr[i] > self->howling_num_curr[i] + 10 )
		{//啸叫点已确认，但静音持续时间过长，已超过持续帧数 + 10
			if( i == self->howling_count_curr - 1 )
			{
				self->howling_count_curr -= 1;
				break;
			}
			else
			{
				self->howling_count_curr -= 1;
				self->howling_magn_curr[i]   = self->howling_magn_curr[self->howling_count_curr];
				self->howling_pin_curr[i]    = self->howling_pin_curr[self->howling_count_curr];
				self->howling_num_curr[i]    = self->howling_num_curr[self->howling_count_curr];
				self->howling_frezee_curr[i] = self->howling_frezee_curr[self->howling_count_curr];

				i -= 1;
				continue;
			}
		}

		if( self->howling_frezee_curr[i] >= 50 || self->howling_num_curr[i] >= 20 )
		{//静音时间太长应删除该频点，或者该帧峰值数超过20（此时该频点已加入历史啸叫队列）
			if( i == self->howling_count_curr - 1 )
			{
				self->howling_count_curr -= 1;
				break;
			}
			else
			{
				self->howling_count_curr -= 1;
				self->howling_magn_curr[i]   = self->howling_magn_curr[self->howling_count_curr];
				self->howling_pin_curr[i]    = self->howling_pin_curr[self->howling_count_curr];
				self->howling_num_curr[i]    = self->howling_num_curr[self->howling_count_curr];
				self->howling_frezee_curr[i] = self->howling_frezee_curr[self->howling_count_curr];
				
				i -= 1;
			}
		}
	}

	////////////////////////////////////////////////////////////////////////
	//如果连续3帧语音，则所有啸叫信息重置，重新进行啸叫侦测
	if( self->frametype != 0 )
	{
		for( i = 0; i < 513; ++i )
		{
			if( self->howlingpin[i] >= 20 )
			{
				self->howling_history[i] = 1;
			}
		}
		for( i = 0; i < real_peak_count; ++i )
		{
			if( self->howlingpin[peak_pin[i]] > 0 )
				continue;

			if( self->howling_history[peak_pin[i]] == 1 )
			{
				self->howlingpin[peak_pin[i]] = 1;
			}
		}
	}
	else
	{
		HowlingFPoolRemoveAllFilter((HowlingFilterPool*)(self->filterPool));
		memset(self->howling_history, 0, sizeof(int)*513);
		self->howling_history_count = 0;
		self->reset = 1;
	}
	/*
	if( self->speech < 3 )
	{
		for( i = 0; i < 513; ++i )
		{
			if( self->howlingpin[i] >= 20 )
			{
				self->howling_history[i] = 1;
			}
		}
		for( i = 0; i < real_peak_count; ++i )
		{
			if( self->howlingpin[peak_pin[i]] > 0 )
				continue;

			if( self->howling_history[peak_pin[i]] == 1 )
			{
				self->howlingpin[peak_pin[i]] = 1;
			}
		}
	}
	else
	{
		HowlingFPoolRemoveAllFilter((HowlingFilterPool*)(self->filterPool));
		memset(self->howling_history, 0, sizeof(int)*513);
		self->howling_history_count = 0;
		self->reset = 1;
	}
	*/

	return 0;
}


int static HowlingPinJudgeEx( AcousticfeedbackSupressionC* self,
	                        int* howlingpin,
	                        int* howlingcount)
{
	//not use now!
	int i, j, k;
	int find;
	int num[30]  = {0};

	if( self == NULL || self->initFlag != 1 )
		return -1;

	*howlingcount = 0;
	
	for( i = 0; i < self->howling_count_curr; ++i )
	{
		//首先移除偏移量过大的频点(暂时定为+2\-2)
		if( self->howling_offset_curr[i] > 2 || self->howling_offset_curr[i] < -2 )
		{
			if( i == self->howling_count_curr - 1 )
			{
				self->howling_count_curr -= 1;
				break;
			}
			else
			{
				self->howling_magn_curr[i]   = self->howling_magn_curr[self->howling_count_curr - 1];
				self->howling_pin_curr[i]    = self->howling_pin_curr[self->howling_count_curr - 1];
				self->howling_num_curr[i]    = self->howling_num_curr[self->howling_count_curr - 1];
				self->howling_frezee_curr[i] = self->howling_frezee_curr[self->howling_count_curr - 1];
				self->howling_offset_curr[i] = self->howling_offset_curr[self->howling_count_curr - 1];

				self->howling_count_curr -= 1;
				i -= 1;

				continue;
			}
		}

		if( self->howling_num_curr[i] >= 5 && self->howling_frezee_curr[i] <= 50 )
		{
			if( *howlingcount < 30 )
			{
				find = 0;
				for( k = 0; k < *howlingcount; ++k )
				{
					if( howlingpin[k] == self->howling_pin_curr[i] )
					{
						find++;
						num[k] += self->howling_num_curr[i];
						break;
					}
				}
				if( find == 0 )
				{
					howlingpin[*howlingcount] = self->howling_pin_curr[i];
					num[*howlingcount] = self->howling_num_curr[i];
					*howlingcount++;
				}
			}
		}

		if( self->howling_frezee_curr[i] > 50 )
		{//静音时间太长，删除该频点
			if( i == self->howling_count_curr - 1 )
			{
				self->howling_count_curr -= 1;
				break;
			}
			else
			{
				self->howling_magn_curr[i]   = self->howling_magn_curr[self->howling_count_curr - 1];
				self->howling_pin_curr[i]    = self->howling_pin_curr[self->howling_count_curr - 1];
				self->howling_num_curr[i]    = self->howling_num_curr[self->howling_count_curr - 1];
				self->howling_frezee_curr[i] = self->howling_frezee_curr[self->howling_count_curr - 1];

				self->howling_count_curr -= 1;
				i -= 1;
			}
		}
	}

	/////////////////////////////////////////////////
	//newly added, update history howling pins
	for( i = 0; i < *howlingcount; ++i )
	{
		if( num[i] >= 20 )
		{
			find = 0;
			for( j = 0; j < self->howling_history_count; ++j )
			{
				if( howlingpin[i] == self->howling_history[j] )
				{
					find = 1;
					break;
				}
			}
			if( find == 0 )
			{
				self->howling_history[self->howling_history_count] = howlingpin[i];
				self->howling_history_count++;
			}
		}
	}
	//end of added

	return 0;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////
int YTXAfs_CreateCore( AcousticfeedbackSupressionC** self )
{
	*self = (AcousticfeedbackSupressionC*)malloc( sizeof(AcousticfeedbackSupressionC) );
	if( *self != NULL )
	{
		(*self)->fs       = 0;
		(*self)->initFlag = 0;
		(*self)->blockInd = 0;
		(*self)->groupInd = 0;
		(*self)->bmute    = 0;
		(*self)->reset    = 1;
		(*self)->rc_alpha = 0.8f;
		(*self)->filterPool = NULL;
		
		return 0;
	}

	return -1;
}

int YTXAfs_InitCore( AcousticfeedbackSupressionC* self, uint32_t fs )
{
	HowlingFilterPool* HFPool = NULL;

	if( self == NULL )
		return -1;

	if( fs == 8000 || fs == 16000 )
		self->fs = fs;
	else
		return -1;

	if( fs == 8000 )
	{
		memset(self->dataBuf, 0, sizeof(float) * AFSEX_ANAL_BLOCKL_MAX);
		memset(self->datamem, 0, sizeof(float) * AFSEX_ANAL_BLOCKL_MAX);
		memset(self->magn, 0, sizeof(float) * HALF_AFSEX_ANAL_BLOCKL);

		self->anaLen   = 1024;
		self->magnLen  = 513;
		self->blockLen = 80;
		self->window   = kBlocks960w1024;

		self->start_pin= 64;  //512*500/4000 (500Hz)
		self->end_pin  = 511; //512*4000/4000 (4.0KHz)

		self->blockgroup = 12; //once 12 blocks up to a group, to do fft
	}
	else if( fs == 16000 )
	{
		memset(self->dataBuf, 0, sizeof(float) * AFSEX_ANAL_BLOCKL_MAX);
		memset(self->datamem, 0, sizeof(float) * AFSEX_ANAL_BLOCKL_MAX);
		memset(self->magn, 0, sizeof(float) * HALF_AFSEX_ANAL_BLOCKL);
		self->anaLen   = 1024;
		self->magnLen  = 513;
		self->blockLen = 160;
		self->window   = kBlocks960w1024;

		self->start_pin= 32;  //512*500/8000 (500Hz)
		self->mid_pin  = 64;  //512*1000/8000 (1000Hz)
		self->end_pin  = 511; //512*8000/8000 (8000Hz)

		self->blockgroup = 6; //once 6 blocks up to a group, to do fft

		self->bands_ex[0] = 32;
		self->bands_ex[1] = 64;
		self->bands_ex[2] = 128;
		self->bands_ex[3] = 256;
		self->bands_ex[4] = 512;

		self->bands_howl_count[0] = 0;  //0hz~500hz
		self->bands_howl_count[1] = 2;  //500hz~1Khz
		self->bands_howl_count[2] = 2;  //1Khz~2Khz
		self->bands_howl_count[3] = 3;  //2Khz~4Khz
		self->bands_howl_count[4] = 4;  //4Khz~8Khz
	}

	//
	if( HowlingFPoolCreate( &HFPool ) == -1 )
		return -1;

	self->filterPool = HFPool;
	self->bmute      = 0;
	self->reset      = 1;
	self->rc_alpha   = 0.9f;
	self->energy_mute= 325*325*160;

	//init fft arrays
	self->ip[0]      = 0;
	self->blockInd   = 0;
	self->groupInd   = 0;
	self->peak_maxcount = 8;
	self->peak_lowcount = 2;

	//newly added 
	memset( self->howling_magn_buf, 0, sizeof(float)*2048 );
	memset( self->howling_pin_buf, 0, sizeof(int)*2048 );
	memset( self->howling_num_buf, 0, sizeof(int)*2048 );
	memset( self->howling_frezee_buf, 0, sizeof(int)*2048 );
	memset( self->howling_offset_buf, 0, sizeof(int)*2048 );

	self->howling_count_prev  = 0;
	self->howling_magn_prev   = self->howling_magn_buf;
	self->howling_pin_prev    = self->howling_pin_buf;
	self->howling_num_prev    = self->howling_num_buf;
	self->howling_frezee_prev = self->howling_frezee_buf;
	self->howling_offset_prev = self->howling_offset_buf;

	self->howling_count_curr  = 0;
	self->howling_magn_curr   = self->howling_magn_buf + 1024;
	self->howling_pin_curr    = self->howling_pin_buf + 1024;
	self->howling_num_curr    = self->howling_num_buf + 1024;
	self->howling_frezee_curr = self->howling_frezee_buf + 1024;
	self->howling_offset_curr = self->howling_offset_buf + 1024;

	//////////////////////////////////////////////////////////////////////////
	memset( self->howling_history, 0, sizeof(int)*513 );
	self->howling_history_count = 0;
	memset( self->howlingpin, 0, sizeof(int)*513 );
	self->howlingpincount   = 0;

//	memset( self->energys, 0, sizeof(float)*12 );
// 	self->bands[0] = 13;
// 	self->bands[1] = 26;
// 	self->bands[2] = 39;
// 	self->bands[3] = 51;
// 	self->bands[4] = 64;
// 	self->bands[5] = 128;
// 	self->bands[6] = 192;
// 	self->bands[7] = 256;
// 	self->bands[8] = 320;
// 	self->bands[9] = 384;
// 	self->bands[10]= 448;
// 	self->bands[11]= 512;
//	self->speech = 0;

	memset( self->energys_ex, 0, sizeof(float)*5);

	self->howling_testcount = 0;
	//end of newly added

	self->initFlag   = 1;
	self->pav        = 0.f;

	return 0;
}


int YTXAfs_AnalyzeCore( AcousticfeedbackSupressionC* self, 
	                    const float* inframe,
					    FILE* logfile )
{
	//
	float windata[AFSEX_ANAL_BLOCKL_MAX];
	//float magn[HALF_AFSEX_ANAL_BLOCKL];
	float real[HALF_AFSEX_ANAL_BLOCKL];
	float imag[HALF_AFSEX_ANAL_BLOCKL];
	//
	float peak_magn[12];
	int   peak_pin[12];
	int   peak_inter_l[12];  //峰值区间左端点
	int   peak_inter_r[12];  //峰值区间右端点
	int   peak_count = 10;
	float papr[12];
	int   real_count;

	float* ftmp;
	int* itmp;
	int i;

	if( self == NULL || self->initFlag != 1 )
		return -1;

	if( Silence(inframe, self->blockLen) == 1 )
	{
		self->bmute = 1;  //静音
		return 0;
	}

	self->bmute = 0;      //非静音
	memcpy( self->datamem + self->blockInd * self->blockLen, inframe, sizeof(float) * self->blockLen );
	self->blockInd++;

	if( self->blockInd == self->blockgroup )
	{
		self->groupInd++;

		//do FFT
		UpdateBuffer( self->datamem, self->blockLen * self->blockgroup, self->anaLen, self->dataBuf );
		Windowing( self->window, self->dataBuf, self->anaLen, windata );
		FFT( self, windata, self->anaLen, self->magnLen, real, imag, self->magn );

		//statics spectrum 
		SpectrumPeak( self, self->magn, peak_magn, peak_pin, peak_inter_l, peak_inter_r, &peak_count );
		//computepapr
		ComputePapr( self, self->magn, peak_magn, peak_pin, peak_inter_l, peak_inter_r, peak_count, papr, &real_count );

		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/////////////////////////////howling frequency pin detection realization(core algorithm)//////////////////////////
		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//logfile write information
		if( logfile )
		{
			fprintf(logfile, "****time = [%.4f, %.4f] fs = %d  bw = %.4f, sc = %.4f, peak_cnt = %d***************************\r\n", (self->groupInd-1)*self->blockgroup / 100.0f, self->groupInd*self->blockgroup / 100.0f, self->fs, self->bw, self->sc, peak_count );

			for( i = 0; i < self->howling_count_prev; ++i )
			{
				fprintf(logfile, "PREV for %d : magn = %.2f\t, pin = %d(%.2f)\t, num = %d\t, freeze = %d\t, offset = %d\r\n", i, self->howling_magn_prev[i],
					self->howling_pin_prev[i], self->howling_pin_prev[i]*self->fs / 1024.f, self->howling_num_prev[i], self->howling_frezee_prev[i], self->howling_offset_prev[i]);
			}
			fprintf(logfile, "\r\n");

			for( i = 0; i < 5; ++i )
			{
				fprintf(logfile, "Bands for %d : energys = %.2f\t, T = %.2f\r\n", i, self->energys_ex[i], self->energys_ex[i]/ self->energys_ex[0] );
			}
			fprintf(logfile, "\r\n");
			
			if( self->frametype == 0 )
				fprintf(logfile, "xxxxxxxxxxxxx 语音帧xxxxxxxxxxxxx\r\n" );
			else if( self->frametype == 1 )
				fprintf(logfile, "xxxxxxxxxxxxx 啸叫帧xxxxxxxxxxxxx\r\n" );
			else if( self->frametype == 2 )
				fprintf(logfile, "xxxxxxxxxxxxx 语音+啸叫帧xxxxxxxxxxxxx\r\n" );
			else if( self->frametype == 3 )
				fprintf(logfile, "xxxxxxxxxxxxx 噪声帧xxxxxxxxxxxxx\r\n" );
			else if( self->frametype == 4 )
				fprintf(logfile, "xxxxxxxxxxxxx 未知帧xxxxxxxxxxxxx\r\n" );
			else if( self->frametype == 5 )
				fprintf(logfile, "xxxxxxxxxxxxx 背景噪声xxxxxxxxxxxxx\r\n" );
			fprintf(logfile, "\r\n");


			for( i = 0; i < peak_count; ++i )
			{
				fprintf(logfile, "PEAK for %d : magn = %.2f\t, pin = %d(%.2f)\t, %.4f\r\n", i, peak_magn[i], peak_pin[i], peak_pin[i]*self->fs / 1024.f, papr[i]);
			}
			fprintf(logfile, "\r\n");
		}

		//update the howling data info
		UpdateHowlingDataEx( self, self->magn, peak_magn, peak_pin, peak_inter_l, peak_inter_r, peak_count, real_count );
		RemoveRepeatHowlingPins(self);

		if( logfile )
		{
			for( i = 0; i < self->howling_count_curr; ++i )
			{
				fprintf(logfile, "CURR for %d : magn = %.2f\t, pin = %d(%.2f)\t, num = %d\t, freeze = %d\t, offset = %d\r\n", i, self->howling_magn_curr[i],
					self->howling_pin_curr[i], self->howling_pin_curr[i]*self->fs / 1024.f, self->howling_num_curr[i], self->howling_frezee_curr[i], self->howling_offset_curr[i]);
			}
			fprintf(logfile, "\r\n");
		}

		//judge the howling pins
		HowlingPinJudge(self, peak_magn, peak_pin, real_count);

		if ( logfile )
		{
			fprintf(logfile, "RESULT>>>>>>\r\n");
			for( i = 0; i < self->howlingpincount; ++i )
			{
				fprintf(logfile, "RESULT for %d : pin =%d\t(%.2f)\r\n", i, self->howlingpin[i], self->howlingpin[i]*self->fs / 1024.f);
			}
			for( i = 0; i < self->howling_history_count; ++i )
			{
				fprintf(logfile, "HISTORY for %d : pin =%d\t(%.2f)\r\n", i, self->howling_history[i], self->howling_history[i]*self->fs / 1024.f);
			}
			fprintf(logfile, "<<<<<<RESULT\r\n");
		}

		//交换并更新啸叫数据(已验证通过)
		ftmp = self->howling_magn_curr;
		self->howling_magn_curr = self->howling_magn_prev;
		self->howling_magn_prev = ftmp;

		itmp = self->howling_pin_curr;
		self->howling_pin_curr = self->howling_pin_prev;
		self->howling_pin_prev = itmp;

		itmp = self->howling_num_curr;
		self->howling_num_curr = self->howling_num_prev;
		self->howling_num_prev = itmp;

		itmp = self->howling_frezee_curr;
		self->howling_frezee_curr = self->howling_frezee_prev;
		self->howling_frezee_prev = itmp;

		itmp = self->howling_offset_curr;
		self->howling_offset_curr = self->howling_offset_prev;
		self->howling_offset_prev = itmp;

		self->howling_count_prev = self->howling_count_curr;
		self->howling_testcount  = self->howling_count_curr;
		self->howling_count_curr = 0;

		self->blockInd = 0;
	}

	return 0;
}


int YTXAfs_ProcessCore( AcousticfeedbackSupressionC* self,
	                    const float* inframe,
	                    float* outframe )
{
	float ibuf[160];

	if( self == NULL || self->initFlag != 1 )
		return -1;

	if( self->bmute == 1 )
	{
		memcpy(outframe, inframe, sizeof(float)*self->blockLen);
		return 0;
	}
	
	HowlingFPoolMakeAndUpdateFilter( (HowlingFilterPool*)(self->filterPool), self->fs, self->howlingpin, self->howlingpincount );
	HowlingFPoolMakeAndUpdateFilter( (HowlingFilterPool*)(self->filterPool), self->fs, self->howling_history, self->howling_history_count );

	memcpy( ibuf, inframe, sizeof(float)*self->blockLen );

	HowlingFPoolProcess( (HowlingFilterPool*)(self->filterPool), 
		                 ibuf,
						 outframe,
		                 self->blockLen );

	return 0;
}

int YTXAfs_FreeCore( AcousticfeedbackSupressionC* self )
{
	if( self != NULL )
	{
		HowlingFPoolFree((HowlingFilterPool*)(self->filterPool));
		self->filterPool = NULL;

		//

		free(self);
		self = NULL;
	}

	return 0;
}