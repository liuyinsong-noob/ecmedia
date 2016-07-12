package com.voice.demo.contacts.model;

import java.util.ArrayList;
import com.voice.demo.contacts.utils.ToPinYin;
import android.os.Parcel;
import android.os.Parcelable;

public class ContactBean implements Parcelable,Comparable<ContactBean> {

	private int groupId;

	public int getGroupId() {
		return groupId;
	}

	public void setGroupId(int groupId) {
		this.groupId = groupId;
	}

	private int contactId;

	public String getPhotoPath() {
		return photoPath;
	}

	public void setContactids(String contactids) {
		this.contactids = contactids;
	}

	// 通讯录备份还原用 增加的字段
	private String contactids = "";

	public String getContactids() {
		return contactids;
	}

	public void setPhotoPath(String photoPath) {
		this.photoPath = photoPath;
	}

	private String photoPath = "";
	private String displayName = "";
	private String phoneNum = "";
	private String sortKey = "";
	private Long photoId = Long.valueOf(0);
	private String lookUpKey = "";
	private int selected = 0;
	private String[] morenumbers;

	//if more than 2 numbers
	public String[] getAllnumbers() {
		if(morenumbers==null)
			return null;
		ArrayList<String> al =new ArrayList<String>();
		 al.add(phoneNum);
		 for(String s:morenumbers){
			 al.add(s);
		 }
		 String[] array = al.toArray(new String[0]);
		return array;
	}
	public String[] getMorenumbers() {
		return morenumbers;
	}

	public void setMorenumbers(String[] morenumbers) {
		this.morenumbers = morenumbers;
	}

	private String formattedNumber = "";
	private String pinyin = "";

	public ContactBean(Parcel in) {
		displayName = in.readString();
		contactId = in.readInt();
		phoneNum = in.readString();
		sortKey = in.readString();
		photoId = in.readLong();
		lookUpKey = in.readString();
		selected = in.readInt();
		formattedNumber = in.readString();
		pinyin = in.readString();
		contactids = in.readString();
		photoPath = in.readString();
		groupId = in.readInt();
		morenumbers = in.createStringArray();
	}

	public ContactBean() {
	}

	public int getContactId() {
		return contactId;
	}

	public void setContactId(int contactId) {
		this.contactId = contactId;
	}

	public String getDisplayName() {
		return displayName;
	}

	public void setDisplayName(String displayName) {
		if(displayName!=null)
		this.displayName = displayName;
	}

	public String getPhoneNum() {
		return phoneNum;
	}

	public void setPhoneNum(String phoneNum) {
		this.phoneNum = phoneNum;
	}

	public String getSortKey() {
		return sortKey;
	}

	public void setSortKey(String sortKey) {
		this.sortKey = sortKey;
	}

	public Long getPhotoId() {
		return photoId;
	}

	public void setPhotoId(Long photoId) {
		this.photoId = photoId;
	}

	public String getLookUpKey() {
		return lookUpKey;
	}

	public void setLookUpKey(String lookUpKey) {
		this.lookUpKey = lookUpKey;
	}

	public int getSelected() {
		return selected;
	}

	public void setSelected(int selected) {
		this.selected = selected;
	}

	public String getFormattedNumber() {
		return formattedNumber;
	}

	public void setFormattedNumber(String formattedNumber) {
		this.formattedNumber = formattedNumber;
	}

	public String getPinyin() {
		return pinyin;
	}

	public void setPinyin(String pinyin) {
		this.pinyin = pinyin;
	}

	@Override
	public int describeContents() {
		return 0;
	}

	@Override
	public void writeToParcel(Parcel dest, int flags) {
		dest.writeString(displayName);
		dest.writeInt(contactId);
		dest.writeString(phoneNum);
		dest.writeString(sortKey);
		dest.writeLong(photoId);
		dest.writeString(lookUpKey);
		dest.writeInt(selected);
		dest.writeString(formattedNumber);
		dest.writeString(pinyin);
		dest.writeString(contactids);
		dest.writeString(photoPath);
		dest.writeInt(groupId);
		dest.writeStringArray(morenumbers);
	}

	public static final Parcelable.Creator<ContactBean> CREATOR = new Creator<ContactBean>() {
		@Override
		public ContactBean createFromParcel(Parcel source) {
			return new ContactBean(source);
		}

		@Override
		public ContactBean[] newArray(int size) {
			return new ContactBean[size];
		}
	};

	@Override
	public int hashCode() {
		final int prime = 31;
		int result = 1;
		result = prime * result + ((displayName == null) ? 0 : displayName.hashCode());
		result = prime * result + ((phoneNum == null) ? 0 : phoneNum.hashCode());
		return result;
	}

	@Override
	public boolean equals(Object obj) {
		if (this == obj)
			return true;
		if (obj == null)
			return false;
		if (getClass() != obj.getClass())
			return false;
		ContactBean other = (ContactBean) obj;
		if (displayName == null) {
			if (other.displayName != null)
				return false;
		} else if (!displayName.equals(other.displayName))
			return false;
		if (phoneNum == null) {
			if (other.phoneNum != null)
				return false;
		} else if (!phoneNum.equals(other.phoneNum))
			return false;
		return true;
	}

	@Override
	public int compareTo(ContactBean another) {
		String displayName1 = this.getDisplayName();
		String displayName2 = another.getDisplayName();
		String myGetPingYin1 = ToPinYin.myGetPingYin(displayName1);
		String myGetPingYin2 = ToPinYin.myGetPingYin(displayName2);
		 
		return myGetPingYin1.compareTo(myGetPingYin2);
	}

}
