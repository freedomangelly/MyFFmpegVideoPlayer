//
// Created by freed on 2020/1/26.
//

#include "FFmpegVideo.h"


FFmpegVideo::FFmpegVideo(int streamIndex, FFMpegJniCall *pJniCall,FFmpegPlayerStatus *pPlayerStatus,
                         FFmpegAudio *pAudio):FFmpegMedia(streamIndex,pJniCall,pPlayerStatus) {
    this->pAudio=pAudio;

}

FFmpegVideo::~FFmpegVideo() {
    release();
}

void *threadVideoPlay(void *context) {
    FFmpegVideo *pVideo = (FFmpegVideo *) context;
    AVPacket *pPacket = av_packet_alloc();
    AVFrame *pFrame = av_frame_alloc();

    while (pVideo->pPlayerStatus != NULL && !pVideo->pPlayerStatus->isExit) {
        if(pVideo->pPlayerStatus!=NULL){
            if(pVideo->pPlayerStatus->isPause || pVideo->pPlayerStatus->isSeek){
                av_usleep(10 * 1000);
                continue;
            }
        }
        // 加锁，防止 seek 时获取到脏数据
        pthread_mutex_lock(&pVideo->seekMutex);
        pVideo->pPacketQueue->pop(pPacket);;
        // 是否支持硬解码
        if(pVideo->supportStiffCodec){
            int bsfSendPacketRes=av_bsf_send_packet(pVideo->pBSFContext,pPacket);
            pthread_mutex_unlock(&pVideo->seekMutex);
            if(bsfSendPacketRes==0){
                //开始解码
                while (av_bsf_receive_packet(pVideo->pBSFContext,pPacket)==0){
                    double sleepTime=pVideo->getFrameSleepTime(pPacket->pts);
                    av_usleep(sleepTime*1000000);
                    if(pVideo->pPlayerStatus->isExit){
                        break;
                    }
                    LOGI("onCallDecodePacket");
                    pVideo->pJniCall->onCallDecodePacket(pPacket->size,pPacket->data);
                    av_packet_unref(pPacket);
                }
            }
        } else {
            // Packet 包，压缩的数据，解码成 pcm 数据
            int send_packet_res = avcodec_send_packet(pVideo->pCodecContext, pPacket);
            if (send_packet_res == 0) {
                int receive_frame_res = avcodec_receive_frame(pVideo->pCodecContext, pFrame);
                pthread_mutex_unlock(&pVideo->seekMutex);
                if (receive_frame_res == 0) {
                    double sleepTime = pVideo->getFrameSleepTime(pFrame->pts);
                    av_usleep(sleepTime * 1000000);
                    if(pVideo->pCodecContext->pix_fmt == AV_PIX_FMT_YUV420P){
                        // 不需要转可以直用 OpenGLES 去渲染
                        pVideo->pJniCall->onCallRenderYUV420P(pVideo->pCodecContext->width,
                                pVideo->pCodecContext->height,
                                pFrame->data[0],
                                pFrame->data[1],
                                pFrame->data[2]
                                );
                    } else {
                        // 需要转换
                        sws_scale(pVideo->pSwsContext, pFrame->data,
                                  pFrame->linesize,
                                  0, pFrame->height, pVideo->pFrameYUV420p->data,
                                  pVideo->pFrameYUV420p->linesize);
                        // OpenGLES 去渲染
                        pVideo->pJniCall->onCallRenderYUV420P(pVideo->pCodecContext->width,pVideo->pCodecContext->height,
                                pVideo->pFrameYUV420p->data[0],
                                pVideo->pFrameYUV420p->data[1],
                                pVideo->pFrameYUV420p->data[2]
                                );
                    }
                }
            } else{
                pthread_mutex_unlock(&pVideo->seekMutex);
            }
            av_frame_unref(pFrame);
        }

        // 释放 data 数据，释放 AVPacket 开辟的内存
        av_packet_unref(pPacket);
    }
    // 1. 解引用数据 data ， 2. 销毁 pPacket 结构体内存  3. pPacket = NULL
    av_packet_free(&pPacket);
    av_frame_free(&pFrame);
    LOGI("threadVideoPlay end 1111");
    return 0;
}

void FFmpegVideo::play() {
    // 一个线程去解码播放
    pthread_t playThreadT;
    pthread_create(&playThreadT, NULL, threadVideoPlay, this);
}
void FFmpegVideo::release() {
    FFmpegMedia::release();
    if (pFrameYUV420p != NULL) {
        av_frame_free(&pFrameYUV420p);
        pFrameYUV420p = NULL;
    }
    if (pFrameBuffer != NULL) {
        av_free(pFrameBuffer);
        pFrameBuffer = NULL;
    }

    if (pSwsContext != NULL) {
        sws_freeContext(pSwsContext);
        free(pSwsContext);
        pSwsContext = NULL;
    }

    if(pBSFContext!=NULL){
        av_bsf_free(&pBSFContext);
        av_free(pBSFContext);
        pBSFContext=NULL;
    }

}

void FFmpegVideo::analysisStream(ThreadType threadMode, AVFormatContext *pFormatContext) {
    FFmpegMedia::analysisStream(threadMode, pFormatContext);
    //初始化AVFrame帧
    pFrameYUV420p = av_frame_alloc();
    //计算一针的大小
    int frameSize = av_image_get_buffer_size(AV_PIX_FMT_YUV420P, pCodecContext->width,
                                         pCodecContext->height, 1);
    //计算一针的大小
    pFrameBuffer = (uint8_t *) malloc(frameSize);
    //填充AVFrame数据缓冲
    av_image_fill_arrays(pFrameYUV420p->data, pFrameYUV420p->linesize, pFrameBuffer, AV_PIX_FMT_YUV420P,
                         pCodecContext->width, pCodecContext->height, 1);
    // 3.初始化转换上下文
    pSwsContext = sws_getContext(pCodecContext->width, pCodecContext->height,
                                 pCodecContext->pix_fmt, pCodecContext->width, pCodecContext->height,
                                 AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);
    if(pSwsContext == NULL){
        callPlayerJniError(threadMode,SWS_GET_CONTEXT_ERROR_CODE,"sws get context error.");
    }
    //获取时间比
    int num = pFormatContext->streams[streamIndex]->avg_frame_rate.num;
    int den = pFormatContext->streams[streamIndex]->avg_frame_rate.den;
    if (den != 0 && num != 0) {
        defaultDelayTime = 1.0 * den / num;
    }

    const char *codecName=pCodecContext->codec->name;
    //判断是否支持硬解码
    supportStiffCodec = pJniCall->onCallIsSupportStiffCodec(threadMode,codecName);

    if(supportStiffCodec){
        //支持硬解码
       //1找到相应解码器的过滤器
        const AVBitStreamFilter *pBSFilter=NULL;
        if(strcasecmp("h264",codecName)==0){
            pBSFilter=av_bsf_get_by_name("h264_mp4toannexb");
        } else if (strcasecmp("h265", codecName) == 0) {
            pBSFilter = av_bsf_get_by_name("hevc_mp4toannexb");
        }
        if(pBSFilter == NULL){
            supportStiffCodec = false;
            return;
        }
        //2. 初始化过滤器上下文
        int bsfAllocRes=av_bsf_alloc(pBSFilter,&pBSFContext);
        if(bsfAllocRes !=0){
            supportStiffCodec=false;
            return;
        }
        //3. 添加解码器属性
        AVCodecParameters *pCodecParameters = pFormatContext->streams[streamIndex]->codecpar;
        int codecParametersCopyRes = avcodec_parameters_copy(pBSFContext->par_in, pCodecParameters);
        if (codecParametersCopyRes < 0) {
            supportStiffCodec = false;
            return;
        }
        //4.过滤器分配内存
        int bsfInitRes = av_bsf_init(pBSFContext);
        if (bsfInitRes != 0) {
            supportStiffCodec = false;
            return;
        }
        // 调用 java 层初始化 MediaCodec
        pJniCall->onCallInitMediaCodec(threadMode, codecName, pCodecContext->width,
                                          pCodecContext->height, pCodecContext->extradata_size, pCodecContext->extradata_size,
                                          pCodecContext->extradata, pCodecContext->extradata);
        pBSFContext->time_base_in = timeBase;
    }
}



double FFmpegVideo::getFrameSleepTime(int64_t pts) {
    double times = pts * av_q2d(timeBase);// s
    if (times > currentTime) {
        currentTime = times;
    }
    // 相差多少秒
    double diffTime = pAudio->currentTime - currentTime;

    // 视频快了就慢一点，视频慢了就快一点
    // 但是尽量把时间控制在视频的帧率时间范围左右  1/24  0.04  1/30  0.033
    // 第一次控制 0.016s 到 -0.016s
    if (diffTime > 0.016 || diffTime < -0.016) {
        if (diffTime > 0.016) {
            delayTime = delayTime * 2 / 3;
        } else if (diffTime < -0.016) {
            delayTime = delayTime * 3 / 2;
        }
        // 第二次控制 defaultDelayTime * 2 / 3 到 defaultDelayTime * 3 / 2
        if (delayTime < defaultDelayTime / 2) {
            delayTime = defaultDelayTime * 2 / 3;
        } else if (delayTime > defaultDelayTime * 2) {
            delayTime = defaultDelayTime * 3 / 2;
        }
    }

    // 第三次控制，那这基本是异常情况
    if (diffTime >= 0.25) {
        delayTime = 0;
    } else if (diffTime <= -0.25) {
        delayTime = defaultDelayTime * 2;
    }

    return delayTime;
}