package com.voice.demo.setting;

import java.io.InvalidClassException;
import java.util.ArrayList;

import android.content.Context;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.ArrayAdapter;
import android.widget.CheckBox;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.Toast;

import com.CCP.phone.CameraCapbility;
import com.CCP.phone.CameraInfo;
import com.hisun.phone.core.voice.util.Log4Util;
import com.voice.demo.R;
import com.voice.demo.tools.preference.CCPPreferenceSettings;
import com.voice.demo.tools.preference.CcpPreferences;
import com.voice.demo.ui.CCPBaseActivity;
import com.voice.demo.ui.preference.CCPPreference;

public class VideoCallResolutionSettings extends CCPBaseActivity implements OnItemClickListener{
	public static ArrayList<Integer> positions = new ArrayList<Integer>();
	
	private ListView mListView;
	private VideoCallResolutionApapter mVideocallResoluApapter;
    private int mSavedRosulation = 0;
	@Override
	protected int getLayoutId() {
		return R.layout.video_call_resolution;
	}

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		initLayoutTitleBar();

        mSavedRosulation = CcpPreferences.getSharedPreferences().
                getInt(CCPPreferenceSettings.SETTING_VIDEO_CALL_RESOLUTION.getId(),
                        (Integer)CCPPreferenceSettings.SETTING_VIDEO_CALL_RESOLUTION.getDefaultValue());
		initLayoutView();
	}
	
	@Override
	protected void handleTitleAction(int direction) {
		super.handleTitleAction(direction);
		if(direction == TITLE_RIGHT_ACTION) {
			try {
				CcpPreferences.savePreference(CCPPreferenceSettings.SETTING_VIDEO_CALL_RESOLUTION,
                        mSavedRosulation, true);
			} catch (InvalidClassException e) {
				e.printStackTrace();
			}
			Toast.makeText(this, "设置分辨率成功", Toast.LENGTH_LONG).show();
		}
		finish();
	}
	
	private void initLayoutView() {
		mListView = (ListView) findViewById(R.id.setting_codec_list);
		mListView.setOnItemClickListener(this);
		if(checkeDeviceHelper()) {
			CameraInfo[] cameraInfo = getDeviceHelper().getCameraInfo();
			mVideocallResoluApapter = new VideoCallResolutionApapter(this);

			mVideocallResoluApapter.setData(cameraInfo[0].caps);
            if(cameraInfo.length > 1)
            {
                mVideocallResoluApapter.setData(cameraInfo[1].caps);
            }
			mListView.setAdapter(mVideocallResoluApapter);
		}
	}
	
	
	private void initLayoutTitleBar() {
		handleTitleDisplay(getString(R.string.btn_title_back),getString(R.string.str_setting_select_codec_title), "保存");
	}
	
	public class VideoCallResolutionApapter extends ArrayAdapter<CameraCapbility> {
		private final LayoutInflater mInflater;
		private VideoCallResolutionSettings mActivity;
		/**
		 * @param context
		 * @param resource
		 */
		public VideoCallResolutionApapter(Context context) {
			super(context, 0);
			mActivity = (VideoCallResolutionSettings) context;
			mInflater = (LayoutInflater)context.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
		}
		
		public void setData(CameraCapbility[] data) {
			clear();
			if (data != null) {
				for (CameraCapbility appEntry : data) {
					add(appEntry);
				}
			}
		}
		
		@Override
		public View getView(final int position, View convertView, ViewGroup parent) {
			View view = null;
			ViewHolder mViewHolder = null;
			if(convertView == null || convertView.getTag() == null) {
				view = mInflater.inflate(R.layout.invite_member_list_item, parent, false);

				mViewHolder = new ViewHolder();
				mViewHolder.mCodecName = (TextView) view.findViewById(R.id.name);
				mViewHolder.mCheckBox = (CheckBox) view.findViewById(R.id.check_box);
				
				view.setTag(mViewHolder);
			} else {
				view = convertView;
				mViewHolder = (ViewHolder) view.getTag();
			}
			
			CameraCapbility item = getItem(position);
			if(item != null) {

				mViewHolder.mCodecName.setText(item.height + " x " + item.width);
				mViewHolder.mCheckBox.setChecked(mSavedRosulation == item.height*item.width);

			}
			
			return view;
		}
		
		class ViewHolder {
			TextView mCodecName;
			CheckBox mCheckBox;
		}
	}

	@Override
	public void onItemClick(AdapterView<?> parent, View view, int position,
			long id) {
		Integer object = Integer.valueOf(position);
		if(positions.contains(object)) {
			positions.remove(object);
            mSavedRosulation = 352*288;
		} else {
			positions.clear();
			positions.add(object);
            mSavedRosulation = mVideocallResoluApapter.getItem(position).width *
                    mVideocallResoluApapter.getItem(position).height;
		}
		mVideocallResoluApapter.notifyDataSetChanged();
	}

}
