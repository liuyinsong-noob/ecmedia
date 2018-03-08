package com.yuntongxun.ecsdk.core.voip;

import android.content.Context;
import android.graphics.SurfaceTexture;
import android.opengl.GLES20;
import android.opengl.GLSurfaceView;
import android.opengl.Matrix;
import android.util.AttributeSet;

import com.seu.magicfilter.base.gpuimage.GPUImageFilter;
import com.seu.magicfilter.utils.MagicFilterFactory;
import com.seu.magicfilter.utils.MagicFilterType;
import com.seu.magicfilter.utils.OpenGLUtils;

import java.nio.ByteBuffer;
import java.nio.IntBuffer;
import java.util.concurrent.ConcurrentLinkedQueue;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

/**
 * Created by gezhaoyou on 2018/3/1.
 */

/**
 * ViEFilterRenderView add to parent view, the parent view must keep same Aspect ratio with camera capture Aspect ratio.
 */
public class ViEFilterRenderView extends GLSurfaceView {

    public interface ViERendererCallback {
        public void onFilterRenderReady();
        public  void onIncomingRgbaFrame(byte[] data, int length, int width, int height);
    }

    private static ViEFilterRenderView filterRender;

    public static ViEFilterRenderView createFilterRenderer(Context context) {
        filterRender = new ViEFilterRenderView(context);
        return filterRender;
    }

    public static ViEFilterRenderView getFilterRendererView() {
        return filterRender;
    }
    public boolean isRenderReady(){
        return isRenderReady;
    }
    public SurfaceTexture getRenderTexture(){
        return renderTexture;
    }

    private ViERendererCallback viERendererCallback;
    private GPUImageFilter imageFilter;
    private boolean isRenderReady = false;

    private SurfaceTexture renderTexture;
    private Context mContext;
    private int textureId = OpenGLUtils.NO_TEXTURE;

    private int imageWidth, imageHeight;        // picture with and height
    private int surfaceWidth, surfaceHeight;    // preview surface with and height

    // frame buffer  after filter.
    private ByteBuffer mGLPreviewBuffer;
    private ConcurrentLinkedQueue<IntBuffer> rawFrameBufferCache = new ConcurrentLinkedQueue<IntBuffer>();
    private Thread rgbaFrameProcessWorker;
    private final Object writeLock = new Object();

    // todo: scale image effective not ok, so should't use it.
    boolean needScaleImage = false;
    private float mInputAspectRatio;
    private float mOutputAspectRatio;
    private float[] mProjectionMatrix = new float[16];
    private float[] mSurfaceMatrix = new float[16];
    private float[] mTransformMatrix = new float[16];

    private ViEFilterRenderView(Context context) {
        this(context, null);
    }

    private ViEFilterRenderView(Context context, AttributeSet attrs) {
        super(context, attrs);

        mContext = context;
        setEGLContextClientVersion(2);
        setRenderer(new GPURenderer());
        setRenderMode(RENDERMODE_WHEN_DIRTY);
    }

    /**
     *
     * @param width
     * @param height
     */
    public void setImageFrameSize(int width, int height) {
        imageWidth  =  width;
        imageHeight =  height;
        if(needScaleImage) {
            mInputAspectRatio = imageWidth > imageHeight ?
                    (float) imageWidth / imageHeight : (float) imageWidth / imageHeight;
        }
    }

    public void setCallback(ViERendererCallback callback) {
        viERendererCallback = callback;
    }

    public boolean setFilter(final MagicFilterType type) {
        if (!isRenderReady) {
            return false;
        }

        queueEvent(new Runnable() {
            @Override
            public void run() {
                if (imageFilter != null) {
                    imageFilter.destroy();
                }
                imageFilter = MagicFilterFactory.initFilters(type);
                if (imageFilter != null) {
                    imageFilter.init(getContext().getApplicationContext());
                    imageFilter.onInputSizeChanged(imageWidth, imageHeight);
                    imageFilter.onDisplaySizeChanged(surfaceWidth, surfaceHeight);
                }
            }
        });
        requestRender();
        return true;
    }

    private class GPURenderer implements Renderer {
        @Override
        public void onSurfaceCreated(GL10 gl10, EGLConfig eglConfig) {
            GLES20.glDisable(GL10.GL_DITHER);
            GLES20.glClearColor(0,0, 0, 0);
            GLES20.glEnable(GL10.GL_CULL_FACE);
            GLES20.glEnable(GL10.GL_DEPTH_TEST);

            initImageFilter();
        }

        private void initImageFilter() {
            // create and init image filter.
            if(imageFilter == null) {
                imageFilter = MagicFilterFactory.initFilters(MagicFilterType.NONE);
            }
            imageFilter.init(mContext);

            // init texture
            if(textureId == OpenGLUtils.NO_TEXTURE) {
                textureId = OpenGLUtils.getExternalOESTextureID();
            }
            if(textureId != OpenGLUtils.NO_TEXTURE) {
                renderTexture = new SurfaceTexture(textureId);
                renderTexture.setOnFrameAvailableListener(new SurfaceTexture.OnFrameAvailableListener() {
                    @Override
                    public void onFrameAvailable(SurfaceTexture surfaceTexture) {
                        requestRender();
                    }
                });
            }

            runImageFrameProcessWork();

            //notify callback render ready.
            isRenderReady = true;
            if(viERendererCallback != null) {
                viERendererCallback.onFilterRenderReady();
            }
        }

        @Override
        public void onSurfaceChanged(GL10 gl10, int width, int height) {
            GLES20.glViewport(0, 0, width, height);
            surfaceWidth = width;
            surfaceHeight = height;

            onFilterChanged();
            if(needScaleImage) {
                mOutputAspectRatio = width > height ? (float) width / height : (float) height / width;
                float aspectRatio = mOutputAspectRatio / mInputAspectRatio;
                if (width > height) {
                    Matrix.orthoM(mProjectionMatrix, 0, -1.0f, 1.0f, -aspectRatio, aspectRatio, -1.0f, 1.0f);
                } else {
                    Matrix.orthoM(mProjectionMatrix, 0, -aspectRatio, aspectRatio, -1.0f, 1.0f, -1.0f, 1.0f);
                }
            }
        }

        protected void onFilterChanged(){
            if(imageFilter != null) {
                imageFilter.onInputSizeChanged(imageWidth, imageHeight);
                imageFilter.onDisplaySizeChanged(surfaceWidth, surfaceHeight);
            }
        }

        @Override
        public void onDrawFrame(GL10 gl10) {
            GLES20.glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
            GLES20.glClear(GLES20.GL_COLOR_BUFFER_BIT | GLES20.GL_DEPTH_BUFFER_BIT);

            if(renderTexture == null)
                return;
            renderTexture.updateTexImage();

            if(needScaleImage) {
                renderTexture.getTransformMatrix(mSurfaceMatrix);
                Matrix.multiplyMM(mTransformMatrix, 0, mSurfaceMatrix, 0, mProjectionMatrix, 0);
                imageFilter.setTextureTransformMatrix(mTransformMatrix);
            } else {
                float[] mtx = new float[16];
                renderTexture.getTransformMatrix(mtx);
                imageFilter.setTextureTransformMatrix(mtx);
            }

            if(imageFilter != null){
                imageFilter.onDrawFrame(textureId);
            }

            cacheImageFrame();
        }
    }

    /**
     * cache frmae afer image filter.
     */
    private void cacheImageFrame() {
        // cache filter data
        rawFrameBufferCache.add(imageFilter.getGLFboBuffer());
        synchronized (writeLock) {
            writeLock.notifyAll();
        }
    }

    /**
     * process rgba frame
     */
    private void runImageFrameProcessWork() {

        mGLPreviewBuffer = ByteBuffer.allocateDirect(imageWidth * imageHeight * 4);
        rgbaFrameProcessWorker = new Thread(new Runnable() {
            @Override
            public void run() {
                while (!Thread.interrupted()) {
                    while (!rawFrameBufferCache.isEmpty()) {
                        IntBuffer picture = rawFrameBufferCache.poll();
                        mGLPreviewBuffer.asIntBuffer().put(picture.array());
                        // video frame after filter. zhaoyou
                        if(viERendererCallback != null) {
                            viERendererCallback.onIncomingRgbaFrame(mGLPreviewBuffer.array(), imageWidth * imageHeight * 4, imageWidth, imageHeight);
                        }
                    }

                    // Waiting for next frame
                    synchronized (writeLock) {
                        try {
                            // isEmpty() may take some time, so we set timeout to detect next frame
                            writeLock.wait(500);
                        } catch (InterruptedException ie) {
                            rgbaFrameProcessWorker.interrupt();
                        }
                    }
                }
            }
        });

        rgbaFrameProcessWorker.start();
    }

    public void stopRender() {
        deleteTextures();
        rawFrameBufferCache.clear();

        if (rgbaFrameProcessWorker != null) {
            rgbaFrameProcessWorker.interrupt();
            try {
                rgbaFrameProcessWorker.join();
            } catch (InterruptedException e) {
                e.printStackTrace();
                rgbaFrameProcessWorker.interrupt();
            }
            rgbaFrameProcessWorker = null;
        }
    }

    private void deleteTextures() {
        if (textureId != OpenGLUtils.NO_TEXTURE) {
            queueEvent(new Runnable() {
                @Override
                public void run() {
                    if (imageFilter != null) {
                        imageFilter.destroy();
                    }

                    GLES20.glDeleteTextures(1, new int[]{textureId}, 0);
                    textureId = OpenGLUtils.NO_TEXTURE;
                }
            });
        }
    }
}

