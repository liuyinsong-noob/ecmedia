#include "howling_control_impl.h"

#include <assert.h>

#include "signal_processing_library.h"
#include "audio_buffer.h"
#include "critical_section_wrapper.h"
#include "typedefs.h"

#include "howling_control.h"


namespace cloopenwebrtc {

typedef void Handle;

HowlingControlImpl::HowlingControlImpl(const AudioProcessing* apm, 
		                               CriticalSectionWrapper* crit)
  : ProcessingComponent(),
	apm_(apm),
	crit_(crit) {
		filelog = NULL;//fopen("agc_preproc_add.txt", "wb");
}


HowlingControlImpl::~HowlingControlImpl() {
	if( filelog != NULL)
	{
		fclose(filelog);
		filelog = NULL;
	}
}



int HowlingControlImpl::AnalyzeCaptureAudio(AudioBuffer* audio)
{
	if (!is_component_enabled()) {
		return apm_->kNoError;
	}
	assert(audio->samples_per_split_channel() <= 160);
	assert(audio->num_channels() == num_handles());

	for (int i = 0; i < num_handles(); ++i) {
		Handle* my_handle = static_cast<Handle*>(handle(i));

		WebRtcAfs_Analyze(my_handle, audio->split_bands_const_f(i)[kBand0To8kHz], filelog);
	}

	return apm_->kNoError;
}

int HowlingControlImpl::ProcessCaptureAudio(AudioBuffer* audio)
{
	if (!is_component_enabled()) {
		return apm_->kNoError;
	}
	assert(audio->samples_per_split_channel() <= 160);
	assert(audio->num_channels() == num_handles());

	for (int i = 0; i < num_handles(); ++i) {
		Handle* my_handle = static_cast<Handle*>(handle(i));
		WebRtcAfs_Process( my_handle,
			               audio->split_bands_const_f(i)[kBand0To8kHz],
			               audio->split_bands_f(i)[kBand0To8kHz]
						  );
	}
	return apm_->kNoError;
}


int HowlingControlImpl::Enable(bool enable) {
	CriticalSectionScoped crit_scoped(crit_);
	return EnableComponent(enable);
}

bool HowlingControlImpl::is_enabled() const {
	return is_component_enabled();
}

void* HowlingControlImpl::CreateHandle() const{
	Handle* handle = NULL;

  	if (WebRtcAfs_Create(&handle) != apm_->kNoError)
  	{
  		handle = NULL;
  	} else {
  		assert(handle != NULL);
	}
  
	return handle;
}

int HowlingControlImpl::InitializeHandle(void* handle) const{
	return WebRtcAfs_Init(static_cast<Handle*>(handle), 
		apm_->proc_sample_rate_hz());
}

int HowlingControlImpl::ConfigureHandle(void* handle) const{
	return apm_->kNoError;
}

void HowlingControlImpl::DestroyHandle(void* handle) const{
	WebRtcAfs_Free(static_cast<Handle*>(handle));
}

int HowlingControlImpl::num_handles_required() const{
	return apm_->num_output_channels();
}

int HowlingControlImpl::GetHandleError(void* handle) const{
	assert(handle != NULL);
	return apm_->kUnspecifiedError;
}

}
