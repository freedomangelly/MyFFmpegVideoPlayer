//
// Created by freed on 2020/1/24.
//

#include <zconf.h>
#include <libavcodec/avcodec.h>
#include "FFMpegJniCall.h"
#include "MyConsts.h"

FFMpegJniCall::FFMpegJniCall(JavaVM *javaVm, JNIEnv *jniEnv , jobject jPlayerObj) {
    this->javaVM=javaVm;
    this->jniEnv=jniEnv;
    this->jPlayerObj=jniEnv->NewGlobalRef(jPlayerObj);


    jclass jPlayerClass=jniEnv->GetObjectClass(jPlayerObj);

    prepared_mid = jniEnv->GetMethodID(jPlayerClass, "onPrepared", "()V");
    loadingMid = jniEnv->GetMethodID(jPlayerClass, "onLoading", "(Z)V");
    progressMid = jniEnv->GetMethodID(jPlayerClass, "onProgress", "(II)V");
    errorMid = jniEnv->GetMethodID(jPlayerClass, "onError", "(ILjava/lang/String;)V");
    completeMid = jniEnv->GetMethodID(jPlayerClass, "onComplete", "()V");
    renderMid = jniEnv->GetMethodID(jPlayerClass, "onRenderYUV420P", "(II[B[B[B)V");
    isSupportStiffCodecMid = jniEnv->GetMethodID(jPlayerClass, "isSupportStiffCodec",
                                                  "(Ljava/lang/String;)Z");
    initMediaCodecMid = jniEnv->GetMethodID(jPlayerClass, "initMediaCodec",
                                             "(Ljava/lang/String;II[B[B)V");
    decodePacketMid = jniEnv->GetMethodID(jPlayerClass, "decodePacket", "(I[B)V");

    jPlayCallbackPcmMid=jniEnv->GetMethodID(jPlayerClass,"callbackPcm","([BI)V");
    jPlayMusicInfoMid=jniEnv->GetMethodID(jPlayerClass,"musicInfo","(II)V");
}

FFMpegJniCall::~FFMpegJniCall(){
    jniEnv->DeleteGlobalRef(jPlayerObj);
}
void FFMpegJniCall::onCallPlayerPrepared(ThreadType mode) {
    //子线程用不了主线程jniEnv（native线程）
    //子线程是不共享jniEnv，他们有自己所独有的
    if(mode==THREAD_MAIN){
        jniEnv->CallVoidMethod(jPlayerObj,prepared_mid);
    } else {
        JNIEnv *env;
        if (javaVM->AttachCurrentThread(&env, 0) != JNI_OK) {
            LOGE("get child thread jniEnv error!");
            return;
        }
        env->CallVoidMethod(jPlayerObj, prepared_mid);
        javaVM->DetachCurrentThread();
    }
}

void FFMpegJniCall::onCallLoading(ThreadType threadType, bool loading) {
    if (threadType == THREAD_MAIN) {
        jniEnv->CallVoidMethod(jPlayerObj, loadingMid, loading);
    } else if (threadType == THREAD_CHILD) {
        JNIEnv *env;
        if (javaVM->AttachCurrentThread(&env, 0) != JNI_OK) {
            LOGE("get child thread jnienv error.");
            return;
        }

        env->CallVoidMethod(jPlayerObj, loadingMid, loading);

        javaVM->DetachCurrentThread();
    }
}

void FFMpegJniCall::onCallProgress(ThreadType threadType, int current, int total) {
    if (threadType == THREAD_MAIN) {
        jniEnv->CallVoidMethod(jPlayerObj, progressMid, current, total);
    } else if (threadType == THREAD_CHILD) {
        JNIEnv *env;
        if (javaVM->AttachCurrentThread(&env, 0) != JNI_OK) {
            LOGE("get child thread jnienv error.");
            return;
        }
        env->CallVoidMethod(jPlayerObj, progressMid, current, total);
        javaVM->DetachCurrentThread();
    }
}

void FFMpegJniCall::onCallPlayerError(ThreadType threadMode, int code, const char *msg) {
    if(threadMode==THREAD_MAIN){
        jstring jmsg=jniEnv->NewStringUTF(msg);
        jniEnv->CallVoidMethod(jPlayerObj,errorMid,code,jmsg);
        jniEnv->DeleteLocalRef(jmsg);
    } else{
        JNIEnv *env;
        if(javaVM->AttachCurrentThread(&env,0)!=JNI_OK){
            LOGE("get child thread jniEnv error!");
            return;
        }
        jstring jmsg=env->NewStringUTF(msg);
        env->CallVoidMethod(jPlayerObj,errorMid,code,jmsg);
        env->DeleteLocalRef(jmsg);
        javaVM->DetachCurrentThread();
    }

}

jbyteArray setArrayByte(JNIEnv *env,uint8_t *bytes,int size){
    jbyteArray jData=env->NewByteArray(size);
    env->SetByteArrayRegion(jData, 0, size, reinterpret_cast<const jbyte *>(bytes));

    return jData;
}


void FFMpegJniCall::onCallComplete(ThreadType threadType) {
    if (threadType == THREAD_MAIN) {
        jniEnv->CallVoidMethod(jPlayerObj, completeMid);
    } else if (threadType == THREAD_CHILD) {
        JNIEnv *env;
        if (javaVM->AttachCurrentThread(&env, 0) != JNI_OK) {
            LOGE("get child thread jnienv error.");
            return;
        }
        env->CallVoidMethod(jPlayerObj, completeMid);
        javaVM->DetachCurrentThread();
    }
}

bool FFMpegJniCall::onCallIsSupportStiffCodec(ThreadType threadType, const char *codecName) {
    bool support = false;

    if (threadType == THREAD_MAIN) {
        jstring jCodecName = jniEnv->NewStringUTF(codecName);
        support = jniEnv->CallBooleanMethod(jPlayerObj, isSupportStiffCodecMid, jCodecName);
        jniEnv->DeleteLocalRef(jCodecName);
    } else if (threadType == THREAD_CHILD) {
        JNIEnv *env;
        if (javaVM->AttachCurrentThread(&env, 0) != JNI_OK) {
            LOGE("get child thread jnienv error.");
            return false;
        }

        jstring jCodecName = env->NewStringUTF(codecName);
        support = env->CallBooleanMethod(jPlayerObj, isSupportStiffCodecMid, jCodecName);
        env->DeleteLocalRef(jCodecName);

        javaVM->DetachCurrentThread();
    }

    return support;
}

void
FFMpegJniCall::onCallRenderYUV420P(int width, int height, uint8_t *fy, uint8_t *fu, uint8_t *fv) {
    JNIEnv *jniEnv;
    if (javaVM->AttachCurrentThread(&jniEnv, 0) != JNI_OK) {
        LOGE("get child thread jnienv error.");
        return;
    }

    jbyteArray y = jniEnv->NewByteArray(width * height);
    jniEnv->SetByteArrayRegion(y, 0, width * height, reinterpret_cast<const jbyte *>(fy));

    jbyteArray u = jniEnv->NewByteArray(width * height / 4);
    jniEnv->SetByteArrayRegion(u, 0, width * height / 4, reinterpret_cast<const jbyte *>(fu));

    jbyteArray v = jniEnv->NewByteArray(width * height / 4);
    jniEnv->SetByteArrayRegion(v, 0, width * height / 4, reinterpret_cast<const jbyte *>(fv));

    jniEnv->CallVoidMethod(jPlayerObj, renderMid, width, height, y, u, v);

    jniEnv->DeleteLocalRef(y);
    jniEnv->DeleteLocalRef(u);
    jniEnv->DeleteLocalRef(v);

    javaVM->DetachCurrentThread();
}

void
FFMpegJniCall::onCallInitMediaCodec(ThreadType threadType, const char *mime, int width, int height,
                                    int csd0Size, int csd1Size, uint8_t *csd0, uint8_t *csd1) {
    if (threadType == THREAD_MAIN) {
        jstring type = jniEnv->NewStringUTF(mime);
        jbyteArray csd0 = jniEnv->NewByteArray(csd0Size);
        jniEnv->SetByteArrayRegion(csd0, 0, csd0Size, reinterpret_cast<const jbyte *>(csd0));
        jbyteArray csd1 = jniEnv->NewByteArray(csd1Size);
        jniEnv->SetByteArrayRegion(csd1, 0, csd1Size, reinterpret_cast<const jbyte *>(csd1));

        jniEnv->CallVoidMethod(jPlayerObj, initMediaCodecMid, type, width, height, csd0, csd1);

        jniEnv->DeleteLocalRef(csd0);
        jniEnv->DeleteLocalRef(csd1);
        jniEnv->DeleteLocalRef(type);
    } else {
        JNIEnv *jniEnv;
        if (javaVM->AttachCurrentThread(&jniEnv, 0) != JNI_OK) {
            LOGE("get child thread jnienv error.");
        }

        jstring type = jniEnv->NewStringUTF(mime);
        jbyteArray jCsd0 = jniEnv->NewByteArray(csd0Size);
        jniEnv->SetByteArrayRegion(jCsd0, 0, csd0Size, reinterpret_cast<const jbyte *>(csd0));
        jbyteArray jCsd1 = jniEnv->NewByteArray(csd1Size);
        jniEnv->SetByteArrayRegion(jCsd1, 0, csd1Size, reinterpret_cast<const jbyte *>(csd1));

        jniEnv->CallVoidMethod(jPlayerObj, initMediaCodecMid, type, width, height, jCsd0, jCsd1);

        jniEnv->DeleteLocalRef(jCsd0);
        jniEnv->DeleteLocalRef(jCsd1);
        jniEnv->DeleteLocalRef(type);
        javaVM->DetachCurrentThread();
    }
}

void FFMpegJniCall::onCallDecodePacket(int dataSize, uint8_t *data) {
    JNIEnv *jniEnv;
    if (javaVM->AttachCurrentThread(&jniEnv, 0) != JNI_OK) {
        LOGE("get child thread jnienv error.");
    }
    jbyteArray jData = jniEnv->NewByteArray(dataSize);
    jniEnv->SetByteArrayRegion(jData, 0, dataSize, reinterpret_cast<const jbyte *>(data));
    jniEnv->CallVoidMethod(jPlayerObj, decodePacketMid, dataSize, jData);
    jniEnv->DeleteLocalRef(jData);
    javaVM->DetachCurrentThread();
}

void FFMpegJniCall::callMusicInfo(ThreadType mode, int sampleRate, int channels) {
    if(mode==THREAD_MAIN){
        jniEnv->CallVoidMethod(jPlayerObj,jPlayMusicInfoMid,sampleRate,channels);
    } else {
        JNIEnv *env;
        if (javaVM->AttachCurrentThread(&env, 0) != JNI_OK) {
            LOGE("get child thread jniEnv error!");
            return;
        }
        env->CallVoidMethod(jPlayerObj, jPlayMusicInfoMid,sampleRate,channels);
        javaVM->DetachCurrentThread();
    }
}
void FFMpegJniCall::callCallbackPcm(ThreadType mode, uint8_t *bytes, int size) {
    if(mode == THREAD_MAIN){
        jbyteArray jData=jniEnv->NewByteArray(size);
        jniEnv->SetByteArrayRegion(jData, 0, size, reinterpret_cast<const jbyte *>(bytes));
        jniEnv->CallVoidMethod(jPlayerObj,jPlayCallbackPcmMid,jData,size);
    } else {
        JNIEnv *env;
        if (javaVM->AttachCurrentThread(&env, 0) != JNI_OK) {
            LOGE("get child thread jniEnv error!");
            return;
        }
        jbyteArray jData=env->NewByteArray(size);
        env->SetByteArrayRegion(jData, 0, size, reinterpret_cast<const jbyte *>(bytes));
        env->CallVoidMethod(jPlayerObj, jPlayCallbackPcmMid,jData,size);
//        env->DeleteGlobalRef(jData);
        javaVM->DetachCurrentThread();
    }
}
