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

import com.hisun.phone.core.voice.util.Log4Util;
import com.voice.demo.CCPApplication;
import com.voice.demo.tools.CCPConfig;
import com.voice.demo.ui.CCPHelper;

import android.content.Context;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteDatabase.CursorFactory;
import android.database.sqlite.SQLiteOpenHelper;

/**
 * 
 * @author Jorstin Chan
 * @version Time: 2013-8-28
 */
public abstract class AbstractSQLManager {
	public static final int VERSION_2 = 2;
	public static final int VERSION_3560 = 3560;
	public static final String TAG = AbstractSQLManager.class.getName();
	private static CCPDBHelper databaseHelper;
	private static SQLiteDatabase sqliteDB;
	
	private String DATABASE_NAME = null;

	public AbstractSQLManager() {
		openDatabase(CCPApplication.getInstance(), CCPApplication.getInstance().getVersionCode());
	}

	private void openDatabase(Context context, int databaseVersion) {
		DATABASE_NAME = CCPConfig.VoIP_ID + "_ccp_demo.db";
		
		Log4Util.d(CCPHelper.DEMO_TAG, "AbstractSQLManager.openDatabase database name : " + DATABASE_NAME);
		if (databaseHelper == null) {
			databaseHelper = new CCPDBHelper(context, databaseVersion);
		}
		if (sqliteDB == null) {
			sqliteDB = databaseHelper.getWritableDatabase();
		}
	}

	public void destroy() {
		try {
			if (databaseHelper != null) {
				databaseHelper.close();
				databaseHelper = null;
			}
			
			closeDB();
			
			release();
			
		} catch (Exception e) {
			Log4Util.e(e.toString());
		}
	}

	private void open(boolean isReadonly) {
		if(databaseHelper == null ) {
			openDatabase(CCPApplication.getInstance(), CCPApplication.getInstance().getVersionCode());
		}
		
		if (sqliteDB == null) {
			if (isReadonly) {
				sqliteDB = databaseHelper.getReadableDatabase();
			} else {
				sqliteDB = databaseHelper.getWritableDatabase();
			}
		}
	}

	public final void reopen() {
		closeDB();
		open(false);
		Log4Util.w("[SQLiteManager] reopen this db.");
	}

	private void closeDB() {
		if (sqliteDB != null) {
			sqliteDB.close();
			sqliteDB = null;
		}
	}

	protected final SQLiteDatabase sqliteDB() {
		return sqliteDB(false);
	}
	
	/**
	 * @param isReadonly
	 * @return
	 */
	protected final SQLiteDatabase sqliteDB(boolean isReadonly) {
		open(isReadonly);
		return sqliteDB;
	}
	
	// ---------------------------------------------------------------
	
	class BaseColumn {
		
		public static final String UNREAD_NUM = "unread_num";    
	}

	class IMMessageColumn extends BaseColumn{
		
		public static final String IM_MESSAGE_ID 					= "MSGID"; 						// message id
		public static final String IM_SESSION_ID 					= "SESSIONID";					// Identifies a dialogue
		public static final String IM_MESSAGE_TYPE 					= "MSG_TYPE";  					// The type of information (Attached, text)
		public static final String IM_MESSAGE_SENDER 				= "SENDER";						// the message sender
		public static final String IM_READ_STATUS 					= "ISREAD";    					// if message read or not
		public static final String IM_SEND_STATUS 					= "IM_STATE";   				// Send state
		public static final String IM_DATE_CREATE 					= "CREATEDATE";   				// IM message creation time (server)
		public static final String IM_CURRENT_DATE 					= "CURDATE";   					// IM news (local time to create and manage)
		public static final String IM_USER_DATE 					= "USERDATA";   				// User defined extension field
		public static final String IM_MESSAGE_CONTENT 				= "MSGCONTENT"; 				// Information content
		public static final String IM_FILE_URL 						= "FILEURL";               		// Attached url on the server.
		public static final String IM_FILE_PATH 					= "FILEPATH";               	// Local storage address (attachment sending or receiving complete)
		public static final String IM_FILE_EXT 						= "FILEEXT";               		// Attachment extension
		public static final String IM_DURATION 						= "DURATION";               	// If the attachment for the voice file, for the voice file time
	}
	
	// Group information
	class IMGroupInfoColumn extends BaseColumn{
		public static final String GROUP_ID 						= "GROUPID";
		public static final String GROUP_NAME 						= "NAME"; 						// group name 
		public static final String GROUP_OWNER	 					= "OWNER"; 						// Group creator information
		public static final String GROUP_TYPE 						= "TYPE"; 						// Group property type (whether it is normal group, VIP group.)
		public static final String GROUP_DECLARED 					= "DECLARED"; 					// The group information bulletin
		public static final String GROUP_DATECREATED 				= "CREATE_DATE"; 				// Group creation time
		public static final String GROUP_MEMBER_COUNTS				= "COUNT"; 						// The number of group members
		public static final String GROUP_PERMISSION 				= "PERMISSION"; 				// Join the required permission
		
	}
	
	class IMGroupNoticeColumn extends BaseColumn{
		public static final String NOTICE_ID 						= "ID";							// notice message id 
		public static final String NOTICE_VERIFYMSG 				= "VERIFY_MSG"; 				// i.e. Apply or invite additional reason
		public static final String NOTICE_TYPE 						= "MSGTYPE"; 					// The system notification message type (i.e.  invitation or apply)
		public static final String NOTICE_OPERATION_STATE 			= "STATE"; 						// The system message operation (i.e. rejected or by)
		public static final String NOTICE_GROUPID 					= "GROUPID"; 					// Group ID message belongs to
		public static final String NOTICE_WHO					 	= "WHO"; 						// Participants in the system message
		public static final String NOTICE_READ_STATUS 				= "ISREAD"; 					// The status of the message (read or unread)
		public static final String NOTICE_DATECREATED 				= "CURDATE"; 					// date 
		
	}
	
	
	 class CCPDBHelper extends SQLiteOpenHelper {
		static final String DESC = "DESC";
		static final String ASC = "ASC";

		static final String TABLES_NAME_IM_GROUP_INFO 			= "im_group_info";
		static final String TABLES_NAME_IM_GROUP_NOTICE 		= "im_group_notice";
		static final String TABLES_NAME_IM_MESSAGE 				= "im_message";
		

		final String[] TABLE_NAME = { TABLES_NAME_IM_GROUP_INFO,
				TABLES_NAME_IM_GROUP_NOTICE, 
				TABLES_NAME_IM_MESSAGE , 

		};

		public CCPDBHelper(Context context, int version) {
			this(context, DATABASE_NAME, null, version);
		}

		public CCPDBHelper(Context context, String name, CursorFactory factory,
				int version) {
			super(context, name, factory, version);
		}

		@Override
		public void onCreate(SQLiteDatabase db) {
			createTables(db);
		}

		@Override
		public void onUpgrade(SQLiteDatabase db, int oldVersion, int newVersion) {
			if (oldVersion != newVersion) {
				if(oldVersion <= VERSION_3560) {
					dropTableByTableName(db, new String[]{"im_message" , "group_info"});
					dropTableByTableName(db, new String[]{DATABASE_NAME});
					createTables(db);
				}
			}
		}

		/*private void updateTables(SQLiteDatabase db) {

			// If you need to update the database table structure, this
			// operation
		}*/

		void createTables(SQLiteDatabase db) {
			try {

				// Create group information table
				createTableForGroupInfo(db);

				// IM��Ϣ
				createTableForIMGroupMessage(db);

				// Create a group the notification table
				createTableForGroupNotice(db);
			} catch (Exception e) {
				Log4Util.e(TAG + " " + e.toString());
			}
		}

		private void createTableForGroupInfo(SQLiteDatabase db) {

			String sql = "CREATE TABLE IF NOT EXISTS "
					+ TABLES_NAME_IM_GROUP_INFO
					+ " ( " //ID INTEGER PRIMARY KEY AUTOINCREMENT, "
					+ IMGroupInfoColumn.GROUP_ID + " TEXT PRIMARY KEY , "
					+ IMGroupInfoColumn.GROUP_NAME + " TEXT , "
					+ IMGroupInfoColumn.GROUP_DATECREATED + " TEXT , "
					+ IMGroupInfoColumn.GROUP_DECLARED + " TEXT , "
					+ IMGroupInfoColumn.GROUP_OWNER + " TEXT , "
					+ IMGroupInfoColumn.GROUP_MEMBER_COUNTS + " INTEGER , "
					+ IMGroupInfoColumn.GROUP_PERMISSION + " INTEGER , "
					+ IMGroupInfoColumn.GROUP_TYPE + " INTEGER)";
			Log4Util.v(TAG + ":" + sql);
			db.execSQL(sql);
		}

		private void createTableForIMGroupMessage(SQLiteDatabase db) {

			String sql = "CREATE TABLE IF NOT EXISTS " + TABLES_NAME_IM_MESSAGE
					+ " ( " //ID INTEGER PRIMARY KEY AUTOINCREMENT, "
					+ IMMessageColumn.IM_MESSAGE_ID + " TEXT PRIMARY KEY , "
					+ IMMessageColumn.IM_SESSION_ID + " TEXT NOT NULL, "
					+ IMMessageColumn.IM_MESSAGE_TYPE + "  INTEGER NOT NULL, "
					+ IMMessageColumn.IM_MESSAGE_SENDER + " TEXT ," 
					+ IMMessageColumn.IM_READ_STATUS + "  INTEGER NOT NULL DEFAULT 0, " 
					+ IMMessageColumn.IM_SEND_STATUS + "  INTEGER NOT NULL, "
					+ IMMessageColumn.IM_DATE_CREATE + " TEXT , "
					+ IMMessageColumn.IM_CURRENT_DATE + " TEXT , "
					+ IMMessageColumn.IM_USER_DATE + " TEXT , "
					+ IMMessageColumn.IM_MESSAGE_CONTENT + " TEXT , "
					+ IMMessageColumn.IM_FILE_URL + " TEXT , "
					+ IMMessageColumn.IM_FILE_PATH + " TEXT , "
					+ IMMessageColumn.IM_FILE_EXT + " TEXT , "
					+ IMMessageColumn.IM_DURATION + " INTEGER)";
			Log4Util.v(TAG + ":" + sql);
			db.execSQL(sql);
		}

		private void createTableForGroupNotice(SQLiteDatabase db) {

			String sql = "CREATE TABLE IF NOT EXISTS "
					+ TABLES_NAME_IM_GROUP_NOTICE
					+ " ("+IMGroupNoticeColumn.NOTICE_ID+ " INTEGER PRIMARY KEY AUTOINCREMENT, "
					+ IMGroupNoticeColumn.NOTICE_VERIFYMSG + " TEXT , "
					+ IMGroupNoticeColumn.NOTICE_OPERATION_STATE + " INTEGER , "
					+ IMGroupNoticeColumn.NOTICE_TYPE + " INTEGER , "
					+ IMGroupNoticeColumn.NOTICE_GROUPID + " TEXT , "
					+ IMGroupNoticeColumn.NOTICE_DATECREATED + " TEXT , "
					+ IMGroupNoticeColumn.NOTICE_WHO + " TEXT , "
					+ IMGroupNoticeColumn.NOTICE_READ_STATUS + " INTEGER)";
			Log4Util.v(TAG + ":" + sql);
			db.execSQL(sql);
		}

		// ---------------------------------------------------------------------------------------------------------------
		/**
		 * Drop���б�
		 * 
		 * @param db
		 * @param TABLENAME
		 */
		void dropTableByTableName(SQLiteDatabase db, String[] TABLENAME) {
			StringBuffer sql = new StringBuffer("DROP TABLE IF EXISTS ");
			int len = sql.length();
			for (String name : TABLENAME) {
				try {
					sql.append(name);
					Log4Util.v(TAG + ":" + sql.toString());
					db.execSQL(sql.toString());
				} catch (Exception e) {
					e.printStackTrace();
				} finally {
					sql.delete(len, sql.length());
				}
			}
		}

		void alterTable(SQLiteDatabase db, String table, String text,
				String type, String def) {
			try {
				String sql = "";
				if (def == null || def.equals("")) {
					sql = "alter table " + table + " add " + text + " " + type;
				} else {
					sql = "alter table " + table + " add " + text + " " + type
							+ " default " + def;
				}
				Log4Util.v(TAG + ":" + sql);
				db.execSQL(sql);
			} catch (Exception e) {
				e.printStackTrace();
			}
		}

	}
	
	protected abstract void release();
}
