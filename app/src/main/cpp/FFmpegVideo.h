//
// Created by freed on 2020/1/26.
//

#ifndef MYFFMPEGPLAYER_FFMPEGVIDEO_H
#define MYFFMPEGPLAYER_FFMPEGVIDEO_H


#include "FFmpegMedia.h"
#include "FFmpegAudio.h"
#include <android/native_window.h>
#include <android/native_window_jni.h>

extern "C" {
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libavutil/time.h>
};
class FFmpegVideo : public FFmpegMedia {
public:
    AVFrame *pFrameYUV420p=NULL;
    uint8_t *pFrameBuffer = NULL;
    SwsContext *pSwsContext = NULL;
    FFmpegAudio *pAudio;
    /**
     * 视频的延时时间
     */
    double delayTime = 0;

    /**
     * 默认情况下最合适的一个延迟时间，动态获取
     */
    double defaultDelayTime = 0.04;
    bool  supportStiffCodec=false;
    AVBSFContext *pBSFContext;
public:
    FFmpegVideo(int streamIndex, FFMpegJniCall *pJniCall, FFmpegPlayerStatus *pPlayerStatus, FFmpegAudio *pAudio);

    ~FFmpegVideo();

public:
    void play();

    virtual void analysisStream(ThreadType threadMode, AVFormatContext *pFormatContext);

    void release();

    /**
     * 视频同步音频，计算获取休眠的时间
     * @param pFrame 当前视频帧
     * @return 休眠时间（s）
     */
    double getFrameSleepTime(int64_t pts);
};


#endif //MYFFMPEGPLAYER_FFMPEGVIDEO_H
