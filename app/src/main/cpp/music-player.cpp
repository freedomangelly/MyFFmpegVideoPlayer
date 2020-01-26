#include <jni.h>
#include "MyConsts.h"
#include "com_liuy_myffmpegplayer_ffmpegplayer_MediaPlayerJNI.h"
#include "FFMpegJniCall.h"
#include "MyFFmpeg.h"

#include <android/native_window.h>
#include <android/native_window_jni.h>


// 在 c++ 中采用 c 的这种编译方式
extern "C" {
#include "libavformat/avformat.h"
#include "libswresample/swresample.h"
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"
}

FFMpegJniCall *pJniCall;
MyFFmpeg *pFFmpeg;

JavaVM *pJavaVM = NULL;

// 重写 so 被加载时会调用的一个方法
// 小作业，去了解动态注册
extern "C" JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *javaVM, void *reserved) {
    LOGI("JNI_OnLoad");
    pJavaVM = javaVM;
    JNIEnv *env;
    if (javaVM->GetEnv((void **) &env, JNI_VERSION_1_4) != JNI_OK) {
        return -1;
    }
    return JNI_VERSION_1_4;
}

extern "C" JNIEXPORT void JNICALL
Java_com_liuy_myffmpegplayer_ffmpegplayer_MediaPlayerJNI_nPlay(JNIEnv *env, jobject instance) {
    LOGI("nPlay");
    if (pFFmpeg != NULL) {
        LOGI("nPlay1");
        pFFmpeg->play();
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_liuy_myffmpegplayer_ffmpegplayer_MediaPlayerJNI_nPrepareAsync(JNIEnv *env,
                                                                       jobject instance,
                                                                       jstring url_) {
    LOGI("nPrepareAsync");
    const char *url = env->GetStringUTFChars(url_, 0);
    if (pFFmpeg == NULL) {
        pJniCall = new FFMpegJniCall(pJavaVM, env, instance);
        pFFmpeg = new MyFFmpeg(pJniCall, url);
        pFFmpeg->prepare();
    }
    env->ReleaseStringUTFChars(url_, url);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_liuy_myffmpegplayer_ffmpegplayer_MediaPlayerJNI_nPrepare(JNIEnv *env, jobject instance,
                                                                  jstring url_) {
    LOGI("nPrepare");
    const char *url = env->GetStringUTFChars(url_, 0);
    if (pFFmpeg == NULL) {
        pJniCall = new FFMpegJniCall(pJavaVM, env, instance);
        pFFmpeg = new MyFFmpeg(pJniCall, url);
        pFFmpeg->prepareAsync();
    }
    env->ReleaseStringUTFChars(url_, url);
}
extern "C"
JNIEXPORT void JNICALL Java_com_liuy_myffmpegplayer_ffmpegplayer_MediaPlayerJNI_nPlay2
        (JNIEnv *env, jobject instance, jstring urlSrc) {
    LOGI("nPlay2");
//    player(env, instance, url);
    pJniCall=new FFMpegJniCall(pJavaVM,env,instance);
    const char *url=env->GetStringUTFChars(urlSrc,0);
    LOGI("enter jni Play2 %s",url);
    pFFmpeg=new MyFFmpeg(pJniCall,url);
    pFFmpeg->play();
    env->ReleaseStringUTFChars(urlSrc,url);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_liuy_myffmpegplayer_MainActivity_decodeView(JNIEnv *env, jobject instance,
                                                     jstring uri_, jobject surface) {
    LOGI("decodeView111111111111111");
    const char *uri = env->GetStringUTFChars(uri_, 0);
    av_register_all();
    avformat_network_init();
    AVFormatContext *pFormatContext = NULL;
    int formatOpenInputRes = 0;
    int formatFindStreamInfoRes = 0;
    int audioStramIndex = -1;
    AVCodecParameters *pCodecParameters;
    AVCodec *pCodec = NULL;
    AVCodecContext *pCodecContext = NULL;
    int codecParametersToContextRes = -1;
    int codecOpenRes = -1;
    int index = 0;
    AVPacket *pPacket = NULL;
    AVFrame *pFrame = NULL;
    formatOpenInputRes = avformat_open_input(&pFormatContext, uri, NULL, NULL);


    formatFindStreamInfoRes = avformat_find_stream_info(pFormatContext, NULL);
    //查找音频流的index
    audioStramIndex=av_find_best_stream(pFormatContext,AVMEDIA_TYPE_VIDEO,-1,-1,NULL,0);
    //查找解码
    // 查找音频流的 index
    audioStramIndex = av_find_best_stream(pFormatContext, AVMediaType::AVMEDIA_TYPE_VIDEO, -1, -1,
                                          NULL, 0);

    LOGI("decodeView111111111111112");
    // 查找解码
    pCodecParameters = pFormatContext->streams[audioStramIndex]->codecpar;
    pCodec = avcodec_find_decoder(pCodecParameters->codec_id);

    // 打开解码器
    pCodecContext = avcodec_alloc_context3(pCodec);

    codecParametersToContextRes = avcodec_parameters_to_context(pCodecContext, pCodecParameters);


    codecOpenRes = avcodec_open2(pCodecContext, pCodec, NULL);

    LOGI("decodeView111111111111113");
    // 1. 获取窗体
    ANativeWindow *pNativeWindow = ANativeWindow_fromSurface(env, surface);
    LOGI("decodeView1111111111111131");
    // 2. 设置缓存区的数据
    ANativeWindow_setBuffersGeometry(pNativeWindow, pCodecContext->width, pCodecContext->height,
                                     WINDOW_FORMAT_RGBA_8888);
    LOGI("decodeView1111111111111132");

    // Window 缓冲区的 Buffer
    ANativeWindow_Buffer outBuffer;
    //初始化上下文
    LOGI("decodeView1111111111111134 %d",pCodecContext->pix_fmt);
    SwsContext *pSwsContext = sws_getContext(pCodecContext->width, pCodecContext->height,
                                             pCodecContext->pix_fmt, pCodecContext->width, pCodecContext->height,
                                             AV_PIX_FMT_RGBA, SWS_BILINEAR, NULL, NULL, NULL);
    LOGI("decodeView111111111111114");
    AVFrame *pRgbaFrame=av_frame_alloc();
    int frameSize=av_image_get_buffer_size(AV_PIX_FMT_RGBA,pCodecContext->width,pCodecContext->height,1);
    uint8_t  *frameBuffer= static_cast<uint8_t *>(malloc(frameSize));
    av_image_fill_arrays(pRgbaFrame->data,pRgbaFrame->linesize,frameBuffer,AV_PIX_FMT_RGBA,pCodecContext->width,pCodecContext->height,1);
    pPacket=av_packet_alloc();
    pFrame=av_frame_alloc();
    LOGI("decodeView111111111111115");
    while (av_read_frame(pFormatContext,pPacket)>=0){
        if(pPacket->stream_index == audioStramIndex){
            int codecSendPacketRes = avcodec_send_packet(pCodecContext,pPacket);
            if(codecSendPacketRes==0){
                int codecReceiveFrameRes=avcodec_receive_frame(pCodecContext,pFrame);
                if(codecReceiveFrameRes ==0){
                    index++;
                    LOGE("解码第 %d 帧", index);
                    // 渲染，显示，OpenGLES (高效，硬件支持)，SurfaceView
                    // 硬件加速和不加速有什么区别？cup 主要是用于计算，gpu 图像支持（硬件）
                    // 这个 pFrame->data , 一般 yuv420P 的，RGBA8888，因此需要转换
                    // 假设拿到了转换后的 RGBA 的 data 数据，如何渲染，把数据推到缓冲区
                    sws_scale(pSwsContext, (const uint8_t *const *) pFrame->data, pFrame->linesize,
                              0, pCodecContext->height, pRgbaFrame->data, pRgbaFrame->linesize);
                    // 把数据推到缓冲区
                    ANativeWindow_lock(pNativeWindow, &outBuffer, NULL);
                    memcpy(outBuffer.bits, frameBuffer, frameSize);
                    ANativeWindow_unlockAndPost(pNativeWindow);
                }
            }
        }
        // 解引用
        av_packet_unref(pPacket);
        av_frame_unref(pFrame);
    }
    av_packet_free(&pPacket);
    av_frame_free(&pFrame);


    __av_resources_destroy:
    if (pCodecContext != NULL) {
        avcodec_close(pCodecContext);
        avcodec_free_context(&pCodecContext);
        pCodecContext = NULL;
    }

    if (pFormatContext != NULL) {
        avformat_close_input(&pFormatContext);
        avformat_free_context(pFormatContext);
        pFormatContext = NULL;
    }
    avformat_network_deinit();

    env->ReleaseStringUTFChars(uri_, uri);
}