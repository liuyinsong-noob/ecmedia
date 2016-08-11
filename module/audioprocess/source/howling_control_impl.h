/*
 * @file: howling_control_impl.h
 * @author: Guoqing Zeng
 * @date: 20160615
 * @descripition: howling control implement
 * @version: 
 *         -Newly Create!@20160615
 *
 */
#ifndef __YTXRTC_MODULES_AUDIO_PROCESSING_HOWLING_CONTROL_IMPL_H__
#define __YTXRTC_MODULES_AUDIO_PROCESSING_HOWLING_CONTROL_IMPL_H__


#include "audio_processing.h"
#include "processing_component.h"

namespace cloopenwebrtc {

class AudioBuffer;
class CriticalSectionWrapper;

class HowlingControlImpl : public HowlingControl,
		                   public ProcessingComponent{
  public:
	  HowlingControlImpl(const AudioProcessing* apm,
		                 CriticalSectionWrapper* crit);
	  virtual ~HowlingControlImpl();

	  int AnalyzeCaptureAudio(AudioBuffer* audio);
	  int ProcessCaptureAudio(AudioBuffer* audio);

	  //
	  virtual bool is_enabled() const OVERRIDE;
  private:
	  //
	  virtual int Enable(bool enable) OVERRIDE;

	  // ProcessingComponent implementation.
	  virtual void* CreateHandle() const OVERRIDE;
	  virtual int InitializeHandle(void* handle) const OVERRIDE;
	  virtual int ConfigureHandle(void* handle) const OVERRIDE;
	  virtual void DestroyHandle(void* handle) const OVERRIDE;
	  virtual int num_handles_required() const OVERRIDE;
	  virtual int GetHandleError(void* handle) const OVERRIDE;

	  const AudioProcessing* apm_;
	  CriticalSectionWrapper* crit_;

	  FILE* filelog;  //debug used
	  FILE* file_preproc;
	  FILE* file_postproc;
};

};



#endif