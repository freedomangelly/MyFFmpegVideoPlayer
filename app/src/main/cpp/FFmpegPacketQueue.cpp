//
// Created by freed on 2020/1/26.
//

#include "FFmpegPacketQueue.h"
#include "MyConsts.h"

void FFmpegPacketQueue::push(AVPacket *pPacket) {
    pthread_mutex_lock(&packetMutex);
    pPacketQueue.push(pPacket);
    pthread_cond_signal(&packetCond);
    pthread_mutex_unlock(&packetMutex);
}


int FFmpegPacketQueue::pop(AVPacket *avPacket) {
    pthread_mutex_lock(&packetMutex);
    while (play_status!=NULL && !play_status->isExit){
        if(pPacketQueue.size()>0){
            AVPacket *packet=pPacketQueue.front();
            if(av_packet_ref(avPacket,packet)==0){
                pPacketQueue.pop();
            }
            av_packet_free(&packet);
            break;
        } else{
            pthread_cond_wait(&packetCond,&packetMutex);
        }
    }
    pthread_mutex_unlock(&packetMutex);
    return 0;
}

FFmpegPacketQueue::FFmpegPacketQueue(FFmpegPlayerStatus *play_state) {
    pthread_mutex_init(&packetMutex,NULL);
    pthread_cond_init(&packetCond,NULL);
    this->play_status=play_state;
}

FFmpegPacketQueue::~FFmpegPacketQueue() {
    clear();
    pthread_mutex_destroy(&packetMutex);
    pthread_cond_destroy(&packetCond);
}


bool FFmpegPacketQueue::empty() {
    return pPacketQueue.empty();
}

void FFmpegPacketQueue::clear() {
    pthread_cond_signal(&packetCond);
    pthread_mutex_lock(&packetMutex);

    while (!pPacketQueue.empty()){
        AVPacket *avPacket=pPacketQueue.front();
        pPacketQueue.pop();
        av_packet_free(&avPacket);
    }
    pthread_mutex_unlock(&packetMutex);
}

