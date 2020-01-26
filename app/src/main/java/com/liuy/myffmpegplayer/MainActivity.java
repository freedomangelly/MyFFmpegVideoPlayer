package com.liuy.myffmpegplayer;

import androidx.appcompat.app.AppCompatActivity;

import android.os.Bundle;
import android.os.Environment;
import android.util.Log;
import android.view.Surface;
import android.view.View;
import android.widget.TextView;

import com.liuy.myffmpegplayer.ffmpegplayer.MediaPlayerJNI;
import com.liuy.myffmpegplayer.ffmpegplayer.listener.MediaErrorListener;
import com.liuy.myffmpegplayer.ffmpegplayer.listener.MediaPreparedListener;

import java.io.File;

public class MainActivity extends AppCompatActivity {
//    File mMusicFile = new File(Environment.getExternalStorageDirectory(), "ental.mp3");
    File mMusicFile = new File(Environment.getExternalStorageDirectory(), "1111.mp4");
    // Used to load the 'native-lib' library on application startup.
    private MediaPlayerJNI mPlayer;
    private VideoView mVideoView;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        Log.e("TAG", "file is exist: " + mMusicFile.exists());
        video();

    }

    public void audio(){

        mPlayer = new MediaPlayerJNI();
        mPlayer.setDataSource(mMusicFile.getAbsolutePath());
//
        mPlayer.setOnErrorListener(new MediaErrorListener() {
            @Override
            public void onError(int code, String msg) {
                Log.e("TAG", "error code: " + code);
                Log.e("TAG", "error msg: " + msg);
                // Java 的逻辑代码
            }
        });
//
        mPlayer.setOnPreparedListener(new MediaPreparedListener() {
            @Override
            public void onPrepared() {
                Log.e("TAG", "准备完毕");
                mPlayer.play();
            }
        });

//        mPlayer.prepare();
//        mPlayer.play();
        mPlayer.prepareAsync();
    }

    public void video(){
        mVideoView=findViewById(R.id.video_view);

    }

    public void play(View view){
        mVideoView.play(mMusicFile.getAbsolutePath());
        decodeView(mMusicFile.getAbsolutePath(),mVideoView.getHolder().getSurface());
    }

    private native void decodeView(String absoluteFile, Surface surface);
}
