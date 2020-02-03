package com.liuy.myffmpegplayer;

/**
 * description:
 * author: freed on 2020/1/27
 * email: 674919909@qq.com
 * version: 1.0
 */
public class LivePush {
//    static {
//        System.loadLibrary("native-lib");
//    }

    private String mLiveUrl;

    public LivePush(String liveUrl) {
        this.mLiveUrl = liveUrl;
    }

    private ConnectListener mConnectListener;

    public void setOnConnectListener(ConnectListener connectListener) {
        this.mConnectListener = connectListener;
    }

    public interface ConnectListener{
        void connectError(int errorCode, String errorMsg);
        void connectSuccess();
    }

    public void initConnect(){
        nInitConnect(mLiveUrl);
    }

    private native void nInitConnect(String liveUrl);

    // called from jni
    private void onConnectError(int errorCode, String errorMsg){
        if(mConnectListener != null){
            mConnectListener.connectError(errorCode,errorMsg);
        }
    }

    // called from jni
    private void onConnectSuccess(){
        if(mConnectListener != null){
            mConnectListener.connectSuccess();
        }
    }
}
