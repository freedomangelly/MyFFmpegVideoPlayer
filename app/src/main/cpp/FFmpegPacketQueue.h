//
// Created by freed on 2020/1/26.
//

#ifndef MYFFMPEGPLAYER_FFMPEGPACKETQUEUE_H
#define MYFFMPEGPLAYER_FFMPEGPACKETQUEUE_H

#include <queue>
#include <pthread.h>
#include "FFmpegPlayerStatus.h"

extern  "C"{
#include <libavcodec/avcodec.h>
};


class FFmpegPacketQueue {
public:
    std::queue<AVPacket *> pPacketQueue;
    pthread_mutex_t packetMutex;
    pthread_cond_t packetCond;
    FFmpegPlayerStatus *play_status;

public:
    FFmpegPacketQueue(FFmpegPlayerStatus *fFmpegPlayerStatus);
    ~FFmpegPacketQueue();

public :
    void push(AVPacket *avPacket);

    int pop(AVPacket *av_packet);
    bool empty();
    void clear();
};


#endif //MYFFMPEGPLAYER_FFMPEGPACKETQUEUE_H
