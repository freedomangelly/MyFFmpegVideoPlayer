//
// Created by freed on 2020/1/24.
//

#ifndef MYFFMPEGPLAYER_FFMPEGJNICALL_H
#define MYFFMPEGPLAYER_FFMPEGJNICALL_H

#include <jni.h>

enum ThreadType{
    THREAD_CHILD,THREAD_MAIN
};

class FFMpegJniCall {
public :
    JavaVM *javaVM;
    JNIEnv *jniEnv;
    jobject  jPlayerObj;

    jmethodID  prepared_mid;
    jmethodID  errorMid;
    jmethodID  loadingMid;
    jmethodID  progressMid;
    jmethodID  completeMid;
    jmethodID  renderMid;
    jmethodID  isSupportStiffCodecMid;
    jmethodID  initMediaCodecMid;
    jmethodID  decodePacketMid;


    jmethodID  jPlayMusicInfoMid;
    jmethodID  jPlayCallbackPcmMid;
public:
    FFMpegJniCall(JavaVM *javaVm ,JNIEnv *jniEnv,jobject jPlayerObj);
    ~FFMpegJniCall();

private:


public :

    void onCallPlayerPrepared(ThreadType mode);

    void onCallLoading(ThreadType threadType,bool loading);

    void onCallProgress(ThreadType threadType,int current,int total);

    void onCallPlayerError(ThreadType threadMode, int code, const char *msg);

    void onCallComplete(ThreadType threadType);

    void onCallRenderYUV420P(int width,int height,uint8_t *fy,uint8_t *fu,uint8_t *fv);

    bool onCallIsSupportStiffCodec(ThreadType threadType,const char *codecName);

    void onCallInitMediaCodec(ThreadType threadType,const char *mine,int width,int height,int csd0Size,int csd1Size,uint8_t *csd0,uint8_t *csd1);

    void onCallDecodePacket(int i,uint8_t *string);

public:

    void callMusicInfo(ThreadType mode,int sampleRate, int channels);

    void callCallbackPcm(ThreadType mode,uint8_t *bytes,int size);

};


#endif //MYFFMPEGPLAYER_FFMPEGJNICALL_H
