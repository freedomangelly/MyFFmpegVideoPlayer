package com.liuy.myffmpegplayer.ffmpegplayer.render;

import android.content.Context;
import android.opengl.GLSurfaceView;
import android.util.AttributeSet;

public class FFmpegVideoView extends GLSurfaceView{
    private FFmpegRender mDZRender;

    public FFmpegVideoView(Context context) {
        this(context, null);
    }

    public FFmpegVideoView(Context context, AttributeSet attrs) {
        super(context, attrs);
        setEGLContextClientVersion(2);
        mDZRender = new FFmpegRender(context);
        setRenderer(mDZRender);
        setRenderMode(GLSurfaceView.RENDERMODE_WHEN_DIRTY);

        mDZRender.setOnRenderListener(new FFmpegRender.RenderListener() {
            @Override
            public void onRender() {
                requestRender();
            }
        });
    }

    public void setYUVData(int width, int height, byte[] y, byte[] u, byte[] v)
    {
        if(mDZRender != null)
        {
            mDZRender.setYUVRenderData(width, height, y, u, v);
            requestRender();
        }
    }

    public FFmpegRender getWlRender() {
        return mDZRender;
    }
}
