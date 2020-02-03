//
// Created by freed on 2020/1/25.
//

#ifndef MYFFMPEGPLAYER_FFMPEGAUDIO_H
#define MYFFMPEGPLAYER_FFMPEGAUDIO_H

#include <pthread.h>

extern "C"{
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
};

#include "MyConsts.h"
#include "FFMpegJniCall.h"
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>

#include "FFmpegPlayerStatus.h"
#include "FFmpegPlayerStatus.h"
#include "FFmpegPacketQueue.h"
#include "FFmpegMedia.h"
#include "libavutil/time.h"

class FFmpegAudio :public FFmpegMedia{
public:
    pthread_t playThreadT;
    SwrContext *swrContext=NULL;
    SLPlayItf  slPlayItf=NULL;
    SLObjectItf  pPlayer=NULL;
    SLObjectItf  mixObject=NULL;
    SLObjectItf  engineObj=NULL;
    SLAndroidSimpleBufferQueueItf  androidBufferQueueItf;
    uint8_t *convertOutBuffer=NULL;
    int frameBufferSize=0;

public:
    FFmpegAudio(int audioStreamIndex, FFMpegJniCall *pJniCall, FFmpegPlayerStatus *pPlayerStatus);
    ~FFmpegAudio();
//    ~FFmpegAudio(JNIEnv *env);
    void play();

    int resampleAudio();
    void initCreateOpenSLES();
    void privateAnalysisStream(ThreadType threadMode, AVFormatContext *pFormatContext);
    int getSampleRateForOpenSLES(int sampleRate);
    void pause();
    void resume();
    void stop();
    void release();
    void analysisStream(ThreadType threadMode,AVFormatContext *streams);
//    void callPlayerJniError(ThreadType threadMode,int code,char *msg);

private:
//    void FFmpegAudio::PlayAudioTack();

};


#endif //MYFFMPEGPLAYER_FFMPEGAUDIO_H
