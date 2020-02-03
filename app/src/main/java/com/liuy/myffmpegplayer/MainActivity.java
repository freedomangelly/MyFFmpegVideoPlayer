package com.liuy.myffmpegplayer;

import androidx.appcompat.app.AppCompatActivity;

import android.os.Bundle;
import android.os.Environment;
import android.util.Log;
import android.view.Surface;
import android.view.View;
import android.widget.SeekBar;
import android.widget.TextView;

import com.liuy.myffmpegplayer.ffmpegplayer.MediaPlayerJNI;
import com.liuy.myffmpegplayer.ffmpegplayer.listener.MediaErrorListener;
import com.liuy.myffmpegplayer.ffmpegplayer.listener.MediaPreparedListener;
import com.liuy.myffmpegplayer.ffmpegplayer.listener.MediaProgressListener;
import com.liuy.myffmpegplayer.ffmpegplayer.render.FFmpegVideoView;

import java.io.File;

public class MainActivity extends AppCompatActivity {
    private MediaPlayerJNI mPlayer;
    private static final String TAG="MainActivity";
    File mMusicFile = new File(Environment.getExternalStorageDirectory(), "1111.mp4");
    String url = "http://v1-default.bytecdn.cn/86c7ca6fa771c8968b1a72fd7a90f1a5/5d15bc0a/video/m/2209a3e637ccbe24258a3725e0a30134e5911619251f00004a66592c6771/?rc=ajZvdjgzOmd2azMzZTczM0ApQHRAbzY7NDo4MzgzMzc4NDUzNDVvQGg1dilAZzN3KUBmM3UpZHNyZ3lrdXJneXJseHdmNzZAcG0vYG9yLzJxXy0tMS0vc3MtbyNvIzAvMy0xLy4uLjYwNTM2LTojbyM6YS1vIzpgLXAjOmB2aVxiZitgXmJmK15xbDojMy5e&vfrom=xgplayer";
    //    File mMusicFile = new File(Environment.getExternalStorageDirectory(), "1111.mp4");
    // Used to load the 'native-lib' library on application startup.
    private FFmpegVideoView mVideoView;
    private SeekBar mSeekBar;
    private int mTotalTime,mProgress;
    private boolean seeking;
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        setupView();
        Log.e("TAG", "file is exist: " + mMusicFile.exists());
//        audio();
//        video();
//        push();
    }

    private void setupView() {
        mSeekBar = findViewById(R.id.seek_bar);
        mVideoView = findViewById(R.id.video_view);
        mPlayer = new MediaPlayerJNI();
        mPlayer.setVideoView(mVideoView);
        mPlayer.setDataSource(mMusicFile.getAbsolutePath());
        mPlayer.setOnPreparedListener(new MediaPreparedListener() {
            @Override
            public void onPrepared() {
                mPlayer.start();
            }
        });
        mPlayer.prepareAsync();

        mPlayer.setOnProgressListener(new MediaProgressListener() {
            @Override
            public void onProgress(int current, int total) {
                mTotalTime = total;
                if (!seeking) {
                    mSeekBar.setProgress(current * 100 / total);
                }
                Log.e("TAG", current+"/"+total);
            }
        });

        mSeekBar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                mProgress = progress * mTotalTime / 100;
                Log.i("info","seek onProgressChanged");
            }

            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {

                Log.i("info","seek onStartTrackingTouch");
                seeking = true;
            }

            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {
                Log.i("info","seek onStopTrackingTouch");
                mPlayer.seekTo(mProgress);
                seeking = false;
                // Log.e("TAG", mProgress+"/"+mProgress);
            }
        });
    }

    public void push(){
//        mLivePush=new LivePush("rtmp://203.195.152.181/myapp/mystream");
//        mLivePush.setOnConnectListener(new LivePush.ConnectListener() {
//            @Override
//            public void connectError(int errorCode, String errorMsg) {
//                Log.e("TAG", "errorCode:" + errorCode);
//                Log.e("TAG", "errorMsg:" + errorMsg);
//            }
//
//            @Override
//            public void connectSuccess() {
//                Log.e("TAG", "connectSuccess:可以推流了");
//            }
//        });
//        mLivePush.initConnect();
    }

    public void audio(){

//        mPlayer = new MediaPlayerJNI();
//        mPlayer.setDataSource(mMusicFile.getAbsolutePath());
////
//        mPlayer.setOnErrorListener(new MediaErrorListener() {
//            @Override
//            public void onError(int code, String msg) {
//                Log.e("TAG", "error code: " + code);
//                Log.e("TAG", "error msg: " + msg);
//                // Java 的逻辑代码
//            }
//        });
////
//        mPlayer.setOnPreparedListener(new MediaPreparedListener() {
//            @Override
//            public void onPrepared() {
//                Log.e("TAG", "准备完毕");
//                mPlayer.play();
//            }
//        });
//
////        mPlayer.prepare();
////        mPlayer.play();
//        mPlayer.prepareAsync();
    }

    public void video(){
//        mVideoView=findViewById(R.id.video_view);

    }

    public void play(View view){
//        mVideoView.play(mMusicFile.getAbsolutePath());
    }

    public void pause(View view) {
        mPlayer.pause();
    }

    public void resume(View view) {
        if (mPlayer.isPlaying()) {
            mPlayer.resume();
        }else {
            mPlayer.prepareAsync();
        }
    }
    public void stop(View view) {
        mPlayer.stop();
    }

    public void seek(View view) {
        mPlayer.seekTo(30);
    }
}
