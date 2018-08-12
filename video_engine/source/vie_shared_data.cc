/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "process_thread.h"
#include "../system_wrappers/include/cpu_info.h"
#include "../system_wrappers/include/trace.h"
#include "vie_channel_manager.h"
#include "vie_defines.h"
#include "vie_input_manager.h"
#include "vie_render_manager.h"
#ifdef ENABLE_SCREEN_SHARE
#include "vie_desktop_share_manager.h"
#endif
#include "vie_shared_data.h"


namespace yuntongxunwebrtc {

ViESharedData::ViESharedData(const Config& config)
    : number_cores_(CpuInfo::DetectNumberOfCores()),
      channel_manager_(new ViEChannelManager(0, number_cores_, config)),
      input_manager_(new ViEInputManager(0, config)),
      render_manager_(new ViERenderManager(0)),
#ifdef ENABLE_SCREEN_SHARE
	  desktop_share_manager_(new ViEDesktopShareManager(0)),
#endif
      module_process_thread_(ProcessThread::CreateProcessThread()),
    module_process_thread_pacer_(ProcessThread::CreateProcessThread()),
      last_error_(0) {
  //Trace::CreateTrace();
  channel_manager_->SetModuleProcessThread(module_process_thread_, module_process_thread_pacer_);
  input_manager_->SetModuleProcessThread(module_process_thread_);
  module_process_thread_->Start();
          module_process_thread_pacer_->Start();
}

ViESharedData::~ViESharedData() {
  // Release these ones before the process thread and the trace.
  input_manager_.reset();
  channel_manager_.reset();
  render_manager_.reset();
#ifdef ENABLE_SCREEN_SHARE
  desktop_share_manager_.reset();
#endif
  module_process_thread_->Stop();
  ProcessThread::DestroyProcessThread(module_process_thread_);
    module_process_thread_pacer_->Stop();
    ProcessThread::DestroyProcessThread(module_process_thread_pacer_);
  Trace::ReturnTrace();
}

void ViESharedData::SetLastError(const int error) const {
  last_error_ = error;
}

int ViESharedData::LastErrorInternal() const {
  int error = last_error_;
  last_error_ = 0;
  return error;
}

int ViESharedData::NumberOfCores() const {
  return number_cores_;
}

}  // namespace webrtc
