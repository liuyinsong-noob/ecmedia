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
package com.voice.demo.group;

import java.io.File;
import java.io.IOException;
import java.text.DecimalFormat;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;

import com.voice.demo.R;
import com.voice.demo.group.model.SDCardFile;
import com.voice.demo.group.utils.MimeTypesTools;
import com.voice.demo.ui.CCPBaseActivity;

import android.app.AlertDialog;
import android.app.Dialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.graphics.drawable.Drawable;
import android.os.Bundle;
import android.os.Environment;
import android.view.*;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.ArrayAdapter;
import android.widget.ImageView;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.Toast;

/**
 * 
 * @author Jorstin Chan
 * @version Time: 2013-9-11
 */
public class FileBrowserActivity extends CCPBaseActivity implements View.OnClickListener , OnItemClickListener{

	private static final String SDCARD_ROOT = Environment.getExternalStorageDirectory().getAbsolutePath();
	
	
	private TextView mFileCurrentPath;
	private ListView mFileBrowser;
	
	// Record the current parent folder
	File currentParent;

	// File an array of records all the current path of the folder
	File[] currentFiles;
	
	private String filePath = null;
	
	private ImageView mFileUplevel;

	private ArrayList<SDCardFile> sFiles;

	private String fileName;
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		
		
		handleTitleDisplay(getString(R.string.btn_title_back)
				, getString(R.string.app_title_file_browser)
				, null);
		
		initResourceRefs();
		
		initializationRootDirectory();
	}


	private void initializationRootDirectory() {
		File fileRoot = new File(SDCARD_ROOT);
		if (fileRoot.exists()) {

			currentParent = fileRoot;
			inflateListView(fileRoot.listFiles());

		}
	}


	private void initResourceRefs() {
		mFileCurrentPath = (TextView) findViewById(R.id.file_currentPath);
		
		mFileUplevel = (ImageView) findViewById(R.id.file_uplevel);
		mFileUplevel.setOnClickListener(this);
		
		mFileBrowser = (ListView) findViewById(R.id.fileBrowser);
		mFileBrowser.setOnItemClickListener(this);
		TextView fileEmpty = (TextView) findViewById(R.id.no_file_view);
		mFileBrowser.setEmptyView(fileEmpty);
		
		
	}
	
	
	@Override
	public void onItemClick(AdapterView<?> parent, View view, int position,
			long id) {
		
		// If the user clicks the file, pop-up dialog box prompts 
		// the file name and size, whether the user is prompted to send the file
		if (currentFiles[position].isFile()) {
			fileName = currentFiles[position].getName();
			filePath = currentFiles[position].getAbsolutePath();
			long fileLength = currentFiles[position].length();
			String fileSize = null;
			
			// According to the length of the file modification 
			// bytes or kb or mb ..
			if(fileLength == 0) {
				Toast.makeText(getApplicationContext(), R.string.choose_uploadfile_invalidate, Toast.LENGTH_SHORT).show();
				return ;
			} else if (fileLength < 1024L ) {
				fileSize = fileLength + "bytes";
			} else if (fileLength >= 1024 && fileLength < 1048576L ) {
				fileSize = fileLength / 1024L + " KB";
			} else {
				DecimalFormat decimalFormat = new DecimalFormat("##0.00");
				fileSize = decimalFormat.format(fileLength / 1048576.0D) + " MB";
			}
			showFileInfoDialog(getString(R.string.file_name) + fileName +  "\n" + getString(R.string.file_size) + fileSize);
			return;
		}
		filePath = null;
		// list all documents for users to click on the folder
		File[] tem = currentFiles[position].listFiles();
		if (tem == null/* || tem.length == 0*/) {
			Toast.makeText(getApplicationContext(), R.string.choose_uploadfile_invalidate, Toast.LENGTH_SHORT).show();
			// can't access ...
		} else {
			// The list item corresponding to obtain 
			// user click the folder, the parent folder.
			currentParent = currentFiles[position];
			
			// update ListView again ..
			inflateListView(tem);
		}
	}
	
	
	@Override
	public void onClick(View v) {
		switch (v.getId()) {
		case R.id.file_uplevel:
			try {

				if (!currentParent.getCanonicalPath().equals(SDCARD_ROOT)) {

					// 获取上一级目录
					currentParent = currentParent.getParentFile();
					// 列出当前目录下的所有文件
					currentFiles = currentParent.listFiles();
					
					// 再次更新ListView
					inflateListView(currentFiles);
				}
			} catch (Exception e) {
				e.printStackTrace();
			}
			break;
		default:
			break;
		}
	}
	
	
	/**
	 * 根据文件夹填充ListView
	 * 
	 * @param files
	 */
	private void inflateListView(File[] files) {

		// Save all files in the current parent folder and the folder
		ArrayList<File> tempFile = new ArrayList<File>();
		if(sFiles == null ) {
			sFiles = new ArrayList<SDCardFile>();
		}
		sFiles.clear();
		for (int i = 0; i < files.length; i++) {
			File file = files[i];
			Drawable drawable = null ;
			
			SDCardFile sFile = new SDCardFile();
			sFile.fileName = file.getName();
			
			if(file.isDirectory()) {
				drawable = getResources().getDrawable(R.drawable.files_icon);
				sFile.isDirectory = true;
			} else {
				drawable = MimeTypesTools.getDrawableForFileName(FileBrowserActivity.this, file.getName());
				sFile.isDirectory = false;
			}
			
			if(drawable == null ) {
				drawable = getResources().getDrawable(R.drawable.file_unkonw_icon);
			}
			sFile.drawable = drawable;
			
			if(sFile.isHidden()) {
				continue;
			}
			
			tempFile.add(file);
			sFiles.add(sFile);
		}
		
		if(tempFile != null) {
			
			Collections.sort(tempFile, new Comparator<File>() {
				@Override
				public int compare(File lsdfile, File rsdfile) {
					//Need to sort directory first?
					boolean isLhsDirectory = lsdfile.isDirectory();
					boolean isRhsDirectory = rsdfile.isDirectory();
					if (isLhsDirectory || isRhsDirectory) {
						if (isLhsDirectory && isRhsDirectory) {
							//Apply sort mode
							return lsdfile.compareTo(rsdfile);
						}
						return (isLhsDirectory) ? -1 : 1;
					}
					
					//Apply sort mode
					return lsdfile.compareTo(rsdfile);
				}
			});
			currentFiles = tempFile.toArray(new File[]{});
		}
		
		
		try {
			mFileCurrentPath.setText(currentParent.getCanonicalPath());
		} catch (IOException e) {
			e.printStackTrace();
		}
		
		Collections.sort(sFiles, new Comparator<SDCardFile>() {
            @Override
            public int compare(SDCardFile lsdfile, SDCardFile rsdfile) {
                //Need to sort directory first?
                boolean isLhsDirectory = lsdfile.isDirectory();
                boolean isRhsDirectory = rsdfile.isDirectory();
                if (isLhsDirectory || isRhsDirectory) {
                    if (isLhsDirectory && isRhsDirectory) {
                        //Apply sort mode
                        return lsdfile.compareTo(rsdfile);
                    }
                    return (isLhsDirectory) ? -1 : 1;
                }

                //Apply sort mode
                return lsdfile.compareTo(rsdfile);
            }

        });
		
		FileListAdapter mFileAdapter = new FileListAdapter(getApplicationContext(), sFiles);
		mFileBrowser.setAdapter(mFileAdapter);
	}
	
	
	class FileListAdapter extends ArrayAdapter<SDCardFile> {

		LayoutInflater mInflater;
		public FileListAdapter(Context context,List<SDCardFile> sfList) {
			super(context, 0, sfList);
			
			mInflater = getLayoutInflater();
		}

		@Override
		public View getView(int position, View convertView, ViewGroup parent) {
			
			FileHolder holder;
			if (convertView == null|| convertView.getTag() == null) {
				convertView = mInflater.inflate(R.layout.list_item_file, null);
				holder = new FileHolder();
				
				holder.icon = (ImageView) convertView.findViewById(R.id.icon);
				holder.text = (TextView) convertView.findViewById(R.id.text);
			} else {
				holder = (FileHolder) convertView.getTag();
			}
			
			SDCardFile sFile = getItem(position);
			if(sFile != null ) {
				holder.text.setText(sFile.fileName);
				holder.icon.setImageDrawable(sFile.drawable);
			}
			
			
			return convertView;
		}
		
		
		class FileHolder {
			ImageView icon;
			TextView text;
		}
	}

	
	@Override
	protected void onDestroy() {
		super.onDestroy();
		
		if(sFiles != null ) {
			for (int i = 0; i < sFiles.size(); i++) {
				Drawable drawable = sFiles.get(i).drawable;
				if(drawable != null ) {
					drawable = null;
				}
			}
			sFiles = null;
		}
	}
	
	
	void showFileInfoDialog(String text){
		Dialog dialog = null;
		AlertDialog.Builder builder = new AlertDialog.Builder(this);
		builder.setTitle(R.string.title_confirm_send);
		View view = getLayoutInflater().inflate(R.layout.dialog_file_info, null);
		TextView file = (TextView) view.findViewById(R.id.file_info);
		file.setText(text);
		builder.setPositiveButton(R.string.dialog_btn,
				new DialogInterface.OnClickListener() {

			public void onClick(DialogInterface dialog, int which) {
				Intent intent = new Intent(FileBrowserActivity.this , GroupChatActivity.class);
				intent.putExtra("file_name", fileName);
				intent.putExtra("file_url", filePath);
				
				setResult(RESULT_OK , intent);
				dialog.dismiss();
				finish();
			}

		});
		builder.setNegativeButton(R.string.dialog_cancle_btn,
				new DialogInterface.OnClickListener() {
			
			public void onClick(DialogInterface dialog, int which) {
				filePath = null;
				dialog.dismiss();
			}
			
		});
		builder.setView(view);
		dialog = builder.create();
		dialog.show();
		dialog.setCanceledOnTouchOutside(false);
	}


	@Override
	protected int getLayoutId() {
		return R.layout.layout_file_browser;
	}
}