#include <stdio.h>
#include <stdlib.h>
#include "howling_control.h"
#include "afs_core.h"


int32_t WebRtcAfs_Create( void** AfsInst )
{
	AcousticfeedbackSupressionC* self;
	self = (AcousticfeedbackSupressionC*)malloc( sizeof(AcousticfeedbackSupressionC) );

	if( self != NULL )
	{
		self->fs       = 0;
		self->initFlag = 0;
		self->blockInd = 0;
		self->groupInd = 0;
		self->filterPool = NULL;

		*AfsInst = (void*)self;

		return 0;
	}

	return 0;
}

int32_t WebRtcAfs_Free( void* AfsInst )
{
	return YTXAfs_FreeCore( (AcousticfeedbackSupressionC*)AfsInst );
}

int32_t WebRtcAfs_Init( void* AfsInst, uint32_t fs )
{
	return YTXAfs_InitCore((AcousticfeedbackSupressionC*)AfsInst, fs);
}

int32_t WebRtcAfs_Analyze( void* AfsInst, const float* spframe, FILE* flog )
{
	return YTXAfs_AnalyzeCore( (AcousticfeedbackSupressionC*)AfsInst, spframe, flog );
}

int32_t WebRtcAfs_Process( void* AfsInst, const float* spframe, float* outframe )
{
	return YTXAfs_ProcessCore( (AcousticfeedbackSupressionC*)AfsInst, spframe, outframe );
}













































