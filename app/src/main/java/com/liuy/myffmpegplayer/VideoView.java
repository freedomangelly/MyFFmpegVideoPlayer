package com.liuy.myffmpegplayer;

import android.content.Context;
import android.graphics.PixelFormat;
import android.util.AttributeSet;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

import com.liuy.myffmpegplayer.ffmpegplayer.MediaPlayerJNI;
import com.liuy.myffmpegplayer.ffmpegplayer.listener.MediaPreparedListener;

/**
 * description:
 * author: freed on 2020/1/26
 * email: 674919909@qq.com
 * version: 1.0
 */
public class VideoView extends SurfaceView implements MediaPreparedListener {
    private MediaPlayerJNI mPlayer;
    public VideoView(Context context) {
        this(context,null);
    }

    public VideoView(Context context, AttributeSet attrs) {
        this(context, attrs,0);
    }

    public VideoView(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
        init();
    }

    public void init(){
        SurfaceHolder surfaceHolder=getHolder();
        surfaceHolder.setFormat(PixelFormat.RGBA_8888);
        mPlayer=new MediaPlayerJNI();
        mPlayer.setOnPreparedListener(this);
    }

    public void play(String url){
        stop();
        mPlayer.setDataSource(url);
        mPlayer.prepareAsync();
    }

    private void stop() {

    }


    @Override
    public void onPrepared() {
        mPlayer.play();
    }
}
