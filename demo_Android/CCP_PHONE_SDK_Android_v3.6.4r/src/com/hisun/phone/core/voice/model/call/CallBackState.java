/**
 * 
 */
package com.hisun.phone.core.voice.model.call;

import com.hisun.phone.core.voice.model.Response;

/**
 * @author chao
 *
 */
public class CallBackState extends Response {
	/**
	 * 
	 */
	private static final long serialVersionUID = -3497497559197847530L;
	
	public String accountSid;
	public String sipCode;
	public String to;
	public String from;
	public String callSid;
	public String dateCreated;
	public String dateUpdated;
	public String duration;
	public String endTime;
	public String price;
	public String startTime;
	// 状态，取值有0（队列），1（响铃），2（正在通话），3（已完成），4（失败），5（占线），6（没有接听），7（取消）
	public String state;
	public String uri;
}
