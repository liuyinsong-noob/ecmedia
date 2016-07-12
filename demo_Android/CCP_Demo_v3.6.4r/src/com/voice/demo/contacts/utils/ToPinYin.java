package com.voice.demo.contacts.utils;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;

import android.text.TextUtils;
import net.sourceforge.pinyin4j.PinyinHelper;
import net.sourceforge.pinyin4j.format.HanyuPinyinCaseType;
import net.sourceforge.pinyin4j.format.HanyuPinyinOutputFormat;
import net.sourceforge.pinyin4j.format.HanyuPinyinToneType;
import net.sourceforge.pinyin4j.format.HanyuPinyinVCharType;
import net.sourceforge.pinyin4j.format.exception.BadHanyuPinyinOutputFormatCombination;

public class ToPinYin {

	public static List<String> getPinyinList(List<String> list) {
		List<String> pinyinList = new ArrayList<String>();
		for (Iterator<String> i = list.iterator(); i.hasNext();) {
			String str = (String) i.next();
			try {
				String pinyin = getPinYin(str);
				pinyinList.add(pinyin);
			} catch (BadHanyuPinyinOutputFormatCombination e) {
				e.printStackTrace();
			}
		}
		return pinyinList;
	}

	public static String getPinYin(String zhongwen) throws BadHanyuPinyinOutputFormatCombination {

		String zhongWenPinYin = "";
		if(TextUtils.isEmpty(zhongwen))
		return zhongwen;
		char[] chars = zhongwen.toCharArray();

		for (int i = 0; i < chars.length; i++) {
			String[] pinYin = PinyinHelper.toHanyuPinyinStringArray(chars[i], getDefaultOutputFormat());
			if (pinYin != null) {
				zhongWenPinYin = zhongWenPinYin + pinYin[0];
			} else {
				zhongWenPinYin += java.lang.Character.toString(chars[i]).toUpperCase();
			}
		}
		return zhongWenPinYin;
	}

	private static HanyuPinyinOutputFormat getDefaultOutputFormat() {
		HanyuPinyinOutputFormat format = new HanyuPinyinOutputFormat();
		format.setCaseType(HanyuPinyinCaseType.UPPERCASE);
		format.setToneType(HanyuPinyinToneType.WITHOUT_TONE);
		format.setVCharType(HanyuPinyinVCharType.WITH_U_AND_COLON);
		return format;
	}

	public static String myGetPingYin(String inputString) {
		HanyuPinyinOutputFormat format = new HanyuPinyinOutputFormat();
		format.setCaseType(HanyuPinyinCaseType.UPPERCASE);//大写方式
		format.setToneType(HanyuPinyinToneType.WITHOUT_TONE);
		format.setVCharType(HanyuPinyinVCharType.WITH_U_AND_COLON);
		 
		char[] input = inputString.trim().toCharArray();
		String output = "";
		
		try {
			for (int i = 0; i < input.length; i++) {
				if (java.lang.Character.toString(input[i]).matches("[\\u4E00-\\u9FA5]+")) {
					String[] temp = PinyinHelper.toHanyuPinyinStringArray(input[i], format);
					output += temp[0];
				} else
					output += java.lang.Character.toString(input[i]).toUpperCase();//大写方式
			}
		} catch (BadHanyuPinyinOutputFormatCombination e) {
			e.printStackTrace();
			return inputString;
		}
		return output;
	}
	 
	
	/** 
     * 获取汉字串拼音首字母，英文字符不变 
     * @param chinese 汉字串 
     * @return 汉语拼音首字母 
     */ 
    public static String getFirstSpell(String chinese) { 
            StringBuffer pybf = new StringBuffer(); 
            char[] arr = chinese.toCharArray(); 
            HanyuPinyinOutputFormat defaultFormat = new HanyuPinyinOutputFormat(); 
            defaultFormat.setCaseType(HanyuPinyinCaseType.LOWERCASE); 
            defaultFormat.setToneType(HanyuPinyinToneType.WITHOUT_TONE); 
            for (int i = 0; i < arr.length; i++) { 
                    if (arr[i] > 128) { 
                            try { 
                                    String[] temp = PinyinHelper.toHanyuPinyinStringArray(arr[i], defaultFormat); 
                                    if (temp != null) { 
                                            pybf.append(temp[0].charAt(0)); 
                                    } 
                            } catch (BadHanyuPinyinOutputFormatCombination e) { 
                                    e.printStackTrace(); 
                            } 
                    } else { 
                            pybf.append(arr[i]); 
                    } 
            } 
            return pybf.toString().replaceAll("\\W", "").trim(); 
    } 

    /** 
     * 获取汉字串拼音，英文字符不变 
     * @param chinese 汉字串 
     * @return 汉语拼音 
     */ 
    public static String getFullSpell(String chinese) { 
            StringBuffer pybf = new StringBuffer(); 
            char[] arr = chinese.toCharArray(); 
            HanyuPinyinOutputFormat defaultFormat = new HanyuPinyinOutputFormat(); 
            defaultFormat.setCaseType(HanyuPinyinCaseType.LOWERCASE); 
            defaultFormat.setToneType(HanyuPinyinToneType.WITHOUT_TONE); 
            for (int i = 0; i < arr.length; i++) { 
                    if (arr[i] > 128) { 
                            try { 
                                    pybf.append(PinyinHelper.toHanyuPinyinStringArray(arr[i], defaultFormat)[0]); 
                            } catch (BadHanyuPinyinOutputFormatCombination e) { 
                                    e.printStackTrace(); 
                            } 
                    } else { 
                            pybf.append(arr[i]); 
                    } 
            } 
            return pybf.toString(); 
    }

}
