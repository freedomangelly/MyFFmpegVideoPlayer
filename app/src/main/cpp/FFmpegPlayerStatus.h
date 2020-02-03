//
// Created by freed on 2020/1/26.
//

#ifndef MYFFMPEGPLAYER_FFMPEGPLAYERSTATUS_H
#define MYFFMPEGPLAYER_FFMPEGPLAYERSTATUS_H


class FFmpegPlayerStatus {
public:
    bool isExit = false;
    bool isLoading=false;
    bool isPause = false;
    bool isSeek = false;

public:
    FFmpegPlayerStatus();
};


#endif //MYFFMPEGPLAYER_FFMPEGPLAYERSTATUS_H
