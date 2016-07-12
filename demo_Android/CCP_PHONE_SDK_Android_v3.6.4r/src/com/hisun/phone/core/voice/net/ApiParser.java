/**
 * 
 */
package com.hisun.phone.core.voice.net;

import java.io.IOException;
import java.io.InputStream;

import org.xmlpull.v1.XmlPullParser;
import org.xmlpull.v1.XmlPullParserException;
import org.xmlpull.v1.XmlPullParserFactory;

import android.text.TextUtils;

import com.hisun.phone.core.voice.exception.CCPXmlParserException;
import com.hisun.phone.core.voice.model.Response;
import com.hisun.phone.core.voice.model.UploadImessage;
import com.hisun.phone.core.voice.model.call.CallBackState;
import com.hisun.phone.core.voice.model.chatroom.ChatRoomList;
import com.hisun.phone.core.voice.model.chatroom.ChatRoomMemberList;
import com.hisun.phone.core.voice.model.chatroom.Chatroom;
import com.hisun.phone.core.voice.model.chatroom.ChatroomDismissMsg;
import com.hisun.phone.core.voice.model.chatroom.ChatroomExitMsg;
import com.hisun.phone.core.voice.model.chatroom.ChatroomJoinMsg;
import com.hisun.phone.core.voice.model.chatroom.ChatroomMember;
import com.hisun.phone.core.voice.model.chatroom.ChatroomMemberForbidOpt;
import com.hisun.phone.core.voice.model.chatroom.ChatroomMsg;
import com.hisun.phone.core.voice.model.chatroom.ChatroomMsg.ForbidOptions;
import com.hisun.phone.core.voice.model.chatroom.ChatroomRemoveMemberMsg;
import com.hisun.phone.core.voice.model.im.IMAttachedMsg;
import com.hisun.phone.core.voice.model.im.IMCooperMsg;
import com.hisun.phone.core.voice.model.im.IMDismissGroupMsg;
import com.hisun.phone.core.voice.model.im.IMInviterJoinGroupReplyMsg;
import com.hisun.phone.core.voice.model.im.IMInviterMsg;
import com.hisun.phone.core.voice.model.im.IMJoinGroupMsg;
import com.hisun.phone.core.voice.model.im.IMProposerMsg;
import com.hisun.phone.core.voice.model.im.IMQuitGroupMsg;
import com.hisun.phone.core.voice.model.im.IMRemoveMemeberMsg;
import com.hisun.phone.core.voice.model.im.IMReplyJoinGroupMsg;
import com.hisun.phone.core.voice.model.im.IMTextMsg;
import com.hisun.phone.core.voice.model.im.NewMediaMsgList;
import com.hisun.phone.core.voice.model.interphone.InterphoneControlMic;
import com.hisun.phone.core.voice.model.interphone.InterphoneControlMicMsg;
import com.hisun.phone.core.voice.model.interphone.InterphoneExitMsg;
import com.hisun.phone.core.voice.model.interphone.InterphoneInviteMsg;
import com.hisun.phone.core.voice.model.interphone.InterphoneJoinMsg;
import com.hisun.phone.core.voice.model.interphone.InterphoneMember;
import com.hisun.phone.core.voice.model.interphone.InterphoneMemberList;
import com.hisun.phone.core.voice.model.interphone.InterphoneMsg;
import com.hisun.phone.core.voice.model.interphone.InterphoneOverMsg;
import com.hisun.phone.core.voice.model.interphone.InterphoneReleaseMicMsg;
import com.hisun.phone.core.voice.model.setup.SoftSwitch;
import com.hisun.phone.core.voice.model.setup.SoftSwitch.Clpss;
import com.hisun.phone.core.voice.model.setup.SubAccount;
import com.hisun.phone.core.voice.model.videoconference.VideoConference;
import com.hisun.phone.core.voice.model.videoconference.VideoConferenceDismissMsg;
import com.hisun.phone.core.voice.model.videoconference.VideoConferenceExitMsg;
import com.hisun.phone.core.voice.model.videoconference.VideoConferenceJoinMsg;
import com.hisun.phone.core.voice.model.videoconference.VideoConferenceList;
import com.hisun.phone.core.voice.model.videoconference.VideoConferenceMember;
import com.hisun.phone.core.voice.model.videoconference.VideoConferenceMemberList;
import com.hisun.phone.core.voice.model.videoconference.VideoConferenceMsg;
import com.hisun.phone.core.voice.model.videoconference.VideoConferenceRemoveMemberMsg;
import com.hisun.phone.core.voice.model.videoconference.VideoConferenceSwitch;
import com.hisun.phone.core.voice.model.videoconference.VideoPartnerPortrait;
import com.hisun.phone.core.voice.model.videoconference.VideoPartnerPortraitList;
import com.hisun.phone.core.voice.util.VoiceUtil;
/**
 * @version 1.0.0
 */
public final class ApiParser {

	public static final int KEY_SOFTSWITCH_ADDRESS = 980100;
	public static final int KEY_NETWORK_CALLBACK = 980101;
	public static final int KEY_SUBACCOUNT = 980102;
	
	public static final int KEY_PARSER_STATUSCODE = 980106;
	public static final int KEY_MESSAGE_ARRIVED = 980107;
	
	public static final int KEY_STARTINTERPHONE = 980108;
	public static final int KEY_CONTROLMIC = 980109;
	public static final int KEY_RELEASEMIC = 980110;
	public static final int KEY_INTERPHONE_MEMBER_LIST = 980111;
	public static final int KEY_STARTCHATROOM = 980112;
	public static final int KEY_QUERY_CHATROOM_LIST = 980113;
	public static final int KEY_INVITE_CHATROOM = 980114;
	public static final int KEY_CHATROOM_MEMBER_LIST = 980115;
	// IM
	public static final int KEY_SEND_MEIDAMSG = 980116;
	public static final int KEY_MEDIAMSG_INFO = 980117;
	
	// GROUP IM PUSH
	public static final int KEY_TEXT_MESSAGE_ARRIVED = 980118;
	
	public static final int KEY_DISMISS_CHATROOM = 980119;
	public static final int KEY_REMOVE_MEMBER_CHATROOM = 980120;
	
	/**
	 * Parser xml for dismiss Video Conference
	 * @version 3.5
	 */
	public static final int KEY_DISMISS_VIDEOCONFENERCE = 980121;
	
	/**
	 * Parser xml for remove member dismiss Video Conference
	 * @version 3.5
	 */
	public static final int KEY_REMOVE_MEMBER_VIDEOCONFENERCE = 980122;
	
	/**
	 * switch Video Conference to other voip
	 * @version 3.5
	 */
	public static final int KEY_SWITCH_VIDEOCONFENERCE = 980123;
	
	/**
	 * start new Video Conference
	 * @version 3.5
	 */
	public static final int KEY_START_VIDEOCONFENERCE = 980124;
	
	/**
	 * query Video Conference list 
	 * @version 3.5
	 */
	public static final int KEY_QUERY_VIDEOCONFENERCE_LIST = 980125;

	/**
	 * invit member join Video Conference
	 * @version 3.5
	 */
	public static final int KEY_INVITE_VIDEOCONFENERCE = 980126;
	
	/**
	 * query member where in Video Conference
	 * @version 3.5
	 */
	public static final int KEY_VIDEOCONFENERCE_MEMBER_LIST = 980127;
	
	/**
	 * 
	 */
	public static final int KEY_VIDEOCONFENERCE_PORTRAIT = 980128;
	
	public static final int KEY_NETWORK_GROUPID = 980129;
	public static final int KEY_CHATROOM_SPEAK_OPREATE = 980130;
	
	public static final int KEY_REQUEST_DEFAULT = 980131;
	
	
	private static int VAR_INTERPHONE_INVITE = 201;
	private static int VAR_INTERPHONE_JOIN = 202;
	private static int VAR_INTERPHONE_EXIT = 203;
	private static int VAR_INTERPHONE_OVER = 204;
	private static int VAR_INTERPHONE_CONTROL_MIC = 205;
	private static int VAR_INTERPHONE_RELEASE_MIC = 206;
	
	private static int VAR_CHATROOM_JOIN = 301;
	private static int VAR_CHATROOM_EXIT = 302;
	private static int VAR_CHATROOM_DISMISS = 303;
	private static int VAR_CHATROOM_REMOVEMEMBER = 304;
	private static int VAR_CHATROOM_SPEAK_OPT = 305;
	
	private static int VAR_APPLY_JOIN_GROUP_MSG = 401;
	private static int VAR_ACCEPT_OR_REJECT_JOIN_GROUP_MSG = 402;
	private static int VAR_INVITE_JOIN_GROUP_MSG = 403;
	private static int VAR_DELETE_FROM_GROUP_MSG = 404;
	private static int VAR_EXIT_FROM_GROUP_MSG = 405;
	private static int VAR_DELETE_GROUP_MSG = 406;
	private static int VAR_INVITE_JOIN_GROUP_UNVALIDATION_MSG = 407;
	private static int VAR_REPLY_GROUP_APPLY = 408;
	private static int VAR_RECEIVE_INSTANCE_MSG = 501;
	private static int VAR_RECEIVE_INSTANCE_MSG_JJ = 901;
	
	
	private static int VAR_VIDEO_CONFERENCE_JOIN = 601;
	private static int VAR_VIDEO_CONFERENCE_EXIT = 602;
	private static int VAR_VIDEO_CONFERENCE_DISMISS = 603;
	private static int VAR_VIDEO_CONFERENCE_REMOVEMEMBER = 604;
	private static int VAR_VIDEO_CONFERENCE_SWITCH = 605;
	
	
	private ApiParser() {

	}

	public static Response doParser(int parseType, InputStream is)
			throws CCPXmlParserException {
		if (is == null) {
			throw new IllegalArgumentException("resource is null.");
		}
		
		XmlPullParser xmlParser = null;
		Response response = null;
		
		try {
			
			xmlParser = XmlPullParserFactory.newInstance().newPullParser();
			xmlParser.setInput(is, null);
			xmlParser.nextTag();
			
			String rootName = xmlParser.getName();
			if (!isRootNode(rootName)) {
				throw new IllegalArgumentException("xml root node is invalid.");
			}
			
			// sip��Ϣ
			if (parseType == KEY_MESSAGE_ARRIVED) {
				int var = parseReceiveMsgVar(xmlParser);
				response = getResponseByVar(var);
				parseReceiveMsg(xmlParser, response, var);
				
			}else{
				response = getResponseByKey(parseType);
				xmlParser.require(XmlPullParser.START_TAG, null, rootName);
				while (xmlParser.nextTag() != XmlPullParser.END_TAG) {
					String tagName = xmlParser.getName();
					if (tagName != null && (tagName.equals("statusCode") || tagName.equals("statuscode"))) {
						String text = xmlParser.nextText();
						response.statusCode = text;
					} else if (tagName != null && tagName.equals("statusMsg")) {
						String text = xmlParser.nextText();
						response.statusMsg = text;
					} else {
						if (parseType == KEY_SOFTSWITCH_ADDRESS) {
							parseSoftSwitchBody(xmlParser, response);
						} else if (parseType == KEY_NETWORK_GROUPID) {
							parseNetworkGroupId(xmlParser, response);
						} else if (parseType == KEY_NETWORK_CALLBACK) {
							parseCallBackBody(xmlParser, (CallBackState)response);
						} else if (parseType == KEY_SUBACCOUNT) {
							parseSubAccountBody(xmlParser, (SubAccount)response);
							
						}else if (parseType == KEY_STARTINTERPHONE) {
							parseStartInterPhone(xmlParser, response);
						} else if (parseType == KEY_CONTROLMIC) {
							parseControlMIC(xmlParser, response);
						} else if (parseType == KEY_INTERPHONE_MEMBER_LIST) {
							parseInterPhoneMemberList(xmlParser, response);
							
							// ChatRoom
						} else if (parseType == KEY_STARTCHATROOM) {
							parseStartChatRoom(xmlParser, response);
						}  else if (parseType == KEY_CHATROOM_MEMBER_LIST) {
							parseChatRoomMemberList(xmlParser, response);
						} else if (parseType == KEY_QUERY_CHATROOM_LIST) {
							parseChatRoomList(xmlParser, response);
							
						} else if (parseType == KEY_SEND_MEIDAMSG) {
							parseMultiMedia(xmlParser, (UploadImessage)response);
							
						} else if (parseType == KEY_MEDIAMSG_INFO) {
							parseReceiveMediaMsgList(xmlParser, (NewMediaMsgList)response);
							
						}  else if(parseType == KEY_TEXT_MESSAGE_ARRIVED) {
							parseReceiveGroupMessage(xmlParser,(IMTextMsg) response);
							
						}  else if (parseType == KEY_START_VIDEOCONFENERCE) {
							parseStartVideoConference(xmlParser, response);
						}  else if (parseType == KEY_VIDEOCONFENERCE_MEMBER_LIST) {
							parseVideoConferenceMemberList(xmlParser, response);
						} else if (parseType == KEY_QUERY_VIDEOCONFENERCE_LIST) {
							parseVideoConferenceList(xmlParser, response);
							
						} else if (parseType == KEY_VIDEOCONFENERCE_PORTRAIT) {
							parseVideoConferencePortraitList(xmlParser, response);
							
						} else {
							xmlParser.nextText();
						}
					}
				}
				xmlParser.require(XmlPullParser.END_TAG, null, rootName);
				xmlParser.next();
				xmlParser.require(XmlPullParser.END_DOCUMENT, null, null);
			}
			print(response);
		} catch (Exception e) {
			e.printStackTrace();
			if (response != null) {
				response.released();
				response = null;
			}
			throw new CCPXmlParserException("ApiParser.doParser parse xml occur errors:" + e.getMessage());
		} finally {
			if (is != null) {
				try {
					is.close();
				} catch (IOException e) {
					e.printStackTrace();
					is = null;
				}
			}
			xmlParser = null;
		}
		
		return response;
	}


	/**
	 * @param xmlParser
	 * @param response
	 * @throws Exception 
	 * @throws IOException 
	 * @throws XmlPullParserException 
	 */
	private static void parseNetworkGroupId(XmlPullParser xmlParser,
			Response response) throws XmlPullParserException, IOException, Exception {
		while (xmlParser.nextTag() != XmlPullParser.END_TAG) {
			String tagName = xmlParser.getName();
			SoftSwitch ss = (SoftSwitch) response;
			
			if (tagName.equals("nwgid")) {
				String text = xmlParser.nextText();
				if (text != null && !text.equals("")) {
					ss.setNetworkGroupId(text.trim());
				}
			} else {
				xmlParser.nextText();
			}
		}
	}

	private static Response getResponseByKey(int key) {
		if (key == KEY_NETWORK_CALLBACK) {
			return new CallBackState();
		} else if (key == KEY_SUBACCOUNT) {
			return new SubAccount();
		}else if (key == KEY_STARTINTERPHONE){
			return new InterphoneMsg();
		} else if (key == KEY_INTERPHONE_MEMBER_LIST){
			return new InterphoneMemberList();
		} else if(key == KEY_CONTROLMIC){
			return new InterphoneControlMic();
		} else if (key == KEY_SOFTSWITCH_ADDRESS || key == KEY_NETWORK_GROUPID){
			return new SoftSwitch();
			
			// ChatRoom 
		} else if (key == KEY_STARTCHATROOM){
			return new ChatroomMsg();
		}  else if (key == KEY_CHATROOM_MEMBER_LIST) {
			return new ChatRoomMemberList();
		} else if (key == KEY_QUERY_CHATROOM_LIST) {
			return new ChatRoomList();
			
			// im
		} else if (key == KEY_SEND_MEIDAMSG) {
			return new UploadImessage();
		} else if (key == KEY_MEDIAMSG_INFO) {
			return new NewMediaMsgList();
		} else if (key == KEY_TEXT_MESSAGE_ARRIVED) {
			return new IMTextMsg();
		
		
		} else if (key == KEY_START_VIDEOCONFENERCE) {
			return new VideoConferenceMsg();
		} else if (key == KEY_VIDEOCONFENERCE_MEMBER_LIST) {
			return new VideoConferenceMemberList();
		} else if (key == KEY_QUERY_VIDEOCONFENERCE_LIST) {
			return new VideoConferenceList();
		} else if (key == KEY_VIDEOCONFENERCE_PORTRAIT) {
			return new VideoPartnerPortraitList();
		}
		
		
		return new Response();
	}
	
	private static Response getResponseByVar(int key) {
		if (key == VAR_INTERPHONE_INVITE){
			return new InterphoneInviteMsg();
		} else if (key == VAR_INTERPHONE_JOIN){
			return new InterphoneJoinMsg();
		} else if (key == VAR_INTERPHONE_EXIT){
			return new InterphoneExitMsg();
		} else if (key == VAR_INTERPHONE_OVER){
			return new InterphoneOverMsg();
		} else if (key == VAR_INTERPHONE_CONTROL_MIC){
			return new InterphoneControlMicMsg();
		} else if (key == VAR_INTERPHONE_RELEASE_MIC){
			return new InterphoneReleaseMicMsg();
			
			// ChatRoom var .. 
		}else if (key == VAR_CHATROOM_JOIN) {
			return new ChatroomJoinMsg();
		} else if (key == VAR_CHATROOM_EXIT) {
			return new ChatroomExitMsg();
		} else if (key == VAR_CHATROOM_DISMISS){
			return new ChatroomDismissMsg();
		} else if (key == VAR_CHATROOM_REMOVEMEMBER){
			return new ChatroomRemoveMemberMsg();
		} else if (key == VAR_CHATROOM_SPEAK_OPT) {
			return new ChatroomMemberForbidOpt();
			//instance msg var
		} else if (key == VAR_APPLY_JOIN_GROUP_MSG) {
			return new IMProposerMsg();
		} else if (key == VAR_ACCEPT_OR_REJECT_JOIN_GROUP_MSG) {
			return new IMReplyJoinGroupMsg();
		} else if (key == VAR_REPLY_GROUP_APPLY) {
			return new IMInviterJoinGroupReplyMsg();
		} else if (key == VAR_INVITE_JOIN_GROUP_MSG) {
			return new IMInviterMsg();
		} else if (key == VAR_DELETE_FROM_GROUP_MSG) {
			return new IMRemoveMemeberMsg();
		} else if (key == VAR_EXIT_FROM_GROUP_MSG) {
			return new IMQuitGroupMsg();
		} else if (key == VAR_DELETE_GROUP_MSG) {
			return new IMDismissGroupMsg();
		}else if (key == VAR_INVITE_JOIN_GROUP_UNVALIDATION_MSG){
			return new IMJoinGroupMsg();
		} else if (key == VAR_RECEIVE_INSTANCE_MSG) {
			return new IMAttachedMsg();
		} else if (key == VAR_RECEIVE_INSTANCE_MSG_JJ) {
			return new IMCooperMsg();
		
		
		}else if (key == VAR_VIDEO_CONFERENCE_JOIN) {
			return new VideoConferenceJoinMsg();
		} else if (key == VAR_VIDEO_CONFERENCE_EXIT) {
			return new VideoConferenceExitMsg();
		} else if (key == VAR_VIDEO_CONFERENCE_DISMISS){
			return new VideoConferenceDismissMsg();
		} else if (key == VAR_VIDEO_CONFERENCE_REMOVEMEMBER){
			return new VideoConferenceRemoveMemberMsg();
			
		} else if (key == VAR_VIDEO_CONFERENCE_SWITCH){
			return new VideoConferenceSwitch();
			
		}
			
		return new Response();
	}
	
	private static void print(Response r) {
		if (r != null) {
			r.print();
		}
	}
	
	private static boolean isRootNode(String rootName) {
		if (rootName == null || (!rootName.equalsIgnoreCase("Response") && !rootName.equalsIgnoreCase("VoiceMessage") && !rootName.equalsIgnoreCase("InstanceMessage"))) {
			return false;
		}
		return true;
	}

	private static void parseCallBackBody(XmlPullParser xmlParser, CallBackState response) throws Exception {
		while (xmlParser.nextTag() != XmlPullParser.END_TAG) {
			String tagName = xmlParser.getName();
			if (tagName.equals("callSid")) {
				String text = xmlParser.nextText();
				if (text != null && !text.equals("")) {
					response.callSid = text;
				}
			} else if (tagName.equals("dateCreated")) {
				String text = xmlParser.nextText();
				if (text != null && !text.equals("")) {
					response.dateCreated = text;
				}
			} else {
				xmlParser.nextText();
			}
		}
	}
	
	private static void parseSoftSwitchBody(XmlPullParser xmlParser, Response response) throws Exception {
		while (xmlParser.nextTag() != XmlPullParser.END_TAG) {
			String tagName = xmlParser.getName();
			SoftSwitch ss = (SoftSwitch) response;
			
			if(!TextUtils.isEmpty(tagName) && "clpss".equals(tagName.toLowerCase())) {
				Clpss clpss = new Clpss();
				while (xmlParser.nextTag() != XmlPullParser.END_TAG) {
					parseSoftSwitchClpssBody(xmlParser, clpss);
				}
				ss.addSoftClpsses(clpss);
			} else  if (tagName.equals("p2p")) {
				String text = xmlParser.nextText();
				if (text != null && !text.equals("")) {
					ss.setP2PServerPort(text.trim());
				}
			} else if (tagName.equals("control")) {
				String text = xmlParser.nextText();
				if (text != null && !text.equals("")) {
					ss.setControl(text.trim());
				}
			}  else if (tagName.equals("nwgid")) {
				String text = xmlParser.nextText();
				if (text != null && !text.equals("")) {
					ss.setNetworkGroupId(text.trim());
				}
			} else {
				xmlParser.nextText();
			}
		}
	}

	/**
	 * @param xmlParser
	 * @param ss
	 */
	private static void parseSoftSwitchClpssBody(XmlPullParser xmlParser,
			Clpss clpss) throws Exception{
		String tagName = xmlParser.getName();
		if (tagName.equals("ip")) {
			String text = xmlParser.nextText();
			if (text != null && !text.equals("")) {
				clpss.setIp(text);
			}
		} else if (tagName.equals("port")) {
			String text = xmlParser.nextText();
			if (text != null && !text.equals("")) {
				clpss.setPort(text);
			}
		} else {
			xmlParser.nextText();
		}
	}

	private static void parseSubAccountBody(XmlPullParser xmlParser, SubAccount response) throws Exception {
		while (xmlParser.nextTag() != XmlPullParser.END_TAG) {
			String tagName = xmlParser.getName();
			if (tagName.equals("subAccountSid")) {
				String text = xmlParser.nextText();
				if (text != null && !text.equals("")) {
					response.accountSid = text.trim();
				}
			} else if (tagName.equals("appId")) {
				String text = xmlParser.nextText();
				if (text != null && !text.equals("")) {
					response.appId = text.trim();
				}
			} else if (tagName.equals("subToken")) {
				String text = xmlParser.nextText();
				if (text != null && !text.equals("")) {
					response.authToken = text.trim();
				}
			} else if (tagName.equals("dateCreated")) {
				String text = xmlParser.nextText();
				if (text != null && !text.equals("")) {
					response.dateCreated = text.trim();
				}
			} else if (tagName.equals("friendlyName")) {
				String text = xmlParser.nextText();
				if (text != null && !text.equals("")) {
					response.friendlyName = text.trim();
				}
			} else if (tagName.equals("parentAccountSid")) {
				String text = xmlParser.nextText();
				if (text != null && !text.equals("")) {
					response.parentAccountSid = text.trim();
				}
			} else if (tagName.equals("voipAccount")) {
				String text = xmlParser.nextText();
				if (text != null && !text.equals("")) {
					response.sipCode = text.trim();
				}
			} else if (tagName.equals("voipPwd")) {
				String text = xmlParser.nextText();
				if (text != null && !text.equals("")) {
					response.sipPwd = text.trim();
				}
			} else if (tagName.equals("status")) {
				String text = xmlParser.nextText();
				if (text != null && !text.equals("")) {
					response.status = text.trim();
				}
			} else if (tagName.equals("type")) {
				String text = xmlParser.nextText();
				if (text != null && !text.equals("")) {
					response.type = text.trim();
				}
			} else if (tagName.equals("uri")) {
				String text = xmlParser.nextText();
				if (text != null && !text.equals("")) {
					response.uri = text.trim();
				}
			} else if (tagName.equals("SubresourceUris")) {
				xmlParser.require(XmlPullParser.START_TAG, null, "SubresourceUris");
				while (xmlParser.nextTag() != XmlPullParser.END_TAG) {
					tagName = xmlParser.getName();
					if (tagName.equals("calls")) {
						String text = xmlParser.nextText();
						if (text != null && !text.equals("")) {
							SubAccount.SubresourceUris.calls = text.trim();
						}
					} else if (tagName.equals("conferences")) {
						String text = xmlParser.nextText();
						if (text != null && !text.equals("")) {
							SubAccount.SubresourceUris.conferences = text.trim();
						}
					} else if (tagName.equals("smsMessages")) {
						String text = xmlParser.nextText();
						if (text != null && !text.equals("")) {
							SubAccount.SubresourceUris.smsMessages = text.trim();
						}
					} 
				}
				xmlParser.require(XmlPullParser.END_TAG, null, "SubresourceUris");
			} else {
				xmlParser.nextText();
			}
		}
	}
	
	

	private static void parseStartInterPhone(XmlPullParser xmlParser, Response response) throws Exception {
		String tagName = xmlParser.getName();
		InterphoneMsg interphone = (InterphoneMsg) response;
		if (tagName != null && tagName.equals("interphoneId")) {
			String text = xmlParser.nextText();
			interphone.interphoneId = text;
		} else {
			xmlParser.nextText();
		}
	}

	private static void parseControlMIC(XmlPullParser xmlParser, Response response) throws Exception {
		InterphoneControlMic mic = (InterphoneControlMic) response;
		String tagName = xmlParser.getName();
		if (tagName != null && tagName.equals("voipAccount")) {
			String text = xmlParser.nextText();
			mic.speaker = text;
		} else {
			xmlParser.nextText();
		}
	}
	
	private static void parseInterPhoneMemberList(XmlPullParser xmlParser, Response response) throws Exception {

		InterphoneMemberList list = (InterphoneMemberList) response;
		String tagName = xmlParser.getName();
		if (tagName != null && tagName.equals("count")) {
			String text = xmlParser.nextText();
			list.count = text;
		} else if (tagName != null && tagName.equals("member")) {
			InterphoneMember member = new InterphoneMember();
			while (xmlParser.next() != XmlPullParser.END_TAG) {
				tagName = xmlParser.getName();
				if (tagName != null && tagName.equals("voipAccount")) {
					String text = xmlParser.nextText();
					member.voipId = text;
				} else if (tagName != null && tagName.equals("type")) {
					String text = xmlParser.nextText();
					member.type = text;
				} else if (tagName != null && tagName.equals("online")) {
					String text = xmlParser.nextText();
					member.online = text;
				} else if (tagName != null && tagName.equals("mic")) {
					String text = xmlParser.nextText();
					member.mic = text;
				} else {
					xmlParser.nextText();
				}
			}
			list.interphoneMemberList.add(member);
		}
	}
	
	///
	private static void parseStartChatRoom(XmlPullParser xmlParser, Response response) throws Exception {
		String tagName = xmlParser.getName();
		ChatroomMsg chatRoom = (ChatroomMsg) response;
		if (tagName != null && tagName.equals("roomId")) {
			String text = xmlParser.nextText();
			chatRoom.setRoomNo(text);
		} else {
			xmlParser.nextText();
		}
	}
	
	// ChatRoom member list ...
	private static void parseChatRoomMemberList(XmlPullParser xmlParser, Response response) throws Exception {

		ChatRoomMemberList list = (ChatRoomMemberList) response;
		String tagName = xmlParser.getName();
		if (tagName != null && tagName.equals("count")) {
			String text = xmlParser.nextText();
			list.count = text;
		} else if (tagName != null && tagName.equals("member")) {
			ChatroomMember member = new ChatroomMember();
			while (xmlParser.next() != XmlPullParser.END_TAG) {
				tagName = xmlParser.getName();
				if (tagName != null && tagName.equals("number")) {
					String text = xmlParser.nextText();
					member.setNumber(text);
				} else if (tagName != null && tagName.equals("type")) {
					String text = xmlParser.nextText();
					member.setType(text);
				} else if (tagName != null && tagName.equals("forbid")) {
					String text = xmlParser.nextText();
					if(TextUtils.isEmpty(text) || text.length() < 2) {
						continue;
					}
					String listenOpt = text.substring(0, 1);
					String speakOpt = text.substring(1, 2);
					ForbidOptions options;
					try {
						options = new ForbidOptions(Integer.parseInt(speakOpt), Integer.parseInt(listenOpt));
					} catch (NumberFormatException  e) {
						options = new ForbidOptions(ForbidOptions.OPTION_SPEAK_FREE,ForbidOptions.OPTION_LISTEN_FREE);
					}
					member.setOptions(options);
				}  else {
					xmlParser.nextText();
				}
			}
			list.chatRoomInfos.add(member);
		}
	}
	
	
	private static void parseChatRoomList(XmlPullParser xmlParser, Response response) throws Exception {
		
		ChatRoomList list = (ChatRoomList) response;
		String tagName = xmlParser.getName();
		if (tagName != null && tagName.equals("count")) {
			String text = xmlParser.nextText();
			list.count = text;
		} else if (tagName != null && tagName.equals("ChatRoom")) {
			Chatroom cRoomInfo = new Chatroom();
			while (xmlParser.next() != XmlPullParser.END_TAG) {
				tagName = xmlParser.getName();
				// version rest 2014-
				if (tagName != null && tagName.equals("roomId")) {
					String text = xmlParser.nextText();
					cRoomInfo.setRoomNo(text);
				} else if (tagName != null && tagName.equals("roomName")) {
					String text = xmlParser.nextText();
					cRoomInfo.setRoomName(text);
				} else if (tagName != null && tagName.equals("creator")) {
					String text = xmlParser.nextText();
					cRoomInfo.setCreator(text);
				} else if (tagName != null && tagName.equals("square")) {
					String text = xmlParser.nextText();
					cRoomInfo.setSquare(text);
				} else if (tagName != null && tagName.equals("joined")) {
					String text = xmlParser.nextText();
					cRoomInfo.setJoined(text);
				} else if (tagName != null && tagName.equals("validate")) {
					String text = xmlParser.nextText();
					cRoomInfo.setValidate(text);
				} else if (tagName != null && tagName.equals("keywords")) {
					String text = xmlParser.nextText();
					cRoomInfo.setKeywords(text);
				} else {
					xmlParser.nextText();
				}
			}
			list.chatroomInfos.add(cRoomInfo);
		}
	}
	
	private static int parseReceiveMsgVar(XmlPullParser xmlParser) throws Exception{
		int var = 0;
		String tagName = xmlParser.getName();
		if (tagName != null && (tagName.equals("VoiceMessage") || tagName.equals("InstanceMessage"))) {
			var = Integer.parseInt(xmlParser.getAttributeValue(null, "var"));
		}
		return var;
	}

	private static void parseReceiveMsg(XmlPullParser xmlParser, Response response, int var) throws Exception {
		String tagName = xmlParser.getName();
		if (tagName != null && (tagName.equals("VoiceMessage") || tagName.equals("InstanceMessage"))) {
			while (xmlParser.nextTag() != XmlPullParser.END_TAG) {
				if (var == VAR_INTERPHONE_INVITE) {
					tagName = xmlParser.getName();
					InterphoneInviteMsg msg = (InterphoneInviteMsg) response;
					if (tagName != null && tagName.equals("interphoneId")) {
						String text = xmlParser.nextText();
						msg.interphoneId = text;
					} else if (tagName != null && tagName.equals("dateCreated")) {
						String text = xmlParser.nextText();
						msg.dateCreated = text;
					} else if (tagName != null && tagName.equals("from")) {
						String text = xmlParser.nextText();
						msg.from = text;
					} else {
						xmlParser.nextText();
					}
				} else if (var == VAR_INTERPHONE_JOIN) {
					tagName = xmlParser.getName();
					InterphoneJoinMsg msg = (InterphoneJoinMsg) response;
					if (tagName != null && tagName.equals("interphoneId")) {
						String text = xmlParser.nextText();
						msg.interphoneId = text;
					} else if (tagName != null && tagName.equals("who")) {
						String text = xmlParser.nextText();
						if (text != null) {
							msg.whos = text.split(",");
						}
					} else {
						xmlParser.nextText();
					}
				} else if (var == VAR_INTERPHONE_EXIT) {
					tagName = xmlParser.getName();
					InterphoneExitMsg msg = (InterphoneExitMsg) response;
					if (tagName != null && tagName.equals("interphoneId")) {
						String text = xmlParser.nextText();
						msg.interphoneId = text;
					} else if (tagName != null && tagName.equals("who")) {
						String text = xmlParser.nextText();
						if (text != null) {
							msg.whos = text.split(",");
						}
					} else {
						xmlParser.nextText();
					}
				} else if (var == VAR_INTERPHONE_OVER) {
					tagName = xmlParser.getName();
					InterphoneOverMsg msg = (InterphoneOverMsg) response;
					if (tagName != null && tagName.equals("interphoneId")) {
						String text = xmlParser.nextText();
						msg.interphoneId = text;
					} else {
						xmlParser.nextText();
					}
				} else if (var == VAR_INTERPHONE_CONTROL_MIC){
					tagName = xmlParser.getName();
					InterphoneControlMicMsg msg = (InterphoneControlMicMsg) response;
					if (tagName != null && tagName.equals("interphoneId")) {
						String text = xmlParser.nextText();
						msg.interphoneId = text;
					} else if (tagName != null && tagName.equals("who")) {
						String text = xmlParser.nextText();
						msg.who = text;
					} else {
						xmlParser.nextText();
					}
				} else if (var == VAR_INTERPHONE_RELEASE_MIC){
					tagName = xmlParser.getName();
					InterphoneReleaseMicMsg msg = (InterphoneReleaseMicMsg) response;
					if (tagName != null && tagName.equals("interphoneId")) {
						String text = xmlParser.nextText();
						msg.interphoneId = text;
					} else if (tagName != null && tagName.equals("who")) {
						String text = xmlParser.nextText();
						msg.who = text;
					} else {
						xmlParser.nextText();
					}
					
					
					// ChatRoom ...
				} else if (var == VAR_CHATROOM_JOIN) {
					tagName = xmlParser.getName();
					ChatroomJoinMsg msg = (ChatroomJoinMsg) response;
					if (tagName != null && tagName.equals("chatroomId")) {
						String text = xmlParser.nextText();
						msg.setRoomNo(text);
					} else if (tagName != null && tagName.equals("who")) {
						String text = xmlParser.nextText();
						if (text != null) {
							msg.setWhos(text.split(","));
						}
					} else {
						xmlParser.nextText();
					}
				} else if (var == VAR_CHATROOM_EXIT) {
					tagName = xmlParser.getName();
					ChatroomExitMsg msg = (ChatroomExitMsg) response;
					if (tagName != null && tagName.equals("chatroomId")) {
						String text = xmlParser.nextText();
						msg.setRoomNo(text);
					} else if (tagName != null && tagName.equals("who")) {
						String text = xmlParser.nextText();
						if (text != null) {
							msg.setWhos(text.split(","));
						}
					} else {
						xmlParser.nextText();
					}
				} else if (var == VAR_CHATROOM_DISMISS) {
					tagName = xmlParser.getName();
					ChatroomDismissMsg msg = (ChatroomDismissMsg) response;
					if (tagName != null && tagName.equals("chatroomId")) {
						String text = xmlParser.nextText();
						msg.setRoomNo(text);
					} else {
						xmlParser.nextText();
					}
				} else if (var == VAR_CHATROOM_REMOVEMEMBER) {
					tagName = xmlParser.getName();
					ChatroomRemoveMemberMsg msg = (ChatroomRemoveMemberMsg) response;
					if (tagName != null && tagName.equals("chatroomId")) {
						String text = xmlParser.nextText();
						msg.setRoomNo(text);
					} else if (tagName != null && tagName.equals("who")) {
						String text = xmlParser.nextText();
						msg.setWho(text);
					} else {
						xmlParser.nextText();
					}
				} else if (var == VAR_CHATROOM_SPEAK_OPT) {
					tagName = xmlParser.getName();
					ChatroomMemberForbidOpt forbidOpt = (ChatroomMemberForbidOpt) response;
					if (tagName != null && tagName.equals("roomId")) {
						String text = xmlParser.nextText();
						forbidOpt.setRoomNo(text);
					} else if (tagName != null && tagName.equals("who")) {
						String text = xmlParser.nextText();
						forbidOpt.setMember(text);
					} else if (tagName != null && tagName.equals("forbid")) {
						String text = xmlParser.nextText();
						if(TextUtils.isEmpty(text) || text.length() < 2) {
							continue;
						}
						String listenOpt = text.substring(0, 1);
						String speakOpt = text.substring(1, 2);
						ForbidOptions options;
						try {
							options = new ForbidOptions(Integer.parseInt(speakOpt), Integer.parseInt(listenOpt));
						} catch (NumberFormatException  e) {
							options = new ForbidOptions(ForbidOptions.OPTION_SPEAK_FREE,ForbidOptions.OPTION_LISTEN_FREE);
						}
						forbidOpt.setOptions(options);
					} else {
						xmlParser.nextText();
					}
					
				} else if (var == VAR_APPLY_JOIN_GROUP_MSG || var == VAR_INVITE_JOIN_GROUP_UNVALIDATION_MSG) {
					tagName = xmlParser.getName();
					IMProposerMsg msg = null;
					if(var == VAR_APPLY_JOIN_GROUP_MSG) {
						msg = (IMProposerMsg) response;
					} else {
						// JJ game push
						msg = (IMJoinGroupMsg) response;
					}
					
					if (tagName != null && tagName.equals("groupId")) {
						String text = xmlParser.nextText();
						msg.setGroupId(text);
					} else if (tagName != null && tagName.equals("proposer")) {
						String text = xmlParser.nextText();
						msg.setProposer(text);
					} else if (tagName != null && tagName.equals("dateCreated")) {
						String text = xmlParser.nextText();
						msg.setDateCreated(text);
					} else if (tagName != null && tagName.equals("declared")) {
						String text = xmlParser.nextText();
						msg.setDeclared(text);
					} else {
						xmlParser.nextText();
					}
				} else if (var == VAR_REPLY_GROUP_APPLY) {
					tagName = xmlParser.getName();
					IMInviterJoinGroupReplyMsg msg = (IMInviterJoinGroupReplyMsg) response;
					if (tagName != null && tagName.equals("groupId")) {
						String text = xmlParser.nextText();
						msg.setGroupId(text);
					} else if (tagName != null && tagName.equals("declared")) {
						String text = xmlParser.nextText();
						msg.setDeclared(text);
					} else if (tagName != null && tagName.equals("inviter")) {
						String text = xmlParser.nextText();
						msg.setAdmin(text);
					} else if (tagName != null && tagName.equals("confirm")) {
						String text = xmlParser.nextText();
						msg.setConfirm(text);
					} else if (tagName != null && tagName.equals("member")) {
						String text = xmlParser.nextText();
						msg.setMember(text);
					} else {
						xmlParser.nextText();
					}
				} else if (var == VAR_ACCEPT_OR_REJECT_JOIN_GROUP_MSG) {
					tagName = xmlParser.getName();
					IMReplyJoinGroupMsg msg = (IMReplyJoinGroupMsg) response;
					if (tagName != null && tagName.equals("groupId")) {
						String text = xmlParser.nextText();
						msg.setGroupId(text);
					} else if (tagName != null && tagName.equals("admin")) {
						String text = xmlParser.nextText();
						msg.setAdmin(text);
					} else if (tagName != null && tagName.equals("confirm")) {
						String text = xmlParser.nextText();
						msg.setConfirm(text);
					} else if (tagName != null && tagName.equals("member")) {
						String text = xmlParser.nextText();
						msg.setMember(text);
					} else {
						xmlParser.nextText();
					}
				} else if (var == VAR_INVITE_JOIN_GROUP_MSG) {
					tagName = xmlParser.getName();
					IMInviterMsg msg = (IMInviterMsg) response;
					if (tagName != null && tagName.equals("groupId")) {
						String text = xmlParser.nextText();
						msg.setGroupId(text);
					} else if (tagName != null && tagName.equals("admin")) {
						String text = xmlParser.nextText();
						msg.setAdmin(text);
					} else if (tagName != null && tagName.equals("confirm")) {
						String text = xmlParser.nextText();
						msg.setConfirm(text);
					} else if (tagName != null && tagName.equals("declared")) {
						String text = xmlParser.nextText();
						msg.setDeclared(text);
					} else {
						xmlParser.nextText();
					}
				} else if (var == VAR_DELETE_FROM_GROUP_MSG) {
					// 5.5  Ⱥ�����Աɾ����Ա - PUSH����ɾ�����û�        �޸�Ϊ  PUSH�������û�
					tagName = xmlParser.getName();
					IMRemoveMemeberMsg msg = (IMRemoveMemeberMsg) response;
					if (tagName != null && tagName.equals("groupId")) {
						String text = xmlParser.nextText();
						msg.setGroupId(text);
					} else if (tagName != null && tagName.equals("member")) {
						String text = xmlParser.nextText();
						msg.setWho(text);
					} else {
						xmlParser.nextText();
					}
				} else if (var == VAR_REPLY_GROUP_APPLY) {
					
				} else if (var == VAR_EXIT_FROM_GROUP_MSG) {
					tagName = xmlParser.getName();
					IMQuitGroupMsg msg = (IMQuitGroupMsg) response;
					if (tagName != null && tagName.equals("groupId")) {
						String text = xmlParser.nextText();
						msg.setGroupId(text);
					} else if (tagName != null && tagName.equals("member")) {
						String text = xmlParser.nextText();
						msg.setMember(text);
					} else {
						xmlParser.nextText();
					}
				} else if (var == VAR_DELETE_GROUP_MSG) {
					tagName = xmlParser.getName();
					IMDismissGroupMsg msg = (IMDismissGroupMsg) response;
					if (tagName != null && tagName.equals("groupId")) {
						String text = xmlParser.nextText();
						msg.setGroupId(text);
					} else {
						xmlParser.nextText();
					}
				} else if (var == VAR_RECEIVE_INSTANCE_MSG 
						|| var == VAR_RECEIVE_INSTANCE_MSG_JJ) {
					tagName = xmlParser.getName();
					IMAttachedMsg msg = null;
					if(var == VAR_RECEIVE_INSTANCE_MSG_JJ) {
						msg = (IMCooperMsg) response;
					} else {
						// JJ game push
						msg = (IMAttachedMsg) response;
					}
					if(tagName != null && tagName.equals("msgId")){
						String text = xmlParser.nextText();
						if (text != null && !text.equals("")) {
							msg.setMsgId(text);
						}
					} else if (tagName != null && tagName.equals("dateCreated")) {
						String text = xmlParser.nextText();
						if (text != null && !text.equals("")) {
							msg.setDateCreated(text);
						}
					} else if (tagName != null && tagName.equals("message")) {
						String text = xmlParser.nextText();
						if (text != null && !text.equals("")) 
						{
							if(var == VAR_RECEIVE_INSTANCE_MSG_JJ){
								((IMCooperMsg) response).setMessage(text);
							}
						}
					} else if (tagName != null && tagName.equals("sender")) {
						String text = xmlParser.nextText();
						if (text != null && !text.equals("")) {
							msg.setSender(text);
						}
					} else if (tagName != null && tagName.equals("receiver")) {
						String text = xmlParser.nextText();
						if (text != null && !text.equals("")) {
							msg.setReceiver(text);
						}
					} else if (tagName != null && tagName.equals("fileExt")) {
						String text = xmlParser.nextText();
						if (text != null && !text.equals("")) {
							msg.setExt(text);
						}
					}  else if (tagName != null && tagName.equals("type")) {
						String text = xmlParser.nextText();
						if (text != null && !text.equals("")) 
						{
							if(var == VAR_RECEIVE_INSTANCE_MSG_JJ){
								msg.setChunked(false);
								((IMCooperMsg) response).setType(Integer.parseInt(text));
							}else{
								msg.setChunked( "1".equals(text) ? true : false);
							}
						}
					} else if (tagName != null && tagName.equals("fileSize")) {
						String text = xmlParser.nextText();
						if (text != null && !text.equals("")) {
							msg.setFileSize(Integer.parseInt(text));
						}
					} else if (tagName != null && tagName.equals("fileUrl")) {
						String text = xmlParser.nextText();
						if (text != null && !text.equals("")) {
							msg.setFileUrl(text);
						}
						// Modified v3.0.1 send text messages (group or point to point) format and analysis
					}  else if (tagName != null && tagName.equals("userData")) {
						String text = xmlParser.nextText();
						if (text != null && !text.equals("")) {
							msg.setUserData(text);
						}
					} else {
						xmlParser.nextText();
					}
					
					// Video Conference
				} else if (var == VAR_VIDEO_CONFERENCE_JOIN) {
					tagName = xmlParser.getName();
					VideoConferenceJoinMsg msg = (VideoConferenceJoinMsg) response;
					if (tagName != null && tagName.equals("roomId")) {
						String text = xmlParser.nextText();
						msg.setConferenceId(text);
					} else if (tagName != null && tagName.equals("who")) {
						String text = xmlParser.nextText();
						if (text != null) {
							msg.setWhos(text.split(","));
						}
						
						// ��·��Ƶ�����Ա������Ƶ����
						// 2014/8/27 �����ӿ�
					} else if (tagName != null && tagName.equals("videoState")) {
							try {
								msg.setPublishStatus(Integer.parseInt(xmlParser.nextText()));
							} catch (Exception e) {
								e.printStackTrace();
							}
						} else if (tagName != null && tagName.equals("videoSource")) {
							try {
								String text = xmlParser.nextText();
								if(!TextUtils.isEmpty(text) && text.indexOf(":") != -1) {
									String[] split = text.split(":");
									msg.setIp(split[0]);
									msg.setPort(Integer.parseInt(split[1]));
								}
							} catch (Exception e) {
								e.printStackTrace();
							}
					} else {
						xmlParser.nextText();
					}
				} else if (var == VAR_VIDEO_CONFERENCE_EXIT) {
					tagName = xmlParser.getName();
					VideoConferenceExitMsg msg = (VideoConferenceExitMsg) response;
					if (tagName != null && tagName.equals("roomId")) {
						String text = xmlParser.nextText();
						msg.setConferenceId(text);
					} else if (tagName != null && tagName.equals("who")) {
						String text = xmlParser.nextText();
						if (text != null) {
							msg.setWhos(text.split(","));
						}
					} else {
						xmlParser.nextText();
					}
				} else if (var == VAR_VIDEO_CONFERENCE_DISMISS) {
					tagName = xmlParser.getName();
					VideoConferenceDismissMsg msg = (VideoConferenceDismissMsg) response;
					if (tagName != null && tagName.equals("roomId")) {
						String text = xmlParser.nextText();
						msg.setConferenceId(text);
					} else {
						xmlParser.nextText();
					}
				} else if (var == VAR_VIDEO_CONFERENCE_REMOVEMEMBER) {
					tagName = xmlParser.getName();
					VideoConferenceRemoveMemberMsg msg = (VideoConferenceRemoveMemberMsg) response;
					if (tagName != null && tagName.equals("roomId")) {
						String text = xmlParser.nextText();
						msg.setConferenceId(text);
					} else if (tagName != null && tagName.equals("who")) {
						String text = xmlParser.nextText();
						msg.setWho(text);
					} else {
						xmlParser.nextText();
					}
				}  else if (var == VAR_VIDEO_CONFERENCE_SWITCH) {
					tagName = xmlParser.getName();
					VideoConferenceSwitch msg = (VideoConferenceSwitch) response;
					if (tagName != null && tagName.equals("roomId")) {
						String text = xmlParser.nextText();
						msg.setConferenceId(text);
					} else if (tagName != null && tagName.equals("who")) {
						String text = xmlParser.nextText();
						msg.setWho(text);
					} else {
						xmlParser.nextText();
					}
				} 
			}
		} else {
			xmlParser.nextText();
		}
	}
	
	/**
	 * �����ı���Ϣ
	 * @param xmlParser
	 * @param response
	 * @throws XmlPullParserException
	 * @throws IOException
	 */
	private static void parseReceiveGroupMessage(XmlPullParser xmlParser,
			IMTextMsg response) throws XmlPullParserException, IOException {
		String tagName = xmlParser.getName();
		if(tagName.equals("msgid")){ 
			// The underlying database updates, modifications to the text message 
			// reporting interface, parameter added message ID.
			String text = xmlParser.nextText();
			if (text != null && !text.equals("")) {
				response.setMsgId(text);
			}
		} else if (tagName.equals("sender")) {
			String text = xmlParser.nextText();
			if (text != null && !text.equals("")) {
				response.setSender(text);
			}
		} else if (tagName.equals("receiver")) {
			String text = xmlParser.nextText();
			if (text != null && !text.equals("")) {
				response.setReceiver(text);

			}
		} else if (tagName.equals("message")) {
			String text = xmlParser.nextText();
			if (text != null && !text.equals("")) {
				response.setMessage(text);

			}
		} else if (tagName.equals("userdata")) {
			String text = xmlParser.nextText();
			if (text != null && !text.equals("")) {
				response.setUserData(text);

			}
		} else if (tagName.equals("time")) {
			String text = xmlParser.nextText();
			if (text != null && !text.equals("")) {
				try {
					response.setDateCreated(VoiceUtil.getTextReceiveDate(text));
				} catch (Exception e) {
					response.setDateCreated(text);
				}

			}
		} else {
			xmlParser.nextText();
		}
	}
	
	/**
	 * 
	 * @param xmlParser
	 * @param response
	 * @throws IOException 
	 * @throws XmlPullParserException 
	 */
	private static void parseMultiMedia(XmlPullParser xmlParser,
			UploadImessage response) throws XmlPullParserException, IOException {
		response.mediaMsg = new IMAttachedMsg();
		while (xmlParser.nextTag() != XmlPullParser.END_TAG) {
			String tagName = xmlParser.getName();
			if (tagName.equals("msgId")) {
				String text = xmlParser.nextText();
				if (text != null && !text.equals("")) {
					response.mediaMsg.setMsgId(text);
				}
			} else if (tagName.equals("uploadUrl")) {
				String text = xmlParser.nextText();
				if (text != null && !text.equals("")) {
					response.setUploadUrl(text);

				}
			} else if (tagName.equals("uploadToken")) {
				String text = xmlParser.nextText();
				if (text != null && !text.equals("")) {
					response.setUploadToken(text);
				}
			} else if (tagName != null && tagName.equals("dateCreated")) {
				String text = xmlParser.nextText();
				if (text != null && !text.equals("")) {
					response.mediaMsg.setDateCreated(text);
				}
			} else {
				xmlParser.nextText();
			}
		}
	}
	
	private static void parseReceiveMediaMsgList(XmlPullParser xmlParser, NewMediaMsgList nMsgList)
			throws XmlPullParserException, IOException {
		String tagName = xmlParser.getName();
		if (tagName != null && tagName.equals("count")) {
			String text = xmlParser.nextText();
			nMsgList.count = text;
		} else if (tagName != null && tagName.equals("InstanceMessage")) {
			IMAttachedMsg msgInfo = new IMAttachedMsg();
			while (xmlParser.nextTag() != XmlPullParser.END_TAG) {
				parseReceiveMediaMsg(xmlParser, msgInfo);
			}
			nMsgList.newMsgs.add(msgInfo);
		} else if (tagName != null && tagName.equals("InstanceMessageText")) {
			IMTextMsg msgInfo = new IMTextMsg();
			while (xmlParser.nextTag() != XmlPullParser.END_TAG) {
				parseReceiveMediaTextMsg(xmlParser, msgInfo);
			}
			nMsgList.newMsgs.add(msgInfo);
		} else {
			xmlParser.nextText();
		}
	}
	
	private static void parseReceiveMediaTextMsg(XmlPullParser xmlParser,
			IMTextMsg msgInfo) throws XmlPullParserException, IOException {
		String tagName = xmlParser.getName();
		if(tagName != null && tagName.equals("dateCreated")){
			String text = xmlParser.nextText();
			if (text != null && !text.equals("")) {
				msgInfo.setDateCreated(text);
			}
		} else if (tagName != null && tagName.equals("sender")) {
			String text = xmlParser.nextText();
			if (text != null && !text.equals("")) {
				msgInfo.setSender(text);
			}
		} else if (tagName != null && tagName.equals("receiver")) {
			String text = xmlParser.nextText();
			if (text != null && !text.equals("")) {
				msgInfo.setReceiver(text);
			}
		} else if (tagName != null && tagName.equals("content")) {
			String text = xmlParser.nextText();
			if (text != null && !text.equals("")) {
				msgInfo.setMessage(text);
			}
		} else {
			xmlParser.nextText();
		}
	}
	
	private static void parseReceiveMediaMsg(XmlPullParser xmlParser,
			IMAttachedMsg msgInfo) throws XmlPullParserException, IOException {
		String tagName = xmlParser.getName();
		if(tagName != null && tagName.equals("msgId")){
			String text = xmlParser.nextText();
			if (text != null && !text.equals("")) {
				msgInfo.setMsgId(text);
			}
		} else if (tagName != null && tagName.equals("dateCreated")) {
			String text = xmlParser.nextText();
			if (text != null && !text.equals("")) {
				msgInfo.setDateCreated(text);
			}
		} else if (tagName != null && tagName.equals("sender")) {
			String text = xmlParser.nextText();
			if (text != null && !text.equals("")) {
				msgInfo.setSender(text);
			}
		} else if (tagName != null && tagName.equals("receiver")) {
			String text = xmlParser.nextText();
			if (text != null && !text.equals("")) {
				msgInfo.setReceiver(text);
			}
		} else if (tagName != null && tagName.equals("fileExt")) {
			String text = xmlParser.nextText();
			if (text != null && !text.equals("")) {
				msgInfo.setExt(text);
			}
		} else if (tagName != null && tagName.equals("fileSize")) {
			String text = xmlParser.nextText();
			if (text != null && !text.equals("")) {
				msgInfo.setFileSize(Integer.parseInt(text));
			}
		} else if (tagName != null && tagName.equals("fileUrl")) {
			String text = xmlParser.nextText();
			if (text != null && !text.equals("")) {
				msgInfo.setFileUrl(text);
			}
		} else {
			xmlParser.nextText();
		}
	}

	
	
	/**
	 * 
	* <p>Title: parseStartVideoConference</p>
	* <p>Description: </p>
	* @param xmlParser
	* @param response
	* @throws Exception
	 */
	private static void parseStartVideoConference(XmlPullParser xmlParser, Response response) throws Exception {
		String tagName = xmlParser.getName();
		VideoConferenceMsg videoConference = (VideoConferenceMsg) response;
		if (tagName != null && tagName.equals("roomId")) {
			String text = xmlParser.nextText();
			videoConference.setConferenceId(text);
		} else {
			xmlParser.nextText();
		}
	}
	
	/**
	 * 
	* <p>Title: parseVideoConferenceMemberList</p>
	* <p>Description: </p>
	* @param xmlParser
	* @param response
	* @throws Exception
	 */
	private static void parseVideoConferenceMemberList(XmlPullParser xmlParser, Response response) throws Exception {

		VideoConferenceMemberList list = (VideoConferenceMemberList) response;
		String tagName = xmlParser.getName();
		if (tagName != null && tagName.equals("count")) {
			String text = xmlParser.nextText();
			list.count = text;
		} else if (tagName != null && tagName.equals("member")) {
			VideoConferenceMember member = new VideoConferenceMember();
			while (xmlParser.next() != XmlPullParser.END_TAG) {
				tagName = xmlParser.getName();
				if (tagName != null && tagName.equals("number")) {
					String text = xmlParser.nextText();
					member.setNumber(text);
				} else if (tagName != null && tagName.equals("type")) {
					String text = xmlParser.nextText();
					int type = 0;
					try {
						type = Integer.parseInt(text);
					} catch (Exception e) {
						e.printStackTrace();
					}
					member.setType(type);
				} else if (tagName != null && tagName.equals("screen")) {
					String text = xmlParser.nextText();
					int screen = 0;
					try {
						screen = Integer.parseInt(text);
					} catch (Exception e) {
						e.printStackTrace();
					}
					member.setScreen(screen);
					
				// ��·��Ƶ�����Ա������Ƶ����
				// 2014/8/27 �����ӿ�
				} else if (tagName != null && tagName.equals("videoState")) {
					try {
						member.setPublishStatus(Integer.parseInt(xmlParser.nextText()));
					} catch (Exception e) {
						e.printStackTrace();
					}
				} else if (tagName != null && tagName.equals("videoSource")) {
					try {
						String text = xmlParser.nextText();
						if(!TextUtils.isEmpty(text) && text.indexOf(":") != -1) {
							String[] split = text.split(":");
							member.setIp(split[0]);
							member.setPort(Integer.parseInt(split[1]));
						}
					} catch (Exception e) {
						e.printStackTrace();
					}
				} else {
					xmlParser.nextText();
				}
			}
			list.videoConferenceMembers.add(member);
		}
	}
	
	
	/**
	 * 
	* <p>Title: parseVideoConferenceList</p>
	* <p>Description: </p>
	* @param xmlParser
	* @param response
	* @throws Exception
	 */
	private static void parseVideoConferenceList(XmlPullParser xmlParser, Response response) throws Exception {
		
		VideoConferenceList list = (VideoConferenceList) response;
		String tagName = xmlParser.getName();
		if (tagName != null && tagName.equals("count")) {
			String text = xmlParser.nextText();
			list.count = text;
		} else if (tagName != null && tagName.equals("VideoConf")) {
			VideoConference vConference = new VideoConference();
			while (xmlParser.next() != XmlPullParser.END_TAG) {
				tagName = xmlParser.getName();
				if (tagName != null && tagName.equals("roomId")) {
					String text = xmlParser.nextText();
					vConference.setConferenceId(text);
				} else if (tagName != null && tagName.equals("roomName")) {
					String text = xmlParser.nextText();
					vConference.setConferenceName(text);
				} else if (tagName != null && tagName.equals("creator")) {
					String text = xmlParser.nextText();
					vConference.setCreator(text);
				} else if (tagName != null && tagName.equals("square")) {
					String text = xmlParser.nextText();
					vConference.setSquare(text);
				} else if (tagName != null && tagName.equals("joined")) {
					String text = xmlParser.nextText();
					vConference.setJoinNum(text);
				} else if (tagName != null && tagName.equals("validate")) {
					String text = xmlParser.nextText();
					vConference.setValidate(text);
				} else if (tagName != null && tagName.equals("keywords")) {
					String text = xmlParser.nextText();
					vConference.setKeywords(text);
				} else if (tagName != null && tagName.equals("isMultiVideo")) {
					String text = xmlParser.nextText();
					try {
						boolean isMulitVideo = "1".equals(text);
						vConference.setMultiVideo(isMulitVideo);
					} catch (Exception e) {
						// TODO: handle exception
					}
				} else {
					xmlParser.nextText();
				}
			}
			list.videoConferences.add(vConference);
		}
	}
	private static void parseVideoConferencePortraitList(XmlPullParser xmlParser, Response response) throws Exception {
		
		VideoPartnerPortraitList list = (VideoPartnerPortraitList) response;
		String tagName = xmlParser.getName();
		if (tagName != null && tagName.equals("count")) {
			String text = xmlParser.nextText();
			list.count = text;
		} else if (tagName != null && tagName.equals("fileList")) {
			VideoPartnerPortrait vPortrait = new VideoPartnerPortrait();
			while (xmlParser.next() != XmlPullParser.END_TAG) {
				tagName = xmlParser.getName();
				if (tagName != null && tagName.equals("dateUpdate")) {
					String text = xmlParser.nextText();
					vPortrait.setDateUpdate(text);
				} else if (tagName != null && tagName.equals("sender")) {
					String text = xmlParser.nextText();
					vPortrait.setVoip(text);
				} else if (tagName != null && tagName.equals("fileName")) {
					String text = xmlParser.nextText();
					vPortrait.setFileName(text);
				} else if (tagName != null && tagName.equals("fileUrl")) {
					String text = xmlParser.nextText();
					vPortrait.setFileUrl(text);
				} else {
					xmlParser.nextText();
				}
			}
			list.videoPartnerPortraits.add(vPortrait);
		}
	}
}
