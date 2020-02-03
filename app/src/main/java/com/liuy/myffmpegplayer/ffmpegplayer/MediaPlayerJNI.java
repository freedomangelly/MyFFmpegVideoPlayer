package com.liuy.myffmpegplayer.ffmpegplayer;

import android.media.MediaCodec;
import android.media.MediaCodecList;
import android.media.MediaFormat;
import android.text.TextUtils;
import android.util.Log;
import android.view.Surface;

import com.liuy.myffmpegplayer.ffmpegplayer.listener.MediaCompleteListener;
import com.liuy.myffmpegplayer.ffmpegplayer.listener.MediaErrorListener;
import com.liuy.myffmpegplayer.ffmpegplayer.listener.MediaLoadListener;
import com.liuy.myffmpegplayer.ffmpegplayer.listener.MediaPauseListener;
import com.liuy.myffmpegplayer.ffmpegplayer.listener.MediaPreparedListener;
import com.liuy.myffmpegplayer.ffmpegplayer.listener.MediaProgressListener;
import com.liuy.myffmpegplayer.ffmpegplayer.render.FFmpegRender;
import com.liuy.myffmpegplayer.ffmpegplayer.render.FFmpegVideoView;

import java.nio.ByteBuffer;
import java.util.HashMap;
import java.util.Map;


/**
 * description:
 * author: freed on 2020/1/18
 * email: 674919909@qq.com
 * version: 1.0
 */
public class MediaPlayerJNI {

    private Surface mSurface;
    private MediaCodec mMediaCodec;
    private MediaCodec.BufferInfo mOutBufferInfo;
    private static final Map<String, String> mCodecMap = new HashMap<>(2);
    private FFmpegVideoView mVideoView;

    static {
        System.loadLibrary("music-player");

        /**
         "video/x-vnd.on2.vp8" - VP8 video (i.e. video in .webm)
         "video/x-vnd.on2.vp9" - VP9 video (i.e. video in .webm)
         "video/avc" - H.264/AVC video
         "video/hevc" - H.265/HEVC video
         "video/mp4v-es" - MPEG4 video
         "video/3gpp" - H.263 video
         "audio/3gpp" - AMR narrowband audio
         "audio/amr-wb" - AMR wideband audio
         "audio/mpeg" - MPEG1/2 audio layer III
         "audio/mp4a-latm" - AAC audio (note, this is raw AAC packets, not packaged in LATM!)
         "audio/vorbis" - vorbis audio
         "audio/g711-alaw" - G.711 alaw audio
         "audio/g711-mlaw" - G.711 ulaw audio
         */
        mCodecMap.put("h264", "video/avc");
        mCodecMap.put("h265", "video/hevc");
    }

    private static final String findMediaCodecName(String codeName) {
        if (mCodecMap.containsKey(codeName)) {
            return mCodecMap.get(codeName);
        }
        return "";
    }

    /**
     * 是否支持硬解码，Call from jni
     *
     * @param codeName ffmpeg 的解码器名字
     * @return 是否支持硬解码
     */
    private final boolean isSupportStiffCodec(String codeName) {
        int codecCount = MediaCodecList.getCodecCount();
        for (int i = 0; i < codecCount; i++) {
            String[] supportedTypes = MediaCodecList.getCodecInfoAt(i).getSupportedTypes();
            for (String type : supportedTypes) {
                if (type.equals(findMediaCodecName(codeName))) {
                    return true;
                }
            }
        }
        return false;
    }

    /**
     * 初始化 MediaCodec
     * Call from jni
     */
    private void initMediaCodec(String codecName, int width, int height, byte[] csd0, byte[] csd1) {
        Log.i("info","initMediaCodec");
        if (mSurface == null) {
            Log.d("TAG", "surface is null");
            return;
        }
        try {
            mOutBufferInfo = new MediaCodec.BufferInfo();
            mVideoView.getWlRender().setRenderType(FFmpegRender.RENDER_MEDIA_CODEC);
            String mime = findMediaCodecName(codecName);
            MediaFormat mediaFormat = MediaFormat.createVideoFormat(mime, width, height);
            mediaFormat.setInteger(MediaFormat.KEY_MAX_INPUT_SIZE, width * height);
            mediaFormat.setByteBuffer("csd-0", ByteBuffer.wrap(csd0));
            mediaFormat.setByteBuffer("csd-1", ByteBuffer.wrap(csd1));
            mMediaCodec = MediaCodec.createDecoderByType(mime);
            mMediaCodec.configure(mediaFormat, mSurface, null, 0);
            mMediaCodec.start();
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    private void decodePacket(int dataSize, byte[] data) {
        Log.i("info","decodePacket "+isPlaying);
        if (mSurface != null && dataSize > 0 && data != null && mMediaCodec != null && isPlaying) {
            try {
                int intputBufferIndex = mMediaCodec.dequeueInputBuffer(10);
                if (intputBufferIndex >= 0) {
                    ByteBuffer byteBuffer = mMediaCodec.getInputBuffers()[intputBufferIndex];
                    byteBuffer.clear();
                    byteBuffer.put(data);
                    mMediaCodec.queueInputBuffer(intputBufferIndex, 0, dataSize, 0, 0);
                }
                int outputBufferIndex = mMediaCodec.dequeueOutputBuffer(mOutBufferInfo, 10);
                Log.i("info","isPlaying2="+isPlaying);
                while (outputBufferIndex >= 0&&isPlaying) {
                    mMediaCodec.releaseOutputBuffer(outputBufferIndex, true);
                    outputBufferIndex = mMediaCodec.dequeueOutputBuffer(mOutBufferInfo, 10);
                }
            } catch (MediaCodec.CryptoException e) {
                e.printStackTrace();
            }
        }
    }

    private void releaseMediaCodec() {
        if (mMediaCodec != null) {
            mMediaCodec.flush();
            mMediaCodec.stop();
            mMediaCodec.release();
            mMediaCodec = null;
            mOutBufferInfo = null;
        }
    }
    //--------------- 硬解码相关  end  ---------------
    /**
     * url 可以是本地文件路径，也可以是 http 链接
     */
    private String source;
    private boolean isPlaying = false;
    private boolean isPause = false;
    private MediaPreparedListener mPreparedListener;

    private MediaLoadListener mLoadListener;

    private MediaPauseListener mPauseListener;

    private MediaProgressListener mProgressListener;

    private MediaErrorListener mErrorListener;

    private MediaCompleteListener mCompleteListener;

    public void setOnPreparedListener(MediaPreparedListener preparedListener) {
        mPreparedListener = preparedListener;
    }

    public void setOnLoadListener(MediaLoadListener loadListener) {
        mLoadListener = loadListener;
    }

    public void setOnPauseListener(MediaPauseListener pauseListener) {
        mPauseListener = pauseListener;
    }

    public void setOnProgressListener(MediaProgressListener progressListener) {
        mProgressListener = progressListener;
    }

    public void setOnErrorListener(MediaErrorListener errorListener) {
        mErrorListener = errorListener;
    }

    public void setOnCompleteListener(MediaCompleteListener completeListener) {
        mCompleteListener = completeListener;
    }

    // Call from jni
    private void onPrepared() {
        Log.i("info","onPrepared");
        if (mPreparedListener != null) {
            mPreparedListener.onPrepared();
        }
    }

    // Called from jni
    private void onLoading(boolean loading) {
//        Log.i("info","onLoading");
        if (mLoadListener != null) {
            mLoadListener.onLoad(loading);
        }
    }

    // Called from jni
    private void onProgress(int current, int total) {
        Log.i("info","onProgress");
        if (mProgressListener != null) {
            mProgressListener.onProgress(current, total);
        }
    }

    // Called from jni
    private void onError(int errorCode, String errorMsg) {
        Log.i("info","onError");
        if (mErrorListener != null) {
            mErrorListener.onError(errorCode, errorMsg);
        }
    }

    // Called from jni
    private void onComplete() {
        Log.i("info","onComplete");
        isPlaying = false;
        if (mCompleteListener != null) {
            mCompleteListener.onComplete();
        }
    }

    // Called from jni
    private void onRenderYUV420P(int width, int height, byte[] y, byte[] u, byte[] v) {
        Log.i("info","onRenderYUV420P");
        if (mVideoView != null) {
            mVideoView.setYUVData(width, height, y, u, v);
        }
    }

    public void setVideoView(FFmpegVideoView videoView) {
        this.mVideoView = videoView;
        mVideoView.getWlRender().setOnSurfaceCreateListener(new FFmpegRender.SurfaceCreateListener() {
            @Override
            public void onSurfaceCreate(Surface surface) {
                mSurface = surface;
            }
        });
    }

    public void setDataSource(String url) {
        this.source = url;
    }
    public void prepare() {
        if (TextUtils.isEmpty(source)) {
            throw new NullPointerException("url is null, please call method setDataSource");
        }
        nPrepare(source);
    }

    /**
     * 异步准备
     */
    public void prepareAsync() {
        if (TextUtils.isEmpty(source)) {
            throw new NullPointerException("url is null, please call method setDataSource");
        }
        nPrepareAsync(source);
    }

    public void start() {
        Log.i("info","start");
        isPlaying=true;
        if (TextUtils.isEmpty(source)) {
            throw new NullPointerException("url is null, please call method setDataSource");
        }
        Log.i("info","start1");
        nStart();
    }

    public void pause() {
        isPause = true;
        nPause();
        if (mPauseListener != null) {
            mPauseListener.onPause(true);
        }
    }

    public void resume() {
        isPause = false;
        nResume();
        if (mPauseListener != null) {
            mPauseListener.onPause(false);
        }
    }

    private native void nPrepareAsync(String source);

    private native void nPrepare(String source);

    private native void nStart();

    private native void nPause();

    private native void nResume();

    public void stop() {
        isPlaying=false;
        Log.i("info","isPlaying="+isPlaying);
        try {
            Thread.sleep(50);
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
        nStop();
        releaseMediaCodec();
    }

    private native void nStop();

    public void seekTo(int seconds) {
        Log.i("info","seekTO"+seconds);
        nSeekTo(seconds);
    }

    private native void nSeekTo(int seconds);

    public boolean isPlaying() {
        return isPlaying;
    }

    private void musicInfo(int sampleRate, int channels) {
        Log.i("info","musicInfo");
//        if (mInfoListener != null) {
//            mInfoListener.musicInfo(sampleRate, channels);
//        }
    }

    private void callbackPcm(byte[] pcmData, int size) {
        Log.i("info","callbackPcm");
//        if (mInfoListener != null) {
//            mInfoListener.callbackPcm(pcmData, size);
//        }
    }
}
