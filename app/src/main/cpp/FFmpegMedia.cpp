//
// Created by freed on 2020/1/26.
//

#include "FFmpegMedia.h"

FFmpegMedia::FFmpegMedia(int streamIndex, FFMpegJniCall *pJniCall,
                         FFmpegPlayerStatus *pPlayerStatus) {
    this->streamIndex = streamIndex;
    this->pJniCall = pJniCall;
    this->pPlayerStatus = pPlayerStatus;
    pPacketQueue = new FFmpegPacketQueue(pPlayerStatus);
    pthread_mutex_init(&seekMutex,NULL);
}

FFmpegMedia::~FFmpegMedia() {
    release();
}

void FFmpegMedia::analysisStream(ThreadType threadMode, AVFormatContext *pFormatContext) {
//    publicAnalysisStream(threadMode,pFormatContext);
//    privateAnalysisStream(threadMode,pFormatContext);
    //根据AVFromatContext取出媒体流，再从媒体流中取出AVCodecParameters
    AVCodecParameters *pCodecParameters=pFormatContext->streams[streamIndex]->codecpar;
    //查找解码器,返回解码器的ID
    AVCodec *pCodec=avcodec_find_decoder(pCodecParameters->codec_id);
    if(!pCodec){
        LOGE("Can't find audio decoder");
        callPlayerJniError(threadMode,FIND_STREAM_ERROR_CODE,"Can't find audio decoder.");
    }
    /**创建AVCodecContext 根据解码器*/
    pCodecContext = avcodec_alloc_context3(pCodec);
    //检测是否创建成功
    int codecContextParametersRes=avcodec_parameters_to_context(pCodecContext,pCodecParameters);
    if(codecContextParametersRes<0){
        LOGE("codec parameters to_context error ;%s",av_err2str(codecContextParametersRes));
        callPlayerJniError(threadMode,codecContextParametersRes,
		av_err2str(codecContextParametersRes));
        return;
    }
    //打开解码器
    int codecOpenRes=avcodec_open2(pCodecContext,pCodec,NULL);
    if(codecOpenRes<0){
        LOGE("codec open error: %s",av_err2str(codecOpenRes));
        callPlayerJniError(threadMode,codecOpenRes,av_err2str(codecOpenRes));
        return;
    }
    duration=pFormatContext->duration/AV_TIME_BASE;
    timeBase=pFormatContext->streams[streamIndex]->time_base;


}

void FFmpegMedia::release() {
    if (pCodecContext != NULL) {
        avcodec_close(pCodecContext);
        avcodec_free_context(&pCodecContext);
        pCodecContext = NULL;
    }
    if (pPacketQueue) {
        delete (pPacketQueue);
        pPacketQueue = NULL;
    }
    pthread_mutex_destroy(&seekMutex);
}

void FFmpegMedia::callPlayerJniError(ThreadType threadMode, int code, char *msg) {
    release();
    if(pJniCall!=NULL){
        pJniCall->onCallPlayerError(threadMode, code, msg);
    }
}

void FFmpegMedia::seek(uint64_t seconds) {
    if(duration<=0){
        return;
    }
    pthread_mutex_lock(&seekMutex);
    if(seconds>=0 && seconds<duration){
        pPacketQueue->clear();
        lastUpdateTime=0;
        currentTime=0;
        avcodec_flush_buffers(pCodecContext);
    }
    pthread_mutex_unlock(&seekMutex);

}

