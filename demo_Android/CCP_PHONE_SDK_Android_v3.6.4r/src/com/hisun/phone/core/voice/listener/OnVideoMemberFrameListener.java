/*
 *  Copyright (c) 2013 The CCP project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a Beijing Speedtong Information Technology Co.,Ltd license
 *  that can be found in the LICENSE file in the root of the web site.
 *
 *   http://www.yuntongxun.com
 *
 *  An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */
package com.hisun.phone.core.voice.listener;

import com.hisun.phone.core.voice.model.CloopenReason;

/**
 * <p>Title: OnVideoMemberFrameListener.java</p>
 * <p>Description: </p>
 * <p>Copyright: Copyright (c) 2014</p>
 * <p>Company: Beijing Speedtong Information Technology Co.,Ltd</p>
 * @author Jorstin Chan
 * @date 2014-8-29
 * @version 1.0
 */
public interface OnVideoMemberFrameListener {

	void onPublisVideoFrameRequest(CloopenReason reason);
	void onStopVideoFrameRequest(CloopenReason reason);
}
