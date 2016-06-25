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
#include "typedefs.h"
#include "afs_defines.h"


//////////////////////////////////////////////////////////////////////////
typedef struct AcousticfeedbackSupressionC_
{
	uint32_t fs;

	float dataBuf[AFSEX_ANAL_BLOCKL_MAX];
	float datamem[AFSEX_ANAL_BLOCKL_MAX];

	const float* window;

	int anaLen;
	int magnLen;
	int blockLen;
	//
	int start_pin;     //开始频率点
	int end_pin;       //结束频率点
	int peak_maxcount; //峰值最大值,默认10个

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

	float howling_magn_buf[1024];
	int   howling_pin_buf[1024];
	int   howling_num_buf[1024];
	int   howling_frezee_buf[1024];
	int   howling_offset_buf[1024];

	int   howling_history[512];      //历史啸叫频点
	int   howling_history_count;     //历史啸叫频点数

	int   howlingpin[30];            //啸叫点
	int   howlingpincount;           //啸叫点计数
	
	int   howling_testcount;

	void* filterPool;
	//end of newly added
	
	// FFT work arrays.
	int ip[AFSEX_IP_LENGTH];
	float wfft[AFSEX_W_LENGTH];

	int blockInd;
	int blockgroup;
	int groupInd;
	int initFlag;

	//
	unsigned char* ptmp_data;

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