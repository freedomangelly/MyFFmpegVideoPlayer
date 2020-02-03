//
// Created by freed on 2020/1/25.
//


#include "FFmpegAudio.h"
#include "MyConsts.h"
#include "libavcodec/avcodec.h"

FFmpegAudio::FFmpegAudio(int audioStreamIndex, FFMpegJniCall *pJniCall ,FFmpegPlayerStatus *pPlayerStatus)
            :FFmpegMedia(audioStreamIndex,pJniCall,pPlayerStatus){
}

FFmpegAudio::~FFmpegAudio() {
    release();
}

void FFmpegAudio::analysisStream(ThreadType threadMode, AVFormatContext *streams) {
    FFmpegMedia::analysisStream(threadMode,streams);
    //声道>0,并且声道布局未指定
if(pCodecContext->channels>0 && pCodecContext->channel_layout==0){
    //根据通道的layout返回通道的个数
    pCodecContext->channel_layout=av_get_default_channel_layout(pCodecContext->channels);
} else if(pCodecContext->channels ==0 && pCodecContext->channel_layout>0){
    //根据通道的layout返回通道的个数
    pCodecContext->channels=av_get_channel_layout_nb_channels(pCodecContext->channel_layout);
}
// 初始化 SwrContext
    /*struct SwrContext *s; 用返回值来接收了，直接传 NULL
    int64_t out_ch_layout; 输出的声道布局
    enum AVSampleFormat out_sample_fmt; 输出的采样位数
    int out_sample_rate; 输出的采样率
    int64_t in_ch_layout; 输入的声道布局
    enum AVSampleFormat in_sample_fmt; 输入的采样位数
    int in_sample_rate; 输入的采样率
    int log_offset;
    void *log_ctx;*/
    uint64_t out_ch_layout=AV_CH_LAYOUT_STEREO;
    enum AVSampleFormat out_sample_fmt=AV_SAMPLE_FMT_S16;
    int out_sample_rate=pCodecContext->sample_rate;
    uint64_t in_ch_layout=pCodecContext->channel_layout;
    enum AVSampleFormat  in_sample_fmt=pCodecContext->sample_fmt;
    int in_sample_rate=pCodecContext->sample_rate;
    /**
     * 参数1：重采样上下文

参数2：输出的layout, 如：5.1声道…

参数3：输出的样本格式。Float, S16, S24

参数4：输出的样本率。可以不变。

参数5：输入的layout。

参数6：输入的样本格式。

参数7：输入的样本率。

参数8，参数9，日志，不用管，可直接传0

针对音频的播放速度，可以通过样本率的改变而改变。
     */
    LOGI("analysisStream 444 %d %d %d %d %d %d ",out_ch_layout,out_sample_fmt,out_sample_rate,in_ch_layout,in_sample_fmt,in_sample_rate);
    swrContext = swr_alloc_set_opts(NULL,out_ch_layout,out_sample_fmt,out_sample_rate,in_ch_layout,in_sample_fmt,in_sample_rate,0,0);
    int swrInitRes=swr_init(swrContext);// 初始化上下文
    if(swrContext==NULL || swrInitRes<0){
        LOGE("init SwrContext error : %s %d", av_err2str(swrInitRes),swrInitRes);
        callPlayerJniError(threadMode,swrInitRes,av_err2str(swrInitRes));
        return;
    }
    frameBufferSize=pCodecContext->frame_size *2 *2;
    convertOutBuffer= static_cast<uint8_t *>(malloc(frameBufferSize));
}

void *threadDecodePlay(void *context){
    FFmpegAudio *audio= static_cast<FFmpegAudio *>(context);
    LOGE("audio -> %p", audio);
    audio->initCreateOpenSLES();
    pthread_exit((void *)1);
}

int FFmpegAudio::resampleAudio() {
//    int dataSize=0;
    AVPacket *pPacket=av_packet_alloc();
    AVFrame *pFrame=av_frame_alloc();
    while (pPlayerStatus!=NULL && !pPlayerStatus->isExit){
        //是不是暂停或者seek中
        if(pPlayerStatus !=NULL) {
            if (pPlayerStatus->isPause || pPlayerStatus->isSeek) {
                av_usleep(10 * 1000);
                continue;
            }
        }
        if(pPacketQueue!=NULL && pPacketQueue->empty()){
            if(pPlayerStatus !=NULL && pPlayerStatus->isLoading!=true){
                pPlayerStatus->isLoading=true;
                if(pJniCall!=NULL){
                    pJniCall->onCallLoading(THREAD_CHILD,pPlayerStatus->isLoading);
                }
            }
            continue;
        } else{
            if(pPlayerStatus!=NULL && pPlayerStatus->isLoading == true){}
            pPlayerStatus->isLoading=false;
            if(pJniCall!=NULL){
                pJniCall->onCallLoading(THREAD_CHILD,pPlayerStatus->isLoading);
            }
        }
        pthread_mutex_lock(&seekMutex);
       pPacketQueue->pop(pPacket);
        //pPacket 包，压缩的数据，解码成pcm数据
        int send_packet_res=avcodec_send_packet(pCodecContext,pPacket);
        if(send_packet_res==0){
            int receiver_frame_res=avcodec_receive_frame(pCodecContext,pFrame);
            pthread_mutex_unlock(&seekMutex);
            if(receiver_frame_res==0){

                //调用重采样的方法 返回值是返回重采样的个数，也就是 pFrame->nb_samples
                swr_convert(swrContext, &convertOutBuffer, pFrame->nb_samples,
                                     (const uint8_t **) (pFrame->data), pFrame->nb_samples);
                //dataSize帧数不够

//                dataSize=dataSize *2 *2; //*每个点占2位*每个点两通道
//                LOGI("解码音频帧：%d,%d",dataSize,pFrame->nb_samples);
                // write 写到缓冲区 pFrame.data -> javabyte
                // size 是多大，装 pcm 的数据
                // 1s 44100 点  2通道 ，2字节    44100*2*2
                // 1帧不是一秒，pFrame->nb_samples点

                //设置当前的时间，方便回调进度给java,方便视频同步音频
//                double times=av_frame_get_best_effort_timestamp(pFrame) * av_q2d(timeBase);//s
                int times=pFrame->pts * av_q2d(timeBase);//s
                if(times > currentTime){
                    currentTime=times;
                }

                break;
            }
        } else{
            pthread_mutex_unlock(&seekMutex);
        }
        av_packet_unref(pPacket);
        av_frame_unref(pFrame);
    }
    av_packet_free(&pPacket);
    av_frame_free(&pFrame);
    return 0;
}


void FFmpegAudio::play() {
//    //一个线程读取
//    pthread_t readPacketTheadT;
//    pthread_create(&readPacketTheadT,NULL,threadReadPacket,this);
//    pthread_detach(readPacketTheadT);

    //一个线程播放
//    pthread_t  playThreadT;
    pthread_create(&playThreadT, NULL, threadDecodePlay, this);
//    pthread_detach(playThreadT);
}

void bufferQueueCallback(SLAndroidSimpleBufferQueueItf bufferQueueItf, void *context) {
    FFmpegAudio *audio = (FFmpegAudio *) context;
    if (audio != NULL) {
        audio->resampleAudio();
        audio->currentTime +=
                audio->frameBufferSize / ((double) (audio->pCodecContext->sample_rate * 2 * 2));
        // 0.5 回调更新一次进度
        if (audio->currentTime - audio->lastUpdateTime > 1) {
            audio->lastUpdateTime = audio->currentTime;
            if (audio->pJniCall != NULL) {
                audio->pJniCall->onCallProgress(THREAD_CHILD, audio->currentTime,
                                                   audio->duration);
            }
        }

        if (audio->duration > 0 && audio->duration <= audio->currentTime) {
            audio->pJniCall->onCallComplete(THREAD_CHILD);
        }
        (*bufferQueueItf)->Enqueue(bufferQueueItf, (char *) audio->convertOutBuffer,
                                   audio->frameBufferSize);
    }
}

int FFmpegAudio::getSampleRateForOpenSLES(int sampleRate) {
    return sampleRate * 1000;
}

void bufferQueueItf(SLAndroidSimpleBufferQueueItf caller, void *pContext);

void FFmpegAudio::initCreateOpenSLES() {
    LOGI("initCreateOpenSLES111111 start");
    /*OpenSLES OpenGLES 都是自带的
    XXXES 与 XXX 之间可以说是基本没有区别，区别就是 XXXES 是 XXX 的精简
    而且他们都有一定规则，命名规则 slXXX() , glXXX3f*/
    // 3.1 创建引擎接口对象
//    SLObjectItf engineObject = NULL;
    SLEngineItf engine;

    SLEnvironmentalReverbItf  outputMixEnvitonmentalReverb;
    SLEnvironmentalReverbSettings reverbSettings=SL_I3DL2_ENVIRONMENT_PRESET_STONECORRIDOR;

    const SLInterfaceID ids[1]={SL_IID_ENVIRONMENTALREVERB};
    const SLboolean interfaceRequired[1]={SL_BOOLEAN_FALSE};
    LOGI("initCreateOpenSLES111112 start");
    slCreateEngine(&engineObj, 0, 0, 0, 0, 0);
    LOGI("initCreateOpenSLES111113 start");
    // realize the engine
    (*engineObj)->Realize(engineObj, SL_BOOLEAN_FALSE);
    // get the engine interface, which is needed in order to create other objects
    (*engineObj)->GetInterface(engineObj, SL_IID_ENGINE, &engine);
    // 3.2 设置混音器
    (*engine)->CreateOutputMix(engine, &mixObject, 1, ids, interfaceRequired);
    (*mixObject)->Realize(mixObject, SL_BOOLEAN_FALSE);
    (*mixObject)->GetInterface(mixObject, SL_IID_ENVIRONMENTALREVERB,&outputMixEnvitonmentalReverb);
    (*outputMixEnvitonmentalReverb)->SetEnvironmentalReverbProperties(
	outputMixEnvitonmentalReverb,&reverbSettings);
    // 3.3 创建播放器
    SLDataLocator_AndroidBufferQueue androidBufferQueue = {
            SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 2};
    LOGI("initCreateOpenSLES111114 start");
    SLDataFormat_PCM formatPcm = {
            SL_DATAFORMAT_PCM,
            2,
            static_cast<SLuint32>(getSampleRateForOpenSLES(pCodecContext->sample_rate)),
            SL_PCMSAMPLEFORMAT_FIXED_16,
            SL_PCMSAMPLEFORMAT_FIXED_16,
            SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT,
            SL_BYTEORDER_LITTLEENDIAN
    };
    LOGI("initCreateOpenSLES111115 start");
    SLDataSource audioSrc = {&androidBufferQueue, &formatPcm};
    SLDataLocator_OutputMix outputMix = {SL_DATALOCATOR_OUTPUTMIX, mixObject};
    SLDataSink audioSnk = {&outputMix, NULL};
    const SLInterfaceID interfaceIds[3] = {SL_IID_BUFFERQUEUE, SL_IID_VOLUME, SL_IID_PLAYBACKRATE};
    const SLboolean pInterfaceIds[3] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE};
    LOGI("initCreateOpenSLES111116 start");
    (*engine)->CreateAudioPlayer(engine, &pPlayer, &audioSrc, &audioSnk, 3,
                                       interfaceIds, pInterfaceIds);
    LOGI("initCreateOpenSLES111117 start");
    (*pPlayer)->Realize(pPlayer, SL_BOOLEAN_FALSE);
    (*pPlayer)->GetInterface(pPlayer, SL_IID_PLAY, &slPlayItf);
    LOGI("initCreateOpenSLES111118 start");
    // 3.4 设置缓存队列和回调函数
//    SLAndroidSimpleBufferQueueItf playerBufferQueue;
    (*pPlayer)->GetInterface(pPlayer, SL_IID_BUFFERQUEUE, &androidBufferQueueItf);
    (*androidBufferQueueItf)->RegisterCallback(androidBufferQueueItf, bufferQueueCallback,
                                                     this);
    LOGI("initCreateOpenSLES111119 start");
    // 3.5 设置播放状态
    (*slPlayItf)->SetPlayState(slPlayItf, SL_PLAYSTATE_PLAYING);
    LOGI("initCreateOpenSLES1111110 start");
    // 3.6 调用回调函数
    bufferQueueCallback(androidBufferQueueItf, this);
    LOGI("initCreateOpenSLES111111 end");
}

void FFmpegAudio::pause() {
    if (slPlayItf != NULL) {
        (*slPlayItf)->SetPlayState(slPlayItf, SL_PLAYSTATE_PAUSED);
    }
}

void FFmpegAudio::resume() {
    if (slPlayItf != NULL) {
        (*slPlayItf)->SetPlayState(slPlayItf, SL_PLAYSTATE_PLAYING);
    }
}

void FFmpegAudio::stop() {
    if (slPlayItf != NULL) {
        (*slPlayItf)->SetPlayState(slPlayItf, SL_PLAYSTATE_STOPPED);
    }
}

void FFmpegAudio::release() {
    FFmpegMedia::release();
    stop();
    if (pPlayer != NULL) {
        (*pPlayer)->Destroy(pPlayer);
        pPlayer = NULL;
    }

    if (mixObject != NULL) {
        (*mixObject)->Destroy(mixObject);
        mixObject = NULL;
    }

    if (engineObj != NULL) {
        (*engineObj)->Destroy(engineObj);
        engineObj = NULL;
    }

    free(convertOutBuffer);
    if (swrContext != NULL) {
        swr_free(&swrContext);
        swrContext = NULL;
    }
}


void FFmpegAudio::privateAnalysisStream(ThreadType threadMode, AVFormatContext *pFormatContext){
    int64_t out_ch_layout=AV_CH_LAYOUT_STEREO;
    enum AVSampleFormat out_sample_fmt=AVSampleFormat ::AV_SAMPLE_FMT_S16;
    int out_sample_rate = AUDIO_SAMPLE_RATE;
    int64_t in_ch_layout = pCodecContext->channel_layout;
    enum AVSampleFormat in_sample_fmt = pCodecContext->sample_fmt;
    int in_sample_rate = pCodecContext->sample_rate;
    swrContext = swr_alloc_set_opts(NULL, out_ch_layout, out_sample_fmt,
                                     out_sample_rate, in_ch_layout, in_sample_fmt, in_sample_rate, 0, NULL);
    if (swrContext == NULL) {
        // 提示错误
        callPlayerJniError(threadMode, SWR_ALLOC_SET_OPTS_ERROR_CODE, "swr alloc set opts error");
        return;
    }
    int swrInitRes = swr_init(swrContext);
    if (swrInitRes < 0) {
        callPlayerJniError(threadMode, SWR_CONTEXT_INIT_ERROR_CODE, "swr context swr init error");
        return;
    }

    convertOutBuffer = (uint8_t *) malloc(pCodecContext->frame_size * 2 * 2);
    pJniCall->callMusicInfo(threadMode,AUDIO_SAMPLE_RATE,2);
}

void bufferQueueItf(SLAndroidSimpleBufferQueueItf caller, void *pContext) {
    FFmpegAudio *pFFmepg = (FFmpegAudio *) pContext;
    int dataSize = pFFmepg->resampleAudio();
    pFFmepg->pJniCall->callCallbackPcm(THREAD_CHILD,pFFmepg->convertOutBuffer,dataSize);
    // 这里为什么报错，留在后面再去解决
    (*caller)->Enqueue(caller, pFFmepg->convertOutBuffer, dataSize);
}





