/* @file: howling_control.h 
*  @author: Guoqing Zeng
*  @date: 20160615
*  @description: further to encapsulate afs_core.h and howlingfilter_core.h for easy using.
*  @reference: independent research and development.
*  @version:  Newly Create! @20160615
*/

#ifndef __WEBRTC_MODULES_AUDIO_PROCESSING_HOWLING_CONTROL_H__
#define __WEBRTC_MODULES_AUDIO_PROCESSING_HOWLING_CONTROL_H__

#include "typedefs.h"

#ifdef __cplusplus
extern "C" {
#endif

	int32_t WebRtcAfs_Create( void** AfsInst );
	
	int32_t WebRtcAfs_Free( void* AfsInst );

	int32_t WebRtcAfs_Init( void* AfsInst, uint32_t fs );

	int32_t WebRtcAfs_Analyze( void* AfsInst, const float* spframe, FILE* flog );

	int32_t WebRtcAfs_Process( void* AfsInst, const float* spframe, float* outframe );


#ifdef __cplusplus
}
#endif


#endif