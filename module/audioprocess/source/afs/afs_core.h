/* @ file: afs_core.h 
*  @ author: Guoqing Zeng
*  @ description: Howling detecting and howling controling, include
*                 1.  howling detector
*                 2.  trap filter pool & iir filters  
*  @ reference: independent research and development.
*  @ version: Newly Created! @20160323
*             Added iir_filter Pool function! @20160606
*             Modified APIs
*               -YTXAfs_AnalyzeCore( AcousticfeedbackSupressionCEX* self, 
*                                    const float* inframe,
*                                    FILE* logfile = NULL );
*               -YTXAfs_ProcessCore( AcousticfeedbackSupressionCEX* self,
*                                    const float* inframe,
*                                    float* outframe );
*
*/

#ifndef __AFS_CORE_H__
#define __AFS_CORE_H__

#include <stdio.h>
#include "afs_defines.h"


//////////////////////////////////////////////////////////////////////////
typedef struct AcousticfeedbackSupressionC_
{
	uint32_t fs;

	float dataBuf[AFSEX_ANAL_BLOCKL_MAX];
	float datamem[AFSEX_ANAL_BLOCKL_MAX];
	float magn[HALF_AFSEX_ANAL_BLOCKL];

	const float* window;

	int anaLen;
	int magnLen;
	int blockLen;
	//
	int start_pin;     //开始频点, low     500Hz
	int mid_pin;       //中间频点, middle  1000Hz
	int end_pin;       //结束频点, high    8000Hz

	int peak_maxcount; //峰值最大值，默认8个
	int peak_lowcount; //低频峰值个数，默认为2个 [0, 500Hz]

	//newly added
	float* howling_magn_prev;
	int*   howling_pin_prev;
	int*   howling_num_prev;
	int*   howling_frezee_prev;
	int*   howling_offset_prev;
	int    howling_count_prev;

	float* howling_magn_curr;
	int*   howling_pin_curr;
	int*   howling_num_curr;
	int*   howling_frezee_curr;
	int*   howling_offset_curr;
	int    howling_count_curr;

	float howling_magn_buf[2048];
	int   howling_pin_buf[2048];
	int   howling_num_buf[2048];
	int   howling_frezee_buf[2048];
	int   howling_offset_buf[2048];

	int   howling_history[513];      //历史啸叫频点
	int   howling_history_count;     //历史啸叫频点数

	int   howlingpin[513];           //啸叫点
	int   howlingpincount;           //啸叫点计数

	int   howling_testcount;         //

	void* filterPool;
	int   bmute;                     //是否静音

	//
	float energy;                    //总能量
	float energy_mute;               //静音能量(325)

	float rc_alpha;                  //0.8
	int   reset;                     //是否重置
	//end of newly added
	
	// FFT work arrays.
	int ip[AFSEX_IP_LENGTH];
	float wfft[AFSEX_W_LENGTH];

	int blockInd;
	int blockgroup;
	int groupInd;
	int initFlag;
	float pav;

	//
	//float energys[12];    // |  0   |   1 |   2 |   3 |  4 |  5 |  6 |  7 |  8 |  9 | 10 | 11 |
	                      // |0  200|  400|  600|  800|  1k|  2k|  3k|  4k|  5k|  6k|  7k|  8k| 
	//int bands[12];
	//int speech;
	float bw;
	float sc;

	float energys_ex[5];       //  500 | 1000 | 2000 | 4000 | 8000 | = low  mid_low  mid  mid_high  high
	int   bands_ex[5];         //   32    64     128    256    512
	int   bands_howl_count[5]; // 默认分段啸叫点个数 0  2  2  3  4
	int   frametype;           // 0 for speech
	                           // 1 for howling
	                           // 2 for speech + howling
	                           // 3 for noise
	                           // 4 for unknown 
	                           // 5 背景噪音

}AcousticfeedbackSupressionC;

int YTXAfs_CreateCore( AcousticfeedbackSupressionC** self );

int YTXAfs_InitCore( AcousticfeedbackSupressionC* self, uint32_t fs );

int YTXAfs_AnalyzeCore( AcousticfeedbackSupressionC* self, 
	                    const float* inframe,
						FILE* logfile );

int YTXAfs_ProcessCore( AcousticfeedbackSupressionC* self,
	                    const float* inframe,
						float* outframe );	

int YTXAfs_FreeCore( AcousticfeedbackSupressionC* self );

///////////////////////////////////////////////////////////////////////////////////////////


#endif