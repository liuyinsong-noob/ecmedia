#include "audio_processing.h"
#include "module_common_types.h"

int main()
{
	cloopenwebrtc::AudioProcessing* audioProcessingModulePtr = cloopenwebrtc::AudioProcessing::Create();
  
	cloopenwebrtc::AudioFrame frame;
	frame.num_channels_ = 1;
	frame.sample_rate_hz_ = audioProcessingModulePtr->input_sample_rate_hz();

	audioProcessingModulePtr->AnalyzeReverseStream(&frame);
  
	delete audioProcessingModulePtr;
	return 0;
}