//
// Created by freed on 2020/1/26.
//

#include "FFmpegVideo.h"


FFmpegVideo::FFmpegVideo(int streamIndex, FFMpegJniCall *pJniCall,
                         FFmpegPlayerStatus *pPlayerStatus, FFmpegAudio *pAudio):FFmpegMedia(streamIndex,pJniCall,pPlayerStatus) {
    this->pAudio=pAudio;

}

FFmpegVideo::~FFmpegVideo() {
    release();
}

void *threadVideoPlay(void *context) {
    FFmpegVideo *pVideo = (FFmpegVideo *) context;

    // 获取当前线程的 JNIEnv， 通过 JavaVM
    JNIEnv *env;
    if (pVideo->pJniCall->javaVM->AttachCurrentThread(&env, 0) != JNI_OK) {
        LOGE("get child thread jniEnv error!");
        return 0;
    }
    // 1. 获取窗体
    ANativeWindow *pNativeWindow = ANativeWindow_fromSurface(env, pVideo->surface);
    pVideo->pJniCall->javaVM->DetachCurrentThread();

    // 2. 设置缓存区的数据
    ANativeWindow_setBuffersGeometry(pNativeWindow, pVideo->pCodecContext->width,
                                     pVideo->pCodecContext->height, WINDOW_FORMAT_RGBA_8888);
    // Window 缓冲区的 Buffer
    ANativeWindow_Buffer outBuffer;

    AVPacket *pPacket = NULL;
    AVFrame *pFrame = av_frame_alloc();

    while (pVideo->pPlayerStatus != NULL && !pVideo->pPlayerStatus->isExit) {
        pPacket = pVideo->pPacketQueue->pop();
        // Packet 包，压缩的数据，解码成 pcm 数据
        int codecSendPacketRes = avcodec_send_packet(pVideo->pCodecContext, pPacket);
        if (codecSendPacketRes == 0) {
            int codecReceiveFrameRes = avcodec_receive_frame(pVideo->pCodecContext, pFrame);
            if (codecReceiveFrameRes == 0) {
                // 渲染，显示，OpenGLES (高效，硬件支持)，SurfaceView
                // 硬件加速和不加速有什么区别？cup 主要是用于计算，gpu 图像支持（硬件）
                // 这个 pFrame->data , 一般 yuv420P 的，RGBA8888，因此需要转换
                // 假设拿到了转换后的 RGBA 的 data 数据，如何渲染，把数据推到缓冲区
                sws_scale(pVideo->pSwsContext, (const uint8_t *const *) pFrame->data,
                          pFrame->linesize,
                          0, pVideo->pCodecContext->height, pVideo->pRgbaFrame->data,
                          pVideo->pRgbaFrame->linesize);

                // 在播放之前判断一下需要休眠多久
                double frameSleepTime = pVideo->getFrameSleepTime(pFrame);
                av_usleep(frameSleepTime * 1000000);

                // 把数据推到缓冲区
                ANativeWindow_lock(pNativeWindow, &outBuffer, NULL);
                memcpy(outBuffer.bits, pVideo->pFrameBuffer, pVideo->frameSize);
                ANativeWindow_unlockAndPost(pNativeWindow);
            }
        }
        // 解引用
        av_packet_unref(pPacket);
        av_frame_unref(pFrame);
    }
    // 1. 解引用数据 data ， 2. 销毁 pPacket 结构体内存  3. pPacket = NULL
    av_packet_free(&pPacket);
    av_frame_free(&pFrame);

    return 0;
}

void FFmpegVideo::play() {
    // 一个线程去解码播放
    pthread_t playThreadT;
    pthread_create(&playThreadT, NULL, threadVideoPlay, this);
    pthread_detach(playThreadT);
}

void FFmpegVideo::privateAnalysisStream(ThreadMode threadMode, AVFormatContext *pFormatContext) {
    // 3.初始化转换上下文
    pSwsContext = sws_getContext(pCodecContext->width, pCodecContext->height,
                                 pCodecContext->pix_fmt, pCodecContext->width, pCodecContext->height,
                                 AV_PIX_FMT_RGBA, SWS_BILINEAR, NULL, NULL, NULL);
    pRgbaFrame = av_frame_alloc();
    frameSize = av_image_get_buffer_size(AV_PIX_FMT_RGBA, pCodecContext->width,
                                         pCodecContext->height, 1);
    pFrameBuffer = (uint8_t *) malloc(frameSize);
    av_image_fill_arrays(pRgbaFrame->data, pRgbaFrame->linesize, pFrameBuffer, AV_PIX_FMT_RGBA,
                         pCodecContext->width, pCodecContext->height, 1);
    int num = pFormatContext->streams[streamIndex]->avg_frame_rate.num;
    int den = pFormatContext->streams[streamIndex]->avg_frame_rate.den;
    if (den != 0 && num != 0) {
        defaultDelayTime = 1.0f * den / num;
    }
}

void FFmpegVideo::release() {
    FFmpegMedia::release();
    if (pSwsContext != NULL) {
        sws_freeContext(pSwsContext);
        free(pSwsContext);
        pSwsContext = NULL;
    }

    if (pFrameBuffer != NULL) {
        free(pFrameBuffer);
        pFrameBuffer = NULL;
    }

    if (pRgbaFrame != NULL) {
        av_frame_free(&pRgbaFrame);
        pRgbaFrame = NULL;
    }
    // 大家要注意, pJniCall 需要在 DZFFmpeg 之后销毁
    if (pJniCall != NULL) {
        pJniCall->jniEnv->DeleteGlobalRef(surface);
    }
}

void FFmpegVideo::setSurface(jobject surface) {
    this->surface = pJniCall->jniEnv->NewGlobalRef(surface);
}
double FFmpegVideo::getFrameSleepTime(AVFrame *pFrame) {
    double times = av_frame_get_best_effort_timestamp(pFrame) * av_q2d(timeBase);// s
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