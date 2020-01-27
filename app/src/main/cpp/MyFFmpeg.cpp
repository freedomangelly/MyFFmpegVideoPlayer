//
// Created by freed on 2020/1/25.
//

#include "MyFFmpeg.h"
#include "MyConsts.h"

MyFFmpeg::MyFFmpeg(FFMpegJniCall *jniCall, const char *url) {
    this->jniCall=jniCall;
//    this->url=url;
    this->url= static_cast<char *>(malloc(strlen(url) + 1));
    LOGI("enter MyFFmpeg Play2 %s",url);
    memcpy((void *) this->url, url, strlen(url) + 1);

    pPlayerStatus=new FFmpegPlayerStatus();
}

MyFFmpeg::~MyFFmpeg() {
    release();
}

void *threadReadPacket(void *context) {
    MyFFmpeg *pFFmpeg = (MyFFmpeg *) context;
    while (pFFmpeg->pPlayerStatus != NULL && !pFFmpeg->pPlayerStatus->isExit) {
        AVPacket *pPacket = av_packet_alloc();
        if (av_read_frame(pFFmpeg->pFormatContext, pPacket) >= 0) {
            if (pPacket->stream_index == pFFmpeg->pAudio->streamIndex) {
                pFFmpeg->pAudio->pPacketQueue->push(pPacket);
            } else if (pPacket->stream_index == pFFmpeg->pVideo->streamIndex) {
                pFFmpeg->pVideo->pPacketQueue->push(pPacket);
            } else {
                // 1. 解引用数据 data ， 2. 销毁 pPacket 结构体内存  3. pPacket = NULL
                av_packet_free(&pPacket);
            }
        } else {
            // 1. 解引用数据 data ， 2. 销毁 pPacket 结构体内存  3. pPacket = NULL
            av_packet_free(&pPacket);
            // 睡眠一下，尽量不去消耗 cpu 的资源，也可以退出销毁这个线程
            // break;
        }
    }
    return 0;
}

//void *threadPlay(void *context){
//    MyFFmpeg *pFFmpeg= (MyFFmpeg *)(context);
//    pFFmpeg->prepare(THREAD_CHILD);
//    return 0;
//}

void MyFFmpeg::play() {
    //主线程
//    MyFFmpeg *pFFmpeg= (MyFFmpeg *)(this);
//    pFFmpeg->prepare();

//子线程
    pthread_t readPacketTreadT;
    pthread_create(&readPacketTreadT,NULL,threadReadPacket,this);
    pthread_detach(readPacketTreadT);
    LOGI("nPlay 2");
    if(pAudio!=NULL){
        LOGI("nPlay 3");
        pAudio->play();
    }

    if(pVideo!=NULL){
        LOGI("nPlay 4");
        pVideo->play();
    }

}

void MyFFmpeg::prepare(ThreadMode threadMode) {


    av_register_all();
    avformat_network_init();

    int formatOpenInputRes = 0;
    int formatFindStreamInfoRes = 0;
    LOGI("1111111111111111111111111");
    formatOpenInputRes = avformat_open_input(&pFormatContext, url, NULL, NULL);
    if (formatOpenInputRes != 0) {
        // 第一件事，需要回调给 Java 层(下次课讲)
        // 第二件事，需要释放资源
        LOGE("format open input error: %s == %d", av_err2str(formatOpenInputRes),formatOpenInputRes);
        LOGE("format open input error: url= %s", url);
        callPlayerJniError(threadMode,formatOpenInputRes, av_err2str(formatOpenInputRes));
        return;
    }
    LOGI("1111111111111111111111112");
    formatFindStreamInfoRes = avformat_find_stream_info(pFormatContext, NULL);
    if (formatFindStreamInfoRes < 0) {
        LOGE("format find stream info error: %s", av_err2str(formatFindStreamInfoRes));
        // 这种方式一般不推荐这么写，但是的确方便
        callPlayerJniError(threadMode,formatFindStreamInfoRes, av_err2str(formatFindStreamInfoRes));
        return;
    }
    LOGI("1111111111111111111111113");
    // 查找音频流的 index
    int audioStramIndex = av_find_best_stream(pFormatContext, AVMediaType::AVMEDIA_TYPE_AUDIO, -1, -1,
                                          NULL, 0);
    if (audioStramIndex < 0) {
        LOGE("format audio stream error: %s");
        // 这种方式一般不推荐这么写，但是的确方便
        callPlayerJniError(threadMode,FIND_STREAM_ERROR_CODE, "format audio stream error");
        return;
    }
    LOGI("1111111111111111111111114");

    pAudio=new FFmpegAudio(audioStramIndex,jniCall,pPlayerStatus);
    LOGI("1111111111111111111111115");
    pAudio->analysisStream(threadMode,pFormatContext);
    LOGI("1111111111111111111111116");
    int videoStreamIndex=av_find_best_stream(pFormatContext,AVMEDIA_TYPE_VIDEO,-1,-1,NULL,0);
    //如果没有音频就直接播放视频
    LOGI("1111111111111111111111117");
    if (videoStreamIndex < 0) {
        LOGE("format video stream error.");
        // 这种方式一般不推荐这么写，但是的确方便
        callPlayerJniError(threadMode, FIND_STREAM_ERROR_CODE, "find video stream error");
        return;
    }
    LOGI("1111111111111111111111118");
    //不是我的事我不干，但是搭建也不要想的过于复杂
    pVideo=new FFmpegVideo(videoStreamIndex,jniCall,pPlayerStatus,pAudio);
    LOGI("1111111111111111111111119");
    pVideo->analysisStream(threadMode,pFormatContext);

    LOGI("111111111111111111111111 10");
    jniCall->callPlayerPrepared(threadMode);
    LOGI("111111111111111111111111 11");

}
void *threadPrepare(void *context) {
    MyFFmpeg *pFFmpeg = (MyFFmpeg *) context;
    pFFmpeg->prepare(THREAD_CHILD);
    return 0;
}
void MyFFmpeg::prepareAsync() {
    pthread_t prepareThreadT;
    pthread_create(&prepareThreadT, NULL, threadPrepare, this);
    pthread_detach(prepareThreadT);
//    this->prepare(THREAD_MAIN);
}

void MyFFmpeg::callPlayerJniError(ThreadMode threadMode,int code, char *msg) {
    release();
    jniCall->callPlayerError(threadMode,code, msg);
}

void MyFFmpeg::release() {

    if(pFormatContext!=NULL){
        avformat_close_input(&pFormatContext);
        avformat_free_context(pFormatContext);
        pFormatContext=NULL;
    }

    avformat_network_deinit();
    if(url!=NULL){
        free((void *) url);
        url=NULL;
    }

    if(pPlayerStatus !=NULL){
        delete(pPlayerStatus);
        pPlayerStatus=NULL;
    }
    if(pAudio!=NULL){
        delete (pAudio);
        pAudio=NULL;
    }
    if(pVideo!=NULL){
        delete(pVideo);
        pVideo=NULL;
    }

}

void MyFFmpeg::prepare() {
    prepare(THREAD_MAIN);
}

void MyFFmpeg::setSurface(jobject surface) {
    if(pVideo!=NULL){
        pVideo->setSurface(surface);
    }

}


