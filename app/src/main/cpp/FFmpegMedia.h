//
// Created by freed on 2020/1/26.
//

#ifndef MYFFMPEGPLAYER_FFMPEGMEDIA_H
#define MYFFMPEGPLAYER_FFMPEGMEDIA_H

#include "FFMpegJniCall.h"
#include "FFmpegPacketQueue.h"
#include "FFmpegPlayerStatus.h"
#include "MyConsts.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/time.h>
};

class FFmpegMedia {
public:
    /**
     * 当前流的角标（音频/视频/字母）
     */
    int streamIndex=-1;
    /**
     * 解码器的上下文
     * enum AVMediaType codec_type：编解码器的类型（视频，音频...）

struct AVCodec  *codec：采用的解码器AVCodec（H.264,MPEG2...）

int bit_rate：平均比特率

uint8_t *extradata; int extradata_size：针对特定编码器包含的附加信息（例如对于H.264解码器来说，存储SPS，PPS等）

AVRational time_base：根据该参数，可以把PTS转化为实际的时间（单位为秒s）

int width, height：如果是视频的话，代表宽和高

int refs：运动估计参考帧的个数（H.264的话会有多帧，MPEG2这类的一般就没有了）

int sample_rate：采样率（音频）

int channels：声道数（音频）

enum AVSampleFormat sample_fmt：采样格式

int profile：型（H.264里面就有，其他编码标准应该也有）

int level：级（和profile差不太多）
     */
    AVCodecContext *pCodecContext=NULL;
    /**
     *jni的回调函数
     */
    FFMpegJniCall *pJniCall=NULL;
    /**
     * AVpacket队列
     */
    FFmpegPacketQueue *pPacketQueue=NULL;
    /**
     * 播放状态
     */
    FFmpegPlayerStatus *pPlayerStatus=NULL;
    /**
     * 整个视频的时长
     */
    int duration=0;
    /**
     * 当前播放的时间
     */
    double currentTime=0;
    /**
     * 上次更新的时间。主要用于控制回调到java层的频率
     */
    double lastUpdateTime=0;
    /**
     * 时间机
     */
    AVRational timeBase;

    /**
     * seek时的mutex
     */
    pthread_mutex_t seekMutex;

public :
    FFmpegMedia(int streamIndex,FFMpegJniCall *pJniCall,FFmpegPlayerStatus *pPlayerStatus);

    ~FFmpegMedia();

public :
    /**
     * 播放方法，纯虚函数
     */
    virtual void play()=0;

    /**
     * 解析公共的解码器上下文
     */
    void analysisStream(ThreadType threadMode,AVFormatContext *pFormatContext);
//    virtual void privateAnalysisStream(ThreadType threadMode, AVFormatContext *pFormatContext) = 0;

    virtual void release();

    /**
     * 准备解析数据过程中出错的回调
     * @param threadType 线程类型
     * @param errorCode 错误码
     * @param errorMsg 错误信息
     */
    void callPlayerJniError(ThreadType threadMode, int code, char *msg);

    virtual void seek(uint64_t seconds);

private:
    void publicAnalysisStream(ThreadType threadMode, AVFormatContext *pFormatContext);
};


#endif //MYFFMPEGPLAYER_FFMPEGMEDIA_H
