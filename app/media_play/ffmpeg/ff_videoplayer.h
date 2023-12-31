#ifndef FF_VIDEO_PLAYER_H_
#define FF_VIDEO_PLAYER_H_

#include <QElapsedTimer>
#include <QImage>
#include <QObject>
#include <QTime>

#include "codec/ffmpegdecoder.h"
#include "codec/ffmpegwriter.h"
#include "common/media_info.h"
#include "media_play/video_player.h"
#include "util/cthread.h"

class FFVideoPlayer : public VideoPlayer, public CThread
{
public:
    explicit FFVideoPlayer(QObject* parent = nullptr);
    ~FFVideoPlayer();

    void Start() override;
    void Pause() override;
    void Stop() override;
    void Resume() override;

    void StartRecord(const char* file) override;
    void StopRecord() override;

protected:
    bool DoPrepare() override;
    void DoTask() override;
    void DoFinish() override;

private:
    void DoRecordTask(DecodeFrame* frame);

private:
    std::unique_ptr<FFmpegDecoder> decoder_;
    std::unique_ptr<FFmpegWriter> writer_;

    QByteArray filename_;
};

#endif
