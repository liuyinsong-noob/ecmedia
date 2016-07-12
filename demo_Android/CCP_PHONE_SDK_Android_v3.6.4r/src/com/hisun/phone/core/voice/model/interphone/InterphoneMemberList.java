package com.hisun.phone.core.voice.model.interphone;

import java.util.ArrayList;

import com.hisun.phone.core.voice.model.Response;

public class InterphoneMemberList extends Response{

	/**
	 * 实时对讲成员对象列表
	 */
	private static final long serialVersionUID = 1734519105385738377L;
	public String count;
	public ArrayList<InterphoneMember> interphoneMemberList = new ArrayList<InterphoneMember>();

}
