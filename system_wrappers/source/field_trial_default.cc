// Copyright (c) 2014 The WebRTC project authors. All Rights Reserved.
//
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file in the root of the source
// tree. An additional intellectual property rights grant can be found
// in the file PATENTS.  All contributing project authors may
// be found in the AUTHORS file in the root of the source tree.
//

#include "field_trial.h"

// Clients of webrtc that do not want to configure field trials can link with
// this instead of providing their own implementation.
namespace cloopenwebrtc {
namespace field_trial {

std::string FindFullName(const std::string& name) {

	if (name == "WebRTC-BitrateProbing")
	{
		return "Enabled";
	}
	
  return std::string();
}

    
int FindFullName() {
    
    return 1;
}
}  // namespace field_trial
}  // namespace webrtc
