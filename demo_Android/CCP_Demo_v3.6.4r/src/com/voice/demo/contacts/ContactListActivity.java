package com.voice.demo.contacts;

import java.util.ArrayList;
import java.util.List;
import java.util.Locale;
import java.util.Map;


import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.os.SystemClock;
import android.provider.ContactsContract;
import android.text.Editable;
import android.text.TextUtils;
import android.text.TextWatcher;
import android.view.View;
import android.view.ViewTreeObserver;
import android.view.View.OnClickListener;
import android.view.ViewTreeObserver.OnGlobalLayoutListener;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemLongClickListener;
import android.widget.EditText;
import android.widget.ListView;
import android.widget.Toast;
import android.widget.AdapterView.OnItemClickListener;

import com.voice.demo.CCPApplication;
import com.voice.demo.R;
import com.voice.demo.contacts.model.ContactBean;
import com.voice.demo.contacts.utils.ToPinYin;
import com.voice.demo.tools.CCPUtil;
/**
 * 
 * 
* <p>Title: ContactListActivity.java</p>
* <p>Description: </p>
* <p>Copyright: Copyright (c) 2014</p>
* <p>Company: http://www.yuntongxun.com</p>
* @author Trevor Pan
* @date 2014-3-11
* @version x.x
 */
public class ContactListActivity extends Activity implements OnClickListener{
	private QuickAlphabeticBar mQuickAlphabeticBar;
	private ListView lv;
	public List<ContactBean> allContactList;// 所有联系人
	private ContactHomeAdapter mContactHomeAdapter;
	private View img_del;
	private EditText tv_contactsearch;
	private CCPApplication application;
	public static final int CHOOSE_CONTACT = 20008;
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.contactlist_page);
		initViews();
		addViewsListeners();
		  
		application = CCPApplication.getInstance();
		
		if(application.getContactBeanList()==null||application.getContactBeanList().size()==0)
		T9Service.initSQL();
		
		new Thread(new Runnable() {
			@Override
			public void run() {
				SystemClock.sleep(500);
				ContactListActivity.this.runOnUiThread(new Runnable() {
					@Override
					public void run() {
							allContactList = application.getContactBeanList();
							setAdapter(allContactList);
					}
				});
			}
		}).start();
		
	}
	 
	@Override
	protected void onResume() {
		//联系人重新获取
		// （三星手机）删除联系人的时候有个严重bug 被删除的联系人还在页面  之前项目也有这个 只是长按删除直接条目给删掉了其实没从根本解决！
		//一切流程正常 但是ONREUNM调用比 T9service的setContactBeanList方法要快
		//所以getContactBeanList方法没来得及拿到最新的
		if (allContactList != application.getContactBeanList()||allContactList.size()==0) {
			allContactList = application.getContactBeanList();
			setAdapter(allContactList);
			
		}

		super.onResume();
	}

	private void addViewsListeners() {
		lv.setOnItemClickListener(new OnItemClickListener() {
			@Override
			public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
				ContactBean contactBean = (ContactBean) mContactHomeAdapter.getItem(position);
				//外边入口 选择联系人的情况
				if(getIntent()!=null&&getIntent().getExtras()!=null){
					String[] morenumbers = contactBean.getMorenumbers();
					String phone=contactBean.getPhoneNum();
					if(TextUtils.isEmpty(phone)){
						Toast.makeText(getApplicationContext(),
								"此人无电话号！", Toast.LENGTH_SHORT)
								.show();
						return;
					}
					if(morenumbers!=null && morenumbers.length>0){
						// 有多个号码 弹出可选择页面
						AlertDialog.Builder builder = new AlertDialog.Builder(ContactListActivity.this);
						ArrayList<String> morenumberList = new ArrayList<String>();
						morenumberList.add(phone);
						for (int i = 0; i < morenumbers.length; i++) {
							morenumberList.add(morenumbers[i]);
						}
						final String[] telNumber = (String[]) morenumberList.toArray(new String[0]);
						builder.setTitle("选择号码").setItems(telNumber, new DialogInterface.OnClickListener() {
							public void onClick(DialogInterface dialog, int which) {
								Intent data = new Intent();
								data.putExtra("phone",CCPUtil.remove86(telNumber[which]) );
								setResult(CHOOSE_CONTACT, data);
								finish();
							}
						}).show();
					}else{
						Intent data = new Intent();
						data.putExtra("phone",CCPUtil.remove86(phone) );
						setResult(CHOOSE_CONTACT, data);
						finish();
					}
					
					
				}else{
					// 单击编辑联系人
					Intent intent2 = new Intent(Intent.ACTION_EDIT, Uri.withAppendedPath(ContactsContract.Contacts.CONTENT_URI, String.valueOf(contactBean.getContactId())));
					startActivity(intent2);
				}
				
			}

		});
		 
		lv.setOnItemLongClickListener(new OnItemLongClickListener() {

			@Override
			public boolean onItemLongClick(AdapterView<?> arg0, View arg1,
					int arg2, long arg3) {
				return false;
			}
		});
		
		tv_contactsearch = (EditText) this.findViewById(R.id.tv_contactsearch);
	 
				tv_contactsearch.addTextChangedListener(new TextWatcher() {
					public void onTextChanged(final CharSequence s, int start, int before, int count) {
						if (!TextUtils.isEmpty(s)) {
							
							img_del.setVisibility(View.VISIBLE);
							// 搜索功能
							List<ContactBean> queryByName = queryByName(s);
							setAdapter(queryByName);
						} else if (before > 0) {
							img_del.setVisibility(View.INVISIBLE);
							setAdapter(allContactList);
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

	private void initViews() {
		lv = (ListView) findViewById(R.id.acbuwa_list);
		
		mQuickAlphabeticBar = (QuickAlphabeticBar)this.findViewById(R.id.fast_scroller);
		mQuickAlphabeticBar.init(ContactListActivity.this);
		mQuickAlphabeticBar.setListView(lv);
		mQuickAlphabeticBar.setHight(mQuickAlphabeticBar.getHeight());
		mQuickAlphabeticBar.setVisibility(View.VISIBLE);
		
		View addContactBtn = findViewById(R.id.addContactBtn);
		addContactBtn.setOnClickListener(this);
		if(getIntent()!=null&&getIntent().getExtras()!=null){
			addContactBtn.setVisibility(View.INVISIBLE);
		}else{
			addContactBtn.setVisibility(View.VISIBLE);
		}
		View backBtn =  findViewById(R.id.back);
		backBtn.setOnClickListener(this);
		
		img_del = findViewById(R.id.img_del);
		img_del.setOnClickListener(this);
		
	}
	
	
	
	private void setAdapter(List<ContactBean> list) {
		if(list==null)
			return;
		mContactHomeAdapter = new ContactHomeAdapter(this, list, mQuickAlphabeticBar);
		lv.setAdapter(mContactHomeAdapter);
		
	}

	@Override
	public void onClick(View v) {
		switch (v.getId()) {
		case R.id.addContactBtn:
			Uri insertUri = android.provider.ContactsContract.Contacts.CONTENT_URI;
			Intent intent = new Intent(Intent.ACTION_INSERT, insertUri);
			startActivity(intent);
			break;
		case R.id.back:
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
		if(allContactList!=null)
		for(ContactBean c:allContactList){
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
