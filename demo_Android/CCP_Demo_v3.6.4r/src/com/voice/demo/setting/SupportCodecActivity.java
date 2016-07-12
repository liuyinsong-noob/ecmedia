package com.voice.demo.setting;

import java.util.ArrayList;

import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.ArrayAdapter;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.Toast;

import com.hisun.phone.core.voice.Device.Codec;
import com.voice.demo.R;
import com.voice.demo.ui.CCPBaseActivity;

public class SupportCodecActivity extends CCPBaseActivity implements OnItemClickListener{

	public static ArrayList<Integer> positions = new ArrayList<Integer>();
	
	private ListView mListView;
	private SupportCodecApapter mCodecApapter;
	@Override
	protected int getLayoutId() {
		return R.layout.support_codec;
	}
	
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		
		initLayoutTitleBar();
		
		initLayoutView();
	}

	@Override
	protected void handleTitleAction(int direction) {
		if(direction == TITLE_RIGHT_ACTION) {
			handleEnable();
			finish();
			return;
		}
		super.handleTitleAction(direction);
	}

	private void handleEnable() {
		if(checkeDeviceHelper()) {
			
			for(int i = 0; i < mCodecApapter.getCount() ; i ++) {
				
				Codec item = mCodecApapter.getItem(i);
				getDeviceHelper().setCodecEnabled(item, positions.contains(Integer.valueOf(i)));
			}
			
			return ;
		}
		Toast.makeText(this, "设置失败", Toast.LENGTH_LONG).show();
	}

	
	@Override
	protected void onDestroy() {
		super.onDestroy();
		positions.clear();
	}

	private void initLayoutView() {
		mListView = (ListView) findViewById(R.id.setting_codec_list);
		Codec[] values = Codec.values();
		mCodecApapter = new SupportCodecApapter(this);
		mCodecApapter.setData(values);
		mListView.setAdapter(mCodecApapter);
	}


	private void initLayoutTitleBar() {
		handleTitleDisplay(getString(R.string.btn_title_back),
				getString(R.string.str_setting_select_codec_title), null);
	}


	@Override
	public void onItemClick(AdapterView<?> parent, View view, int position,
			long id) {
		final Integer object = Integer.valueOf(position);
		if(positions.contains(object)) {
			positions.remove(object);
		} else {
			
			Codec item = mCodecApapter.getItem(position);
			positions.add(object);
		}
		mCodecApapter.notifyDataSetChanged();
		
	}
	
	
	public static class SupportCodecApapter extends ArrayAdapter<Codec> {
		private final LayoutInflater mInflater;
		private SupportCodecActivity mActivity;
		/**
		 * @param context
		 * @param resource
		 */
		public SupportCodecApapter(Context context) {
			super(context, 0);
			mActivity = (SupportCodecActivity) context;
			mInflater = (LayoutInflater)context.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
		}
		
		public void setData(Codec[] data) {
			clear();
			if (data != null) {
				for (Codec appEntry : data) {
					add(appEntry);
				}
			}
		}
		
		@Override
		public View getView(final int position, View convertView, ViewGroup parent) {
			View view = null;
			ViewHolder mViewHolder = null;
			if(convertView == null || convertView.getTag() == null) {
				view = mInflater.inflate(R.layout.codec_item, parent, false);

				mViewHolder = new ViewHolder();
				mViewHolder.mCodecName = (TextView) view.findViewById(R.id.name);
				mViewHolder.mCheckBox = (SlipButton) view.findViewById(R.id.switch_btn);
				
				view.setTag(mViewHolder);
			} else {
				view = convertView;
				mViewHolder = (ViewHolder) view.getTag();
			}
			Codec item = getItem(position);
			boolean codecEnabled = mActivity.getDeviceHelper().getCodecEnabled(getItem(position));
			mViewHolder.mCheckBox.setChecked(codecEnabled);
			if(item != null) {
				mViewHolder.mCodecName.setText(getCodecName(item.getValue()));
				mViewHolder.mCheckBox.SetOnChangedListener(getCodecName(position).toString(), new SlipButton.OnChangedListener() {
					
					@Override
					public void onChanged(String strname, boolean checkState) {
						if(getCodecName(position).toString().equals(strname)) {
							
							if((position == Codec.Codec_H264.getValue() || position == Codec.Codec_VP8.getValue()) && !checkState){
								AlertDialog.Builder builder = new AlertDialog.Builder(mActivity);
								builder.setTitle("提示");
								builder.setMessage(getCodecName(position) + " 是视频编码,确定要关闭？");
								builder.setPositiveButton("不启用", new DialogInterface.OnClickListener() {
								
									@Override
									public void onClick(DialogInterface arg0, int arg1) {
										arg0.dismiss();
										mActivity.getDeviceHelper().setCodecEnabled(getItem(position), false);
									}
								}).setNegativeButton("取消", null);
								AlertDialog dialog = builder.create();
								dialog.setCancelable(false);
								dialog.show();
								return;
							}
							mActivity.getDeviceHelper().setCodecEnabled(getItem(position), checkState);
						}
					}
				});
			}
			
			return view;
		}
		
		class ViewHolder {
			TextView mCodecName;
			SlipButton mCheckBox;
		}
	}


	public static CharSequence getCodecName(int value) {
		switch (value) {
		case 0:
			return "Codec_PCMU";
		case 1:
			return "Codec_G729";
		case 2:
			return "Codec_OPUS48";
		case 3:
			return "Codec_OPUS16";
		case 4:
			return "Codec_OPUS8";
		case 5:
			return "Codec_VP8";
		case 6:
			return "Codec_H264";
		default:
			break;
		}
		return null;
	}
}
