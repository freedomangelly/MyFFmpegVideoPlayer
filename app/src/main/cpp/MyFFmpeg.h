//
// Created by freed on 2020/1/25.
//

#ifndef MYFFMPEGPLAYER_MYFFMPEG_H
#define MYFFMPEGPLAYER_MYFFMPEG_H

#include <pthread.h>
#include "FFMpegJniCall.h"
#include "FFmpegAudio.h"
#include "FFmpegVideo.h"

extern "C"{
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
};
class MyFFmpeg {
public:
    FFMpegJniCall *jniCall=NULL;
    char* url=NULL;
    pthread_t preparedThread;
    /**
     * struct AVInputFormat *iformat;//输入数据的封装格式。仅解封装用，由avformat_open_input()设置。
struct AVOutputFormat *oformat;//输出数据的封装格式。仅封装用，调用者在avformat_write_header()之前设置。
AVIOContext *pb;// I/O上下文。
解封装：由用户在avformat_open_input()之前设置（然后用户必须手动关闭它）或通过avformat_open_input()设置。
封装：由用户在avformat_write_header()之前设置。 调用者必须注意关闭/释放IO上下文。
unsigned int nb_streams;//AVFormatContext.streams中元素的个数。
AVStream **streams;//文件中所有流的列表。
char filename[1024];//输入输出文件名。
int64_t start_time;//第一帧的位置。
int64_t duration;//流的持续时间
int64_t bit_rate;//总流比特率（bit / s），如果不可用则为0。
int64_t probesize;
//从输入读取的用于确定输入容器格式的数据的最大大小。
仅封装用，由调用者在avformat_open_input()之前设置。
AVDictionary *metadata;//元数据
AVCodec *video_codec;//视频编解码器
AVCodec *audio_codec;//音频编解码器
AVCodec *subtitle_codec;//字母编解码器
AVCodec *data_codec;//数据编解码器
int (*io_open)(struct AVFormatContext *s, AVIOContext **pb, const char *url, int flags, AVDictionary **options);
//打开IO stream的回调函数。
void (*io_close)(struct AVFormatContext *s, AVIOContext *pb);
//关闭使用AVFormatContext.io_open()打开的流的回调函数。
     */
    AVFormatContext *av_format_context = NULL;
    FFmpegAudio *audio=NULL;
    FFmpegVideo *pVideo=NULL;
    pthread_mutex_t releaseMutex;
    FFmpegPlayerStatus *pPlayerStatus;

public :
    MyFFmpeg(FFMpegJniCall *jniCall ,const char* url);
    ~MyFFmpeg();

public:
    void prepare();
    void preparedAudio(ThreadType threadType);
    void prepareAsync();
    void start();
    void onPause();
    void onResume();
    void release();
    void stop();
    void releasePreparedRes(ThreadType threadType,int errorCode,const char *errorMsg);

    void seek(uint64_t sdconds);

    void decodeFrame();

};


#endif //MYFFMPEGPLAYER_MYFFMPEG_H
