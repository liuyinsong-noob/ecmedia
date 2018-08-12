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
package com.voice.demo.tools;

import com.hisun.phone.core.voice.util.Log4Util;
import com.voice.demo.ui.CCPHelper;

import android.graphics.Bitmap;

/**
 * 
* <p>Title: CCPBitmapUtils.java</p>
* <p>Description: BitmapFactory.decodeByteArray(byte[] data, int offset, int length)，
* The byte[] is aimed at the picture data integrity, not including the file header</p>
* <p>Copyright: Copyright (c) 2012</p>
* <p>Company: http://www.yuntongxun.com</p>
* @author Jorstin Chan
* @date 2013-11-7
* @version  3.5
 */
public class CCPBitmapUtils {

	/*
	 * 获取位图的RGB数据
	 */
	public static byte[] getRGBByBitmap(Bitmap bitmap) {
		if (bitmap == null) {
			return null;
		}

		int width = bitmap.getWidth();
		int height = bitmap.getHeight();

		int size = width * height;

		int pixels[] = new int[size];
		bitmap.getPixels(pixels, 0, width, 0, 0, width, height);

		byte[] data = convertColorToByte(pixels);

		return data;
	}

	/*
	 * 像素数组转化为RGB数组
	 */
	public static byte[] convertColorToByte(int color[]) {
		if (color == null) {
			return null;
		}

		byte[] data = new byte[color.length * 3];
		for (int i = 0; i < color.length; i++) {
			data[i * 3] = (byte) (color[i] >> 16 & 0xff);
			data[i * 3 + 1] = (byte) (color[i] >> 8 & 0xff);
			data[i * 3 + 2] = (byte) (color[i] & 0xff);
		}

		return data;

	}

	/*
	 * byte[] data保存的是纯RGB的数据，而非完整的图片文件数据
	 */
	static public Bitmap createCCPBitmap(byte[] data, int width, int height) {
		int[] colors = convertByteToColor(data);
		if (colors == null) {
			return null;
		}

		Bitmap bmp = null;

		try {
			bmp = Bitmap.createBitmap(colors, 0, width, width, height,
					Bitmap.Config.ARGB_8888);
		} catch (Exception e) {
			// TODO: handle exception

			return null;
		} finally {
			colors = null;
		}

		return bmp;
	}

	/*
	 * 将RGB数组转化为像素数组
	 */
	private static int[] convertByteToColor(byte[] data) {
		int size = data.length;
		if (size == 0) {
			return null;
		}

		// 理论上data的长度应该是3的倍数，这里做个兼容
		int arg = 0;
		if (size % 3 != 0) {
			arg = 1;
		}

		int[] color = new int[size / 3 + arg];
		//int red, green, blue;

		if (arg == 0) { 
			// 正好是3的倍数
			for (int i = 0; i < color.length; ++i) {

				color[i] = (data[i * 3] << 16 & 0x00FF0000)
						| (data[i * 3 + 1] << 8 & 0x0000FF00)
						| (data[i * 3 + 2] & 0x000000FF) | 0xFF000000;
			}
		} else { 
			// 不是3的倍数
			for (int i = 0; i < color.length - 1; ++i) {
				color[i] = (data[i * 3] << 16 & 0x00FF0000)
						| (data[i * 3 + 1] << 8 & 0x0000FF00)
						| (data[i * 3 + 2] & 0x000000FF) | 0xFF000000;
			}

			// 最后一个像素用黑色填充
			color[color.length - 1] = 0xFF000000; 
		}

		return color;
	}
	
	
	/**
	 * @param data
	 * @return
	 */
	public static int convertByteToInt(byte data) {

		int heightBit = (int) ((data >> 4) & 0x0F);
		int lowBit = (int) (0x0F & data);

		return heightBit * 16 + lowBit;
	}

	/**
	 * @param data
	 * @return
	 */
	public static int[] convertByteToColor2(byte[] data) {
		int size = data.length;
		if (size == 0) {
			return null;
		}

		int arg = 0;
		if (size % 3 != 0) {
			arg = 1;
		}

		int[] color = new int[size / 3 + arg];
		int red, green, blue;

		if (arg == 0) {
			for (int i = 0; i < color.length; ++i) {
				red = convertByteToInt(data[i * 3]);
				green = convertByteToInt(data[i * 3 + 1]);
				blue = convertByteToInt(data[i * 3 + 2]);

				color[i] = (red << 16) | (green << 8) | blue | 0xFF000000;
			}
		} else {
			for (int i = 0; i < color.length - 1; ++i) {
				red = convertByteToInt(data[i * 3]);
				green = convertByteToInt(data[i * 3 + 1]);
				blue = convertByteToInt(data[i * 3 + 2]);
				color[i] = (red << 16) | (green << 8) | blue | 0xFF000000;
			}

			color[color.length - 1] = 0xFF000000;
		}

		return color;
	}

	/**
	 * @param frame
	 * @return
	 */
	static Bitmap decodeFrameToBitmap(byte[] frame , int width, int height) {
		try {
			
			String format = String
			.format("CCPBitmapUtils.decodeFrameToBitmap length: %d , width:%d , height %d .",
					frame.length, width, height);
			Log4Util.d(CCPHelper.DEMO_TAG, format);
			int[] colors = convertByteToColor(frame);
			if (colors == null) {
				return null;
			}
				
			Bitmap bmp = Bitmap.createBitmap(colors, 0, width, width, height,
					Bitmap.Config.ARGB_8888);
			return bmp;
			
		} catch (OutOfMemoryError oError) {
			oError.printStackTrace();
			Log4Util.d("CCPBitmapUtils.decodeFrameToBitmap error ,  allocate memory failed.");
		} catch (Exception e) {
		}
		return null;
	}

}
