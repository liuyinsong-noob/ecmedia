#include "afs_core.h"
#include "fft4g.h"
#include <math.h>
#include <assert.h>
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
	//-45 dBFS = 185
	int i;
	for( i = 0; i < frame_length; ++i )
	{
		if( frame[i] > 185 || frame[i] < -185 )
			return 0;
	}

	return 1;
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
int static SpectrumPeak( AcousticfeedbackSupressionC* self,
	                      const float* magn,
						  float* peak_magn,
						  int* peak_pin,
						  int* peak_inter_l,
						  int* peak_inter_r,
						  int* peak_count)
{
	int i, j, k;
	int left, right;

	if( self == NULL || self->initFlag != 1 )
		return -1;

	//statics spectrum info
	*peak_count = self->peak_maxcount;
	for( i = 0; i < self->peak_maxcount; ++i )
	{
		peak_magn[i] = 0;
		peak_pin[i] = -1;
		for( j = self->start_pin; j <= self->end_pin; ++j )
		{
			if( peak_magn[i] < magn[j] )
			{
				int k = 0;
				for( k = 0; k < i; ++k )
				{
					if( j >= peak_inter_l[k] && j <= peak_inter_r[k] )
					{
						break;
					}
				}
				if( k == i )
				{
					peak_magn[i] = magn[j];
					peak_pin[i] = j;
				}
			}
		}

		if( peak_pin[i] == -1 )
		{
			*peak_count = i - 1;
			break;
		}
		//限制峰值最小值，至少大于peak_magn[0]的0.05
		// 			if( peak_magn[i] < peak_magn[0]*0.05f )
		// 			{
		// 				self->peak_count = i - 1;
		// 				break;
		// 			}

		//计算峰值区间
		peak_inter_l[i] = peak_pin[i];
		peak_inter_r[i] = peak_pin[i];

		//确定左边界
		left = 0;
		for( k = 0; k < i; ++k )
		{
			if( peak_pin[i] > peak_inter_r[k] && left < peak_inter_r[k] )
				left = peak_inter_r[k];
		}


		//搜索确定左边界点
		for( k = peak_inter_l[i]; k > left; --k)
		{
			if( magn[k] * 1.5f < magn[k-1] )
				break;
			else
				peak_inter_l[i] = k-1;
		}

		//确定右边界
		right = self->magnLen;
		for( k = 0; k < i; ++k )
		{
			if( peak_pin[i] < peak_inter_l[k] && right > peak_inter_l[k] )
				right = peak_inter_l[k];
		}

		//搜索确定右边界
		for( k = peak_inter_r[i]; k < right; ++k )
		{
			if( magn[k] * 1.5f < magn[k+1] )
				break;
			else
				peak_inter_r[i] = k+1;
		}
	}

	//重新修正左右边界,有待进一步验证
	for( i = 0; i < *peak_count; ++i )
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

	return 0;
}


//////////////////////////////////////////////////////////////////////////
// compute papr（ 计算峰值功率比（Peak-to-Average Power Ratio）)
//
// Input:
//      *|magn| is the magnitude of frequency domain.
//      *|peak_magn| is the peak magnitude value in the frequency domain.
//      *|peak_inter_l| is the left edge of the peak interval.
//      *|peak_inter_r| is the right edge of the peak interval.
//      *|peak_count| is the num of the peaks in the frequency domain.
//
// Output:
//      *|papr| is the peak-to-average power ratio.
//      *|real_count| is the real num of the papr.
//      *|pav| is the left energy equals to total energy substract num of peak_count peak's energy.
//
int static ComputePapr( AcousticfeedbackSupressionC* self,
	                    const float* magn,
						const float* peak_magn,
						const int* peak_inter_l,
						const int* peak_inter_r,
						int peak_count,
						float* papr,
						int* real_peak_count,
						float* pav,
						FILE* flog
						)
{
	int i, k, m;
	int leak_count = 0;
	float fpav = 0.0f;
	//float fpav2= 0.0f; 

	if( self == NULL || self->initFlag != 1 )
		return -1;

	//计算峰值功率比（Peak-to-Average Power Ratio）
	for( i = 0; i < self->magnLen; ++i )
	{
		fpav += magn[i]*magn[i];
	}
	
	
	for( i = 0; i < peak_count; ++i )
	{
		for( k = peak_inter_l[i]; k <= peak_inter_r[i]; ++k )
		{
			fpav -= magn[k]*magn[k];
		}
		leak_count += (peak_inter_r[i] - peak_inter_l[i] + 1);
	}

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

	fpav /= (self->magnLen - leak_count);
	fpav += 0.00000001f;

	*real_peak_count = 0;
	for( i = 0; i < peak_count; ++i )
	{
		papr[i] = 10.f*log10(peak_magn[i]*peak_magn[i] / fpav);
		if( ( papr[i] > 0.f || (papr[i] > -10.0f && peak_magn[i] > 10000.f) ) && peak_magn[i] > 2000.f )
			(*real_peak_count)++;
	}

	*pav = fpav;

	return 0;
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
	int howlingcount = 0;
	int num[30]  = {0};
	int i, j, k;
	int find;

	self->howlingpincount = 0;

	if( self == NULL || self->initFlag != 1 )
		return -1;

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

		if( self->howling_num_curr[i] >= 3 && self->howling_frezee_curr[i] <= 50 )
		{
			if( howlingcount < 30 )
			{
				find = 0;
				for( k = 0; k < howlingcount; ++k )
				{
					if( self->howlingpin[k] == self->howling_pin_curr[i] )
					{
						find = 1;
						num[k] += self->howling_num_curr[i];
						break;
					}
				}
				if( find == 0 )
				{
					self->howlingpin[howlingcount] = self->howling_pin_curr[i];
					num[howlingcount] = self->howling_num_curr[i];
					howlingcount++;
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
	for( i = 0; i < howlingcount; ++i )
	{
		if( num[i] >= 20 )
		{
			find = 0;
			for( j = 0; j < self->howling_history_count; ++j )
			{
				if( self->howlingpin[i] == self->howling_history[j] )
				{
					find = 1;
					break;
				}
			}
			if( find == 0 )
			{
				self->howling_history[self->howling_history_count] = self->howlingpin[i];
				self->howling_history_count++;
			}
		}
	}
	//end of added
	
	//参照历史啸叫点，进一步完善当前的啸叫检测结果
	for( i = 0; i < real_peak_count; ++i )
	{
		find = 0;
		for( j = 0; j < howlingcount; ++j )
		{
			if( self->howlingpin[j] == peak_pin[i] )
			{
				find = 1;
				break;
			}
		}

		if( find == 0 && peak_magn[i] > 5000.f )
		{
			for( j = 0; j < self->howling_history_count; ++j )
			{
				if( peak_pin[i] == self->howling_history[j] 
				|| peak_pin[i] == self->howling_history[j] - 1 
					|| peak_pin[i] == self->howling_history[j] + 1 )
				{
					if( howlingcount < 30 )
					{
						self->howlingpin[howlingcount] = peak_pin[i];
						howlingcount++;
					}
					break;
				}
			}
		}
	}
	
	self->howlingpincount = howlingcount;

	return 0;
}


int static HowlingPinJudgeEx( AcousticfeedbackSupressionC* self,
	                        int* howlingpin,
	                        int* howlingcount)
{
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

		if( self->howling_num_curr[i] >= 3 && self->howling_frezee_curr[i] <= 50 )
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
		(*self)->filterPool = NULL;
		(*self)->ptmp_data  = NULL;

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
		self->anaLen   = 512;
		self->magnLen  = 257;
		self->blockLen = 80;
		self->window   = NULL;
	}
	else if( fs == 16000 )
	{
		memset(self->dataBuf, 0, sizeof(float) * AFSEX_ANAL_BLOCKL_MAX);
		memset(self->datamem, 0, sizeof(float) * AFSEX_ANAL_BLOCKL_MAX);
		self->anaLen   = 1024;
		self->magnLen  = 513;
		self->blockLen = 160;
		self->window   = kBlocks960w1024;

		self->start_pin= 64; //512*1000/8000 (1.0KHz)
		self->end_pin  = 480;//512*7500/8000 (7.5KHz)
	}

	//
	self->ptmp_data = (unsigned char*)malloc(4096*1000);
	memset(self->ptmp_data, 0, 4096*1000);

	//
	if( HowlingFPoolCreate( &HFPool ) == -1 )
		return -1;

	self->filterPool = HFPool;


	//init fft arrays
	self->ip[0]      = 0;
	self->blockInd   = 0;
	self->blockgroup = 6; //once 6 blocks up to a group, to do fft
	self->groupInd   = 0;
	self->peak_maxcount = 10;

	//newly added 
	memset( self->howling_magn_buf, 0, sizeof(float)*1024 );
	memset( self->howling_pin_buf, 0, sizeof(int)*1024 );
	memset( self->howling_num_buf, 0, sizeof(int)*1024 );
	memset( self->howling_frezee_buf, 0, sizeof(int)*1024 );
	memset( self->howling_offset_buf, 0, sizeof(int)*1024 );

	self->howling_count_prev  = 0;
	self->howling_magn_prev   = self->howling_magn_buf;
	self->howling_pin_prev    = self->howling_pin_buf;
	self->howling_num_prev    = self->howling_num_buf;
	self->howling_frezee_prev = self->howling_frezee_buf;
	self->howling_offset_prev = self->howling_offset_buf;

	self->howling_count_curr  = 0;
	self->howling_magn_curr   = self->howling_magn_buf + 512;
	self->howling_pin_curr    = self->howling_pin_buf + 512;
	self->howling_num_curr    = self->howling_num_buf + 512;
	self->howling_frezee_curr = self->howling_frezee_buf + 512;
	self->howling_offset_curr = self->howling_offset_buf + 512;

	//////////////////////////////////////////////////////////////////////////
	memset( self->howling_history, 0, sizeof(int)*512 );
	self->howling_history_count = 0;
	//
	self->howlingpincount = 0;

	self->howling_testcount = 0;
	//end of newly added

	self->initFlag   = 1;

	return 0;
}


int YTXAfs_AnalyzeCore( AcousticfeedbackSupressionC* self, 
	                    const float* inframe,
					    FILE* logfile )
{
// 	if( self == NULL || self->initFlag != 1 )
// 		return -1;
	//
	float windata[AFSEX_ANAL_BLOCKL_MAX];
	float magn[HALF_AFSEX_ANAL_BLOCKL];
	float real[HALF_AFSEX_ANAL_BLOCKL];
	float imag[HALF_AFSEX_ANAL_BLOCKL];
	//
	float peak_magn[10];
	int   peak_pin[10];
	int   peak_inter_l[10];  //峰值区间左端点
	int   peak_inter_r[10];  //峰值区间右端点
	int   peak_count = 10;
	float papr[10];
	int   real_count;
	float pav;

	float* ftmp;
	int* itmp;
	int i;

	memcpy( self->datamem + self->blockInd * self->blockLen, inframe, sizeof(float) * self->blockLen );
	self->blockInd++;

	if( self->blockInd == 6 )
	{
		self->groupInd++;

		//do FFT
		UpdateBuffer( self->datamem, self->blockLen * 6, self->anaLen, self->dataBuf );
		Windowing( self->window, self->dataBuf, self->anaLen, windata );
		FFT( self, windata, self->anaLen, self->magnLen, real, imag, magn );

		//statics spectrum info
		SpectrumPeak(self, magn, peak_magn, peak_pin, peak_inter_l, peak_inter_r, &peak_count);
		//computepapr
		ComputePapr( self, magn, peak_magn, peak_inter_l, peak_inter_r, peak_count, papr, &real_count, &pav, logfile );

		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/////////////////////////////howling frequency pin detection realization(core algorithm)//////////////////////////
		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//logfile write information
		if( logfile )
		{
			fprintf(logfile, "***************************time = [%.4f, %.4f] %d %.2f %.2f***************************\r\n", (self->groupInd-1)*0.06, self->groupInd*0.06, self->fs, magn[0], magn[self->magnLen-1]);

			for( i = 0; i < self->howling_count_prev; ++i )
			{
				fprintf(logfile, "PREV for %d : magn = %.2f\t, pin = %d(%.2f)\t, num = %d\t, freeze = %d\t, offset = %d\r\n", i, self->howling_magn_prev[i],
					self->howling_pin_prev[i], self->howling_pin_prev[i]*8000.0/512.f, self->howling_num_prev[i], self->howling_frezee_prev[i], self->howling_offset_prev[i]);
			}
			fprintf(logfile, "\r\n");


			for( i = 0; i < peak_count; ++i )
			{
				fprintf(logfile, "PEAK for %d : magn = %.2f\t, pin = %d(%.2f)\t, %.4f\r\n", i, peak_magn[i], peak_pin[i], peak_pin[i]*8000.0/512.f, papr[i]);
			}
			fprintf(logfile, "\r\n");
		}

		//update the howling data info
		UpdateHowlingData( self, magn, peak_magn, peak_pin, peak_count, real_count, pav );

		if( logfile )
		{
			for( i = 0; i < self->howling_count_curr; ++i )
			{
				fprintf(logfile, "CURR for %d : magn = %.2f\t, pin = %d(%.2f)\t, num = %d\t, freeze = %d\t, offset = %d\r\n", i, self->howling_magn_curr[i],
					self->howling_pin_curr[i], self->howling_pin_curr[i]*8000.0/512.f, self->howling_num_curr[i], self->howling_frezee_curr[i], self->howling_offset_curr[i]);
			}
			fprintf(logfile, "\r\n");
		}

		//judge the howling pins
		HowlingPinJudge(self, peak_magn, peak_pin, real_count);

		if ( logfile )
		{
			fprintf(logfile, "RESULT>>>>\r\n");
			for( i = 0; i < self->howlingpincount; ++i )
			{
				fprintf(logfile, "RESULT for %d : pin =%d\t(%.2f)\r\n", i, self->howlingpin[i], self->howlingpin[i]*8000/512.f);
			}
			for( i = 0; i < self->howling_history_count; ++i )
			{
				fprintf(logfile, "HISTORY for %d : pin =%d\t(%.2f)\r\n", i, self->howling_history[i], self->howling_history[i]*8000/512.f);
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

	//
	HowlingFPoolMakeAndUpdateFilter( (HowlingFilterPool*)(self->filterPool), 16000, self->howlingpin, self->howlingpincount );

	memcpy( ibuf, inframe, sizeof(float)*self->blockLen );

	HowlingFPoolProcess( (HowlingFilterPool*)(self->filterPool), 
		                 ibuf,
						 outframe,
		                 self->blockLen);

	//
	return 0;
}

int YTXAfs_FreeCore( AcousticfeedbackSupressionC* self )
{
	if( self != NULL )
	{
		HowlingFPoolFree((HowlingFilterPool*)(self->filterPool));
		self->filterPool = NULL;

		if( self->ptmp_data != NULL )
		{
			free(self->ptmp_data);
			self->ptmp_data = NULL;
		}
		free(self);
		self = NULL;
	}

	return 0;
}