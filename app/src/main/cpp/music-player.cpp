#include <jni.h>
#include "MyConsts.h"
#include "com_liuy_myffmpegplayer_ffmpegplayer_MediaPlayerJNI.h"
#include "FFMpegJniCall.h"
#include "MyFFmpeg.h"

#include <android/native_window.h>
#include <android/native_window_jni.h>


// 在 c++ 中采用 c 的这种编译方式
extern "C" {
#include "libavformat/avformat.h"
#include "libswresample/swresample.h"
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"
}

FFMpegJniCall *pJniCall;
MyFFmpeg *pFFmpeg;

JavaVM *pJavaVM = NULL;

// 重写 so 被加载时会调用的一个方法
// 小作业，去了解动态注册
extern "C" JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *javaVM, void *reserved) {
    LOGI("JNI_OnLoad");
    pJavaVM = javaVM;
    JNIEnv *env;
    if (javaVM->GetEnv((void **) &env, JNI_VERSION_1_4) != JNI_OK) {
        return -1;
    }
    return JNI_VERSION_1_4;
}

extern "C" JNIEXPORT void JNICALL Java_com_liuy_myffmpegplayer_ffmpegplayer_MediaPlayerJNI_nPrepareAsync
        (JNIEnv * env, jobject object, jstring string){
    LOGI("nPrepareAsync");
    const char *url=env->GetStringUTFChars(string,0);
    if(pFFmpeg == NULL){
        if(pJniCall==NULL){
            LOGI("nPrepareAsync 111");
            pJniCall=new FFMpegJniCall(pJavaVM,env,object);
            LOGI("nPrepareAsync 222");
        }
        LOGI("nPrepareAsync 333");
        pFFmpeg=new MyFFmpeg(pJniCall,url);
        LOGI("nPrepareAsync 444");
        pFFmpeg->prepareAsync();
        LOGI("nPrepareAsync 555");
    }
    env->ReleaseStringUTFChars(string,url);
    LOGI("nPrepareAsync 666");
}

/*
 * Class:     com_liuy_myffmpegplayer_ffmpegplayer_MediaPlayerJNI
 * Method:    nPrepare
 * Signature: (Ljava/lang/String;)V
 */
extern "C" JNIEXPORT void JNICALL Java_com_liuy_myffmpegplayer_ffmpegplayer_MediaPlayerJNI_nPrepare
        (JNIEnv *env, jobject instance, jstring url_){
    const char *url = env->GetStringUTFChars(url_, 0);
    if (pFFmpeg == NULL) {
        if (pJniCall == NULL) {
            pJniCall = new FFMpegJniCall(pJavaVM, env, instance);
        }
        pFFmpeg = new MyFFmpeg(pJniCall, url);
        pFFmpeg->prepare();
    }
    env->ReleaseStringUTFChars(url_, url);
}

/*
 * Class:     com_liuy_myffmpegplayer_ffmpegplayer_MediaPlayerJNI
 * Method:    nStart
 * Signature: ()V
 */
extern "C" JNIEXPORT void JNICALL Java_com_liuy_myffmpegplayer_ffmpegplayer_MediaPlayerJNI_nStart
        (JNIEnv *env, jobject instance){
    LOGI("start 11111");
    if(pFFmpeg){
        LOGI("start 22222222");
        pFFmpeg->start();
        LOGI("start 3333333333");
    }
}

/*
 * Class:     com_liuy_myffmpegplayer_ffmpegplayer_MediaPlayerJNI
 * Method:    nPause
 * Signature: ()V
 */
extern "C" JNIEXPORT void JNICALL Java_com_liuy_myffmpegplayer_ffmpegplayer_MediaPlayerJNI_nPause
        (JNIEnv *env, jobject insance){
    if(pFFmpeg){
        pFFmpeg->onPause();
    }
}

/*
 * Class:     com_liuy_myffmpegplayer_ffmpegplayer_MediaPlayerJNI
 * Method:    nResume
 * Signature: ()V
 */
extern "C" JNIEXPORT void JNICALL Java_com_liuy_myffmpegplayer_ffmpegplayer_MediaPlayerJNI_nResume
        (JNIEnv *env, jobject insance){
    if(pFFmpeg){
        pFFmpeg->onResume();
    }
}
/*
 * Class:     com_liuy_myffmpegplayer_ffmpegplayer_MediaPlayerJNI
 * Method:    nStop
 * Signature: ()V
 */
extern "C" JNIEXPORT void JNICALL Java_com_liuy_myffmpegplayer_ffmpegplayer_MediaPlayerJNI_nStop
        (JNIEnv *env, jobject instance){
    if(pFFmpeg!=NULL){
        pFFmpeg->stop();
        pFFmpeg->release();
        delete pFFmpeg;
        pFFmpeg=NULL;
    }
    if(pJniCall!=NULL){
        delete(pJniCall);
        pJniCall=NULL;
    }
}


/*
 * Class:     com_liuy_myffmpegplayer_ffmpegplayer_MediaPlayerJNI
 * Method:    nSeekTo
 * Signature: (I)V
 */
extern "C" JNIEXPORT void JNICALL Java_com_liuy_myffmpegplayer_ffmpegplayer_MediaPlayerJNI_nSeekTo
        (JNIEnv *env, jobject object, jint seconds){
    LOGI("jni seek 111 %d",seconds);
    if(pFFmpeg!=NULL){
        pFFmpeg->seek(seconds);
    }
}