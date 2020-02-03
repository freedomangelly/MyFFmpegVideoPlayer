//
// Created by freed on 2020/2/2.
//

#ifndef MYFFMPEGPLAYER_ANDROID_LOG_H
#define MYFFMPEGPLAYER_ANDROID_LOG_H
#include <android/log.h>
#define TAG "MY_FFMPEG_TAG"
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR,TAG,__VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,TAG,__VA_ARGS__)
#endif //MYFFMPEGPLAYER_ANDROID_LOG_H
