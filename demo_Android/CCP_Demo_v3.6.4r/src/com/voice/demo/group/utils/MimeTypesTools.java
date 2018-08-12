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
package com.voice.demo.group.utils;

import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.content.res.XmlResourceParser;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.drawable.Drawable;
import android.text.TextUtils;
import java.io.IOException;
import java.util.List;
import org.xmlpull.v1.XmlPullParserException;

import com.voice.demo.R;

/**
 * @version Time: 2013-7-26
 */
public class MimeTypesTools {
	
	private static boolean hasLoadMimeType = false;

	public static Bitmap drawableToBitmap(Drawable drawable) {
		int width = drawable.getIntrinsicWidth();
		int height = drawable.getIntrinsicHeight();
		Bitmap bitmap = Bitmap.createBitmap(width, height, drawable
				.getOpacity() != -1 ? Bitmap.Config.ARGB_8888
				: Bitmap.Config.RGB_565);
		Canvas canvas = new Canvas(bitmap);
		drawable.setBounds(0, 0, width, height);
		drawable.draw(canvas);
		return bitmap;
	}

	public static Bitmap getBitmapForFileName(Context context, String fileName) {
		return drawableToBitmap(getDrawableForFileName(context, fileName));
	}

	public static Drawable getDrawableForFileName(Context context,
			String fileName) {
		return getDrawableForMimetype(context, getMimeType(context, fileName));
	}

	public static Drawable getDrawableForMimetype(Context context,
			String mimeType) {
		Drawable icon = null;
		PackageManager pManager = context.getPackageManager();
		Intent intent = new Intent(Intent.ACTION_GET_CONTENT);
		intent.setType(mimeType);
		List<ResolveInfo> rList = pManager.queryIntentActivities(intent, 65536);
		if ((rList != null) && (rList.size() > 0)) {
			ResolveInfo resolveInfo = (ResolveInfo) rList.get(0);
			icon = resolveInfo.loadIcon(pManager);
		}

		if (icon == null) {
			icon = context.getResources().getDrawable(2130837556);
		}

		return icon;
	}

	public static String getMimeType(Context context, String fileName) {
		if (!TextUtils.isEmpty(fileName)) {
			fileName = fileName.toLowerCase();

			MimeTypes mimeTypes = getMimeTypes(context);
			String extension = FileUtils.getExtension(fileName);
			return mimeTypes.getMimeType(extension);
		}

		return null;
	}

	private static MimeTypes getMimeTypes(Context context) {
		return loadMimeTypes(context);
	}

	private static MimeTypes loadMimeTypes(Context context) {
		MimeTypeParser parser = null;
		XmlResourceParser xmlResourceParser = null;
		if (!hasLoadMimeType) {
			try {
				parser = new MimeTypeParser(context, context.getPackageName());
				xmlResourceParser = context.getResources().getXml(R.xml.mimetypes);

				return parser.fromXmlResource(xmlResourceParser);
			} catch (XmlPullParserException e) {
				e.printStackTrace();
			} catch (PackageManager.NameNotFoundException e) {
				e.printStackTrace();
			} catch (IOException e) {
				e.printStackTrace();
			}
			hasLoadMimeType = true;
		}

		return null;
	}
}