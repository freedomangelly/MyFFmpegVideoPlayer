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
};

class FFmpegMedia {
public:
    int streamIndex=-1;
    AVCodecContext *pCodecContext=NULL;
    FFMpegJniCall *pJniCall=NULL;
    FFmpegPacketQueue *pPacketQueue=NULL;
    FFmpegPlayerStatus *pPlayerStatus=NULL;
    //时长
    int duration=0;
    double currentTime=0;
    double lastUpdateTime=0;
    AVRational timeBase;

public :
    FFmpegMedia(int streamIndex,FFMpegJniCall *pJniCall,FFmpegPlayerStatus *pPlayerStatus);

    ~FFmpegMedia();

public :
    virtual void play()=0;
    void analysisStream(ThreadMode threadMode,AVFormatContext *pFormatContext);
    virtual void privateAnalysisStream(ThreadMode threadMode, AVFormatContext *pFormatContext) = 0;

    virtual void release();

    void callPlayerJniError(ThreadMode threadMode, int code, char *msg);

private:
    void publicAnalysisStream(ThreadMode threadMode, AVFormatContext *pFormatContext);
};


#endif //MYFFMPEGPLAYER_FFMPEGMEDIA_H
