/*
 *  Copyright (c) 2013 The CCP project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a Beijing Speedtong Information Technology Co.,Ltd license
 *  that can be found in the LICENSE file in the root of the web site.
 *
 *   http://www.cloopen.com
 *
 *  An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */
package com.voice.demo.sqlite;

import java.io.File;
import java.sql.SQLException;
import java.util.ArrayList;

import android.content.ContentValues;
import android.database.Cursor;
import android.text.TextUtils;

import com.hisun.phone.core.voice.model.im.IMDismissGroupMsg;
import com.hisun.phone.core.voice.model.im.IMInviterJoinGroupReplyMsg;
import com.hisun.phone.core.voice.model.im.IMInviterMsg;
import com.hisun.phone.core.voice.model.im.IMProposerMsg;
import com.hisun.phone.core.voice.model.im.IMQuitGroupMsg;
import com.hisun.phone.core.voice.model.im.IMRemoveMemeberMsg;
import com.hisun.phone.core.voice.model.im.IMReplyJoinGroupMsg;
import com.hisun.phone.core.voice.model.im.InstanceMsg;
import com.hisun.phone.core.voice.util.Log4Util;
import com.voice.demo.group.model.IMChatMessageDetail;
import com.voice.demo.group.model.IMConversation;
import com.voice.demo.group.model.IMGroup;
import com.voice.demo.group.model.IMSystemMessage;
import com.voice.demo.tools.CCPConfig;
import com.voice.demo.tools.CCPUtil;
import com.voice.demo.ui.CCPHelper;

/**
 * The IM message and the system notification message database operation
 * @author Jorstin Chan
 * @version Time: 2013-8-30
 */
public class CCPSqliteManager extends AbstractSQLManager {

	private static CCPSqliteManager instance;
	
	public static CCPSqliteManager getInstance() {
		Log4Util.d(CCPHelper.DEMO_TAG, "instance :" + instance);
		if (instance == null) {
			instance = new CCPSqliteManager();
		}

		return instance;
	}
	
	public CCPSqliteManager() {
		super();
	}
	
	// ------------------------------------------------------------------------------------
	// Database interface
	// Group notification database operation
	public void  insertIMGroupInfo(IMGroup imGroup) throws SQLException {

		if (imGroup == null || TextUtils.isEmpty(imGroup.groupId)) {
			throw new SQLException("[insertIMGroupInfo] The inserted data is empty imGroup : " + imGroup);
		}

		ContentValues values = null;
		try {
			
			values = new ContentValues();
			values.put(IMGroupInfoColumn.GROUP_ID, imGroup.groupId);
			values.put(IMGroupInfoColumn.GROUP_NAME, imGroup.name);
			values.put(IMGroupInfoColumn.GROUP_PERMISSION, imGroup.permission);
			values.put(IMGroupInfoColumn.GROUP_TYPE, imGroup.type);
			
			values.put(IMGroupInfoColumn.GROUP_OWNER, imGroup.owner);
			values.put(IMGroupInfoColumn.GROUP_DATECREATED, imGroup.declared);
			values.put(IMGroupInfoColumn.GROUP_DECLARED, imGroup.declared);
			values.put(IMGroupInfoColumn.GROUP_MEMBER_COUNTS, imGroup.count);

			sqliteDB().insert(CCPDBHelper.TABLES_NAME_IM_GROUP_INFO, null, values);
		} catch (Exception e) {
			e.printStackTrace();
			throw new SQLException(e.getMessage());
		} finally {
			if (values != null) {
				values.clear();
				values = null;
				imGroup = null;
			}
			
		}
		    
	}
	
	/**
	 * Batch updating group information
	 * @param imGroups
	 * @throws SQLException
	 */
	public void insertIMGroupInfos(ArrayList<IMGroup> imGroups)throws SQLException {
		if (imGroups == null ) {
			throw new SQLException("The inserted data is empty.");
		}
		
		try {
			
			// Set the start transaction
			sqliteDB().beginTransaction();
	        
			// Batch processing operation
            for(IMGroup imGroup : imGroups){
            	try {
            		insertIMGroupInfo(imGroup);
				} catch (Exception e) {
					e.printStackTrace();
				}
            }
            
            // Set transaction successful, do not set automatically 
			// rolls back not submitted.
            sqliteDB().setTransactionSuccessful(); 
		} catch (Exception e) {
			e.printStackTrace();
			throw new SQLException(e.getMessage());
		} finally {
			sqliteDB().endTransaction();
		}
	}
	
	/**
	 * Query the existence of the group ID
	 * @param groupId
	 * @return
	 * @throws SQLException
	 */
	public String isExistsGroupId(String groupId) throws SQLException {
		if (TextUtils.isEmpty(groupId)) {
			return null;
		}
		Cursor cursor = null;
		try {
			String where = IMGroupInfoColumn.GROUP_ID + " ='" + groupId + "'";
			cursor = sqliteDB().query(CCPDBHelper.TABLES_NAME_IM_GROUP_INFO, new String[]{IMGroupInfoColumn.GROUP_ID}, where, null, null, null, null);
			if (cursor != null && cursor.getCount() > 0) {
				if(cursor.moveToFirst()) {
					return cursor.getString(cursor.getColumnIndex(IMGroupInfoColumn.GROUP_ID));
				}
			}
		} catch (Exception e) {
			e.printStackTrace();
		} finally {
			if (cursor != null) {
				cursor.close();
				cursor = null;
			}
		}
		return null;
	}
	
	public void updateGroupInfo(IMGroup imGroup) throws SQLException {
		if (imGroup == null || TextUtils.isEmpty(imGroup.groupId)) {
			throw new SQLException("[updateGroupInfo] The update data is empty imGroup : " + imGroup);
		}

		ContentValues values = null;
		try {
			final String where = IMGroupInfoColumn.GROUP_ID + " ='"
					+ imGroup.groupId + "'";
			values = new ContentValues();
			values.put(IMGroupInfoColumn.GROUP_ID, imGroup.groupId);
			values.put(IMGroupInfoColumn.GROUP_NAME, imGroup.name);
			values.put(IMGroupInfoColumn.GROUP_PERMISSION, imGroup.permission);
			values.put(IMGroupInfoColumn.GROUP_TYPE, imGroup.type);

			values.put(IMGroupInfoColumn.GROUP_OWNER, imGroup.type);
			values.put(IMGroupInfoColumn.GROUP_DATECREATED, imGroup.type);
			values.put(IMGroupInfoColumn.GROUP_DECLARED, imGroup.type);
			values.put(IMGroupInfoColumn.GROUP_MEMBER_COUNTS, imGroup.type);

			sqliteDB().update(CCPDBHelper.TABLES_NAME_IM_GROUP_INFO, values,
					where, null);

		} catch (Exception e) {
			e.printStackTrace();
			throw new SQLException(e.getMessage());
		} finally {
			if (values != null) {
				values.clear();
				values = null;
			}
		}
	}

	/**
	 * Batch updating group information
	 * 
	 * @param imGroups
	 * @throws SQLException
	 */
	public void updateGroupInfos(ArrayList<IMGroup> imGroups)
			throws SQLException {
		if (imGroups == null) {
			throw new SQLException("[AbstractSQLManager]The inserted data is empty.");
		}

		try {
			
			// Set the start transaction
			sqliteDB().beginTransaction(); 
			
			// Batch processing operation
			for (IMGroup imGroup : imGroups) {
				try {
					updateGroupInfo(imGroup);
				} catch (Exception e) {
					e.printStackTrace();
				}
			}
			
			// Set transaction successful, do not set automatically 
			// rolls back not submitted.
			sqliteDB().setTransactionSuccessful(); 
		} catch (Exception e) {
			e.printStackTrace();
			throw new SQLException(e.getMessage());
		} finally {
			sqliteDB().endTransaction();
		}
	}

	/**
	 * 查询群组名字
	 * 
	 * @param groupId
	 * @return
	 */
	public String queryGroupName(String groupId) {
		Cursor cursor = null;
		try {
			String where = IMGroupInfoColumn.GROUP_ID + " = '" + groupId + "'";
			cursor = sqliteDB().query(CCPDBHelper.TABLES_NAME_IM_GROUP_INFO,
					new String[] { IMGroupInfoColumn.GROUP_NAME }, where, null,
					null, null, null);

			if ((cursor != null) && (cursor.getCount() > 0)) {
				if (cursor.moveToFirst()) {
					int columnIndex = cursor
							.getColumnIndex(IMGroupInfoColumn.GROUP_NAME);
					return cursor.getString(columnIndex);
				}

			}
		} catch (Exception e) {
			Log4Util.d(CCPHelper.DEMO_TAG,
					"[AbstractSQLManager] queryGroupName: " + e.toString());
		} finally {
			if (cursor != null) {
				cursor.close();
				cursor = null;
			}
		}
		return groupId;
	}
	
	// ------------------------------------------------------------------------------------------------
	// IM message database operation
	public ArrayList<IMChatMessageDetail> queryIMMessagesBySessionId(String sessionId) {
		if (TextUtils.isEmpty(sessionId)) {
			throw new RuntimeException("Error , sessionId is " + sessionId);
		}
		Cursor cursor = null;
		ArrayList<IMChatMessageDetail> imChatMessageDetails = null;
		try {
			String where = IMMessageColumn.IM_SESSION_ID + " ='" + sessionId + "'";
			cursor = sqliteDB().query(CCPDBHelper.TABLES_NAME_IM_MESSAGE, null, where, null, null, null,
					IMMessageColumn.IM_CURRENT_DATE);
			if ((cursor != null) && (cursor.getCount() > 0)) {
				imChatMessageDetails = new ArrayList<IMChatMessageDetail>();
				while (cursor.moveToNext()) {
					
					String messageId = cursor.getString(cursor.getColumnIndex(IMMessageColumn.IM_MESSAGE_ID));
					//sessionId = cursor.getString(cursor.getColumnIndex(IMMessageColumn.IM_SESSION_ID));
					int messageType = cursor.getInt(cursor.getColumnIndex(IMMessageColumn.IM_MESSAGE_TYPE));
					String groupSender = cursor.getString(cursor.getColumnIndex(IMMessageColumn.IM_MESSAGE_SENDER));
					int isRead = cursor.getInt(cursor.getColumnIndex(IMMessageColumn.IM_READ_STATUS));
					int imState = cursor.getInt(cursor.getColumnIndex(IMMessageColumn.IM_SEND_STATUS));
					String dateCreated = cursor.getString(cursor.getColumnIndex(IMMessageColumn.IM_DATE_CREATE));
					String curCreated = cursor.getString(cursor.getColumnIndex(IMMessageColumn.IM_CURRENT_DATE));
					String userData = cursor.getString(cursor.getColumnIndex(IMMessageColumn.IM_USER_DATE));
					String messageContent = cursor.getString(cursor.getColumnIndex(IMMessageColumn.IM_MESSAGE_CONTENT));
					String fileUrl = cursor.getString(cursor.getColumnIndex(IMMessageColumn.IM_FILE_URL));
					String filePath = cursor.getString(cursor.getColumnIndex(IMMessageColumn.IM_FILE_PATH));
					String fileExt = cursor.getString(cursor.getColumnIndex(IMMessageColumn.IM_FILE_EXT));
					
					if(messageType != IMChatMessageDetail.TYPE_MSG_TEXT && !new File(filePath).exists()) {
						
						// if the file is not exist . then ingore.
						continue;
					}
					IMChatMessageDetail detail = new IMChatMessageDetail(messageId, 
							sessionId, 
							messageType, 
							groupSender, 
							isRead, 
							imState, 
							dateCreated, 
							curCreated, 
							userData, 
							messageContent, 
							fileUrl, 
							filePath, 
							fileExt);
					
					imChatMessageDetails.add(detail);
				}
			}
		} catch (Exception e) {
			e.printStackTrace();
		} finally {
			if (cursor != null) {
				cursor.close();
				cursor = null;
			}
		}
		return imChatMessageDetails;
	}
	
	
	/**
	 * for version 3.5 
	* <p>Title: queryNewIMMessagesByMessageId</p>
	* <p>Description: </p>
	* @param messageId
	* @return the new receive im message 
	 */
	public ArrayList<IMChatMessageDetail> queryNewIMMessagesBySessionId(String sessionId) {
		if (TextUtils.isEmpty(sessionId)) {
			throw new RuntimeException("Error , messageId is " + sessionId);
		}
		Cursor cursor = null;
		ArrayList<IMChatMessageDetail> imChatMessageDetails = null;
		try {
			String where = IMMessageColumn.IM_SESSION_ID + " ='" + sessionId + "' and " + IMMessageColumn.IM_READ_STATUS + " =" + IMChatMessageDetail.STATE_UNREAD;
			cursor = sqliteDB().query(CCPDBHelper.TABLES_NAME_IM_MESSAGE, null, where, null, null, null,
					IMMessageColumn.IM_CURRENT_DATE);
			if ((cursor != null) && (cursor.getCount() > 0)) {
				imChatMessageDetails = new ArrayList<IMChatMessageDetail>();
				while (cursor.moveToNext()) {
					
					String messageId = cursor.getString(cursor.getColumnIndex(IMMessageColumn.IM_MESSAGE_ID));
					sessionId = cursor.getString(cursor.getColumnIndex(IMMessageColumn.IM_SESSION_ID));
					int messageType = cursor.getInt(cursor.getColumnIndex(IMMessageColumn.IM_MESSAGE_TYPE));
					String groupSender = cursor.getString(cursor.getColumnIndex(IMMessageColumn.IM_MESSAGE_SENDER));
					int isRead = cursor.getInt(cursor.getColumnIndex(IMMessageColumn.IM_READ_STATUS));
					int imState = cursor.getInt(cursor.getColumnIndex(IMMessageColumn.IM_SEND_STATUS));
					String dateCreated = cursor.getString(cursor.getColumnIndex(IMMessageColumn.IM_DATE_CREATE));
					String curCreated = cursor.getString(cursor.getColumnIndex(IMMessageColumn.IM_CURRENT_DATE));
					String userData = cursor.getString(cursor.getColumnIndex(IMMessageColumn.IM_USER_DATE));
					String messageContent = cursor.getString(cursor.getColumnIndex(IMMessageColumn.IM_MESSAGE_CONTENT));
					String fileUrl = cursor.getString(cursor.getColumnIndex(IMMessageColumn.IM_FILE_URL));
					String filePath = cursor.getString(cursor.getColumnIndex(IMMessageColumn.IM_FILE_PATH));
					String fileExt = cursor.getString(cursor.getColumnIndex(IMMessageColumn.IM_FILE_EXT));
					
					IMChatMessageDetail detail = new IMChatMessageDetail(messageId, 
							sessionId, 
							messageType, 
							groupSender, 
							isRead, 
							imState, 
							dateCreated, 
							curCreated, 
							userData, 
							messageContent, 
							fileUrl, 
							filePath, 
							fileExt);
					imChatMessageDetails.add(detail);
				}	
			}
		} catch (Exception e) {
			e.printStackTrace();
		} finally {
			if (cursor != null) {
				cursor.close();
				cursor = null;
			}
		}
		return imChatMessageDetails;
	}
	
	
	/**
	 * 3.5
	* <p>Title: queryNotDownloadIMVoiceMessages</p>
	* <p>Description: </p>
	* @return
	 */
	public ArrayList<IMChatMessageDetail> queryNotDownloadIMVoiceMessages() {

		Cursor cursor = null;
		ArrayList<IMChatMessageDetail> imChatMessageDetails = null;
		try {
			String where = IMMessageColumn.IM_READ_STATUS + " =" + IMChatMessageDetail.STATE_UNREAD + " and " 
			+ IMMessageColumn.IM_MESSAGE_TYPE + " IN (" + IMChatMessageDetail.TYPE_MSG_FILE + " ," + IMChatMessageDetail.TYPE_MSG_VOICE + ")";
			cursor = sqliteDB().query(CCPDBHelper.TABLES_NAME_IM_MESSAGE, null, where, null, null, null,
					IMMessageColumn.IM_CURRENT_DATE);
			if ((cursor != null) && (cursor.getCount() > 0)) {
				imChatMessageDetails = new ArrayList<IMChatMessageDetail>();
				while (cursor.moveToNext()) {
					
					String messageId = cursor.getString(cursor.getColumnIndex(IMMessageColumn.IM_MESSAGE_ID));
					String sessionId = cursor.getString(cursor.getColumnIndex(IMMessageColumn.IM_SESSION_ID));
					int messageType = cursor.getInt(cursor.getColumnIndex(IMMessageColumn.IM_MESSAGE_TYPE));
					String groupSender = cursor.getString(cursor.getColumnIndex(IMMessageColumn.IM_MESSAGE_SENDER));
					int isRead = cursor.getInt(cursor.getColumnIndex(IMMessageColumn.IM_READ_STATUS));
					int imState = cursor.getInt(cursor.getColumnIndex(IMMessageColumn.IM_SEND_STATUS));
					String dateCreated = cursor.getString(cursor.getColumnIndex(IMMessageColumn.IM_DATE_CREATE));
					String curCreated = cursor.getString(cursor.getColumnIndex(IMMessageColumn.IM_CURRENT_DATE));
					String userData = cursor.getString(cursor.getColumnIndex(IMMessageColumn.IM_USER_DATE));
					String messageContent = cursor.getString(cursor.getColumnIndex(IMMessageColumn.IM_MESSAGE_CONTENT));
					String fileUrl = cursor.getString(cursor.getColumnIndex(IMMessageColumn.IM_FILE_URL));
					String filePath = cursor.getString(cursor.getColumnIndex(IMMessageColumn.IM_FILE_PATH));
					String fileExt = cursor.getString(cursor.getColumnIndex(IMMessageColumn.IM_FILE_EXT));
					
					if(new File(filePath).exists()) {
						continue;
					}
					
					IMChatMessageDetail detail = new IMChatMessageDetail(messageId, 
							sessionId, 
							messageType, 
							groupSender, 
							isRead, 
							imState, 
							dateCreated, 
							curCreated, 
							userData, 
							messageContent, 
							fileUrl, 
							filePath, 
							fileExt);
					imChatMessageDetails.add(detail);
				}	
			}
		} catch (Exception e) {
			e.printStackTrace();
		} finally {
			if (cursor != null) {
				cursor.close();
				cursor = null;
			}
		}
		return imChatMessageDetails;
	}
	
	/**
	 * 
	* <p>Title: queryPageIMMessagesBySessionId</p>
	* <p>Description: Such as: select * from flycrocodile limit 15 offset 20
	* Mean: 20 records from the flycrocodile table to select 15 records
	* e.g. CCPSqliteManager.getInstance().queryPageIMMessagesBySessionId(params[0] , 1 , 1);</p>
	* @param sessionId
	* @param num    The number of the page displayed
	* @param offset  Skip many of the records;
	* @return
	 */
	public ArrayList<IMChatMessageDetail> queryPageIMMessagesBySessionId(String sessionId , int num , int offset) {
		if (TextUtils.isEmpty(sessionId)) {
			throw new RuntimeException("Error , sessionId is " + sessionId);
		}
		Cursor cursor = null;
		ArrayList<IMChatMessageDetail> imChatMessageDetails = null;
		try {
			String where = IMMessageColumn.IM_SESSION_ID + " ='" + sessionId + "'";
			String limit = offset + "," + num;
			cursor = sqliteDB().query(CCPDBHelper.TABLES_NAME_IM_MESSAGE, null, where, null, null, null,
					IMMessageColumn.IM_CURRENT_DATE , limit);
			if ((cursor != null) && (cursor.getCount() > 0)) {
				imChatMessageDetails = new ArrayList<IMChatMessageDetail>();
				while (cursor.moveToNext()) {
					
					String messageId = cursor.getString(cursor.getColumnIndex(IMMessageColumn.IM_MESSAGE_ID));
					//sessionId = cursor.getString(cursor.getColumnIndex(IMMessageColumn.IM_SESSION_ID));
					int messageType = cursor.getInt(cursor.getColumnIndex(IMMessageColumn.IM_MESSAGE_TYPE));
					String groupSender = cursor.getString(cursor.getColumnIndex(IMMessageColumn.IM_MESSAGE_SENDER));
					int isRead = cursor.getInt(cursor.getColumnIndex(IMMessageColumn.IM_READ_STATUS));
					int imState = cursor.getInt(cursor.getColumnIndex(IMMessageColumn.IM_SEND_STATUS));
					String dateCreated = cursor.getString(cursor.getColumnIndex(IMMessageColumn.IM_DATE_CREATE));
					String curCreated = cursor.getString(cursor.getColumnIndex(IMMessageColumn.IM_CURRENT_DATE));
					String userData = cursor.getString(cursor.getColumnIndex(IMMessageColumn.IM_USER_DATE));
					String messageContent = cursor.getString(cursor.getColumnIndex(IMMessageColumn.IM_MESSAGE_CONTENT));
					String fileUrl = cursor.getString(cursor.getColumnIndex(IMMessageColumn.IM_FILE_URL));
					String filePath = cursor.getString(cursor.getColumnIndex(IMMessageColumn.IM_FILE_PATH));
					String fileExt = cursor.getString(cursor.getColumnIndex(IMMessageColumn.IM_FILE_EXT));
					
					IMChatMessageDetail detail = new IMChatMessageDetail(messageId, 
							sessionId, 
							messageType, 
							groupSender, 
							isRead, 
							imState, 
							dateCreated, 
							curCreated, 
							userData, 
							messageContent, 
							fileUrl, 
							filePath, 
							fileExt);
					
					imChatMessageDetails.add(detail);
				}
			}
		} catch (Exception e) {
			e.printStackTrace();
		} finally {
			if (cursor != null) {
				cursor.close();
				cursor = null;
			}
		}
		return imChatMessageDetails;
	}
	
	/**
	 * As the database inserts a new system message
	 */
	public void  insertIMMessage(IMChatMessageDetail imChatMessageDetail) throws SQLException {

		if (imChatMessageDetail == null) {
			throw new SQLException("[AbstractSQLManager] The inserted data is empty : " + imChatMessageDetail);
		}
		
		if(!TextUtils.isEmpty(isExistsIMmessageId(imChatMessageDetail.getMessageId()))){
			return;
		}

		ContentValues values = null;
		try {
			values = new ContentValues();
			
			values.put(IMMessageColumn.IM_MESSAGE_ID, imChatMessageDetail.getMessageId());
			values.put(IMMessageColumn.IM_SESSION_ID, imChatMessageDetail.getSessionId());
			values.put(IMMessageColumn.IM_MESSAGE_TYPE, imChatMessageDetail.getMessageType());
			values.put(IMMessageColumn.IM_MESSAGE_SENDER, imChatMessageDetail.getGroupSender());
			values.put(IMMessageColumn.IM_READ_STATUS, imChatMessageDetail.getReadStatus());
			values.put(IMMessageColumn.IM_SEND_STATUS, imChatMessageDetail.getImState());
			values.put(IMMessageColumn.IM_DATE_CREATE, imChatMessageDetail.getDateCreated());
			values.put(IMMessageColumn.IM_CURRENT_DATE, imChatMessageDetail.getCurDate());
			values.put(IMMessageColumn.IM_USER_DATE, imChatMessageDetail.getUserData());
			values.put(IMMessageColumn.IM_MESSAGE_CONTENT, imChatMessageDetail.getMessageContent());
			values.put(IMMessageColumn.IM_FILE_URL, imChatMessageDetail.getFileUrl());
			values.put(IMMessageColumn.IM_FILE_PATH, imChatMessageDetail.getFilePath());
			values.put(IMMessageColumn.IM_FILE_EXT, imChatMessageDetail.getFileExt());
			values.put(IMMessageColumn.IM_DURATION, imChatMessageDetail.getDuration());

			sqliteDB().insert(CCPDBHelper.TABLES_NAME_IM_MESSAGE, null, values);
		} catch (Exception e) {
			e.printStackTrace();
			throw new SQLException(e.getMessage());
		} finally {
			if (values != null) {
				values.clear();
				values = null;
			}
		}
		    
	}
	

	/**
	 * Query the existence of the group ID
	 * @param groupId
	 * @return
	 * @throws SQLException
	 */
	public String isExistsIMmessageId(String messageId) throws SQLException {
		if (TextUtils.isEmpty(messageId)) {
			return null;
		}
		Cursor cursor = null;
		try {
			String where = IMMessageColumn.IM_MESSAGE_ID + " ='" + messageId + "'";
			cursor = sqliteDB().query(CCPDBHelper.TABLES_NAME_IM_MESSAGE, new String[]{IMMessageColumn.IM_MESSAGE_ID}, where, null, null, null, null);
			if (cursor != null && cursor.getCount() > 0) {
				if(cursor.moveToFirst()) {
					return cursor.getString(cursor.getColumnIndex(IMMessageColumn.IM_MESSAGE_ID));
				}
			}
		} catch (Exception e) {
			e.printStackTrace();
		} finally {
			if (cursor != null) {
				cursor.close();
				cursor = null;
			}
		}
		return null;
	}
	
	/**
	 * Delete all IM messages
	 * @throws SQLException
	 */
	public void deleteAllIMMessage() throws SQLException {
		try {
			sqliteDB().delete(CCPDBHelper.TABLES_NAME_IM_MESSAGE, null, null);
		} catch (Exception e) {
			throw new SQLException(e.getMessage());
		}
	}
	
	/**
	 *  query voice or accessories are stored in local path according to the session id
	 * @param sessionId
	 * @return
	 * @throws SQLException
	 */
	public ArrayList<String> queryIMMessageFileLocalPathBySession(String sessionId) throws SQLException {
		if(TextUtils.isEmpty(sessionId)) {
			throw new SQLException("Sql execute error , that sessionId is " + sessionId);
		}
		Cursor cursor = null;
		ArrayList<String> filePaths = null;
		try {
			String where = IMMessageColumn.IM_SESSION_ID + "='" + sessionId + "' and " 
							+ IMMessageColumn.IM_MESSAGE_TYPE + " <> " + IMChatMessageDetail.TYPE_MSG_TEXT;
			cursor = sqliteDB().query(CCPDBHelper.TABLES_NAME_IM_MESSAGE,
					new String[] {IMMessageColumn.IM_FILE_PATH }, where, null,
					null, null, null);
			filePaths = new ArrayList<String>();
			if ((cursor != null) && (cursor.getCount() > 0)) {
				while (cursor.moveToNext()) {
					String filePath = cursor.getString(cursor.getColumnIndex(IMMessageColumn.IM_FILE_PATH));
					filePaths.add(filePath);
				}

			}
		} catch (Exception e) {
			Log4Util.d(CCPHelper.DEMO_TAG,
					"[AbstractSQLManager] queryIMMessageFileLocalPathBySession: " + e.toString());
		} finally {
			if (cursor != null) {
				cursor.close();
				cursor = null;
			}
		}
		return filePaths;
	}
	
	/**
	 * Delete the record only to send success or failed to send message to delete operation
	 * @param sessionId
	 * @throws SQLException
	 */
	public void deleteIMMessageBySessionId(String sessionId) throws SQLException {
		try {
			String where = IMMessageColumn.IM_SESSION_ID + "='" + sessionId + "'";
			sqliteDB().delete(CCPDBHelper.TABLES_NAME_IM_MESSAGE, where, null);
		} catch (Exception e) {
			throw new SQLException(e.getMessage());
		}
	}
	
	
	/**
	 * Delete the message where messageID
	 * @param sessionId
	 * @throws SQLException
	 */
	public void deleteIMMessageByMessageId(String messageId) throws SQLException {
		try {
			String where = IMMessageColumn.IM_MESSAGE_ID + "='" + messageId + "'";
			sqliteDB().delete(CCPDBHelper.TABLES_NAME_IM_MESSAGE, where, null);
		} catch (Exception e) {
			throw new SQLException(e.getMessage());
		}
	}
	
	/**
	 * Depending on the type of update database system message
	 * @param messageId
	 * @param type
	 * @throws SQLException
	 */
	public void updateIMMessageStatus(String messageId , int type )
			throws SQLException {
		if (TextUtils.isEmpty(messageId)) {
			throw new SQLException("The insterted message ID is empty ：" + messageId);
		}

		ContentValues values = null;
		try {
			final String where = IMMessageColumn.IM_MESSAGE_ID + " ='" + messageId + "'";
			values = new ContentValues();
			values.put(IMMessageColumn.IM_READ_STATUS, type);
			sqliteDB().update(CCPDBHelper.TABLES_NAME_IM_MESSAGE, values,
					where, null);

		} catch (Exception e) {
			e.printStackTrace();
			throw new SQLException(e.getMessage());
		} finally {
			if (values != null) {
				values.clear();
				values = null;
			}
		}
	}
	
	/**
	 * Depending on the type of update database system message
	 * @param messageId
	 * @param type
	 * @throws SQLException
	 */
	public void updateIMMessageDate(String messageId , String dateCreate )
			throws SQLException {
		if (TextUtils.isEmpty(messageId)) {
			throw new SQLException("The insterted message ID is empty ：" + messageId);
		}

		ContentValues values = null;
		try {
			final String where = IMMessageColumn.IM_MESSAGE_ID + " ='" + messageId + "'";
			values = new ContentValues();
			values.put(IMMessageColumn.IM_CURRENT_DATE, dateCreate);
			sqliteDB().update(CCPDBHelper.TABLES_NAME_IM_MESSAGE, values,
					where, null);

		} catch (Exception e) {
			e.printStackTrace();
			throw new SQLException(e.getMessage());
		} finally {
			if (values != null) {
				values.clear();
				values = null;
			}
		}
	}
	
	/**
	 * Depending on the type of update database system message
	 * @param messageId
	 * @param type
	 * @throws SQLException
	 */
	public void updateIMMessageStatusBySessionId(String sessionId , int type ) throws SQLException {
		if (TextUtils.isEmpty(sessionId)) {
			throw new SQLException("The IM sessionId is empty ：" + sessionId);
		}
		
		ContentValues values = null;
		try {
			final String where = IMMessageColumn.IM_SESSION_ID + " ='" + sessionId + "'";
			values = new ContentValues();
			values.put(IMMessageColumn.IM_READ_STATUS, type);
			sqliteDB().update(CCPDBHelper.TABLES_NAME_IM_MESSAGE, values,
					where, null);
			
		} catch (Exception e) {
			e.printStackTrace();
			throw new SQLException(e.getMessage());
		} finally {
			if (values != null) {
				values.clear();
				values = null;
			}
		}
	}
	/**
	 * Depending on the type of update database system message
	 * @param messageId
	 * @param type
	 * @throws SQLException
	 */
	public void updateIMMessageUnreadStatusToReadBySessionId(String sessionId , int type ) throws SQLException {
		if (TextUtils.isEmpty(sessionId)) {
			throw new SQLException("The IM sessionId is empty ：" + sessionId);
		}
		
		ContentValues values = null;
		try {
			final String where = IMMessageColumn.IM_SESSION_ID + " ='" + sessionId + "' and " + IMMessageColumn.IM_READ_STATUS + " =" + IMChatMessageDetail.STATE_UNREAD;
			values = new ContentValues();
			values.put(IMMessageColumn.IM_READ_STATUS, type);
			sqliteDB().update(CCPDBHelper.TABLES_NAME_IM_MESSAGE, values,
					where, null);
			
		} catch (Exception e) {
			e.printStackTrace();
			throw new SQLException(e.getMessage());
		} finally {
			if (values != null) {
				values.clear();
				values = null;
			}
		}
	}
	
	/**
	 * Depending on the type of update database system message
	 * @param messageId
	 * @param type
	 * @throws SQLException
	 * @version 3.5
	 */
	public void updateIMMessageUnreadStatusToRead(ArrayList<IMChatMessageDetail> imChatMessages, int type ) throws SQLException {
		if (imChatMessages == null) {

			return;
		}
		
		for(IMChatMessageDetail chatMessageDetail :imChatMessages) {
			ContentValues values = null;
			try {
				final String where = IMMessageColumn.IM_MESSAGE_ID + " ='" + chatMessageDetail.getMessageId() + "' and " + IMMessageColumn.IM_READ_STATUS + " =" + IMChatMessageDetail.STATE_UNREAD;
				values = new ContentValues();
				values.put(IMMessageColumn.IM_READ_STATUS, type);
				sqliteDB().update(CCPDBHelper.TABLES_NAME_IM_MESSAGE, values,
						where, null);
				
			} catch (Exception e) {
				e.printStackTrace();
				throw new SQLException(e.getMessage());
			} finally {
				if (values != null) {
					values.clear();
					values = null;
				}
			}
		}

	}
	
	/**
	 * All is in sending messages in the transmission failure
	 * @throws SQLException
	 */
	public void  updateAllIMMessageSendFailed() throws SQLException {
		
		ContentValues values = null;
		try {
			String where = IMMessageColumn.IM_SEND_STATUS+ " = " + IMChatMessageDetail.STATE_IM_SENDING;
			
			values = new ContentValues();
			values.put(IMMessageColumn.IM_SEND_STATUS, IMChatMessageDetail.STATE_IM_SEND_FAILED);
			
			sqliteDB().update(CCPDBHelper.TABLES_NAME_IM_MESSAGE, values,
					where, null);
		} catch (Exception e) {
			e.printStackTrace();
			throw new SQLException(e.getMessage());
		} finally {
			if (values != null) {
				values.clear();
				values = null;
			}
		}
		
	}
	
	/**
	 * Updates status message
	 * @throws SQLException
	 */
	public void  updateIMMessageSendStatusByMessageId(String messageId ,int status) throws SQLException {
		if (TextUtils.isEmpty(messageId)) {
			throw new SQLException("The IM message messageId is empty ：" + messageId);
		}
		ContentValues values = null;
		try {
			String where = IMMessageColumn.IM_SEND_STATUS+ " = " + IMChatMessageDetail.STATE_IM_SENDING
							+ " and " + IMMessageColumn.IM_MESSAGE_ID + " = '" + messageId + "'";
			
			values = new ContentValues();
			values.put(IMMessageColumn.IM_SEND_STATUS, status);
			
			sqliteDB().update(CCPDBHelper.TABLES_NAME_IM_MESSAGE, values,
					where, null);
		} catch (Exception e) {
			e.printStackTrace();
			throw new SQLException(e.getMessage());
		} finally {
			if (values != null) {
				values.clear();
				values = null;
			}
		}
		
	}
	
	
	
	/**
	 * 查询IM信息会话列表
	 * @return
	 * @throws Exception
	 */
	public ArrayList<IMConversation> queryIMConversation() throws Exception {
		Cursor cursor = null;
		Cursor rawQuery = null;
		ArrayList<IMConversation> imConversations = null;
		try {
			String sql = "select aa.* , bb.unread_num from ((select a." 
						+ IMMessageColumn.IM_SESSION_ID + " ,a." 
						+ IMMessageColumn.IM_CURRENT_DATE+ " ,a."
						+ IMMessageColumn.IM_MESSAGE_CONTENT + ",a."
						+ IMMessageColumn.IM_MESSAGE_TYPE +" , b."
						+ IMGroupInfoColumn.GROUP_NAME + " from ( " 
						+ CCPDBHelper.TABLES_NAME_IM_MESSAGE + " as a LEFT JOIN " 
						+ CCPDBHelper.TABLES_NAME_IM_GROUP_INFO+ " as b ON a."
						+ IMMessageColumn.IM_SESSION_ID + " = b."
						+ IMGroupInfoColumn.GROUP_ID + ") group by " 
						+ IMMessageColumn.IM_SESSION_ID + " order by " 
						+ IMMessageColumn.IM_CURRENT_DATE + " " 
						+ CCPDBHelper.DESC+" )as aa LEFT JOIN (select count("+IMMessageColumn.IM_READ_STATUS+") " + BaseColumn.UNREAD_NUM + " ," 
						+ IMMessageColumn.IM_SESSION_ID
						+ " from " + CCPDBHelper.TABLES_NAME_IM_MESSAGE + " where "+IMMessageColumn.IM_READ_STATUS+" = " + IMChatMessageDetail.STATE_UNREAD
						+ " group by " + IMMessageColumn.IM_SESSION_ID  
						+ ") as bb ON aa."
						+ IMMessageColumn.IM_SESSION_ID + " = bb."
						+ IMMessageColumn.IM_SESSION_ID + " )";
			
			cursor = sqliteDB().rawQuery(sql, null);

			if ((cursor != null) && (cursor.getCount() > 0)) {
				imConversations = new ArrayList<IMConversation>();
				while (cursor.moveToNext()) {
					String contactId = cursor.getString(cursor.getColumnIndex(IMMessageColumn.IM_SESSION_ID));
					String groupName = cursor.getString(cursor.getColumnIndex(IMGroupInfoColumn.GROUP_NAME));
					String dateCreated = cursor.getString(cursor.getColumnIndex(IMMessageColumn.IM_CURRENT_DATE));
					String msgContent = cursor.getString(cursor.getColumnIndex(IMMessageColumn.IM_MESSAGE_CONTENT));
					int msgType = cursor.getInt(cursor.getColumnIndex(IMMessageColumn.IM_MESSAGE_TYPE));
					int unreadNum = cursor.getInt(cursor.getColumnIndex(BaseColumn.UNREAD_NUM));
					
					IMConversation session = new IMConversation();
					session.setId(contactId); // 会话
					session.setDateCreated(dateCreated);
					session.setUnReadNum(unreadNum + "");
					session.setType(IMConversation.CONVER_TYPE_MESSAGE);
					if(TextUtils.isEmpty(groupName)) {
						session.setContact(contactId);
					} else{
						session.setContact(groupName);
					}
						if(msgType == IMChatMessageDetail.TYPE_MSG_TEXT) {
							session.setRecentMessage(msgContent);
						} else if (msgType == IMChatMessageDetail.TYPE_MSG_FILE ) {
							session.setRecentMessage("[文件]");
						} else if (msgType == IMChatMessageDetail.TYPE_MSG_VOICE ) {
							session.setRecentMessage("[语音]");
						}
					imConversations.add(session);
				}
			}
			
			// String sSql = "select * from im_group_notice as a LEFT JOIN (select count(*) from im_group_notice where isread = 1) as b"
			String sSql = "SELECT a.*,b.unread_num FROM (select * from " 
				+ CCPDBHelper.TABLES_NAME_IM_GROUP_NOTICE + " order by " 
				+ IMGroupNoticeColumn.NOTICE_DATECREATED + " desc limit 1) AS a LEFT JOIN (SELECT COUNT(*) unread_num FROM " 
				+ CCPDBHelper.TABLES_NAME_IM_GROUP_NOTICE  + " WHERE " 
				+ IMGroupNoticeColumn.NOTICE_READ_STATUS + " = " 
				+ IMChatMessageDetail.STATE_UNREAD + ") AS b";
			
			
			rawQuery = sqliteDB().rawQuery(sSql, null);
			IMConversation noticeImConversation = null;
			if(rawQuery != null && rawQuery.getCount() > 0 ) {
				if(rawQuery.moveToFirst()) {
					String dateCreated = rawQuery.getString(rawQuery.getColumnIndex(IMGroupNoticeColumn.NOTICE_DATECREATED));
					String msgContent = rawQuery.getString(rawQuery.getColumnIndex(IMGroupNoticeColumn.NOTICE_VERIFYMSG));
					int unreadNum = rawQuery.getInt(rawQuery.getColumnIndex(BaseColumn.UNREAD_NUM));
					
					noticeImConversation = new IMConversation();
					noticeImConversation.setId("group_nitice");
					noticeImConversation.setContact("系统消息");
					noticeImConversation.setDateCreated(dateCreated);
					noticeImConversation.setRecentMessage(msgContent);
					noticeImConversation.setType(IMConversation.CONVER_TYPE_SYSTEM);
					noticeImConversation.setUnReadNum(unreadNum + "");
				}
			}
			
			if(imConversations == null ) {
				imConversations = new ArrayList<IMConversation>();
			}
			
			if(noticeImConversation != null ) {
				imConversations.add(0, noticeImConversation);
			}
			
		} catch (Exception e) {
			e.printStackTrace();
			Log4Util.d(CCPHelper.DEMO_TAG, "[AbstractSQLManager] queryIMConversation: "
					+ e.toString());
		} finally {
			if (cursor != null) {
				cursor.close();
				cursor = null;
			}
			if (rawQuery != null) {
				rawQuery.close();
				rawQuery = null;
			}
		}
		return imConversations;
	}	
	
	// ---------------------------------------------------------------------------------------------
	// system notice database operation
	
	/**
	 * As the database inserts a new system message
	 */
	public void  insertNoticeMessage(IMSystemMessage imSystemMessage) throws SQLException {

		if (imSystemMessage == null) {
			throw new SQLException("[AbstractSQLManager] The inserted data is empty  : " + imSystemMessage);
		}
		if ((TextUtils.isEmpty(imSystemMessage.getGroupId()))) {
			throw new SQLException("[AbstractSQLManager] notice message data id update is empty : " + imSystemMessage.getGroupId());
		}

		ContentValues values = null;
		try {
			
			values = new ContentValues();
			values.put(IMGroupNoticeColumn.NOTICE_TYPE, imSystemMessage.getMessageType());
			values.put(IMGroupNoticeColumn.NOTICE_VERIFYMSG, imSystemMessage.getVerifyMsg());
			values.put(IMGroupNoticeColumn.NOTICE_OPERATION_STATE, imSystemMessage.getState());
			values.put(IMGroupNoticeColumn.NOTICE_GROUPID, imSystemMessage.getGroupId());
			values.put(IMGroupNoticeColumn.NOTICE_WHO, imSystemMessage.getWho());
			values.put(IMGroupNoticeColumn.NOTICE_READ_STATUS, imSystemMessage.getIsRead());
			values.put(IMGroupNoticeColumn.NOTICE_DATECREATED, imSystemMessage.getCurDate());

			sqliteDB().insert(CCPDBHelper.TABLES_NAME_IM_GROUP_NOTICE, null, values);
		} catch (Exception e) {
			e.printStackTrace();
			throw new SQLException(e.getMessage());
		} finally {
			if (values != null) {
				values.clear();
				values = null;
			}
		}
		    
	}
	
	/**
	 * Group message notification
	 * Because the group received InstanceMsg
	 * So the need to convert IMSystemMessage database operations
	 * 
	 * @param instanceMsg
	 * @param type
	 * @throws SQLException
	 */
	public void  insertNoticeMessage(InstanceMsg instanceMsg , int type) throws SQLException {
		
		String dateCreated = CCPUtil.getDateCreate();
		String verifyMsg = "";
		String groupId = "";
		String proposer = "";
		int state = IMSystemMessage.SYSTEM_MESSAGE_NONEED_REPLAY;
		if(type == IMSystemMessage.SYSTEM_TYPE_APPLY_JOIN || type == IMSystemMessage.SYSTEM_TYPE_APPLY_JOIN_UNVALIDATION) {
			IMProposerMsg imProposerMsg = (IMProposerMsg) instanceMsg;
			verifyMsg = imProposerMsg.getDeclared();
			groupId = imProposerMsg.getGroupId();
			proposer = imProposerMsg.getProposer();
			
			if (type == IMSystemMessage.SYSTEM_TYPE_APPLY_JOIN_UNVALIDATION) {
				verifyMsg = proposer + " 加入了群组";
			} else{
				state = IMSystemMessage.SYSTEM_MESSAGE_NEED_REPLAY;
			}
			
		} else if (type == IMSystemMessage.SYSTEM_TYPE_INVITE_JOIN) {
			IMInviterMsg imInviterMsg = (IMInviterMsg) instanceMsg;
			groupId = imInviterMsg.getGroupId();
			verifyMsg = imInviterMsg.getDeclared();
			proposer = imInviterMsg.getAdmin();
			state = IMSystemMessage.SYSTEM_MESSAGE_NEED_REPLAY;
			
		} else if (type == IMSystemMessage.SYSTEM_TYPE_ACCEPT_OR_REJECT_JOIN) { // REJECT OR ACCPET JOIN GROUP
			IMReplyJoinGroupMsg imAcceptRejectMsg = (IMReplyJoinGroupMsg) instanceMsg;
			groupId = imAcceptRejectMsg.getGroupId();
			verifyMsg = imAcceptRejectMsg.getConfirm().equals("0") ? "管理员通过了您的加群请求":"管理员拒绝了您的加群请求";
			proposer = imAcceptRejectMsg.getAdmin();
			
		} else if (type == IMSystemMessage.SYSTEM_TYPE_REMOVE) {
			IMRemoveMemeberMsg imrMemeberMsg = (IMRemoveMemeberMsg) instanceMsg;
			groupId = imrMemeberMsg.getGroupId();
			if(CCPConfig.VoIP_ID.equals(imrMemeberMsg.getWho())) {
				verifyMsg = "您被群管理员移除出群组";
			} else {
				verifyMsg = imrMemeberMsg.getWho() + "被群管理员移除出群组";
			}
			
		} else if (type == IMSystemMessage.SYSTEM_TYPE_GROUP_DISMISS) {
			IMDismissGroupMsg imDismissGroupMsg = (IMDismissGroupMsg) instanceMsg;
			groupId = imDismissGroupMsg.getGroupId();
			verifyMsg = "群管理员解散了群组";
			
		} else if (type == IMSystemMessage.SYSTEM_TYPE_GROUP_MEMBER_QUIT) {
			IMQuitGroupMsg imQuitGroupMsg = (IMQuitGroupMsg) instanceMsg;
			groupId = imQuitGroupMsg.getGroupId();
			proposer = imQuitGroupMsg.getMember();
			verifyMsg = "群成员" + proposer + "退出了群组";
		} else if (type == IMSystemMessage.SYSTEM_TYPE_REPLY_GROUP_APPLY) {
			IMInviterJoinGroupReplyMsg replyMsg = (IMInviterJoinGroupReplyMsg) instanceMsg;
			groupId = replyMsg.getGroupId();
			proposer = replyMsg.getMember();
			if("0".equals(replyMsg.getConfirm())) {
				verifyMsg = "群管理员" + replyMsg.getAdmin() + " 邀请 "  + proposer + "加入群组";
			} else {
				verifyMsg = proposer + "拒绝加入群组";
			}
		}
		/**
			private int state;
			private String groupId;
			private String who;
			private int isRead;
			private String curDate;
		 */
		IMSystemMessage imSystemMessage = new IMSystemMessage();
		imSystemMessage.setCurDate(dateCreated);
		imSystemMessage.setVerifyMsg(verifyMsg);
		imSystemMessage.setGroupId(groupId);
		imSystemMessage.setWho(proposer);
		imSystemMessage.setState(state);
		imSystemMessage.setIsRead(IMSystemMessage.STATE_UNREAD);
		imSystemMessage.setMessageType(type);
		try {
			insertNoticeMessage(imSystemMessage);
		} catch (SQLException e) {
			e.printStackTrace();
		}
		
	}
	
	/**
	 * 查询所有系统验证消息
	 * @return
	 * @throws Exception
	 */
	public ArrayList<IMSystemMessage> queryNoticeMessages() throws Exception {
		Cursor cursor = null;
		ArrayList<IMSystemMessage> imSystemMsgList = null;
		try {
			cursor = sqliteDB().query(CCPDBHelper.TABLES_NAME_IM_GROUP_NOTICE, null, null, null, null, null, IMGroupNoticeColumn.NOTICE_DATECREATED + " desc" );

			if ((cursor != null) && (cursor.getCount() > 0)) {
				imSystemMsgList = new ArrayList<IMSystemMessage>();
				while (cursor.moveToNext()) {
					
					String id = cursor.getString(cursor.getColumnIndex(IMGroupNoticeColumn.NOTICE_ID));
					int msgType = cursor.getInt(cursor.getColumnIndex(IMGroupNoticeColumn.NOTICE_TYPE));
					String verifyMsg = cursor.getString(cursor.getColumnIndex(IMGroupNoticeColumn.NOTICE_VERIFYMSG));
					int state = cursor.getInt(cursor.getColumnIndex(IMGroupNoticeColumn.NOTICE_OPERATION_STATE));
					String groupId = cursor.getString(cursor.getColumnIndex(IMGroupNoticeColumn.NOTICE_GROUPID));
					String who = cursor.getString(cursor.getColumnIndex(IMGroupNoticeColumn.NOTICE_WHO));
					int isRead = cursor.getInt(cursor.getColumnIndex(IMGroupNoticeColumn.NOTICE_READ_STATUS));
					String dateCreated = cursor.getString(cursor.getColumnIndex(IMGroupNoticeColumn.NOTICE_DATECREATED));

					IMSystemMessage iSystemMessage = new IMSystemMessage(id, msgType, verifyMsg, state, groupId, who, isRead, dateCreated);
					
					imSystemMsgList.add(iSystemMessage);
				}
			}
		} catch (Exception e) {
			Log4Util.d(CCPHelper.DEMO_TAG, "[AbstractSQLManager] queryMessageSession: "
					+ e.toString());
		} finally {
			if (cursor != null) {
				cursor.close();
				cursor = null;
			}
		}
		return imSystemMsgList;
	}
	
	/**
	 * 查询系统信息未读信息数
	 * @return
	 * @throws Exception
	 */
	public int queryNoticeMessageUnreadNum() throws Exception {
		Cursor cursor = null;
		try {
			
			
			String where = IMGroupNoticeColumn.NOTICE_READ_STATUS + " = " 
							+ IMSystemMessage.STATE_UNREAD ;
			String sql = "select count(*) " 
				+ BaseColumn.UNREAD_NUM + " from " 
				+ CCPDBHelper.TABLES_NAME_IM_GROUP_NOTICE
				+ " where " + where;
			cursor = sqliteDB().rawQuery(sql, null);
			
			if ((cursor != null) && (cursor.getCount() > 0)) {
				if(cursor.moveToFirst()) {
					int columnIndex = cursor.getColumnIndex(BaseColumn.UNREAD_NUM);
					return cursor.getInt(columnIndex);
				}
				
				return -1 ;
			}
		} catch (Exception e) {
			Log4Util.d(CCPHelper.DEMO_TAG, "[VoiceSQLManager] querySystemUnreadNum: "
					+ e.toString());
		} finally {
			if (cursor != null) {
				cursor.close();
				cursor = null;
			}
		}
		
		return -1;
	}
	

	/**
	 * Depending on the type of update database system message
	 * @param messageId
	 * @param type
	 * @throws SQLException
	 */
	public void updateNoticeMessageStatus(String messageId , int type )
			throws SQLException {
		if (TextUtils.isEmpty(messageId)) {
			throw new SQLException("The notice message ID is empty ：" + messageId);
		}

		ContentValues values = null;
		try {
			final String where = IMGroupNoticeColumn.NOTICE_ID + " ='" + messageId + "'";
			values = new ContentValues();
			values.put(IMGroupNoticeColumn.NOTICE_READ_STATUS, type);
			sqliteDB().update(CCPDBHelper.TABLES_NAME_IM_GROUP_NOTICE, values,
					where, null);

		} catch (Exception e) {
			e.printStackTrace();
			throw new SQLException(e.getMessage());
		} finally {
			if (values != null) {
				values.clear();
				values = null;
			}
		}
	}
	
	/**
	 * Depending on the type of update database system message
	 * @param messageId
	 * @param type
	 * @throws SQLException
	 */
	public void updateNoticeMessageOperationStatus(String messageId , int type )
		throws SQLException {
		if (TextUtils.isEmpty(messageId)) {
			throw new SQLException("The notice message ID is empty ：" + messageId);
		}
		
		ContentValues values = null;
		try {
			final String where = IMGroupNoticeColumn.NOTICE_ID + " ='" + messageId + "'";
			values = new ContentValues();
			values.put(IMGroupNoticeColumn.NOTICE_OPERATION_STATE, type);
			sqliteDB().update(CCPDBHelper.TABLES_NAME_IM_GROUP_NOTICE, values,
					where, null);
			
		} catch (Exception e) {
			e.printStackTrace();
			throw new SQLException(e.getMessage());
		} finally {
			if (values != null) {
				values.clear();
				values = null;
			}
		}
	}
	
	/**
	 * Depending on the type of update database system message
	 * @param messageId
	 * @param type
	 * @throws SQLException
	 */
	public void updateAllNoticeMessageStatus(int type)
		throws SQLException {
		ContentValues values = null;
		try {
			values = new ContentValues();
			values.put(IMGroupNoticeColumn.NOTICE_READ_STATUS, type);
			sqliteDB().update(CCPDBHelper.TABLES_NAME_IM_GROUP_NOTICE, values,
					null, null);
			
		} catch (Exception e) {
			e.printStackTrace();
			throw new SQLException(e.getMessage());
		} finally {
			if (values != null) {
				values.clear();
				values = null;
			}
		}
	}
	
	/**
	 * Depending on the type of update database system message
	 * @param messageId
	 * @param type
	 * @throws SQLException
	 */
	public void updateNoticeMessageRead(String messageId)throws SQLException {
		updateNoticeMessageStatus(messageId, IMSystemMessage.STATE_READED);
	}
	
	/**
	 * delete all system message ..
	 * @throws SQLException
	 */
	public void deleteAllNoticeMessage() throws SQLException {
		try {
			sqliteDB().delete(CCPDBHelper.TABLES_NAME_IM_GROUP_NOTICE, null, null);
		} catch (Exception e) {
			throw new SQLException(e.getMessage());
		}
	}

	/* (non-Javadoc)
	 * @see com.voice.demo.sqlite.AbstractSQLManager#release()
	 */
	@Override
	protected void release() {
		instance = null;
	}
}
