package com.yuntongxun.ecsdk.core.voip;

import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.view.View;

/**
 * Created by Administrator on 11/22 0022.
 */

public class ScreenCaptureAndroid {
    //private ByteBuffer _screenBuffer = null;
    long captureContext = 0;

    public void CaptureScreen(Object view0, long captureObject) {
//        View view = (View) view0;
//        Bitmap bitmap = Bitmap.createBitmap(view.getWidth(),
//                view.getHeight(), Bitmap.Config.ARGB_8888);
//        Canvas canvas = new Canvas(bitmap);
//        view.draw(canvas);
        captureContext = captureObject;

        Message message = new Message();
        message.obj= view0;
        message.what=100;
        handler.sendMessage(message);
        
        return;
    }

    private Handler handler = new Handler(Looper.getMainLooper()){

        @Override
        public void handleMessage(Message msg) {
            switch (msg.what){
                case 100:
                    View view =(View)msg.obj;
                    Bitmap bitmap = Bitmap.createBitmap(view.getWidth(),
                            view.getHeight(), Bitmap.Config.ARGB_8888);
                    Canvas canvas = new Canvas(bitmap);
                    view.draw(canvas);
                    ProvideScreeData(bitmap, captureContext);
                    break;
            }
        }
    };


    public Object GetScreenRect2(Object obj) {
//        int screenWidth;//屏幕宽度
//        int screenHeight;//屏幕高度
//        WindowManager windowManager = activity.getWindowManager();
//        Display display = windowManager.getDefaultDisplay();
//        screenWidth = display.getWidth();
//        screenHeight = display.getHeight();
        return obj;
    }


    public int[] GetScreenRect(Object obj) {
//        int screenWidth;//屏幕宽度
//        int screenHeight;//屏幕高度
//        WindowManager windowManager = activity.getWindowManager();
//        Display display = windowManager.getDefaultDisplay();
//        screenWidth = display.getWidth();
//        screenHeight = display.getHeight();

        View view = (View)obj;
        int[] rect = new int[2];
        rect[0] = view.getWidth();
        rect[1] = view.getHeight();
        return rect;
    }

//    public int GetScreenWidth(Activity activity) {
//        int screenWidth;//屏幕宽度
//        int screenHeight;//屏幕高度
//        WindowManager windowManager = activity.getWindowManager();
//        Display display = windowManager.getDefaultDisplay();
//        screenWidth = display.getWidth();
//        screenHeight = display.getHeight();
//        return screenWidth;
//    }
//
//    public int GetScreenHeight(Object view) {
////        Activity activity = (Activity)activity0;
////        int screenWidth;//屏幕宽度
////        int screenHeight;//屏幕高度
////        WindowManager windowManager = activity.getWindowManager();
////        Display display = windowManager.getDefaultDisplay();
////        screenWidth = display.getWidth();
////        screenHeight = display.getHeight();
//        return 1080;//screenHeight;
//    }

      native void ProvideScreeData(Object obj, long captureObject);
}

