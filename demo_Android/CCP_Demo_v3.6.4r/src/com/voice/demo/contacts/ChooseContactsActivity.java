package com.voice.demo.contacts;

import java.io.InputStream;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.Set;
import java.util.regex.Pattern;

import net.sourceforge.pinyin4j.format.exception.BadHanyuPinyinOutputFormatCombination;
import android.app.Activity;
import android.content.ContentUris;
import android.content.Context;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.net.Uri;
import android.os.Bundle;
import android.os.SystemClock;
import android.provider.ContactsContract;
import android.text.Editable;
import android.text.TextUtils;
import android.text.TextWatcher;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.view.ViewTreeObserver;
import android.view.ViewTreeObserver.OnGlobalLayoutListener;
import android.widget.BaseExpandableListAdapter;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.EditText;
import android.widget.ExpandableListView;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.TextView;
import android.widget.Toast;

import com.voice.demo.CCPApplication;
import com.voice.demo.R;
import com.voice.demo.contacts.model.ContactBean;
import com.voice.demo.contacts.utils.ToPinYin;
import com.voice.demo.tools.CCPUtil;

public class ChooseContactsActivity extends Activity implements
		ExpandableListView.OnChildClickListener,
		ExpandableListView.OnGroupClickListener,OnClickListener {
	private EditText tv_contactsearch;
	private MyexpandableListAdapter mMyexpandableListAdapter;
	private List<ContactBean> groupList = new ArrayList<ContactBean>();
	private ArrayList<String> phonesList = new ArrayList<String>();
	//选中集合
	Map<Integer, List<Integer>> subSelectMap = new HashMap<Integer, List<Integer>>();
	private CCPApplication application;
	private ExpandableListView expandableListView;
	public static final int CHOOSE_CONTACT = 20009;

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.contacts_select_page);
		initViews();

		application = CCPApplication.getInstance();
		if(application.getContactBeanList()==null||application.getContactBeanList().size()==0)
			T9Service.initSQL();
		new Thread(new Runnable() {
			@Override
			public void run() {
				SystemClock.sleep(500);
				ChooseContactsActivity.this.runOnUiThread(new Runnable() {

					@Override
					public void run() {
						groupList = application.getContactBeanList();
						mMyexpandableListAdapter = new MyexpandableListAdapter(
								ChooseContactsActivity.this,mQuickAlphabeticBar);
						expandableListView.setAdapter(mMyexpandableListAdapter);
					}
				});
			}
		}).start();
	}

	private void initViews() {
		expandableListView = (ExpandableListView) findViewById(R.id.acbuwa_list);

		mQuickAlphabeticBar = (QuickAlphabeticBar)this.findViewById(R.id.fast_scroller);
		mQuickAlphabeticBar.init(ChooseContactsActivity.this);
		mQuickAlphabeticBar.setListView(expandableListView);
		mQuickAlphabeticBar.setHight(mQuickAlphabeticBar.getHeight());
		mQuickAlphabeticBar.setVisibility(View.VISIBLE);
		
		img_del = findViewById(R.id.img_del);
		img_del.setOnClickListener(this);
		ImageButton btn_back = (ImageButton) findViewById(R.id.btn_back);
		btn_back.setOnClickListener(this);
		Button btn_sure = (Button) findViewById(R.id.btn_sure);
		btn_sure.setOnClickListener(this);
		
		expandableListView.setOnChildClickListener(this);
		expandableListView.setOnGroupClickListener(this);
		
		tv_contactsearch = (EditText) this.findViewById(R.id.tv_contactsearch);
		 
		tv_contactsearch.addTextChangedListener(new TextWatcher() {
			public void onTextChanged(final CharSequence s, int start, int before, int count) {
				if (!TextUtils.isEmpty(s)) {
					
					img_del.setVisibility(View.VISIBLE);
					// 搜索功能
					groupList= queryByName(s);
					mMyexpandableListAdapter.notifyDataSetChanged();
				} else if (before > 0) {
					img_del.setVisibility(View.INVISIBLE);
					groupList= application.getContactBeanList();
					mMyexpandableListAdapter.notifyDataSetChanged();
				}
			}

			public void beforeTextChanged(CharSequence s, int start, int count, int after) {
			}

			public void afterTextChanged(Editable s) {
			}
		});
		
		// 纵向排列的字母索引条失效的问题
		ViewTreeObserver vto = mQuickAlphabeticBar.getViewTreeObserver();
		vto.addOnGlobalLayoutListener(new OnGlobalLayoutListener() {

			@Override
			public void onGlobalLayout() {
				mQuickAlphabeticBar.getViewTreeObserver().removeGlobalOnLayoutListener(this);
				mQuickAlphabeticBar.setHight(mQuickAlphabeticBar.getHeight());
			}
		});
	}

	private ViewHolder holder;
	private ChildHolder subHolder;
	private View img_del;
	private QuickAlphabeticBar mQuickAlphabeticBar;

	class MyexpandableListAdapter extends BaseExpandableListAdapter {
		private Context context;
		private LayoutInflater inflater;
		private HashMap<String, Integer> alphaIndexer; 
		private String[] sections; 

		public MyexpandableListAdapter(Context context,QuickAlphabeticBar alpha) {
			this.context = context;
			inflater = LayoutInflater.from(context);
			
			this.alphaIndexer = new HashMap<String, Integer>();
			this.sections = new String[groupList.size()];
			
			 
			
			for (int i =0; i <groupList.size(); i++) {
				String name = getAlpha(groupList.get(i).getDisplayName());
				if(!alphaIndexer.containsKey(name)){ 
					alphaIndexer.put(name, i);
				}
			}
			
			Set<String> sectionLetters = alphaIndexer.keySet();
			ArrayList<String> sectionList = new ArrayList<String>(sectionLetters);
			Collections.sort(sectionList);
			sections = new String[sectionList.size()];
			sectionList.toArray(sections);
			alpha.setAlphaIndexer(alphaIndexer);
		}

		// 返回父列表个数
		@Override
		public int getGroupCount() {
			if (groupList == null)
				return 0;
			return groupList.size();
		}

		// 返回子列表个数
		@Override
		public int getChildrenCount(int groupPosition) {
			// return childList.get(groupPosition).size();
			if (groupList.get(groupPosition).getMorenumbers() == null)
				return 0;
			return groupList.get(groupPosition).getMorenumbers().length + 1;
		}

		@Override
		public Object getGroup(int groupPosition) {

			return groupList.get(groupPosition);
		}

		@Override
		public Object getChild(int groupPosition, int childPosition) {
			// return childList.get(groupPosition).get(childPosition);
			return groupList.get(groupPosition).getAllnumbers()[childPosition];
		}

		@Override
		public long getGroupId(int groupPosition) {
			return groupPosition;
		}

		@Override
		public long getChildId(int groupPosition, int childPosition) {
			return childPosition;
		}

		@Override
		public boolean hasStableIds() {

			return true;
		}

		@Override
		public View getGroupView(final int groupPosition, boolean isExpanded,
				View convertView, ViewGroup parent) {
			if (convertView == null) {
				convertView = inflater.inflate(R.layout.contacts_select_item,
						null);
				holder = new ViewHolder();
				holder.cb = (CheckBox) convertView.findViewById(R.id.cb);
				holder.cb.setOnCheckedChangeListener(null);
				holder.cb.setSelected(false);

				holder.qcb = (ImageView) convertView
						.findViewById(R.id.qcb);
				holder.alpha = (TextView) convertView.findViewById(R.id.alpha);
				holder.name = (TextView) convertView.findViewById(R.id.name);
				holder.number = (TextView) convertView
						.findViewById(R.id.number);
				convertView.setTag(holder);
			} else {
				holder = (ViewHolder) convertView.getTag();
			}
			ContactBean cb = groupList.get(groupPosition);
			String name = cb.getDisplayName();
			String number = cb.getPhoneNum();
			holder.name.setText(name);
			String[] morenumbers = cb.getMorenumbers();
			if(TextUtils.isEmpty(number)){
				holder.cb.setVisibility(View.INVISIBLE);
			}else{
				holder.cb.setVisibility(View.VISIBLE);
			}
			if (morenumbers != null && morenumbers.length > 0) {
				holder.number.setText("多号码");
			} else {
				holder.number.setText(number);
			}

			if (0 == cb.getPhotoId()) {
				holder.qcb.setImageResource(R.drawable.head);
			} else {
				Uri uri = ContentUris.withAppendedId(
						ContactsContract.Contacts.CONTENT_URI,
						cb.getContactId());
				InputStream input = ContactsContract.Contacts
						.openContactPhotoInputStream(
								context.getContentResolver(), uri);
				Bitmap contactPhoto = BitmapFactory.decodeStream(input);
				holder.qcb.setImageBitmap(contactPhoto);
			}

			String currentStr = getAlpha(cb.getDisplayName());
			String previewStr = (groupPosition - 1) >= 0 ? getAlpha(groupList
					.get(groupPosition - 1).getDisplayName()) : " ";

			if (!previewStr.equals(currentStr)) {
				holder.alpha.setVisibility(View.VISIBLE);
				holder.alpha.setText(currentStr);
			} else {
				holder.alpha.setVisibility(View.GONE);
			}
			if (subSelectMap.containsKey(groupPosition)) {
				holder.cb.setChecked(true);
			} else {
				holder.cb.setChecked(false);
			}

			return convertView;
		}

		@Override
		public View getChildView(int groupPosition, int childPosition,
				boolean isLastChild, View convertView, ViewGroup parent) {
			if (convertView == null) {
				convertView = inflater.inflate(R.layout.contact_phones_item,
						null);
				subHolder = new ChildHolder();
				subHolder.subCb = (CheckBox) convertView.findViewById(R.id.cb);
				subHolder.textName = (TextView) convertView
						.findViewById(R.id.phone);

				convertView.setTag(subHolder);

			} else {
				subHolder = (ChildHolder) convertView.getTag();
			}

			String phone = (String) getChild(groupPosition, childPosition);
			if (!TextUtils.isEmpty(phone)) {
				subHolder.textName.setText(phone);
			}
			// 选中效果有点问题！！！！
			if (subSelectMap.get(groupPosition) != null
					&& subSelectMap.get(groupPosition).contains(
							Integer.valueOf(childPosition))) {
				subHolder.subCb.setChecked(true);
			} else {
				subHolder.subCb.setChecked(false);
			}

			return convertView;
		}

		@Override
		public boolean isChildSelectable(int groupPosition, int childPosition) {
			return true;
		}
	}

	@Override
	public boolean onGroupClick(final ExpandableListView parent, final View v,
			int groupPosition, final long id) {

			// if have morephones ,should goto child logical
			if (groupList.get(groupPosition).getMorenumbers() != null
					&& groupList.get(groupPosition).getMorenumbers().length > 0) {
			} else {
				
				ContactBean contactBean = (ContactBean) mMyexpandableListAdapter.getGroup(groupPosition);
				if(TextUtils.isEmpty(contactBean.getPhoneNum())){
					Toast.makeText(getApplicationContext(),
							"此人无电话号！", Toast.LENGTH_SHORT)
							.show();
				}
				else if (subSelectMap.containsKey(groupPosition)) {
					holder.cb.setChecked(false);
					subSelectMap.remove(groupPosition);
//					if(phonesList.contains(contactBean.getPhoneNum()))
					phonesList.remove(contactBean.getPhoneNum());
				} else{
					if(phonesList!=null&&phonesList.size()>=5){
						Toast.makeText(getApplicationContext(),
								"最多选择5个号码", Toast.LENGTH_SHORT)
								.show();
						 
					}else{
						holder.cb.setChecked(true);
						subSelectMap.put(groupPosition, new ArrayList<Integer>());
						String mPhone = CCPUtil.remove86(contactBean.getPhoneNum());
						if(!phonesList.contains(mPhone))
							phonesList.add(mPhone);
					}
				}
			}
		return false;
	}

	@Override
	public boolean onChildClick(ExpandableListView parent, View v,
			int groupPosition, int childPosition, long id) {
		// 子条目选中逻辑

		String phone = (String) mMyexpandableListAdapter.getChild(groupPosition, childPosition);
		if (subSelectMap.containsKey(groupPosition)
				&& subSelectMap.get(groupPosition).contains(
						Integer.valueOf(childPosition))) {// 选择一个往map里加
			subHolder.subCb.setChecked(false);//
			
			phonesList.remove(phone);
			
			// 如果只剩一个 要将整个移除
			List<Integer> list = subSelectMap.get(groupPosition);
			if (list.size() == 1) {
				subSelectMap.remove(groupPosition);
			} else {
				list.remove(childPosition);
				subSelectMap.put(groupPosition, list);
			}
		} else {
			if(phonesList!=null&&phonesList.size()>=5){
				Toast.makeText(getApplicationContext(),
						"最多选择5个号码", Toast.LENGTH_SHORT)
						.show();
				 
			}else{
				
				subHolder.subCb.setChecked(true);
				String mPhone = CCPUtil.remove86(phone);
				if(!phonesList.contains(mPhone))
					phonesList.add(mPhone);
				
				List<Integer> list = subSelectMap.get(groupPosition);
				List<Integer> arrayList = new ArrayList<Integer>();
				if (list != null) {
					arrayList = list;
				}
				arrayList.add(Integer.valueOf(childPosition));
				
				subSelectMap.put(groupPosition, arrayList);
			}
		}

//		adapter.notifyDataSetChanged();
		mMyexpandableListAdapter.notifyDataSetInvalidated();
		return false;
	}

	class ViewHolder {
		CheckBox cb;
		ImageView qcb;
		TextView alpha;
		TextView name;
		TextView number;
	}

	class ChildHolder {
		CheckBox subCb;
		TextView textName;
	}

	private String getAlpha(String zhongwen) {
		try {

			String str = ToPinYin.getPinYin(zhongwen);

			if (str == null) {
				return "#";
			}
			if (str.trim().length() == 0) {
				return "#";
			}

			char c = str.trim().substring(0, 1).charAt(0);

			Pattern pattern = Pattern.compile("^[A-Za-z]+$");
			if (pattern.matcher(c + "").matches()) {
				return (c + "").toUpperCase(Locale.getDefault());
			} else {
				return "#";
			}
		} catch (BadHanyuPinyinOutputFormatCombination e) {
			e.printStackTrace();
			return "#";
		}

	}

	@Override
	public void onClick(View v) {
		switch (v.getId()) {
		case R.id.btn_back:
			finish();
			break;
		case R.id.btn_sure:
			Intent data = new Intent();
			data.putStringArrayListExtra("phones", phonesList);
			setResult(CHOOSE_CONTACT, data);
			finish();
			break;
		case R.id.img_del:
			if(tv_contactsearch!=null){
				tv_contactsearch.setText("");
			}
			img_del.setVisibility(View.INVISIBLE);
			break;
		}
	}
	
	private List<ContactBean> queryByName(CharSequence name) {
		List<ContactBean> l= new ArrayList<ContactBean>();
		if(groupList!=null)
		for(ContactBean c:groupList){
			String s = ToPinYin.myGetPingYin(c.getDisplayName()).toLowerCase(Locale.getDefault()) ;
			String abbrStr = ToPinYin.getFirstSpell(c.getDisplayName().toLowerCase(Locale.getDefault()));
			String lowerCase = name.toString().toLowerCase(Locale.getDefault());
				if(c.getDisplayName().toLowerCase(Locale.getDefault()).contains(lowerCase)||c.getPhoneNum().contains(lowerCase)||s.contains(lowerCase)||abbrStr.contains(lowerCase))
				l.add(c);
				else {//姓名和第一个号码都搜不到 再搜其他号码
					String[] morenumbers = c.getMorenumbers();
					if(morenumbers!=null){
						for(int i=0;i<morenumbers.length;i++){
							if(morenumbers!=null && morenumbers[i].contains(name)){
								l.add(c);
							}
						}
					}
				}
			}
			
		return l;

	}
	 
}
