package com.voice.demo.contacts;

import java.io.InputStream;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.regex.Pattern;

import net.sourceforge.pinyin4j.format.exception.BadHanyuPinyinOutputFormatCombination;
import android.content.ContentResolver;
import android.content.ContentUris;
import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.net.Uri;
import android.provider.ContactsContract;
import android.provider.ContactsContract.Contacts;
import android.text.TextUtils;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.ImageView;
import android.widget.QuickContactBadge;
import android.widget.TextView;

import com.voice.demo.R;
import com.voice.demo.contacts.model.ContactBean;
import com.voice.demo.contacts.utils.ToPinYin;

public class ContactHomeAdapter extends BaseAdapter{
	private LayoutInflater inflater;
	private List<ContactBean> list;
	private HashMap<String, Integer> alphaIndexer; 
	private String[] sections; 
	private Context ctx;
	public Map<Integer,Boolean> hashMap;
	private ContentResolver contentResolver;
	
	public ContactHomeAdapter(Context context, List<ContactBean> list, QuickAlphabeticBar alpha) {
		
		this.ctx = context;
		contentResolver = ctx.getContentResolver();
		this.inflater = LayoutInflater.from(context);
		this.list = list; 
		this.alphaIndexer = new HashMap<String, Integer>();
		this.sections = new String[list.size()];
		
		hashMap =new HashMap<Integer, Boolean>(list.size());
		for(int k=0;k<list.size();k++){
			hashMap.put(k, false);
		}
		
		for (int i =0; i <list.size(); i++) {
			String name = getAlpha(list.get(i).getDisplayName());
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

	@Override
	public int getCount() {
		return list.size();
	}

	@Override
	public Object getItem(int position) {
		return list.get(position);
	}

	@Override
	public long getItemId(int position) {
		return position;
	}
	
	public void remove(int position){
		list.remove(position);
	}
	
	@Override
	public View getView(int position, View convertView, ViewGroup parent) {
		
		ViewHolder holder;
		if (convertView == null) {
			convertView = inflater.inflate(R.layout.contact_home_list_item, null);
			holder = new ViewHolder();
			holder.qcb = (QuickContactBadge) convertView.findViewById(R.id.qcb);
			holder.alpha = (TextView) convertView.findViewById(R.id.alpha);
			holder.name = (TextView) convertView.findViewById(R.id.name);
			holder.number = (TextView) convertView
					.findViewById(R.id.number);
			convertView.setTag(holder);
		} else {
			holder = (ViewHolder) convertView.getTag();
		}
		
		ContactBean cb = list.get(position);
		String name = cb.getDisplayName();
		String number = cb.getPhoneNum();
		holder.name.setText(name);
		holder.number.setText(number);
//		holder.qcb.assignContactUri(Contacts.getLookupUri(cb.getContactId(), cb.getLookUpKey()));
		if(0 == cb.getPhotoId()){
			holder.qcb.setImageResource(R.drawable.head);
		}else{
			Uri uri = ContentUris.withAppendedId(ContactsContract.Contacts.CONTENT_URI, cb.getContactId());
			InputStream input = ContactsContract.Contacts.openContactPhotoInputStream(ctx.getContentResolver(), uri); 
			Bitmap contactPhoto = BitmapFactory.decodeStream(input);
			holder.qcb.setImageBitmap(contactPhoto);
		}

		String currentStr = getAlpha(cb.getDisplayName());
		String previewStr = (position - 1) >= 0 ? getAlpha(list.get(position - 1).getDisplayName()) : " ";
		 
		if (!previewStr.equals(currentStr)) {  
			holder.alpha.setVisibility(View.VISIBLE);
			holder.alpha.setText(currentStr);
		} else {
			holder.alpha.setVisibility(View.GONE);
		}
		return convertView;
	}
	
	private static class ViewHolder {
		QuickContactBadge qcb;
		TextView alpha;
		TextView name;
		TextView number;
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
				return (c + "").toUpperCase();  
			} else {
				return "#";
			}
		} catch (BadHanyuPinyinOutputFormatCombination e) {
			e.printStackTrace();
			return "#";
		}
		
	}
	
	public void setSelectedPosition(int position) {
			hashMap.put(position, !hashMap.get(position));
	}
}
