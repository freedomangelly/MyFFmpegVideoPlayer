//
// Created by freed on 2020/1/25.
//

#include "MyFFmpeg.h"
#include "MyConsts.h"

MyFFmpeg::MyFFmpeg(FFMpegJniCall *jniCall, const char *url) {
    this->jniCall=jniCall;
    this->url= static_cast<char *>(malloc(strlen(url) + 1));
    LOGI("enter MyFFmpeg Play2 %s",url);
    memcpy((void *) this->url, url, strlen(url) + 1);
    pthread_mutex_init(&releaseMutex,NULL);
    pPlayerStatus=new FFmpegPlayerStatus();
}

void *decodeAudioThread(void *data){
    MyFFmpeg *fFmpeg= static_cast<MyFFmpeg *>(data);
    fFmpeg->preparedAudio(THREAD_CHILD);
    pthread_exit(0);
}

void MyFFmpeg::prepare() {
    preparedAudio(THREAD_MAIN);
}

void MyFFmpeg::prepareAsync() {
    pthread_create(&preparedThread, NULL, decodeAudioThread, this);
}

void MyFFmpeg::preparedAudio(ThreadType thread_type) {
    pthread_mutex_lock(&releaseMutex);

    /** av_register_all: 作用是初始化所有组件，只有调用了该函数，才能使用复用器和编解码器（源码）*/
    av_register_all();
    /**网络功能的全局初始化（可选的，在使用网络协议时有必要调用）*/
    avformat_network_init();
    int format_open_res = 0;
    int format_find_stream_info_res = 0;
    int audio_stream_index = 0;
    /**函数会读文件头，对 mp4 文件而言，它会解析所有的 box。但它只是把读到的结果保存在对应的数据结构下。这个时候，AVStream 中的很多字段都是空白的。*/
    format_open_res = avformat_open_input(&av_format_context, url, NULL, NULL);
    if (format_open_res < 0) {
        LOGE("Can't open url : %s, %s", url, av_err2str(format_open_res));
        releasePreparedRes(thread_type, format_open_res, av_err2str(format_open_res));
        return;
    }
    /**读取一部分视音频数据并且获得一些相关的信息，会检测一些重要字段，如果是空白的，就设法填充它们。
     * 因为我们解析文件头的时候，已经掌握了大量的信息，avformat_find_stream_info 就是通过这些信息来填充自己的成员，当重要的成员都填充完毕后，
     * 该函数就返回了。这中情况下，该函数效率很高。但对于某些文件，单纯的从文件头中获取信息是不够的，比如 video 的 pix_fmt
     * 是需要调用 h264_decode_frame 才可以获取其pix_fmt的。*/
    format_find_stream_info_res = avformat_find_stream_info(av_format_context, NULL);
    if (format_find_stream_info_res < 0) {
        LOGE("Can't find stream info url : %s, %s", url, av_err2str(format_find_stream_info_res));
        releasePreparedRes(thread_type, format_find_stream_info_res,
                           av_err2str(format_find_stream_info_res));
        return;
    }
    /**获取音视频及字幕的 stream_index , 以前没有这个函数时，我们一般都是写的 for 循环。
     * 此方法为音频解析方法
     */
    audio_stream_index = av_find_best_stream(av_format_context, AVMEDIA_TYPE_AUDIO, -1, -1, NULL,
                                             0);
    if (audio_stream_index < 0) {
        LOGE("Can't find audio stream info url : %s", url);
        releasePreparedRes(thread_type, FIND_AUDIO_STREAM_ERROR_CODE,
                           "Can't find audio stream info url.");
        return;
    }
    audio = new FFmpegAudio(audio_stream_index, jniCall,pPlayerStatus);
    audio->analysisStream(thread_type, av_format_context);
    /**获取音视频及字幕的 stream_index , 以前没有这个函数时，我们一般都是写的 for 循环。
    * 此方法为视频解析方法
    */
    int videoStreamIndex = av_find_best_stream(av_format_context, AVMEDIA_TYPE_VIDEO, -1, -1, NULL,
                                               0);
    if (videoStreamIndex < 0) {
        LOGE("Can't find video stream info url : %s", url);
        releasePreparedRes(thread_type, FIND_VIDEO_STREAM_ERROR_CODE,
                           "Can't find video stream info url.");
        return;
    }
    pVideo = new FFmpegVideo(videoStreamIndex, jniCall,pPlayerStatus,  audio);
    pVideo->analysisStream(thread_type, av_format_context);
//    jniCall->onCallPrepared(thread_type);
    LOGI("preparedAudio onCallPlayerPrepared");
    jniCall->onCallPlayerPrepared(thread_type);
    pthread_mutex_unlock(&releaseMutex);
}

MyFFmpeg::~MyFFmpeg() {
//    release();
}

/**
 * 解码帧
 * @param context
 * @return
 */
void *threadDecodeFrame(void *context) {
    MyFFmpeg *pFFmpeg = (MyFFmpeg *) context;
    int readFrameRes=0;
    while (pFFmpeg->pPlayerStatus != NULL && !pFFmpeg->pPlayerStatus->isExit) {
        AVPacket *pPacket = av_packet_alloc();//初始化帧
        readFrameRes=av_read_frame(pFFmpeg->av_format_context,pPacket);//读取帧
        if (readFrameRes>=0) {
//            LOGI("readFrameRes = %d",readFrameRes);
            if (pFFmpeg->audio->streamIndex == pPacket->stream_index) {
//                LOGI("readFrameRes push audio= %d",pPacket->stream_index);
                pFFmpeg->audio->pPacketQueue->push(pPacket);
            } else if (pFFmpeg->pVideo->streamIndex == pPacket->stream_index) {
//                LOGI("readFrameRes push pVideo= %d",pPacket->stream_index);
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
    pthread_exit((void *)1);
}

//void *threadPlay(void *context){
//    MyFFmpeg *pFFmpeg= (MyFFmpeg *)(context);
//    pFFmpeg->prepare(THREAD_CHILD);
//    return 0;
//}

void MyFFmpeg::start() {

    if (audio == NULL) {
        LOGE("DZAudio is null , prepared may be misleading");
        return;
    }
    if (pVideo == NULL) {
        LOGE("DZVideo is null , prepared may be misleading");
        return;
    }
    this->decodeFrame();
    audio->play();
    pVideo->play();
}

void MyFFmpeg::onPause() {
    if (audio != NULL) {
        audio->pause();
    }

    if (pPlayerStatus != NULL) {
        pPlayerStatus->isPause = true;
    }
}

void MyFFmpeg::onResume() {
    if (audio != NULL) {
        audio->resume();
    }

    if (pPlayerStatus != NULL) {
        pPlayerStatus->isPause = false;
    }
}

void MyFFmpeg::stop() {
    if (pPlayerStatus != NULL) {
        pPlayerStatus->isExit = true;
    }
    if (audio != NULL) {
        audio->stop();
    }


}

void MyFFmpeg::release() {
    pthread_mutex_lock(&releaseMutex);

    if (audio->pPlayerStatus->isExit) {
        return;
    }

    audio->pPlayerStatus->isExit = true;

    if (audio != NULL) {
        audio->release();
        delete audio;
        audio = NULL;
    }

    if (pVideo != NULL) {
        pVideo->release();
        delete pVideo;
        pVideo = NULL;
    }

    if (av_format_context != NULL) {
        avformat_close_input(&av_format_context);
        avformat_free_context(av_format_context);
        av_format_context = NULL;
    }

    jniCall = NULL;
    free(url);

    pthread_mutex_unlock(&releaseMutex);
    pthread_mutex_destroy(&releaseMutex);

}

void MyFFmpeg::releasePreparedRes(ThreadType threadType, int errorCode, const char *errorMsg) {
    pthread_mutex_unlock(&releaseMutex);
    if (av_format_context != NULL) {
        avformat_close_input(&av_format_context);
        avformat_free_context(av_format_context);
        av_format_context = NULL;
    }

    if (jniCall != NULL) {
        jniCall->onCallPlayerError(threadType, errorCode, errorMsg);
    }

    pthread_mutex_destroy(&releaseMutex);
    free(url);
}

void MyFFmpeg::seek(uint64_t seconds) {
    LOGI("seek  111111111");
    if (pPlayerStatus != NULL) {
        pPlayerStatus->isSeek = true;
    }
    LOGI("seek  1111111112");
    if (seconds >= 0) {
        int64_t rel = seconds * AV_TIME_BASE;
        av_seek_frame(av_format_context, -1, rel, AVSEEK_FLAG_BACKWARD);
    }
    LOGI("seek  1111111113");
    if (pVideo != NULL) {
        pVideo->seek(seconds);
    }
    LOGI("seek  1111111114");
    if (audio != NULL) {
        audio->seek(seconds);
    }
    LOGI("seek  1111111115");
    if (pPlayerStatus != NULL) {
        pPlayerStatus->isSeek = false;
    }
}

void MyFFmpeg::decodeFrame() {
    pthread_t decodeFrameThreadT;
    pthread_create(&decodeFrameThreadT, NULL, threadDecodeFrame, this);
}




