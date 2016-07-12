package com.hisun.phone.core.voice.model.interphone;

import com.hisun.phone.core.voice.model.Response;

public class InterphoneMember extends Response {
	/**
	 *  实时对讲成员对象
	 */
	private static final long serialVersionUID = 490792464302561100L;
	public String type;
	public String online;
	public String voipId;
	public String mic;
}
